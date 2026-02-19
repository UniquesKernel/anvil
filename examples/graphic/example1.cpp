#include "error/status.hpp"
#include "graphic/canvas.hpp"
#include <cstdio>

static constexpr unsigned long WIDTH = 800;
static constexpr unsigned long HEIGHT = 400;
int main (void) {
        anvil::graphic::Canvas canvas;

        const unsigned long ERR = anvil::graphic::create(&canvas, WIDTH, HEIGHT, ' ');

        if (ERR != OK) {
                return 1;
        }

        const auto& func = [&](const anvil::graphic::Canvas& canvas){printf("%lu", canvas.width);};
        func(canvas);

        printf("%lu\n", canvas.height);
        printf("%lu\n", canvas.width);
        return 0;
}
