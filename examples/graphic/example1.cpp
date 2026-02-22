#include "error/status.hpp"
#include "graphic/canvas.hpp"
#include <cstdio>

static constexpr unsigned long WIDTH  = 800;
static constexpr unsigned long HEIGHT = 400;

int                            main(void) {
        anvil::graphic::Canvas canvas;

        if (anvil::graphic::create(&canvas, WIDTH, HEIGHT, ' ') != OK) {
                return 1;
        }

        if (anvil::graphic::set(&canvas, 0, 'a') != OK) {
                printf("Failed\n");
        }

        printf("%lu\n", canvas.height);
        printf("%lu\n", canvas.width);
        printf("%c", canvas.buffer[0]);
        return 0;
}
