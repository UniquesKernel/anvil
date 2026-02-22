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
#include "error/status.hpp"

namespace anvil::memory::scratch_allocator {

/**
 * @brief Encapsulates metadata for a scratch allocator, storing information
 *        about the memory region and allocation state.
 *
 * @invariant base != nullptr
 * @invariant capacity > 0
 * @invariant allocated >= 0
 * @invariant allocated <= capacity
 *
 * @note This structure is typically placed at the beginning of the allocated memory region.
 *
 * Field     | Type   | Size (Bytes)   | Description
 * --------- | ------ | -------------- | ---------------------------------------------------------
 * base      | void*  | sizeof(void*)  | Pointer to the start of the usable memory region
 * capacity  | size_t | sizeof(size_t) | Total capacity of the scratch allocator in bytes
 * allocated | size_t | sizeof(size_t) | Current number of bytes allocated from the scratch allocator
 */
struct ScratchAllocator {
        void*       base;
        std::size_t capacity;
        std::size_t allocated;
};
static_assert(sizeof(ScratchAllocator) == 24, "ScratchAllocator size must be 24 bytes"); // NOLINT
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");

/**
 * @brief Creates a scratch allocator that manages a contiguous region of memory.
 *
 * @pre `capacity > 0`.
 * @pre `capacity <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post On success, `*allocator` references allocator metadata followed by a usable region of exactly `capacity` bytes.
 * @post Backing allocation requests `capacity + sizeof(ScratchAllocator) + alignment - 1` bytes before page rounding in the underlying allocator.
 * @post All allocations from ScratchAllocator are aligned to `alignment`.
 * @post Initially the ScratchAllocator has allocated zero bytes.
 *
 * @param[out] allocator     Output location that receives the created allocator.
 * @param[in] capacity      The amount of physical memory to allocate.
 * @param[in] alignment     The alignment of all memory allocated from the ScratchAllocator
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error create(ScratchAllocator** allocator, std::size_t capacity, std::size_t alignment) noexcept;

/**
 * @brief Removes a mapping to a contiguous region of physical memory.
 *
 * @pre `allocator != nullptr`.
 * @pre `*allocator != nullptr`.
 *
 * @post `*allocator == nullptr`
 * @post The system has released all allocated memory back to the OS.
 * @post All outstanding allocations are invalid.
 *
 * @param[in,out] allocator    Reference to the allocator whose memory mapping should be undone.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
[[nodiscard]] Error destroy(ScratchAllocator** allocator) noexcept;

/**
 * @brief Establishes a contiguous sub-region of memory from an allocator's total contiguous region.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocation_size > 0`.
 * @pre `allocation_size <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post On success, `allocator->allocated` increases by `allocation_size + padding`, where `0 <= padding < alignment`.
 * @post The returned memory region is aligned to `alignment`.
 * @post Returned pointer satisfies `(uintptr_t)ptr % alignment == 0`.
 * @post Returns `nullptr` if insufficient capacity remains.
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
[[nodiscard]] void* alloc(ScratchAllocator* allocator, std::size_t allocation_size, std::size_t alignment) noexcept;

/**
 * @brief Re-initialize the state of a ScratchAllocator.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @post All previous allocations from this allocator become invalid.
 * @post `allocator->allocated == 0`.
 * @post Allocated bytes are not cleared.
 *
 * @param[in] allocator     ScratchAllocator that should be reset.
 *
 * @return Error code, zero indicates success while other values indicate error.
 */
[[nodiscard]] Error reset(ScratchAllocator* allocator) noexcept;

} // namespace anvil::memory::scratch_allocator

#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
