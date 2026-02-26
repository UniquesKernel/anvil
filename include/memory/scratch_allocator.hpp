/**
 * @file scratch_allocator.hpp
 * @brief Linear scratch allocator interface for temporary memory allocation.
 *
 * This header defines a linear scratch allocator that eagerly maps physical
 * memory for a contiguous region and serves fast sequential allocations from
 * that region. The allocator is intended for temporary allocations that are
 * invalidated in bulk with `reset`.
 *
 * @note All functions in this module follow fail-fast design; programmer errors
 *       trigger immediate abort with diagnostics.
 *
 * @note This allocator is NOT thread-safe and requires external synchronization
 *       for concurrent use.
 */

#ifndef ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#define ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#include "error/status.hpp"
#include <cstddef>

namespace anvil::memory::scratch_allocator {

/**
 * @brief Internal representation of a linear scratch allocator.
 *
 * Memory layout: [ScratchAllocator metadata][usable memory region]
 *
 * @invariant base != nullptr (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 *
 * @note The structure is placed at the beginning of the allocated memory region.
 * @note Total memory footprint is sizeof(ScratchAllocator) + capacity bytes.
 *
 * Field     | Type   | Size (Bytes)   | Description
 * --------- | ------ | -------------- | ---------------------------------------------------------
 * base      | void*  | sizeof(void*)  | Pointer to the start of the usable memory region
 * capacity  | size_t | sizeof(size_t) | Total capacity of the scratch allocator in bytes
 * allocated | size_t | sizeof(size_t) | Current number of bytes allocated from the scratch allocator
 *
 * @note On 64-bit systems: sizeof(ScratchAllocator) = 8 + 8 + 8 = 24 bytes
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
 * @pre `allocator_out != nullptr`.
 * @pre `*allocator_out == nullptr`.
 * @pre `capacity > 0`.
 * @pre `capacity <= MAX_CAPACITY`.
 * @pre `capacity >= alignment`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post On success, a valid allocator instance is created with metadata followed by
 *       a usable region of exactly `capacity` bytes.
 * @post Backing allocation requests `capacity + sizeof(ScratchAllocator) + alignment - 1` bytes before page rounding in the underlying allocator.
 * @post All allocations from ScratchAllocator are aligned to `alignment`.
 * @post Initially, the allocation watermark is zero.
 *
 * @param[out] allocator_out Output location that receives the created allocator.
 * @param[in] capacity       The amount of usable memory to manage (bytes).
 * @param[in] alignment      Alignment of all memory allocated from the ScratchAllocator.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error create(ScratchAllocator** allocator_out, std::size_t capacity, std::size_t alignment) noexcept;

/**
 * @brief Removes a mapping to a contiguous region of physical memory.
 *
 * @pre `allocator != nullptr`.
 * @pre `*allocator != nullptr`.
 *
 * @post The allocator handle is set to null.
 * @post The system has released all allocated memory back to the OS.
 * @post All outstanding allocations are invalid.
 *
 * @param[in,out] allocator    Reference to the allocator whose memory mapping should be undone.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error destroy(ScratchAllocator** allocator) noexcept;

/**
 * @brief Allocates a contiguous aligned sub-region from allocator capacity.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocation_size > 0`.
 * @pre `allocation_size <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post On success, the allocation watermark increases by
 *       `allocation_size + padding`, where `0 <= padding < alignment`.
 * @post Returned pointer (if non-null) is aligned to `alignment`.
 * @post Returns `nullptr` if insufficient capacity remains.
 *
 * @param[in] allocator         ScratchAllocator from which the allocation should be made.
 * @param[in] allocation_size   Size in bytes of the allocation that should be made.
 * @param[in] alignment         Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes),
 *         or `nullptr` on failure.
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
 * @post The allocation watermark is zero.
 * @post Allocated bytes are not cleared.
 *
 * @param[in] allocator     ScratchAllocator that should be reset.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error reset(ScratchAllocator* allocator) noexcept;

} // namespace anvil::memory::scratch_allocator

#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
