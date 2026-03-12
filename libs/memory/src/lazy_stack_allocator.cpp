#include "memory/lazy_stack_allocator.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/memory_allocation.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace anvil::memory::lazy_stack_allocator {

[[nodiscard]]
Error create(LazyStackAllocator** allocator_out, const std::size_t capacity, const std::size_t alignment) noexcept {
        REQUIRE(allocator_out != nullptr, NULL_PARAMETER);
        REQUIRE(*allocator_out == nullptr, INVALID_ARGUMENTS);
        REQUIRE(capacity > 0, INVALID_ARGUMENTS);
        REQUIRE(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        REQUIRE(capacity >= alignment, INVALID_ARGUMENTS);
        REQUIRE(is_power_of_two(alignment), INVALID_ARGUMENTS);
        REQUIRE(alignment >= MIN_ALIGNMENT, INVALID_ARGUMENTS);
        REQUIRE(alignment <= MAX_ALIGNMENT, INVALID_ARGUMENTS);

        const size_t TOTAL_MEMORY_NEEDED = capacity + sizeof(LazyStackAllocator);

        void*        mem                 = nullptr;
        if (anvil::memory::anvil_memory_alloc_lazy(&mem, TOTAL_MEMORY_NEEDED, alignment) != OK) {
                return OUT_OF_MEMORY;
        }
        *allocator_out                = (LazyStackAllocator*)(mem);

        const uintptr_t RAW_BASE      = (uintptr_t)(*allocator_out) + sizeof(LazyStackAllocator);
        const uintptr_t ALIGNED_BASE  = (RAW_BASE + (alignment - 1)) & ~(alignment - 1);

        (*allocator_out)->base        = (void*)(ALIGNED_BASE);
        (*allocator_out)->capacity    = capacity;
        (*allocator_out)->allocated   = 0;
        (*allocator_out)->stack_depth = 0;

        return OK;
}

[[nodiscard]]
Error destroy(LazyStackAllocator** allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        REQUIRE(*allocator != nullptr, NULL_PARAMETER);

        INVARIANT(anvil_memory_dealloc(*allocator) == OK);
        *allocator = nullptr;

        return OK;
}

[[nodiscard]]
Error reset(LazyStackAllocator* const allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        INVARIANT(allocator->base != nullptr);

        allocator->allocated   = 0;
        allocator->stack_depth = 0;

        return OK;
}

[[nodiscard]]
void* alloc(LazyStackAllocator* const allocator, const size_t allocation_size, const size_t alignment) noexcept {
        REQUIRE(allocator != nullptr, nullptr);
        REQUIRE(allocation_size > 0, nullptr);
        REQUIRE(allocation_size <= MAX_CAPACITY, nullptr);
        REQUIRE(is_power_of_two(alignment), nullptr);
        REQUIRE(MIN_ALIGNMENT <= alignment, nullptr);
        REQUIRE(alignment <= MAX_ALIGNMENT, nullptr);

        INVARIANT(allocator->base != nullptr);
        INVARIANT(allocator->allocated <= allocator->capacity);

        const uintptr_t CURRENT_ADDR     = (uintptr_t)(allocator->base) + allocator->allocated;
        const uintptr_t ALIGNED_ADDR     = (CURRENT_ADDR + (alignment - 1)) & ~(alignment - 1);
        const size_t    OFFSET           = ALIGNED_ADDR - CURRENT_ADDR;
        const size_t    TOTAL_ALLOCATION = allocation_size + OFFSET;

        if (TOTAL_ALLOCATION > allocator->capacity - allocator->allocated) [[unlikely]] {
                return nullptr;
        }

        Metadata*    metadata     = (Metadata*)((uintptr_t)(allocator) - sizeof(Metadata));
        const size_t TOTAL_NEEDED = (ALIGNED_ADDR + allocation_size) - (uintptr_t)(metadata->base);

        if (TOTAL_NEEDED > metadata->capacity) {
                const size_t ADDITIONAL_COMMIT = TOTAL_NEEDED - metadata->capacity;
                if (anvil_memory_commit(allocator, ADDITIONAL_COMMIT) != OK) {
                        return nullptr;
                }
        }

        allocator->allocated += TOTAL_ALLOCATION;
        return (void*)(ALIGNED_ADDR);
}

[[nodiscard]]
Error record(LazyStackAllocator* const allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        INVARIANT(allocator->base != nullptr);

        REQUIRE(allocator->stack_depth < MAX_STACK_DEPTH, INVALID_ARGUMENTS);

        allocator->stack[allocator->stack_depth] = allocator->allocated;
        allocator->stack_depth++;

        return OK;
}

[[nodiscard]]
Error unwind(LazyStackAllocator* const allocator) noexcept {
        REQUIRE(allocator != nullptr, NULL_PARAMETER);
        INVARIANT(allocator->base != nullptr);
        REQUIRE(allocator->stack_depth > 0, INVALID_ARGUMENTS);
        INVARIANT(allocator->stack_depth <= MAX_STACK_DEPTH);

        const uintptr_t RESTORED_ALLOCATED = allocator->stack[allocator->stack_depth - 1];
        allocator->allocated               = RESTORED_ALLOCATED;
        allocator->stack_depth--;

        return OK;
}

} // namespace anvil::memory::lazy_stack_allocator
