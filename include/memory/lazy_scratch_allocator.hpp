/**
 * @file lazy_scratch_allocator.hpp
 * @brief Lazy scratch allocator with on-demand physical memory commitment.
 *
 * This header defines a linear scratch allocator that reserves a contiguous
 * virtual address space and commits physical pages on demand as allocations
 * advance. Like the eager scratch allocator, it serves fast sequential
 * allocations that are invalidated in bulk with `reset`.
 *
 * @note All functions in this module follow fail-fast design; programmer errors
 *       cause immediate abort.
 * @note This allocator is NOT thread-safe and requires external
 *       synchronization for concurrent use.
 */

#ifndef ANVIL_MEMORY_LAZY_SCRATCH_ALLOCATOR_HPP
#define ANVIL_MEMORY_LAZY_SCRATCH_ALLOCATOR_HPP
#include "anvil/types.hpp"
#include "error/status.hpp"

namespace anvil::memory {

/**
 * @brief Representation of a lazy linear scratch allocator.
 *
 * The lazy scratch allocator manages a contiguous virtual address space.
 * All allocations are linear and partition the contiguous buffer, lazily
 * committing additional physical memory when required. The allocator is
 * intended for temporary allocations that are invalidated in bulk by
 * calling `reset`.
 *
 * Memory layout: [LazyScratchAllocator metadata][usable memory region]
 *
 * @invariant base != nullptr (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 *
 * Field     | Type   | Size (Bytes)   | Description
 * --------- | ------ | -------------- | ---------------------------------------------------------
 * base      | void*  | sizeof(void*)  | Pointer to the start of the usable memory region
 * capacity  | u64    | sizeof(u64)    | Total capacity of the scratch allocator in bytes
 * allocated | u64    | sizeof(u64)    | Current number of bytes allocated from the scratch allocator
 *
 * Total size: 24 bytes
 */
struct LazyScratchAllocator {
        void* base;      ///< Start of usable memory region
        u64   capacity;  ///< Total usable capacity in bytes
        u64   allocated; ///< Number of bytes currently handed out from the usable region
};
static_assert(sizeof(LazyScratchAllocator) == 24, "LazyScratchAllocator size must be 24 bytes"); // NOLINT
static_assert(alignof(LazyScratchAllocator) == alignof(void*),
              "LazyScratchAllocator alignment must match void* alignment");

/**
 * @brief Initialize a lazy scratch allocator.
 *
 * Reserves a virtual address range without committing physical memory up front.
 * Physical pages are committed on demand as allocations advance.
 *
 * @pre `allocator_out != nullptr`.
 * @pre `*allocator_out == nullptr`.
 * @pre `capacity > 0`.
 * @pre `capacity <= MAX_CAPACITY`.
 * @pre `capacity >= alignment`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @param[out] allocator_out Output location that receives the created allocator.
 * @param[in] capacity       The amount of usable memory to reserve (bytes).
 * @param[in] alignment      Alignment of all memory allocated from the LazyScratchAllocator.
 *
 * @return Error enumeration code `OK` on success with other values indicating failure.
 */
[[nodiscard]] Error create(LazyScratchAllocator** allocator_out, u64 capacity, u64 alignment) noexcept;

/**
 * @brief Null out an allocator and return the underlying memory to the Operating System.
 *
 * @pre `allocator != nullptr`.
 * @pre `*allocator != nullptr`.
 *
 * @post The `allocator` pointer is set to null.
 * @post The backing memory has been returned to the Operating System.
 * @post All allocations become invalid.
 *
 * @param[out] allocator        The allocator that should be destroyed.
 * @return Error enumeration code where `OK` indicates success and other values indicate failure.
 */
[[nodiscard]] Error destroy(LazyScratchAllocator** allocator) noexcept;

/**
 * @brief Allocate a contiguous region of memory from the allocator's backing memory buffer.
 *
 * Allocate a contiguous region of memory from the allocator's backing memory buffer.
 * Physical memory is committed on demand, in page-sized increments.
 *
 * @pre `allocator != nullptr`.
 * @pre `0 < allocation_size <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @param[in] allocator        LazyScratchAllocator from which the allocation should be made.
 * @param[in] allocation_size  Size in bytes of the requested allocation.
 * @param[in] alignment        Alignment of the returned memory region.
 *
 * @return Pointer to the allocated memory, or `nullptr` on failure.
 */
[[nodiscard]] void* alloc(LazyScratchAllocator* allocator, u64 allocation_size, u64 alignment) noexcept;

/**
 * @brief Reset the allocator to its initialization state.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @param[in] allocator         LazyScratchAllocator that should be reset.
 * @return Error enumeration code with `OK` indicating success and other values
 *         indicating failure.
 *
 * @note All previous allocations from this allocator should be considered
 *       invalid after reset.
 */
[[nodiscard]] Error reset(LazyScratchAllocator* allocator) noexcept;

} // namespace anvil::memory

#endif // ANVIL_MEMORY_LAZY_SCRATCH_ALLOCATOR_HPP
