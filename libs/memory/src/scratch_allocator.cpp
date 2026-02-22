#include "memory/scratch_allocator.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/memory_allocation.hpp"
#include <cstddef>
#include <cstdint>

namespace anvil::memory::scratch_allocator {

Error create(ScratchAllocator** allocator_out, const std::size_t capacity, const std::size_t alignment) noexcept {
        INVARIANT(allocator_out != nullptr, NULL_PARAMETER);
        INVARIANT(*allocator_out == nullptr, NULL_PARAMETER);
        INVARIANT(capacity > 0, INVALID_ARGUMENTS);
        INVARIANT(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        INVARIANT(is_power_of_two(alignment), INVALID_ARGUMENTS);
        INVARIANT(alignment >= MIN_ALIGNMENT, INVALID_ARGUMENTS);
        INVARIANT(alignment <= MAX_ALIGNMENT, INVALID_ARGUMENTS);

        const size_t TOTAL_MEMORY_NEEDED = capacity + sizeof(ScratchAllocator) + alignment - 1;

        void*        mem                 = nullptr;
        if (anvil::memory::anvil_memory_alloc_eager(&mem, TOTAL_MEMORY_NEEDED, alignment) != OK) {
                return OUT_OF_MEMORY;
        }

        *allocator_out                  = static_cast<ScratchAllocator*>(mem);

        const uintptr_t RAW_BASE        = reinterpret_cast<uintptr_t>(*allocator_out) + sizeof(ScratchAllocator);
        const uintptr_t ALIGNED_BASE    = (RAW_BASE + (alignment - 1)) & ~(alignment - 1);

        (*allocator_out)->base          = reinterpret_cast<void*>(ALIGNED_BASE);
        (*allocator_out)->capacity      = capacity;
        (*allocator_out)->allocated     = 0;

        const size_t ACTUALLY_AVAILABLE = reinterpret_cast<uintptr_t>((*allocator_out)->base) + capacity -
                                          reinterpret_cast<uintptr_t>(*allocator_out);

        GUARANTEE(ACTUALLY_AVAILABLE <= TOTAL_MEMORY_NEEDED);

        return OK;
}

Error destroy(ScratchAllocator** allocator) noexcept {
        INVARIANT(allocator != nullptr, NULL_PARAMETER);
        INVARIANT(*allocator != nullptr, NULL_PARAMETER);

        const Error DEALLOC_RESULT = anvil_memory_dealloc(*allocator);
        if (DEALLOC_RESULT != OK) [[unlikely]] {
                return DEALLOC_RESULT;
        }
        *allocator = nullptr;

        return OK;
}

void* alloc(ScratchAllocator* const allocator, const size_t allocation_size, const size_t alignment) noexcept {
        INVARIANT(allocator != nullptr, nullptr);
        INVARIANT(allocation_size > 0, nullptr);
        INVARIANT(allocation_size <= MAX_CAPACITY, nullptr);
        INVARIANT(is_power_of_two(alignment), nullptr);
        INVARIANT(MIN_ALIGNMENT <= alignment, nullptr);
        INVARIANT(alignment <= MAX_ALIGNMENT, nullptr);

        GUARANTEE(allocator->allocated <= allocator->capacity);

        const uintptr_t CURRENT_ADDR     = reinterpret_cast<uintptr_t>(allocator->base) + allocator->allocated;
        const uintptr_t ALIGNED_ADDR     = (CURRENT_ADDR + (alignment - 1)) & ~(alignment - 1);
        const size_t    OFFSET           = ALIGNED_ADDR - CURRENT_ADDR;

        const size_t    TOTAL_ALLOCATION = allocation_size + OFFSET;

        if (TOTAL_ALLOCATION > allocator->capacity - allocator->allocated) [[unlikely]] {
                return nullptr;
        }

        allocator->allocated += TOTAL_ALLOCATION;
        return reinterpret_cast<void*>(ALIGNED_ADDR);
}

Error reset(ScratchAllocator* const allocator) noexcept {
        INVARIANT(allocator, INVALID_ARGUMENTS);
        GUARANTEE(allocator->base);

        // memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated = 0;

        return OK;
}

} // namespace anvil::memory::scratch_allocator
