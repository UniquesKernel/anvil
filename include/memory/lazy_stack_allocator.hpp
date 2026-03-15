/**
 * @file lazy_stack_allocator.hpp
 * @brief Lazy stack allocator with on-demand physical memory commitment.
 *
 * This header defines a stack allocator that reserves a contiguous virtual
 * address space and commits physical pages on demand as allocations advance.
 * The allocator supports record/unwind checkpoints for LIFO-style rollback and
 * a full reset operation for bulk invalidation.
 *
 * @note All functions in this module follow fail-fast design; programmer errors
 *       immediate abort.
 * @note This allocator is NOT thread-safe and requires external
 *       synchronization for concurrent use.
 */

#ifndef ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP
#define ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP

#include "anvil/types.hpp"
#include "constants.hpp"
#include "error/status.hpp"

namespace anvil::memory::lazy_stack_allocator {

/**
 * @brief Representation of a lazy stack allocator with record/unwind capability.
 *
 * The lazy stack allocator manages a contiguous virtual address space. All allocations
 * are linear allocations, that partition the contiguous memory buffer and lazily commits
 * additional physical memory, when required. The lazy stack allocator, supports a record
 * and unwind mechanism, that allows it to record a checkpoint and later restore that checkpoint.
 *
 * Memory layout: [LazyStackAllocator metadata][usable memory region]
 *
 * @invariant base != nullptr (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 * @invariant 0 <= stack_depth <= MAX_STACK_DEPTH
 * @invariant For all i < stack_depth: stack[i] <= allocated
 *
 * @note Any allocation made after a record operation but before an unwind operation should be considered
 * invalid after the unwind operation.
 *
 * Field       | Type     | Size (Bytes)      | Description
 * ----------- | -------- | ----------------- | -------------------------------------------------
 * base        | void*    | sizeof(void*)     | Pointer to the start of the usable memory region
 * capacity    | u64      | sizeof(u64)       | Total capacity of usable memory in bytes
 * allocated   | u64      | sizeof(u64)       | Current number of bytes allocated (watermark)
 * stack_depth | u64      | sizeof(u64)       | Current depth of the record/unwind stack
 * stack       | u64[]    | MAX_STACK_DEPTH*8 | Allocation checkpoints for record/unwind operations
 *
 * Total size: 544 bytes
 */
struct LazyStackAllocator {
        void* base;                                  ///< Start of usable memory region
        u64   capacity;                              ///< Total usable capacity in bytes
        u64   allocated;                             ///< Current allocation watermark
        u64   stack_depth;                           ///< Current record/unwind stack depth
        u64   stack[anvil::memory::MAX_STACK_DEPTH]; ///< Array of allocation checkpoints
};
static_assert(sizeof(LazyStackAllocator) == 544, "LazyStackAllocator size must be 544 bytes"); // NOLINT
static_assert(alignof(LazyStackAllocator) == alignof(void*), "LazyStackAllocator alignment must match void* alignment");

/**
 * @brief Initialize a lazy stack allocator.
 *
 * Initialize a stack allocator using virtual memory addresses.
 * Memory is committed on-demand, consuming only the memory necessary
 * within the granularity of a single memory page.
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
 * @param[in] alignment      Alignment of all memory allocated from the LazyStackAllocator.
 *
 * @return Error enumeration code `OK` on success with other values indicating failure.
 */
[[nodiscard]] Error create(LazyStackAllocator** allocator_out, u64 capacity, u64 alignment) noexcept;

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
 *
 * @note A failure to return the memory to the operating system is treated as a state corruption
 *       and the function will abort the process rather than continue.
 */
[[nodiscard]] Error destroy(LazyStackAllocator** allocator) noexcept;

/**
 * @brief Allocate a contiguous region of memory from the allocator's backing memory buffer.
 *
 * Allocate a contiguous region of memory from the allocator's backing memory buffer.
 * The memory is lazy allocated, in the granularity of one page segments.
 *
 * @pre `allocator != nullptr`.
 * @pre `0 < allocation_size <= MAX_CAPACITY`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @param[in] allocator        LazyStackAllocator from which the allocation should be made.
 * @param[in] allocation_size  Size in bytes of the requested allocation.
 * @param[in] alignment        Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` bytes,
 *         `nullptr` indicates failure to allocate the requested memory.
 */
[[nodiscard]] void* alloc(LazyStackAllocator* allocator, u64 allocation_size, u64 alignment) noexcept;

/**
 * @brief Reset the allocator to it's initialization state.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @param[in] allocator         LazyStackAllocator that should be reset.
 * @return Error enumeration code with `OK` indicating success and other values
 *         indicate failure.
 *
 * @note All previous allocations from this allocator should be considered
 *       invalid after reset.
 */
[[nodiscard]] Error reset(LazyStackAllocator* allocator) noexcept;

/**
 * @brief Records the current allocation state in a LIFO style pattern.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth < MAX_STACK_DEPTH`.
 *
 * @param[in] allocator         LazyStackAllocator whose state should be recorded.
 * @return Error enumeration code with `OK` indicating success and other values
 *         indicate failure.
 */
[[nodiscard]] Error record(LazyStackAllocator* allocator) noexcept;

/**
 * @brief Unwind the allocator in a LIFO pattern to its previously recorded state
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth > 0`.
 *
 * @param[in] allocator         LazyStackAllocator that should be unwound.
 * @return Error enumeration code with `OK` indicating success and other values
 *         indicate failure.
 *
 * @note When `unwind` is called, all allocations between the latest record and the
 *       current unwind call, should be considered invalid.
 */
[[nodiscard]] Error unwind(LazyStackAllocator* allocator) noexcept;

} // namespace anvil::memory::lazy_stack_allocator

#endif // ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP
