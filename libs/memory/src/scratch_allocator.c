#include "memory/scratch_allocator.h"
#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include "memory/error.h"

/**
 * @brief Encapsulates metadata for a scratch allocator, storing information
 *        about the memory region and allocation state.
 *
 * @invariant base != NULL
 * @invariant capacity > 0
 * @invariant allocated >= 0
 * @invariant allocated <= capacity
 * @invariant alloc_mode == EAGER || alloc_mode == LAZY
 *
 * @note This structure is typically placed at the beginning of the allocated memory region.
 *
 * Fields           | Type   | Size (Bytes)  | Description
 * ---------------- | ------ | ------------- | -------------------------------------------------
 * base             | void*  | sizeof(void*) | Pointer to the start of the usable memory region
 * capacity         | size_t | sizeof(size_t)| Total capacity of the scratch allocator in bytes
 * allocated        | size_t | sizeof(size_t)| Current number of bytes allocated from the scratch allocator
 * alloc_mode       | size_t | sizeof(size_t)| Allocation mode (EAGER or LAZY)
 */
typedef struct scratch_allocator_t {
        void*  base;
        size_t capacity;
        size_t allocated;
        size_t alloc_mode;
} ScratchAllocator;
static_assert(sizeof(ScratchAllocator) == 32, "ScratchAllocator size must be 32 bytes");
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");

ScratchAllocator* anvil_memory_scratch_allocator_create(const size_t capacity, const size_t alignment,
                                                        const size_t alloc_mode) {
        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %s", alignment);
        INVARIANT_RANGE(alignment, alignof(void*), (1 << 16));
        INVARIANT(alloc_mode == EAGER || alloc_mode == LAZY, INV_PRECONDITION,
                  "Expected %s = %d (EAGER) or %d (LAZY), but was %d", "alloc_mode", EAGER, "alloc_mode", LAZY,
                  alloc_mode);

        const size_t      total_memory_needed = capacity + sizeof(ScratchAllocator) + alignment - 1;
        ScratchAllocator* allocator           = NULL;

        if (alloc_mode == EAGER) {
                allocator = (ScratchAllocator*)anvil_memory_alloc_eager(total_memory_needed, alignment);
        } else if (alloc_mode == LAZY) {
                allocator = (ScratchAllocator*)anvil_memory_alloc_lazy(total_memory_needed, alignment);
        } else {
                INVARIANT_NOT_NULL(allocator);
        }

        if (CHECK_NULL(allocator)) {
                return NULL;
        }

        allocator->base       = (void*)((uintptr_t)allocator + sizeof(*allocator));
        allocator->base       = (void*)(((uintptr_t)allocator->base + (alignment - 1)) & ~(alignment - 1));
        allocator->capacity   = capacity;
        allocator->allocated  = 0;
        allocator->alloc_mode = alloc_mode;

        return allocator;
}

Error anvil_memory_scratch_allocator_destroy(ScratchAllocator** allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(*allocator);

        /// NOTE: (UniquesKernel) TRY will return early using the provided Error.
        TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}