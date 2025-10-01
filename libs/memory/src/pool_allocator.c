#include "memory/pool_allocator.h"
#include "internal/utility.h"
#include "memory/constants.h"
#include "memory/scratch_allocator.h"

typedef struct pool_allocator_t {
        void*             base;
        size_t            capacity;
        size_t            size;
        uintptr_t*        ring_buffer;
        size_t            head;
        size_t            tail;
        ScratchAllocator* allocator;
} PoolAllocator;

PoolAllocator* anvil_memory_pool_allocator_create(const size_t object_size, const size_t object_count,
                                                  const size_t alignment) {
        INVARIANT_POSITIVE(object_size);
        INVARIANT_POSITIVE(object_count);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "aligment was not a power of two, but was %zu",
                  alignment);

        const size_t   total_memory_needed = (object_count * object_size) + sizeof(PoolAllocator) + alignment - 1;
        PoolAllocator* allocator           = (PoolAllocator*)anvil_memory_alloc_eager(total_memory_needed, alignment);

        if (CHECK_NULL(allocator)) {
                return NULL;
        }

        allocator->base = (void*)((uintptr_t)allocator + sizeof(*allocator));
        const size_t actually_available_capacity =
            total_memory_needed - ((uintptr_t)allocator->base - (uintptr_t)allocator);

        if (actually_available_capacity < (object_count * object_size)) {
                INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, ERR_MEMORY_DEALLOCATION,
                          "Failed to Deallocate memory");
                return NULL;
        }

        allocator->capacity = object_count;
        allocator->size     = object_count;
        allocator->allocator =
            anvil_memory_scratch_allocator_create((object_count + 1) * object_size, alignof(uintptr_t));
        allocator->ring_buffer =
            anvil_memory_scratch_allocator_alloc(allocator->allocator, (object_count + 1) * object_size,
                                                 alignof(uintptr_t));

        for (int i = 0; i < allocator->capacity; ++i) {
                allocator->ring_buffer[i] = ((uintptr_t*)allocator->base + (object_size * i));
        }
        allocator->head = 0;
        allocator->tail = 0;

        return allocator;
}