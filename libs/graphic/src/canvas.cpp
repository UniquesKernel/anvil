#include "anvil/graphic/canvas.hpp"
#include "anvil/error/assert.hpp"
#include "anvil/error/status.hpp"
#include "anvil/memory/scratch_allocator.hpp"
#include <cstring>

Error anvil::graphic::create(Canvas* const canvas_out, const u64 width, const u64 height, const char fill_char = ' ') {
        REQUIRE(canvas_out != nullptr, NULL_PARAMETER);
        REQUIRE(canvas_out->allocator == nullptr, INVALID_ARGUMENTS);
        REQUIRE(canvas_out->buffer == nullptr, INVALID_ARGUMENTS);

        // NOTE: Validate width in range [1, MAX_WIDTH]
        REQUIRE(width - 1 < MAX_WIDTH, INVALID_DIMENSIONS);

        // NOTE: Validate height in range [1, MAX_HEIGHT]
        REQUIRE(height - 1 < MAX_HEIGHT, INVALID_DIMENSIONS);

        canvas_out->width     = width;
        canvas_out->height    = height;
        canvas_out->allocator = nullptr;

        const Error ALLOCATOR_CREATE_ERR =
            memory::create(&canvas_out->allocator, height * width, alignof(u64));

        if (ALLOCATOR_CREATE_ERR != OK) {
                return ALLOCATOR_CREATE_ERR;
        }

        canvas_out->buffer =
            (char*)(memory::alloc(canvas_out->allocator, height * width, alignof(u64)));

        if (!canvas_out->buffer) {
                const Error CLEANUP_ERROR = memory::destroy(&canvas_out->allocator);
                INVARIANT(CLEANUP_ERROR == OK);
                return OUT_OF_MEMORY;
        }

        memset(canvas_out->buffer, fill_char, width * height);

        return OK;
}

Error anvil::graphic::set(Canvas* const canvas_out, const u64 index, const char character) {
        REQUIRE(canvas_out != nullptr, NULL_PARAMETER);
        REQUIRE(canvas_out->buffer != nullptr, NULL_PARAMETER);
        REQUIRE(index < (canvas_out->width * canvas_out->height), INVALID_ARGUMENTS);

        canvas_out->buffer[index] = character;
        return OK;
}

Error anvil::graphic::destroy(Canvas* const canvas_out) {
        REQUIRE(canvas_out != nullptr, NULL_PARAMETER);
        INVARIANT(canvas_out->buffer != nullptr);
        INVARIANT(canvas_out->allocator != nullptr);

        INVARIANT(anvil::memory::destroy(&canvas_out->allocator) == OK);
        canvas_out->height    = 0;
        canvas_out->width     = 0;
        canvas_out->buffer    = nullptr;
        canvas_out->allocator = nullptr;

        return OK;
}
