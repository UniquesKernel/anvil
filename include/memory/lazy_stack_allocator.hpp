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
 *       immediate abort with diagnostics.
 * @note This allocator is NOT thread-safe and requires external
 *       synchronization for concurrent use.
 */

#ifndef ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP
#define ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP

#include "constants.hpp"
#include "error/status.hpp"

namespace anvil::memory::lazy_stack_allocator {

/**
 * @brief Internal representation of a lazy stack allocator with checkpoint/restore capability.
 *
 * This structure manages a contiguous virtual address space with linear
 * allocation semantics and supports a record/unwind mechanism for checkpoint-
 * based memory management. Physical memory is committed lazily on demand.
 *
 * Memory layout: [LazyStackAllocator metadata][usable memory region]
 *
 * @invariant base != nullptr (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 * @invariant 0 <= stack_depth <= MAX_STACK_DEPTH
 * @invariant For all i < stack_depth: stack[i] <= allocated
 *
 * @note The structure is placed at the beginning of the reserved address space.
 * @note Total reservation footprint is sizeof(LazyStackAllocator) + capacity bytes.
 *
 * Field       | Type     | Size (Bytes)      | Description
 * ----------- | -------- | ----------------- | -------------------------------------------------
 * base        | void*    | sizeof(void*)     | Pointer to the start of the usable memory region
 * capacity    | size_t   | sizeof(size_t)    | Total capacity of usable memory in bytes
 * allocated   | size_t   | sizeof(size_t)    | Current number of bytes allocated (watermark)
 * stack_depth | size_t   | sizeof(size_t)    | Current depth of the record/unwind stack
 * stack       | size_t[] | MAX_STACK_DEPTH*8 | Allocation checkpoints for record/unwind operations
 *
 * @note On 64-bit systems: sizeof(LazyStackAllocator) = 8 + 8 + 8 + 8 + (64 * 8) = 544 bytes
 */
struct LazyStackAllocator {
        void*  base;                                  ///< Start of usable memory region
        size_t capacity;                              ///< Total usable capacity in bytes
        size_t allocated;                             ///< Current allocation watermark
        size_t stack_depth;                           ///< Current record/unwind stack depth
        size_t stack[anvil::memory::MAX_STACK_DEPTH]; ///< Array of allocation checkpoints
};
static_assert(sizeof(LazyStackAllocator) == 544, "LazyStackAllocator size must be 544 bytes"); // NOLINT
static_assert(alignof(LazyStackAllocator) == alignof(void*), "LazyStackAllocator alignment must match void* alignment");

/**
 * @brief Creates a lazy (virtual-reserving) stack allocator.
 *
 * Reserves a contiguous virtual address space sufficient for the allocator
 * metadata and `capacity` bytes of usable memory, deferring physical memory
 * commitment until allocations occur.
 *
 * @pre `allocator_out != nullptr`.
 * @pre `*allocator_out == nullptr`.
 * @pre `capacity > 0`.
 * @pre `capacity <= MAX_CAPACITY`.
 * @pre `capacity >= alignment`.
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`.
 *
 * @post On success, a valid allocator instance is created with metadata followed
 *       by a usable region of exactly `capacity` bytes.
 * @post Backing allocation requests
 *       `capacity + sizeof(LazyStackAllocator) + alignment - 1` bytes before
 *       page rounding in the underlying allocator.
 * @post All allocations from LazyStackAllocator are aligned to `alignment`.
 * @post Initially, the allocation watermark is zero and checkpoint depth is zero.
 *
 * @param[out] allocator_out Output location that receives the created allocator.
 * @param[in] capacity       The amount of usable memory to reserve (bytes).
 * @param[in] alignment      Alignment of all memory allocated from the LazyStackAllocator.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error create(LazyStackAllocator** allocator_out, std::size_t capacity, std::size_t alignment) noexcept;

/**
 * @brief Removes the reserved/committed region and releases all resources.
 *
 * @pre `allocator != nullptr`.
 * @pre `*allocator != nullptr`.
 *
 * @post The allocator handle is set to null.
 * @post The system has released all reserved/committed memory back to the OS.
 * @post All outstanding allocations are invalid.
 *
 * @param[in,out] allocator  Reference to the allocator pointer which will be nulled.
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error destroy(LazyStackAllocator** allocator) noexcept;

/**
 * @brief Allocates a contiguous aligned sub-region and commits pages on demand.
 *
 * Advances the internal watermark with proper alignment and, for lazy
 * provisioning, commits additional physical memory to cover the newly
 * allocated span.
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
 * @post Returns `nullptr` if insufficient capacity remains or page commit fails.
 *
 * @param[in] allocator        LazyStackAllocator from which the allocation should be made.
 * @param[in] allocation_size  Size in bytes of the requested allocation.
 * @param[in] alignment        Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes),
 *         or `nullptr` on failure.
 */
[[nodiscard]] void* alloc(LazyStackAllocator* allocator, std::size_t allocation_size, std::size_t alignment) noexcept;

/**
 * @brief Re-initializes the allocator state; prior allocations become invalid.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @post The allocation watermark is zero and checkpoint depth is zero.
 * @post All previous allocations from this allocator are invalid.
 * @post Allocated bytes are not cleared.
 *
 * @param[in] allocator  LazyStackAllocator that should be reset.
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error reset(LazyStackAllocator* allocator) noexcept;

/**
 * @brief Records the current allocation state for later unwinding.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth < MAX_STACK_DEPTH`.
 *
 * @post The current allocation watermark is saved on the internal stack.
 *
 * @param[in] allocator  LazyStackAllocator whose state should be recorded.
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error record(LazyStackAllocator* allocator) noexcept;

/**
 * @brief Unwinds allocations back to the last recorded state.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth > 0`.
 *
 * @post Allocations made after the last record are invalidated.
 * @post The allocator returns to the state at the time of the last record.
 *
 * @param[in] allocator  LazyStackAllocator that should be unwound.
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error unwind(LazyStackAllocator* allocator) noexcept;

} // namespace anvil::memory::lazy_stack_allocator

#endif // ANVIL_MEMORY_LAZY_STACK_ALLOCATOR_HPP
