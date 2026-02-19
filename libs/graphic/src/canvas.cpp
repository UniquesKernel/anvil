#include "graphic/canvas.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include <cstring>


[[nodiscard]]
Error anvil::graphic::create(Canvas* const canvas_out, const unsigned long width, const unsigned long height,
             const char fill_char = ' ') {
        // NOTE: Validate width in range [1, MAX_WIDTH]
        INVARIANT(width - 1 < MAX_WIDTH, INVALID_DIMENSIONS);

        // NOTE: Validate height in range [1, MAX_HEIGHT]
        INVARIANT(height - 1 < MAX_HEIGHT, INVALID_DIMENSIONS);

        canvas_out->width  = width;
        canvas_out->height = height;
        memset(canvas_out->buffer, fill_char, MAX_WIDTH * MAX_HEIGHT);

        return OK;
}

