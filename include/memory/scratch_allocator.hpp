/**
 * @file scratch_allocator.hpp
 * @brief Linear scratch allocator interface for temporary memory allocation
 *
 * This header defines a high-level interface for scratch memory allocation using
 * a linear allocator strategy. The scratch allocator provides fast, sequential
 * memory allocation with alignment guarantees. The allocator is designed for
 * temporary allocations that can be reset in bulk, making it ideal for frame-based
 * or scope-based memory management patterns.
 *
 * @note All functions in this module follow fail-fast design - programmer errors
 *       trigger immediate abort with diagnostics.
 *
 * @note The scratch allocators are **NOT** thread safe and should not be used
 *       in a concurrent environment without proper synchronization.
 */

#ifndef ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#define ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#include "error.hpp"
#include <cstddef>

namespace anvil::memory::scratch_allocator {
struct ScratchAllocator;

/**
 * @brief Creates a scratch allocator that manages a contiguous region of memory.
 *
 * @pre `capacity > 0`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post ScratchAllocator manages `capacity` amount of bytes with worst case being `capacity + page size - 1` amount of bytes.
 * @post All allocations from ScratchAllocator are aligned to `alignment`.
 * @post Initially the ScratchAllocator has allocated zero bytes.
 * @post Object is opaque and only interface operations are defined.
 *
 * @param[in] capacity      The amount of physical memory to allocate.
 * @param[in] alignment     The alignment of all memory allocated from the ScratchAllocator
 *
 * @return Pointer to a ScratchAllocator.
 */
[[nodiscard]] ScratchAllocator* create(const std::size_t capacity, const std::size_t alignment);

/**
 * @brief Removes a mapping to a contiguous region of physical memory.
 *
 * @pre `allocator != nullptr`.
 * @pre `*allocator != nullptr`.

 * @post `*allocator == nullptr`
 * @post The system has released all allocated memory back to the OS.
 * @post All outstanding allocations are invalid.
 *
 * @param[in,out] allocator    Reference to the allocator whose memory mapping should be undone.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
[[nodiscard]] Error             destroy(ScratchAllocator** allocator);

/**
 * @brief Establishes a contiguous sub-region of memory from an allocator's total contiguous region.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocation_size > 0`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post `allocator` shrinks by `allocation_size + padding`, where `0 <= padding < alignment`.
 * @post The returned memory region is zeroed.
 * @post The returned memory region is aligned to `alignment`.
 * @post Returned pointer satisfies `(uintptr_t)ptr % alignment == 0`.
 *
 * @param[in] allocator         ScratchAllocator from which the allocation should be made.
 * @param[in] allocation_size   Size in bytes of the allocation that should be made.
 * @param[in] alignment         Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes).
 *
 * @note Memory usage uncertainty is reduced by making `allocation_size` a multiple of
 * `alignment`.
 */
[[nodiscard]] void*             alloc(ScratchAllocator* const allocator, const std::size_t allocation_size,
                                      const std::size_t alignment);

/**
 * @brief Re-initialize the state of a ScratchAllocator.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @post All previous allocations from this allocator become invalid.
 * @post `allocator` returns to its initial state from `anvil_memory_scratch_allocator_create`.
 *
 * @param[in] allocator     ScratchAllocator that should be reset.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
[[nodiscard]] Error             reset(ScratchAllocator* const allocator);

/**
 * @brief Copies data from one region outside the ScratchAllocator's managed region to a sub-region inside the ScratchAllocator's managed region.
 *
 * @pre `allocator != nullptr`.
 * @pre `src != nullptr`.
 * @pre `n_bytes > 0`.
 *
 * @post ScratchAllocator's capacity shrinks by `n_bytes` bytes with worst case being `n_bytes + page size - 1` amount of bytes.
 * @post The returned memory region contains `n_bytes` amount of data from `src`.
 * @post The returned memory region is aligned to `alignof(void*)`.
 *
 * @param[in] allocator     ScratchAllocator into whose region the outside data should be written.
 * @param[in] src           The outside memory region from where the data should be retrieved.
 * @param[in] n_bytes       The amount of bytes to be read from `src` and written to the allocator's sub-region.
 *
 * @return Pointer to sub-region of `allocator` containing `n_bytes` bytes copied from `src`.
 *
 * @note This operation is non-destructive and does not affect the data stored in `src`.
 */
[[nodiscard]] void* copy(ScratchAllocator* const allocator, const void* const src, const std::size_t n_bytes);

/**
 * @brief Moves data from one region outside the ScratchAllocator's managed region to a sub-region of the ScratchAllocator's managed region, then invalidates the outside region.
 *
 * @pre `allocator != nullptr`.
 * @pre `src != nullptr`.
 * @pre `*src != nullptr`.
 * @pre `free_func != nullptr`.
 * @pre `n_bytes > 0`.
 *
 * @post ScratchAllocator's capacity shrinks by `n_bytes` bytes with worst case being `n_bytes + page size - 1` amount of bytes.
 * @post The returned memory region contains `n_bytes` amount of data from `src`.
 * @post The returned memory region is aligned to `alignof(void*)`.
 * @post `*src == nullptr`.
 *
 * @param[in] allocator     ScratchAllocator into whose region the outside data should be written.
 * @param[in,out] src       The outside memory region from where the data should be retrieved.
 * @param[in] n_bytes       The amount of bytes to be read from `src` and written to the allocator's sub-region.
 * @param[in] free_func     Pointer to the appropriate function that should be used to free the `src` pointer.
 *
 * @return Pointer to sub-region of `allocator` containing `n_bytes` bytes copied from `src`.
 *
 * @note This operation is destructive as `src` is invalid after this operation.
 */
[[nodiscard]] void* move(ScratchAllocator* const allocator, void** src, const std::size_t n_bytes,
                         void (*free_func)(void*));

} // namespace anvil::memory::scratch_allocator

#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
