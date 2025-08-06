#include "memory/stack_allocator.h"
#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include "memory/constants.h"
#include "memory/error.h"
#include <stddef.h>
#include <string.h>

/**
 * @brief Encapsulates metadata for a stack allocator, storing information
 *        about the memory region and allocation state.
 *
 * @invariant base != NULL
 * @invariant capacity > 0
 * @invariant allocated >= 0
 * @invariant allocated <= capacity
 *
 * @note This structure is typically placed at the beginning of the allocated memory region.
 *
 * Fields           | Type   | Size (Bytes)  | Description
 * ---------------- | ------ | ------------- | -------------------------------------------------
 * base             | void*  | sizeof(void*) | Pointer to the start of the usable memory region
 * capacity         | size_t | sizeof(size_t)| Total capacity of the stack allocator in bytes
 * allocated        | size_t | sizeof(size_t)| Current number of bytes allocated from the stack allocator
 */
typedef struct stack_allocator_t {
        void*     base;
        size_t    capacity;
        size_t    allocated;
        size_t    alloc_mode;
        size_t    stack_depth;
        size_t stack[MAX_STACK_DEPTH];
} StackAllocator;
static_assert(sizeof(StackAllocator) == 552, "StackAllocator size must be 552 bytes");
static_assert(alignof(StackAllocator) == alignof(void*), "StackAllocator alignment must match void* alignment");

StackAllocator* anvil_memory_stack_allocator_create(const size_t capacity, const size_t alignment,
                                                    const size_t alloc_mode) {
        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);
        INVARIANT((alloc_mode == EAGER) || (alloc_mode == LAZY), INV_PRECONDITION,
                  "allocation mode, not lazy nor eager, but was %zu", alloc_mode);

        const size_t    total_memory_needed = capacity + sizeof(StackAllocator) + alignment - 1;

        StackAllocator* allocator           = NULL;

        if (alloc_mode == EAGER) {
                allocator = (StackAllocator*)anvil_memory_alloc_eager(total_memory_needed, alignment);
        } else {
                allocator = (StackAllocator*)anvil_memory_alloc_lazy(total_memory_needed, alignment);
        }

        if (CHECK_NULL(allocator)) {
                return NULL;
        }

        allocator->base = (void*)((uintptr_t)allocator + sizeof(*allocator));
        const size_t actual_available_capacity =
            total_memory_needed - ((uintptr_t)allocator->base - (uintptr_t)allocator);

        if (actual_available_capacity < capacity) {
                INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, ERR_MEMORY_DEALLOCATION,
                          "Failed to Deallocate memory");
                return NULL;
        }

        allocator->capacity    = capacity;
        allocator->allocated   = 0;
        allocator->alloc_mode  = alloc_mode;
        allocator->stack_depth = 0;

        return allocator;
}

Error anvil_memory_stack_allocator_destroy(StackAllocator** allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(*allocator);

        TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}

Error anvil_memory_stack_allocator_reset(StackAllocator* const allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(allocator->base);

        memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated = 0;
        allocator->stack_depth = 0;

        return ERR_SUCCESS;
}

void* anvil_memory_stack_allocator_alloc(StackAllocator* const allocator, const size_t allocation_size,
                                         const size_t alignment) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_POSITIVE(allocation_size);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const uintptr_t current_addr     = (uintptr_t)allocator->base + allocator->allocated;
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
        return (void*)aligned_addr;
}
void* anvil_memory_stack_allocator_copy(StackAllocator* const allocator, const void* const src, const size_t n_bytes) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_POSITIVE(n_bytes);

        void* dest = anvil_memory_stack_allocator_alloc(allocator, n_bytes, alignof(void*));

        if (CHECK(dest, ERR_OUT_OF_MEMORY) != ERR_SUCCESS) {
                return NULL;
        }
        memcpy(dest, src, n_bytes);

        INVARIANT(memcmp(dest, src, n_bytes) == 0, INV_INVALID_STATE, "Failed to copy memory to ScratchAllocator");

        return dest;
}

void* anvil_memory_stack_allocator_move(StackAllocator* const allocator, void** src, const size_t n_bytes,
                                        void (*free_func)(void*)) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_NOT_NULL(*src);
        INVARIANT_NOT_NULL(free_func);
        INVARIANT_POSITIVE(n_bytes);

        void* dest = anvil_memory_stack_allocator_alloc(allocator, n_bytes, alignof(void*));

        if (CHECK(dest, ERR_OUT_OF_MEMORY) != ERR_SUCCESS) {
                return NULL;
        }
        memcpy(dest, *src, n_bytes);

        INVARIANT(memcmp(dest, *src, n_bytes) == 0, INV_INVALID_STATE, "Failed to move memory to ScratchAllocator");

        free_func(*src);
        *src = NULL;

        return dest;
}

Error anvil_memory_stack_allocator_record(StackAllocator* const allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(allocator->base);
        INVARIANT_RANGE(allocator->stack_depth, 0, MAX_STACK_DEPTH - 1);

        allocator->stack[allocator->stack_depth] = allocator->allocated;
        allocator->stack_depth++;

        return ERR_SUCCESS;
}

Error anvil_memory_stack_allocator_unwind(StackAllocator* const allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT(allocator->stack_depth > 0, INV_INVALID_STATE, "Cannot unwind from empty stack (stack_depth = %zu)",
                  allocator->stack_depth);
        INVARIANT_RANGE(allocator->stack_depth, 1, MAX_STACK_DEPTH - 1);

        uintptr_t restored_allocated = allocator->stack[allocator->stack_depth - 1];
        allocator->allocated         = restored_allocated;
        allocator->stack_depth--;

        return ERR_SUCCESS;
}