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
 * @invariant base != nullptr
 * @invariant capacity > 0
 * @invariant allocated >= 0
 * @invariant allocated <= capacity
 *
 * @note This structure is typically placed at the beginning of the allocated memory region.
 *
 * Field               | Type               | Size (Bytes)   | Description
 * ------------------- | ------------------ | -------------- | -----------------------------------------------
 * base                | void*              | sizeof(void*)  | Pointer to the start of the usable memory region
 * capacity            | size_t             | sizeof(size_t) | Total capacity of the scratch allocator in bytes
 * allocated           | size_t             | sizeof(size_t) | Current number of bytes allocated from the scratch
 * allocator allocation_strategy | AllocationStrategy | sizeof(size_t) | Allocation strategy (lazy virtual / eager
 * physical)
 */
struct ScratchAllocator {
        void*              base;
        size_t             capacity;
        size_t             allocated;
        AllocationStrategy allocation_strategy;
};
static_assert(sizeof(ScratchAllocator) == 32, "ScratchAllocator size must be 32 bytes");
static_assert(alignof(ScratchAllocator) == alignof(void*), "ScratchAllocator alignment must match void* alignment");
static_assert(sizeof(ScratchAllocator) > 3 * sizeof(size_t), "ScratchAllocator is too small for transfer protocol");

ScratchAllocator* create(const size_t capacity, const size_t alignment) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was %zu", alignment);
        ANVIL_INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const size_t      total_memory_needed = capacity + sizeof(ScratchAllocator) + alignment - 1;

        ScratchAllocator* allocator =
            static_cast<ScratchAllocator*>(anvil_memory_alloc_eager(total_memory_needed, alignment));

        if (!allocator) {
                return nullptr;
        }

        allocator->base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(allocator) + sizeof(*allocator));
        const size_t actually_available_capacity = total_memory_needed - (reinterpret_cast<uintptr_t>(allocator->base) -
                                                                          reinterpret_cast<uintptr_t>(allocator));

        if (actually_available_capacity < capacity) {
                ANVIL_INVARIANT(anvil_memory_dealloc(allocator) == ERR_SUCCESS, INV_INVALID_STATE,
                                "Failed to Deallocate memory");
                return nullptr;
        }

        allocator->capacity            = capacity;
        allocator->allocated           = 0;
        allocator->allocation_strategy = AllocationStrategy::Eager;

        return allocator;
}

Error destroy(ScratchAllocator** allocator) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(*allocator);

        if (*reinterpret_cast<size_t*>(*allocator) == TRANSFER_MAGIC) [[unlikely]] {
                return ERR_SUCCESS;
        }

        const Error dealloc_result = anvil_memory_dealloc(*allocator);
        if (::anvil::error::is_error(dealloc_result)) [[unlikely]] {
                return dealloc_result;
        }
        *allocator = nullptr;

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
                return nullptr;
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
                return nullptr;
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
                return nullptr;
        }
        memcpy(dest, *src, n_bytes);

        ANVIL_INVARIANT(memcmp(dest, *src, n_bytes) == 0, INV_INVALID_STATE,
                        "Failed to move memory to ScratchAllocator");

        free_func(*src);
        *src = nullptr;

        return dest;
}

} // namespace anvil::memory::scratch_allocator