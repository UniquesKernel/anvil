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
 *       immediate abort.
 * @note This allocator is NOT thread-safe and requires external
 *       synchronization for concurrent use.
 */

#ifndef ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#define ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
#include "anvil/types.hpp"
#include "error/status.hpp"

namespace anvil::memory {

/**
 * @brief Representation of a linear scratch allocator.
 *
 * The scratch allocator manages a contiguous region of physical memory.
 * Each allocation takes the next available bytes from that region, so memory
 * is handed out in order until the allocator is reset. The scratch allocator
 * is intended for temporary allocations that are invalidated in bulk by
 * calling `reset`.
 *
 * Memory layout: [ScratchAllocator metadata][usable memory region]
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
struct ScratchAllocator {
        void* base;      ///< Start of usable memory region
        u64   capacity;  ///< Total usable capacity in bytes
        u64   allocated; ///< Number of bytes currently handed out from the usable region
};
static_assert(sizeof(ScratchAllocator) == 24, "ScratchAllocator size must be 24 bytes"); // NOLINT
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");

/**
 * @brief Initialize a scratch allocator.
 *
 * Initialize a scratch allocator using a contiguous region of memory.
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
 * @param[in] capacity       The amount of usable memory to manage (bytes).
 * @param[in] alignment      Alignment of all memory allocated from the ScratchAllocator.
 *
 * @return Error enumeration code `OK` on success with other values indicating failure.
 */
[[nodiscard]] Error create(ScratchAllocator** allocator_out, u64 capacity, u64 alignment) noexcept;

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
[[nodiscard]] Error destroy(ScratchAllocator** allocator) noexcept;

/**
 * @brief Allocate a contiguous region of memory from the allocator's backing memory buffer.
 *
 * @pre `allocator != nullptr`.
 * @pre `0 < allocation_size <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @param[in] allocator        ScratchAllocator from which the allocation should be made.
 * @param[in] allocation_size  Size in bytes of the requested allocation.
 * @param[in] alignment        Alignment of the returned memory region.
 *
 * @return Pointer to the allocated memory, or `nullptr` on failure.
 */
[[nodiscard]] void* alloc(ScratchAllocator* allocator, u64 allocation_size, u64 alignment) noexcept;

/**
 * @brief Reset the allocator to it's initialization state.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @param[in] allocator         ScratchAllocator that should be reset.
 * @return Error enumeration code with `OK` indicating success and other values
 *         indicate failure.
 *
 * @note All previous allocations from this allocator should be considered
 *       invalid after reset.
 */
[[nodiscard]] Error reset(ScratchAllocator* allocator) noexcept;

} // namespace anvil::memory

#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_HPP
