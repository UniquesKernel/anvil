/**
 * @file
 * @brief ASCII canvas type and creation API for terminal rendering.
 * @details Provides the `Canvas` struct and `create` function for allocating
 * and initializing a fixed-size character buffer used to render ASCII graphics
 * in a terminal window.
 */
#ifndef ANVIL_GRAPHIC_CANVAS_HPP
#define ANVIL_GRAPHIC_CANVAS_HPP

#include "error/status.hpp"

namespace anvil::graphic {

static const unsigned long MAX_WIDTH  = 1920;
static const unsigned long MAX_HEIGHT = 1080;

/**
 * @brief Canvas for rendering ASCII graphics
 *
 * A canvas is a large contiguous buffer of `char`
 * values that can be flushed to a terminal for rendering
 * ASCII graphics.
 *
 * @invariant 0 < height <= MAX_HEIGHT
 * @invariant 0 < width <= MAX_WIDTH
 *
 * @note The buffer is statically sized at 2073600 bytes (~2MB).
 *
 * ### Fields
 * | Name       | Type          | Size          |
 * |------------|---------------|---------------|
 * | height     | unsigned long | 8 bytes       |
 * | width      | unsigned long | 8 bytes       |
 * | buffer     | char[]        | 2073600 bytes |
 *
 * **Total Size:** 2073616 bytes
 */
struct Canvas {
        unsigned long height;
        unsigned long width;
        char          buffer[MAX_WIDTH * MAX_HEIGHT]{'\0'};
};

// Canvas must be exactly 1920x1080 chars + 2 unsigned longs (height, width)
static_assert(sizeof(Canvas) == (2073616), "Canvas size must be 2073616 bytes"); // NOLINT

/**
 * @brief Initialize a `Canvas` that can be used for rendering ASCII graphics to a terminal screen
 *
 * @pre 0 < height <= MAX_HEIGHT
 * @pre 0 < width <= MAX_WIDTH
 *
 * @post The first `width` * `height` values in `buffer` are filled with the `fill_char` value
 *
 * @param[out]  canvas_out      The canvas that should be initialized
 * @param[in]   width           Width the canvas is initialized to
 * @param[in]   height          Height the canvas is initialized to
 * @param[in]   fill_char       The character that fills the canvas, defaults to whitespace
 *
 * @return `Error` code enumeration with OK indicating a successful initialization
 */
Error create(Canvas* canvas_out, unsigned long width, unsigned long height, char fill_char);

} // namespace anvil::graphic

#endif // !ANVIL_GRAPHIC_CANVAS_HPP
