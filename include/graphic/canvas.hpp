#ifndef ANVIL_GRAPHIC_CANVAS_HPP
#define ANVIL_GRAPHIC_CANVAS_HPP

#include "error/status.hpp"

namespace anvil::graphic {

static const unsigned long MAX_WIDTH  = 1920;
static const unsigned long MAX_HEIGHT = 1082;

struct Canvas {
        unsigned long height;
        unsigned long width;
        char          buffer[MAX_WIDTH * MAX_HEIGHT]{'\0'};
};

Error create(Canvas* canvas, unsigned long width, unsigned long height, char fill_char);

} // namespace anvil::graphic

#endif // !ANVIL_GRAPHIC_CANVAS_HPP
