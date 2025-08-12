/**
 * @file stack_allocator.h
 * @brief Stack-based memory allocator interface for contiguous memory management
 *
 * This header defines a high-level interface for stack memory allocation using
 * a contiguous memory region strategy. The stack allocator provides efficient,
 * sequential memory allocation with customizable alignment guarantees. The allocator
 * is designed for scenarios where memory allocations follow a stack-like pattern
 * (LIFO - Last In, First Out), making it ideal for nested scope-based memory
 * management and high-performance applications requiring memory locality.
 *
 * @note All functions in this module follow fail-fast design - programmer errors
 *       trigger immediate abort with diagnostics.
 *
 * @note The stack allocators are **NOT** thread safe and should not be used
 *       in a concurrent environment without proper synchronization.
 */

#ifndef ANVIL_MEMORY_STACK_ALLOCATOR_H
#define ANVIL_MEMORY_STACK_ALLOCATOR_H

#include "error.h"
#include <stddef.h>

typedef struct stack_allocator_t StackAllocator;

/**
 * @brief Establishes a region of physical memory that is managed as a contiguous region.
 *
 * @pre `capacity > 0`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post StackAllocator manages `capacity` amount of bytes with worst case being `capacity + page size - 1` amount of bytes.
 * @post All allocations from StackAllocator are aligned to `alignment`.
 * @post Initially the StackAllocator has allocated zero bytes.
 * @post Object is opaque and only interface operations are defined.
 *
 * @param[in] capacity      The amount of physical memory to allocate.
 * @param[in] alignment     The alignment of all memory allocated from the StackAllocator.
 * @param[in] alloc_mode    The allocation mode for the StackAllocator.
 *
 * @return Pointer to a StackAllocator.
 */
StackAllocator*                  anvil_memory_stack_allocator_create(const size_t capacity, const size_t alignment,
                                                                     const size_t alloc_mode);

/**
 * @brief Removes a mapping to a contiguous region of physical memory.
 *
 * @pre `allocator != NULL`.
 * @pre `*allocator != NULL`.
 *
 * @post `*allocator == NULL`
 * @post The system has released all allocated memory back to the OS.
 * @post All outstanding allocations are invalid.
 *
 * @param[in,out] allocator    Reference to the allocator whose memory mapping should be undone.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
Error                            anvil_memory_stack_allocator_destroy(StackAllocator** allocator);

/**
 * @brief Establishes a contiguous sub-region of memory from an allocator's total contiguous region.
 *
 * @pre `allocator != NULL`.
 * @pre `allocation_size > 0`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post `allocator` shrinks by `allocation_size + padding`, where `0 <= padding < alignment`.
 * @post The returned memory region is zeroed.
 * @post The returned memory region is aligned to `alignment`.
 * @post Returned pointer satisfies `(uintptr_t)ptr % alignment == 0`.
 *
 * @param[in] allocator         StackAllocator from which the allocation should be made.
 * @param[in] allocation_size   Size in bytes of the allocation that should be made.
 * @param[in] alignment         Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes).
 *
 * @note Uncertainty in allocator memory usage is improved by making `allocation_size` a multiple of
 * `alignment`.
 */
void* anvil_memory_stack_allocator_alloc(StackAllocator* const allocator, const size_t allocation_size,
                                         const size_t alignment);

/**
 * @brief Re-initialize the state of a StackAllocator.
 *
 * @pre `allocator != NULL`.
 * @pre `allocator->base != NULL`.
 *
 * @post All previous allocations from this allocator become invalid.
 * @post `allocator` has identical state to its initialization state from `anvil_memory_stack_allocator_create`.
 *
 * @param[in] allocator     StackAllocator that should be reset.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
Error anvil_memory_stack_allocator_reset(StackAllocator* const allocator);

/**
 * @brief Writes data from one region outside the StackAllocator's managed region to a sub-region inside the StackAllocator's managed region.
 *
 * @pre `allocator != NULL`.
 * @pre `src != NULL`.
 * @pre `n_bytes > 0`.
 *
 * @post StackAllocator's capacity shrinks by `n_bytes` bytes with worst case being `n_bytes + page size - 1` amount of bytes.
 * @post The returned memory region contains `n_bytes` amount of data from `src`.
 * @post The returned memory region is aligned to `alignof(void*)`.
 *
 * @param[in] allocator     StackAllocator to whose region the outside data should be written.
 * @param[in] src           The outside memory region from where the data should be retrieved.
 * @param[in] n_bytes       The amount of bytes to be read from `src` and written to the allocator's sub-region.
 *
 * @return Pointer to sub-region of `allocator` containing `n_bytes` bytes copied from `src`.
 *
 * @note This operation is non-destructive and does not affect the data stored in `src`.
 */
void* anvil_memory_stack_allocator_copy(StackAllocator* const allocator, const void* const src, const size_t n_bytes);

/**
 * @brief Writes data from one region outside the StackAllocator's managed region to a sub-region of the StackAllocator's managed region, then it invalidates the outside region.
 *
 * @pre `allocator != NULL`.
 * @pre `src != NULL`.
 * @pre `*src != NULL`.
 * @pre `free_func != NULL`.
 * @pre `n_bytes > 0`.
 *
 * @post StackAllocator's capacity shrinks by `n_bytes` bytes with worst case being `n_bytes + page size - 1` amount of bytes.
 * @post The returned memory region contains `n_bytes` amount of data from `src`.
 * @post The returned memory region is aligned to `alignof(void*)`.
 * @post `*src == NULL`.
 *
 * @param[in] allocator     StackAllocator to whose region the outside data should be written.
 * @param[in,out] src       The outside memory region from where the data should be retrieved.
 * @param[in] n_bytes       The amount of bytes to be read from `src` and written to the allocator's sub-region.
 * @param[in] free_func     Pointer to the appropriate function that should be used to free the `src` pointer.
 *
 * @return Pointer to sub-region of `allocator` containing `n_bytes` bytes copied from `src`.
 *
 * @note This operation is destructive as `src` is invalid after this operation.
 */
void* anvil_memory_stack_allocator_move(StackAllocator* const allocator, void** src, const size_t n_bytes,
                                        void (*free_func)(void*));

StackAllocator* anvil_memory_stack_allocator_transfer(StackAllocator* allocator, void* src, const size_t data_size,
                                                     const size_t alignment);
void*          anvil_memory_stack_allocator_absorb(StackAllocator* allocator, void* src, Error (*destroy_fn)(void**));
#endif // ANVIL_MEMORY_STACK_ALLOCATOR_H