#include "memory/stack_allocator.hpp"
#include "internal/memory_allocation.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"


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
 * @invariant allocation_strategy == AllocationStrategy::Eager || allocation_strategy == AllocationStrategy::Lazy
 * @invariant For all i < stack_depth: stack[i] <= allocated
 *
 * @note This is the internal definition. The public API uses an opaque forward declaration.
 * @note The structure is placed at the beginning of the allocated memory region.
 * @note Total memory footprint is sizeof(StackAllocator) + capacity bytes.
 *
 * Field               | Type               | Size (Bytes)      | Description
 * ------------------- | ------------------ | ----------------- |
 * -------------------------------------------------------- base                | void*              | sizeof(void*) |
 * Pointer to the start of the usable memory region capacity            | size_t             | sizeof(size_t)    | Total
 * capacity of usable memory in bytes allocated           | size_t             | sizeof(size_t)    | Current number of
 * bytes allocated (allocation watermark) allocation_strategy | AllocationStrategy | sizeof(size_t)    | Allocation
 * strategy (eager physical / lazy virtual) stack_depth         | size_t             | sizeof(size_t)    | Current depth
 * of the record/unwind stack stack               | size_t[]           | MAX_STACK_DEPTH*8 | Stack of allocation markers
 * for record/unwind operations
 *
 * @note On 64-bit systems: sizeof(StackAllocator) = 8 + 8 + 8 + 8 + 8 + (64 * 8) = 552 bytes
 */
struct StackAllocator {
        void*              base;                                  ///< Start of usable memory region
        size_t             capacity;                              ///< Total usable capacity in bytes
        size_t             allocated;                             ///< Current allocation watermark
        AllocationStrategy allocation_strategy;                   ///< Allocation strategy (eager or lazy provisioning)
        size_t             stack_depth;                           ///< Current record/unwind stack depth
        size_t             stack[anvil::memory::MAX_STACK_DEPTH]; ///< Array of allocation checkpoints
};
static_assert(sizeof(AllocationStrategy) == sizeof(std::size_t), "AllocationStrategy must match size_t size");
static_assert(sizeof(StackAllocator) == 552, "StackAllocator size must be 552 bytes");
static_assert(alignof(StackAllocator) == alignof(void*), "StackAllocator alignment must match void* alignment");

StackAllocator* create(const size_t capacity, const size_t alignment, const AllocationStrategy strategy) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        ANVIL_INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);
        ANVIL_INVARIANT((strategy == AllocationStrategy::Eager) || (strategy == AllocationStrategy::Lazy),
                        INV_PRECONDITION, "allocation strategy, not lazy nor eager, but was %zu",
                        static_cast<std::size_t>(strategy));

        const size_t    total_memory_needed = capacity + sizeof(StackAllocator) + alignment - 1;

        StackAllocator* allocator           = nullptr;

        if (strategy == AllocationStrategy::Eager) {
                allocator = static_cast<StackAllocator*>(anvil_memory_alloc_eager(total_memory_needed, alignment));
        } else {
                allocator = static_cast<StackAllocator*>(anvil_memory_alloc_lazy(total_memory_needed, alignment));
        }

        if (!allocator) {
                return nullptr;
        }

        allocator->base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(allocator) + sizeof(*allocator));
        const size_t actual_available_capacity = total_memory_needed - (reinterpret_cast<uintptr_t>(allocator->base) -
                                                                        reinterpret_cast<uintptr_t>(allocator));

        if (actual_available_capacity < capacity) {
                ANVIL_INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, INV_INVALID_STATE,
                                "Failed to Deallocate memory");
                return nullptr;
        }

        allocator->capacity            = capacity;
        allocator->allocated           = 0;
        allocator->allocation_strategy = strategy;
        allocator->stack_depth         = 0;

        return allocator;
}

Error destroy(StackAllocator** allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(*allocator);

        const Error dealloc_result = anvil_memory_dealloc(*allocator);
        if (::anvil::error::is_error(dealloc_result)) [[unlikely]] {
                return dealloc_result;
        }
        *allocator = nullptr;

        return ERR_SUCCESS;
}

Error reset(StackAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(allocator->base);

        //memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated   = 0;
        allocator->stack_depth = 0;

        return ERR_SUCCESS;
}

void* alloc(StackAllocator* const allocator, const size_t allocation_size, const size_t alignment) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_POSITIVE(allocation_size);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        ANVIL_INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const uintptr_t current_addr     = reinterpret_cast<uintptr_t>(allocator->base) + allocator->allocated;
        const uintptr_t aligned_addr     = (current_addr + (alignment - 1)) & ~(alignment - 1);
        const size_t    offset           = aligned_addr - current_addr;

        const size_t    total_allocation = allocation_size + offset;

        if (total_allocation > allocator->capacity - allocator->allocated) {
                return nullptr;
        }

        if (allocator->allocation_strategy == AllocationStrategy::Lazy) {
                if (anvil_memory_commit(allocator, total_allocation) != ERR_SUCCESS) {
                        return nullptr;
                }
        }
        allocator->allocated += total_allocation;
        return reinterpret_cast<void*>(aligned_addr);
}

[[nodiscard]]
Error record(StackAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(allocator->base);

        if (allocator->stack_depth == MAX_STACK_DEPTH - 1) {
                return ERR_STACK_OVERFLOW;
        }

        allocator->stack[allocator->stack_depth] = allocator->allocated;
        allocator->stack_depth++;

        return ERR_SUCCESS;
}

Error unwind(StackAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT(allocator->stack_depth > 0, INV_INVALID_STATE,
                        "Cannot unwind from empty stack (stack_depth = %zu)", allocator->stack_depth);
        ANVIL_INVARIANT_RANGE(allocator->stack_depth, 1, MAX_STACK_DEPTH - 1);

        uintptr_t restored_allocated = allocator->stack[allocator->stack_depth - 1];
        allocator->allocated         = restored_allocated;
        allocator->stack_depth--;

        return ERR_SUCCESS;
}

} // namespace anvil::memory::stack_allocator
