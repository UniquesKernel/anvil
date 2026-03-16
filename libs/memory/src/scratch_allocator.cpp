#include "memory/scratch_allocator.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include "math/comparison/comparison.hpp"
#include "memory/constants.hpp"
#include "memory/memory_allocation.hpp"
#include <cstdint>

namespace anvil::memory::scratch_allocator {

[[nodiscard]]
Error create(ScratchAllocator** allocator_out, const u64 capacity, const u64 alignment) noexcept {
        REQUIRE(allocator_out != nullptr, NULL_PARAMETER);
        REQUIRE(*allocator_out == nullptr, INVALID_ARGUMENTS);
        REQUIRE(capacity > 0, INVALID_ARGUMENTS);
        REQUIRE(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        REQUIRE(capacity >= alignment, INVALID_ARGUMENTS);
        REQUIRE(anvil::math::comparison::is_power_of_two(alignment), INVALID_ARGUMENTS);
        REQUIRE(alignment >= MIN_ALIGNMENT, INVALID_ARGUMENTS);
        REQUIRE(alignment <= MAX_ALIGNMENT, INVALID_ARGUMENTS);

        const u64 TOTAL_MEMORY_NEEDED = capacity + sizeof(ScratchAllocator);

        void*     mem                 = nullptr;
        if (anvil::memory::anvil_memory_alloc_eager(&mem, TOTAL_MEMORY_NEEDED, alignment) != OK) {
                return OUT_OF_MEMORY;
        }

        *allocator_out               = (ScratchAllocator*)(mem);

        const uintptr_t RAW_BASE     = (uintptr_t)(*allocator_out) + sizeof(ScratchAllocator);
        const uintptr_t ALIGNED_BASE = (RAW_BASE + (alignment - 1)) & ~(alignment - 1);

        (*allocator_out)->base       = (void*)(ALIGNED_BASE);
        (*allocator_out)->capacity   = capacity;
        (*allocator_out)->allocated  = 0;

        return OK;
}

[[nodiscard]]
Error destroy(ScratchAllocator** allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        REQUIRE(*allocator != nullptr, NULL_PARAMETER);

        INVARIANT(anvil_memory_dealloc(*allocator) == OK);
        *allocator = nullptr;

        return OK;
}

[[nodiscard]]
void* alloc(ScratchAllocator* const allocator, const u64 allocation_size, const u64 alignment) noexcept {
        REQUIRE(allocator != nullptr, nullptr);
        REQUIRE(allocation_size > 0, nullptr);
        REQUIRE(allocation_size <= MAX_CAPACITY, nullptr);
        REQUIRE(anvil::math::comparison::is_power_of_two(alignment), nullptr);
        REQUIRE(MIN_ALIGNMENT <= alignment, nullptr);
        REQUIRE(alignment <= MAX_ALIGNMENT, nullptr);

        INVARIANT(allocator->allocated <= allocator->capacity);

        const uintptr_t CURRENT_ADDR     = (uintptr_t)(allocator->base) + allocator->allocated;
        const uintptr_t ALIGNED_ADDR     = (CURRENT_ADDR + (alignment - 1)) & ~(alignment - 1);
        const u64       OFFSET           = ALIGNED_ADDR - CURRENT_ADDR;

        const u64       TOTAL_ALLOCATION = allocation_size + OFFSET;

        if (TOTAL_ALLOCATION > allocator->capacity - allocator->allocated) [[unlikely]] {
                return nullptr;
        }

        allocator->allocated += TOTAL_ALLOCATION;
        return (void*)(ALIGNED_ADDR);
}

[[nodiscard]]
Error reset(ScratchAllocator* const allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        INVARIANT(allocator->base);

        // memset(allocator->base, 0x0, allocator->allocated);
        allocator->allocated = 0;

        return OK;
}

} // namespace anvil::memory::scratch_allocator
