/**
 * @file
 * @brief ASCII canvas type and creation API for terminal rendering.
 * @details Provides the `Canvas` struct and `create` function for allocating
 * and initializing a fixed-size character buffer used to render ASCII graphics
 * in a terminal window.
 */
#ifndef ANVIL_GRAPHIC_CANVAS_HPP
#define ANVIL_GRAPHIC_CANVAS_HPP

#include "anvil/types.hpp"
#include "anvil/error/status.hpp"
#include "anvil/memory/scratch_allocator.hpp"

namespace anvil::graphic {

static const u64 MAX_WIDTH  = 1920;
static const u64 MAX_HEIGHT = 1080;

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
 * ### Fields
 * | Name       | Type                  | Size          |
 * |------------|-----------------------|---------------|
 * | height     | u64                   | 8 bytes       |
 * | width      | u64                   | 8 bytes       |
 * | buffer     | char*                 | 8 bytes       |
 * | allocator  | ScratchAllocator*     | 8 bytes       |
 *
 * **Total Size:** 32 bytes
 */
struct Canvas {
        u64                                          height    = 0;
        u64                                          width     = 0;
        char*                                        buffer    = nullptr;
        memory::ScratchAllocator* allocator = nullptr;
};

static_assert(sizeof(Canvas) == 32, "Canvas size must be 32 bytes"); // NOLINT

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
[[nodiscard]] Error create(Canvas* canvas_out, u64 width, u64 height, char fill_char);

/**
 * @brief Set the value of the buffer index to `character`
 *
 * @pre canvas_out != null
 * @pre index < MAX_WIDTH * MAX_HEIGHT
 *
 * @post position at the (index - 1) is set with the fill_char value
 *
 * @param[out] canvas_out       The canvas whose buffer to write to
 * @param[in] index             The position in the buffer to write to
 * @param[in] character         The character to write to the canvas buffer
 *
 * @return `Error` code enumeration with OK indicating a successful write
 */
[[nodiscard]] Error set(Canvas* canvas_out, u64 index, char character);

[[nodiscard]] Error destroy(Canvas* canvas_out);

} // namespace anvil::graphic

#endif // !ANVIL_GRAPHIC_CANVAS_HPP
