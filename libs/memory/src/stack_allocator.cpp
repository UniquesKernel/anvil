#include "memory/stack_allocator.hpp"
#include "internal/memory_allocation.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

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
 * @invariant base != NULL (after successful initialization)
 * @invariant capacity > 0
 * @invariant 0 <= allocated <= capacity
 * @invariant 0 <= stack_depth <= MAX_STACK_DEPTH
 * @invariant alloc_mode == EAGER || alloc_mode == LAZY
 * @invariant For all i < stack_depth: stack[i] <= allocated
 *
 * @note This is the internal definition. The public API uses an opaque forward declaration.
 * @note The structure is placed at the beginning of the allocated memory region.
 * @note Total memory footprint is sizeof(StackAllocator) + capacity bytes.
 *
 * Field           | Type     | Size (Bytes)         | Description
 * --------------- | -------- | -------------------- | --------------------------------------------------------
 * base            | void*    | sizeof(void*)        | Pointer to the start of the usable memory region
 * capacity        | size_t   | sizeof(size_t)       | Total capacity of usable memory in bytes
 * allocated       | size_t   | sizeof(size_t)       | Current number of bytes allocated (allocation watermark)
 * alloc_mode      | size_t   | sizeof(size_t)       | Allocation strategy: EAGER (physical) or LAZY (virtual)
 * stack_depth     | size_t   | sizeof(size_t)       | Current depth of the record/unwind stack
 * stack           | size_t[] | MAX_STACK_DEPTH * 8  | Stack of allocation markers for record/unwind operations
 *
 * @note On 64-bit systems: sizeof(StackAllocator) = 8 + 8 + 8 + 8 + 8 + (64 * 8) = 552 bytes
 */
struct StackAllocator {
        void*  base;                                  ///< Start of usable memory region
        size_t capacity;                              ///< Total usable capacity in bytes
        size_t allocated;                             ///< Current allocation watermark
        size_t alloc_mode;                            ///< EAGER or LAZY allocation mode
        size_t stack_depth;                           ///< Current record/unwind stack depth
        size_t stack[anvil::memory::MAX_STACK_DEPTH]; ///< Array of allocation checkpoints
};
static_assert(sizeof(StackAllocator) == 552, "StackAllocator size must be 552 bytes");
static_assert(alignof(StackAllocator) == alignof(void*), "StackAllocator alignment must match void* alignment");

StackAllocator* create(const size_t capacity, const size_t alignment, const size_t alloc_mode) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        ANVIL_INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);
        ANVIL_INVARIANT((alloc_mode == EAGER) || (alloc_mode == LAZY), INV_PRECONDITION,
                        "allocation mode, not lazy nor eager, but was %zu", alloc_mode);

        const size_t    total_memory_needed = capacity + sizeof(StackAllocator) + alignment - 1;

        StackAllocator* allocator           = NULL;

        if (alloc_mode == EAGER) {
                allocator = static_cast<StackAllocator*>(anvil_memory_alloc_eager(total_memory_needed, alignment));
        } else {
                allocator = static_cast<StackAllocator*>(anvil_memory_alloc_lazy(total_memory_needed, alignment));
        }

        if (!allocator) {
                return NULL;
        }

        allocator->base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(allocator) + sizeof(*allocator));
        const size_t actual_available_capacity = total_memory_needed - (reinterpret_cast<uintptr_t>(allocator->base) -
                                                                        reinterpret_cast<uintptr_t>(allocator));

        if (actual_available_capacity < capacity) {
                ANVIL_INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, INV_INVALID_STATE,
                                "Failed to Deallocate memory");
                return NULL;
        }

        allocator->capacity    = capacity;
        allocator->allocated   = 0;
        allocator->alloc_mode  = alloc_mode;
        allocator->stack_depth = 0;

        return allocator;
}

Error destroy(StackAllocator** allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(*allocator);

        if (UNLIKELY(*reinterpret_cast<size_t*>(*allocator) == TRANSFER_MAGIC)) {
                return ERR_SUCCESS;
        }

        ANVIL_TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}

Error reset(StackAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(allocator->base);

        memset(allocator->base, 0x0, allocator->allocated);
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
                return NULL;
        }

        if (allocator->alloc_mode == LAZY) {
                if (anvil_memory_commit(allocator, total_allocation) != ERR_SUCCESS) {
                        return NULL;
                }
        }
        allocator->allocated += total_allocation;
        return reinterpret_cast<void*>(aligned_addr);
}
void* copy(StackAllocator* const allocator, const void* const src, const size_t n_bytes) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_POSITIVE(n_bytes);

        void* dest = alloc(allocator, n_bytes, alignof(void*));

        if (!dest) {
                return NULL;
        }
        memcpy(dest, src, n_bytes);

        ANVIL_INVARIANT(memcmp(dest, src, n_bytes) == 0, INV_INVALID_STATE,
                        "Failed to copy memory to ScratchAllocator");

        return dest;
}

void* move(StackAllocator* const allocator, void** src, const size_t n_bytes, void (*free_func)(void*)) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_NOT_NULL(*src);
        ANVIL_INVARIANT_NOT_NULL(free_func);
        ANVIL_INVARIANT_POSITIVE(n_bytes);

        void* dest = alloc(allocator, n_bytes, alignof(void*));

        if (!dest) {
                return NULL;
        }
        memcpy(dest, *src, n_bytes);

        ANVIL_INVARIANT(memcmp(dest, *src, n_bytes) == 0, INV_INVALID_STATE,
                        "Failed to move memory to ScratchAllocator");

        free_func(*src);
        *src = NULL;

        return dest;
}

Error record(StackAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(allocator->base);
        ANVIL_INVARIANT_RANGE(allocator->stack_depth, 0, MAX_STACK_DEPTH - 1);

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

StackAllocator* transfer(StackAllocator* allocator, void* src, const size_t data_size, const size_t alignment) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_RANGE(data_size, 1, allocator->capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was not a power two but was %zu",
                        alignment);

        void* transfer                                                            = reinterpret_cast<void*>(allocator);
        *reinterpret_cast<size_t*>(transfer)                                      = TRANSFER_MAGIC;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + sizeof(size_t)) = data_size;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + 2 * sizeof(size_t)) = alignment;
        memcpy(static_cast<char*>(transfer) + 3 * sizeof(size_t), src, data_size);

        return reinterpret_cast<StackAllocator*>(transfer);
}

void* absorb(StackAllocator* allocator, void* src, Error (*destroy_fn)(void**)) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_NOT_NULL(destroy_fn);

        if (*reinterpret_cast<size_t*>(src) != TRANSFER_MAGIC) {
                return NULL;
        }

        size_t data_size = *reinterpret_cast<size_t*>(static_cast<char*>(src) + sizeof(size_t));
        size_t alignment = *reinterpret_cast<size_t*>(static_cast<char*>(src) + 2 * sizeof(size_t));
        void*  dest      = alloc(allocator, data_size, alignment);

        if (!dest) {
                destroy_fn(&src);
                return NULL;
        }

        *reinterpret_cast<size_t*>(src) = 0x0;
        memcpy(dest, static_cast<char*>(src) + 3 * sizeof(size_t), data_size);

        destroy_fn(&src);
        return dest;
}

} // namespace anvil::memory::stack_allocator
