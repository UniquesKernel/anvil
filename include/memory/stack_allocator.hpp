/**
 * @file stack_allocator.hpp
 * @brief Eager stack allocator interface for contiguous memory management.
 *
 * This header defines a stack allocator that eagerly maps physical memory for a
 * contiguous region and serves fast linear allocations from that region. The
 * allocator supports record/unwind checkpoints for LIFO-style rollback and a
 * full reset operation for bulk invalidation.
 *
 * @note All functions in this module follow fail-fast design; programmer errors
 *       trigger immediate abort with diagnostics.
 *
 * @note This allocator is NOT thread-safe and requires external synchronization
 *       for concurrent use.
 */

#ifndef ANVIL_MEMORY_STACK_ALLOCATOR_HPP
#define ANVIL_MEMORY_STACK_ALLOCATOR_HPP
#include "constants.hpp"
#include "error/status.hpp"

namespace anvil::memory::stack_allocator {

/**
 * @brief Internal representation of a stack allocator with checkpoint/restore capability.
 *
 * This structure manages a contiguous memory region with linear allocation semantics
 * and supports a record/unwind mechanism for checkpoint-based memory management.
 * The allocator maintains an internal stack of allocation markers that enable
 * efficient bulk deallocation back to any recorded checkpoint.
 *
 * Memory layout: [StackAllocator metadata][usable memory region]
 *
 * @invariant base != nullptr (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 * @invariant 0 <= stack_depth <= MAX_STACK_DEPTH
 * @invariant For all i < stack_depth: stack[i] <= allocated
 *
 * @note The structure is placed at the beginning of the allocated memory region.
 * @note Total memory footprint is sizeof(StackAllocator) + capacity bytes.
 *
 * Field       | Type     | Size (Bytes)      | Description
 * ----------- | -------- | ----------------- | -------------------------------------------------
 * base        | void*    | sizeof(void*)     | Pointer to the start of the usable memory region
 * capacity    | size_t   | sizeof(size_t)    | Total capacity of usable memory in bytes
 * allocated   | size_t   | sizeof(size_t)    | Current number of bytes allocated (watermark)
 * stack_depth | size_t   | sizeof(size_t)    | Current depth of the record/unwind stack
 * stack       | size_t[] | MAX_STACK_DEPTH*8 | Allocation checkpoints for record/unwind operations
 *
 * @note On 64-bit systems: sizeof(StackAllocator) = 8 + 8 + 8 + 8 + (64 * 8) = 544 bytes
 */
struct StackAllocator {
        void*  base;                                  ///< Start of usable memory region
        size_t capacity;                              ///< Total usable capacity in bytes
        size_t allocated;                             ///< Current allocation watermark
        size_t stack_depth;                           ///< Current record/unwind stack depth
        size_t stack[anvil::memory::MAX_STACK_DEPTH]; ///< Array of allocation checkpoints
};
static_assert(sizeof(StackAllocator) == 544, "StackAllocator size must be 544 bytes"); // NOLINT
static_assert(alignof(StackAllocator) == alignof(void*), "StackAllocator alignment must match void* alignment");

/**
 * @brief Creates an eager stack allocator over a contiguous memory region.
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
 *       `capacity + sizeof(StackAllocator) + alignment - 1` bytes before page
 *       rounding in the underlying allocator.
 * @post All allocations from StackAllocator are aligned to `alignment`.
 * @post Initially, the allocation watermark is zero and checkpoint depth is zero.
 *
 * @param[out] allocator_out Output location that receives the created allocator.
 * @param[in] capacity       The amount of usable memory to manage (bytes).
 * @param[in] alignment      Alignment of all memory allocated from the StackAllocator.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error create(StackAllocator** allocator_out, std::size_t capacity, std::size_t alignment) noexcept;

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
[[nodiscard]] Error destroy(StackAllocator** allocator) noexcept;

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
 * @param[in] allocator         StackAllocator from which the allocation should be made.
 * @param[in] allocation_size   Size in bytes of the allocation that should be made.
 * @param[in] alignment         Alignment of the returned memory region.
 *
 * @return Pointer to aligned memory region of size `allocation_size` (bytes),
 *         or `nullptr` on failure.
 *
 * @note Uncertainty in allocator memory usage is improved by making `allocation_size` a multiple of
 * `alignment`.
 */
[[nodiscard]] void* alloc(StackAllocator* allocator, std::size_t allocation_size, std::size_t alignment) noexcept;

/**
 * @brief Re-initialize the state of a StackAllocator.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->base != nullptr`.
 *
 * @post All previous allocations from this allocator become invalid.
 * @post The allocation watermark is zero and checkpoint depth is zero.
 * @post Allocated bytes are not cleared.
 *
 * @param[in] allocator     StackAllocator that should be reset.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error reset(StackAllocator* allocator) noexcept;

/**
 * @brief Records the current allocation state for later unwinding.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth < MAX_STACK_DEPTH`.
 *
 * @post The current allocation state is saved on the internal stack.
 *
 * @param[in] allocator     StackAllocator whose state should be recorded.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error record(StackAllocator* allocator) noexcept;

/**
 * @brief Unwinds allocations back to the last recorded state.
 *
 * @pre `allocator != nullptr`.
 * @pre `allocator->stack_depth > 0`.
 *
 * @post Allocations made after the last record are invalidated.
 * @post The allocator returns to the state at the time of the last record.
 *
 * @param[in] allocator     StackAllocator that should be unwound.
 *
 * @return Error code, `OK` on success while other values indicate failure.
 */
[[nodiscard]] Error unwind(StackAllocator* allocator) noexcept;

} // namespace anvil::memory::stack_allocator

#endif // ANVIL_MEMORY_STACK_ALLOCATOR_HPP
