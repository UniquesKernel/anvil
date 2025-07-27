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
 * @post StackAllocator manages `capacity` amount of bytes with worstcase being `capacity + page size - 1` amount of bytes.
 * @post All allocation from StackAllocator is aligned to `alignment`.
 * @post Initially the StackAllocator as allocated zero bytes.
 * @post Object is opaque and only interface operations are defined.
 *
 * @param[in] capacity      The amount of physical memory to allocate.
 * @param[in] alignment     The alignment of the all memory allocated from the StackAllocator
 *
 * @return Pointer to a StackAllocator.
 */
StackAllocator*                  anvil_memory_stack_allocator_create(const size_t capacity, const size_t alignment, const size_t alloc_mode);

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
Error                              anvil_memory_stack_allocator_destroy(StackAllocator** allocator);

/**
 * @brief Establishes a contiguous sub-region of memory from an allocator's total contiguous region.
 *
 * @pre `allocator != NULL`.
 * @pre `allocation_size > 0`.
 * @pre `alignment` is power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post `allocator` shrinks by `allocation_size + padding`, where `0 <= padding < alignment`.
 * @post The returned memory region is zeroed.
 * @post The returned memory region is aligned to `alignment`.
 * @post Returned pointer satisfies `(uintptr_t)ptr % alignment == 0`.
 *
 * @param[in] allocator         StackAllocator from which the allocation should be made.
 * @param[in] allocation_size   Size in bytes of the allocation that should be made.
 * @param[in] alignment         alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes).
 *
 * @note Uncertainty in allocator memory usages is improved by making `allocation_size` a multiple of
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
 * @post `allocator` has identical state to its initialization state from `anvil_memory_scratch_allocator_create`.
 *
 * @param[in] allocator     StackAllocator that should be reset.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
Error anvil_memory_stack_allocator_reset(StackAllocator* const allocator);

#endif // ANVIL_MEMORY_STACK_ALLOCATOR_H