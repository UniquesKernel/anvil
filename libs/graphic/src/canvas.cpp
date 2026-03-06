#include "graphic/canvas.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include "memory/scratch_allocator.hpp"
#include <cstring>

[[nodiscard]]
Error anvil::graphic::create(Canvas* const canvas_out, const unsigned long width, const unsigned long height,
                             const char fill_char = ' ') {
        INVARIANT(canvas_out != nullptr, NULL_PARAMETER);
        INVARIANT(canvas_out->allocator == nullptr, INVALID_ARGUMENTS);
        INVARIANT(canvas_out->buffer == nullptr, INVALID_ARGUMENTS);

        // NOTE: Validate width in range [1, MAX_WIDTH]
        INVARIANT(width - 1 < MAX_WIDTH, INVALID_DIMENSIONS);

        // NOTE: Validate height in range [1, MAX_HEIGHT]
        INVARIANT(height - 1 < MAX_HEIGHT, INVALID_DIMENSIONS);

        canvas_out->width     = width;
        canvas_out->height    = height;
        canvas_out->allocator = nullptr;

        const Error ALLOCATOR_CREATE_ERR =
            memory::scratch_allocator::create(&canvas_out->allocator, height * width, alignof(unsigned long));

        if (ALLOCATOR_CREATE_ERR != OK) {
                return ALLOCATOR_CREATE_ERR;
        }

        canvas_out->buffer = static_cast<char*>(memory::scratch_allocator::alloc(canvas_out->allocator, height * width,
                                                                                 alignof(unsigned long)));

        if (!canvas_out->buffer) {
                const Error CLEANUP_ERROR = memory::scratch_allocator::destroy(&canvas_out->allocator);
                GUARANTEE(CLEANUP_ERROR == OK);
                return OUT_OF_MEMORY;
        }

        memset(canvas_out->buffer, fill_char, width * height);

        return OK;
}

[[nodiscard]]
Error anvil::graphic::set(Canvas* const canvas_out, const unsigned long index, const char character) {
        INVARIANT(canvas_out != nullptr, NULL_PARAMETER);
        INVARIANT(canvas_out->buffer != nullptr, NULL_PARAMETER);
        INVARIANT(index < (canvas_out->width * canvas_out->height), INVALID_ARGUMENTS);

        canvas_out->buffer[index] = character;
        return OK;
}

[[nodiscard]]
Error anvil::graphic::destroy(Canvas* const canvas_out) {
        INVARIANT(canvas_out != nullptr, NULL_PARAMETER);
        GUARANTEE(canvas_out->buffer != nullptr);
        GUARANTEE(canvas_out->allocator != nullptr);

        GUARANTEE(anvil::memory::scratch_allocator::destroy(&canvas_out->allocator) == OK);
        canvas_out->height    = 0;
        canvas_out->width     = 0;
        canvas_out->buffer    = nullptr;
        canvas_out->allocator = nullptr;

        return OK;
}
