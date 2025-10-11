#include "memory/scratch_allocator.hpp"
#include "internal/memory_allocation.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"
#include <cstdalign>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace anvil::memory::scratch_allocator {

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
struct ScratchAllocator {
        void*  base;
        size_t capacity;
        size_t allocated;
        size_t alloc_mode;
};
static_assert(sizeof(ScratchAllocator) == 32, "ScratchAllocator size must be 32 bytes");
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");
static_assert(sizeof(ScratchAllocator) > 3 * sizeof(size_t), "ScratchAllocator is too small for transfer protocol");

ScratchAllocator* create(const size_t capacity, const size_t alignment) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        ANVIL_INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const size_t      total_memory_needed = capacity + sizeof(ScratchAllocator) + alignment - 1;

        ScratchAllocator* allocator = static_cast<ScratchAllocator*>(anvil_memory_alloc_eager(total_memory_needed, alignment));

        if (!allocator) {
                return NULL;
        }

        allocator->base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(allocator) + sizeof(*allocator));
        const size_t actually_available_capacity =
            total_memory_needed - (reinterpret_cast<uintptr_t>(allocator->base) - reinterpret_cast<uintptr_t>(allocator));

        if (actually_available_capacity < capacity) {
                ANVIL_INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, INV_INVALID_STATE,
                                "Failed to Deallocate memory");
                return NULL;
        }

        allocator->capacity   = capacity;
        allocator->allocated  = 0;
        allocator->alloc_mode = EAGER;

        return allocator;
}

Error destroy(ScratchAllocator** allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(*allocator);

        if (UNLIKELY(*reinterpret_cast<size_t*>(*allocator) == TRANSFER_MAGIC)) {
                return ERR_SUCCESS;
        }

        /// NOTE: (UniquesKernel) ANVIL_TRY will return early using the provided Error.
        ANVIL_TRY(anvil_memory_dealloc(*allocator));
        *allocator = NULL;

        return ERR_SUCCESS;
}

void* alloc(ScratchAllocator* const allocator, const size_t allocation_size, const size_t alignment) {
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

        allocator->allocated += total_allocation;
        return reinterpret_cast<void*>(aligned_addr);
}

Error reset(ScratchAllocator* const allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(allocator->base);

        memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated = 0;

        return ERR_SUCCESS;
}

void* copy(ScratchAllocator* const allocator, const void* const src, const size_t n_bytes) {
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

void* move(ScratchAllocator* const allocator, void** src, const size_t n_bytes, void (*free_func)(void*)) {
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

ScratchAllocator* transfer(ScratchAllocator* allocator, void* src, const size_t data_size, const size_t alignment) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_RANGE(data_size, 1, allocator->capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT,
                        "alignment was not a power two but was %zu", alignment);

        void* transfer                                                            = reinterpret_cast<void*>(allocator);
        *reinterpret_cast<size_t*>(transfer)                                      = TRANSFER_MAGIC;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + sizeof(size_t)) = data_size;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + 2 * sizeof(size_t)) = alignment;
        memcpy(static_cast<char*>(transfer) + 3 * sizeof(size_t), src, data_size);

        return reinterpret_cast<ScratchAllocator*>(transfer);
}

void* absorb(ScratchAllocator* allocator, void* src, Error (*destroy_fn)(void**)) {
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

} // namespace anvil::memory::scratch_allocator