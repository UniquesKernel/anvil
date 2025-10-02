#include "memory/scratch_allocator.hpp"
#include "internal/memory_allocation.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Encapsulates metadata for a scratch allocator, storing information
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
 * capacity         | size_t | sizeof(size_t)| Total capacity of the scratch allocator in bytes
 * allocated        | size_t | sizeof(size_t)| Current number of bytes allocated from the scratch allocator
 * alloc_mode       | size_t | sizeof(size_t)| Allocation mode can be either LAZY or EAGER
 */
typedef struct scratch_allocator_t {
        void*  base;
        size_t capacity;
        size_t allocated;
        size_t alloc_mode;
} ScratchAllocator;
static_assert(sizeof(ScratchAllocator) == 32, "ScratchAllocator size must be 32 bytes");
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");
static_assert(sizeof(ScratchAllocator) > 3 * sizeof(size_t), "ScratchAllocator is too small for transfer protocol");

namespace anvil {
namespace memory {
namespace scratch_allocator {

ScratchAllocator* create(const size_t capacity, const size_t alignment) {
        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const size_t      total_memory_needed = capacity + sizeof(ScratchAllocator) + alignment - 1;

        ScratchAllocator* allocator = (ScratchAllocator*)anvil_memory_alloc_eager(total_memory_needed, alignment);

        if (CHECK_NULL(allocator)) {
                return NULL;
        }

        allocator->base = (void*)((uintptr_t)allocator + sizeof(*allocator));
        const size_t actually_available_capacity =
            total_memory_needed - ((uintptr_t)allocator->base - (uintptr_t)allocator);

        if (actually_available_capacity < capacity) {
                INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, INV_INVALID_STATE,
                          "Failed to Deallocate memory");
                return NULL;
        }

        allocator->capacity   = capacity;
        allocator->allocated  = 0;
        allocator->alloc_mode = EAGER;

        return allocator;
}

Error destroy(ScratchAllocator** allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(*allocator);

        if (UNLIKELY((*(size_t*)*allocator) == TRANSFER_MAGIC)) {
                return ERR_SUCCESS;
        }

        /// NOTE: (UniquesKernel) TRY will return early using the provided Error.
        TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}

void* alloc(ScratchAllocator* const allocator, const size_t allocation_size,
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

        allocator->allocated += total_allocation;
        return (void*)aligned_addr;
}

Error reset(ScratchAllocator* const allocator) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(allocator->base);

        memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated = 0;

        return ERR_SUCCESS;
}

void* copy(ScratchAllocator* const allocator, const void* const src,
           const size_t n_bytes) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_POSITIVE(n_bytes);

        void* dest = alloc(allocator, n_bytes, alignof(void*));

        if (CHECK(dest, ERR_OUT_OF_MEMORY) != ERR_SUCCESS) {
                return NULL;
        }
        memcpy(dest, src, n_bytes);

        INVARIANT(memcmp(dest, src, n_bytes) == 0, INV_INVALID_STATE, "Failed to copy memory to ScratchAllocator");

        return dest;
}

void* move(ScratchAllocator* const allocator, void** src, const size_t n_bytes,
           void (*free_func)(void*)) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_NOT_NULL(*src);
        INVARIANT_NOT_NULL(free_func);
        INVARIANT_POSITIVE(n_bytes);

        void* dest = alloc(allocator, n_bytes, alignof(void*));

        if (CHECK(dest, ERR_OUT_OF_MEMORY) != ERR_SUCCESS) {
                return NULL;
        }
        memcpy(dest, *src, n_bytes);

        INVARIANT(memcmp(dest, *src, n_bytes) == 0, INV_INVALID_STATE, "Failed to move memory to ScratchAllocator");

        free_func(*src);
        *src = NULL;

        return dest;
}

ScratchAllocator* transfer(ScratchAllocator* allocator, void* src, const size_t data_size, const size_t alignment) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_RANGE(data_size, 1, allocator->capacity);
        INVARIANT(is_power_of_two(alignment),INV_BAD_ALIGNMENT, "alignment was not a power two but was %zu", alignment);

        void* transfer = (void*)allocator;
        *(size_t*) transfer = TRANSFER_MAGIC;
        *((size_t*) transfer + 1) = data_size;
        *((size_t*) transfer + 2) = alignment;
        memcpy(((size_t*)transfer + 3), src, data_size);

        return (ScratchAllocator*)transfer;
}

void* absorb(ScratchAllocator* allocator, void* src, Error(*destroy_fn)(void**)) {
        INVARIANT_NOT_NULL(allocator);
        INVARIANT_NOT_NULL(src);
        INVARIANT_NOT_NULL(destroy_fn);
        
        if (*(size_t*) src != TRANSFER_MAGIC) {
                return NULL;
        }

        size_t data_size = (*((size_t*) src + 1));
        size_t alignment = (*((size_t*) src + 2));
        void* dest = alloc(allocator, data_size, alignment);
        
        if (!dest) {
                destroy_fn(&src);
                return NULL;
        }

        *(size_t*) src = 0x0;
        memcpy(dest, ((size_t*)src + 3), data_size);
        
        destroy_fn(&src);
        return dest;
}

} // namespace scratch
} // namespace memory
} // namespace anvil