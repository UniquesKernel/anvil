#include "memory/scratch_allocator.h"
#include "internal/error.h"
#include "internal/utility.h"

typedef struct scratch_allocator_t {
        void* base;
} ScratchAllocator;

ScratchAllocator* anvil_memory_scratch_allocator_create(const size_t capacity, const size_t alignment,
                                                        const char alloc_mode) {
        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %s", alignment);
        INVARIANT_RANGE(alignment, alignof(void*), (1 << 16));
        INVARIANT(alloc_mode == EAGER || alloc_mode == LAZY, INV_PRECONDITION,
                  "Expected %s = %d (EAGER) or %d (LAZY), but was %d", #alloc_mode, EAGER, #alloc_mode, LAZY,
                  alloc_mode);

        
        return NULL;
}