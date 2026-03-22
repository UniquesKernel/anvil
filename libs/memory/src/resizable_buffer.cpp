#include "anvil/memory/resizable_buffer.hpp"
#include "anvil/error/assert.hpp"
#include "anvil/error/status.hpp"
#include "anvil/math/comparison/comparison.hpp"
#include "anvil/memory/constants.hpp"
#include "anvil/memory/memory_allocation.hpp"
#include <cstdint>
#include <cstring>

namespace anvil::memory::resizable_buffer {
Error create(ResizableBuffer** buffer_out, u64 capacity, u64 alignment) noexcept {
        REQUIRE(buffer_out != nullptr, NULL_PARAMETER);
        REQUIRE(*buffer_out == nullptr, INVALID_ARGUMENTS);
        REQUIRE(capacity > 0, INVALID_ARGUMENTS);
        REQUIRE(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        REQUIRE(capacity >= alignment, INVALID_ARGUMENTS);
        REQUIRE(anvil::math::comparison::is_power_of_two(alignment), INVALID_ARGUMENTS);
        REQUIRE(alignment >= MIN_ALIGNMENT, INVALID_ARGUMENTS);
        REQUIRE(alignment <= MAX_ALIGNMENT, INVALID_ARGUMENTS);

        const u64 TOTAL_MEMORY_NEEDED = capacity + sizeof(ResizableBuffer);

        void*     mem                 = nullptr;
        if (anvil::memory::anvil_memory_alloc_eager(&mem, TOTAL_MEMORY_NEEDED, alignment) != OK) {
                return OUT_OF_MEMORY;
        }

        *buffer_out                  = (ResizableBuffer*)(mem);

        const uintptr_t RAW_BASE     = (uintptr_t)(*buffer_out) + sizeof(ResizableBuffer);
        const uintptr_t ALIGNED_BASE = (RAW_BASE + (alignment - 1)) & ~(alignment - 1);

        (*buffer_out)->base          = (void*)(ALIGNED_BASE);
        (*buffer_out)->capacity      = capacity;
        (*buffer_out)->alignment     = alignment;

        return OK;
}

void* data(const ResizableBuffer* const buffer) noexcept {
        REQUIRE(buffer != nullptr, nullptr);
        REQUIRE(buffer->base != nullptr, nullptr);
        REQUIRE(buffer->capacity > 0, nullptr);
        REQUIRE(buffer->capacity <= MAX_CAPACITY, nullptr);
        REQUIRE(anvil::math::comparison::is_power_of_two(buffer->alignment), nullptr);
        REQUIRE(buffer->alignment >= MIN_ALIGNMENT, nullptr);
        REQUIRE(buffer->alignment <= MAX_ALIGNMENT, nullptr);

        return buffer->base;
}

Error destroy(ResizableBuffer** buffer) noexcept {
        REQUIRE(buffer != nullptr, NULL_PARAMETER);
        REQUIRE(*buffer != nullptr, NULL_PARAMETER);

        INVARIANT(anvil_memory_dealloc(*buffer) == OK);
        *buffer = nullptr;

        return OK;
}

void* resize(ResizableBuffer** buffer_out, u64 new_size) noexcept {
        REQUIRE(buffer_out != nullptr, nullptr);
        REQUIRE((*buffer_out) != nullptr, nullptr);
        REQUIRE((*buffer_out)->base != nullptr, nullptr);
        REQUIRE((*buffer_out)->capacity > 0, nullptr);
        REQUIRE((*buffer_out)->capacity <= MAX_CAPACITY, nullptr);
        REQUIRE(new_size > 0, nullptr);
        REQUIRE(new_size <= MAX_CAPACITY, nullptr);
        REQUIRE(anvil::math::comparison::is_power_of_two((*buffer_out)->alignment), nullptr);
        REQUIRE((*buffer_out)->alignment >= MIN_ALIGNMENT, nullptr);
        REQUIRE((*buffer_out)->alignment <= MAX_ALIGNMENT, nullptr);

        ResizableBuffer* new_buffer = nullptr;

        if (create(&new_buffer, new_size, (*buffer_out)->alignment) != OK) {
                return nullptr;
        }

        const u64 TRANSFER_SIZE = math::comparison::min((*buffer_out)->capacity, new_size);
        new_buffer->base        = memcpy(new_buffer->base, (*buffer_out)->base, TRANSFER_SIZE);

        if (destroy(buffer_out) != OK) {
                INVARIANT(destroy(&new_buffer) == OK);
                return nullptr;
        }

        *buffer_out = new_buffer;
        return new_buffer->base;
}
} // namespace anvil::memory::resizable_buffer
