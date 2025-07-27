#include "memory/stack_allocator.h"
#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include "memory/constants.h"
#include "memory/error.h"
#include <stddef.h>

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
        void*  base;
        size_t capacity;
        size_t allocated;
        size_t alloc_mode;
} StackAllocator;
static_assert(sizeof(StackAllocator) == 32, "StacjAllocator size must be 32 bytes");
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

        allocator->capacity   = capacity;
        allocator->allocated  = 0;
        allocator->alloc_mode = alloc_mode;

        return allocator;
}

Error anvil_memory_stack_allocator_destroy(StackAllocator** allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(*allocator);

        TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}