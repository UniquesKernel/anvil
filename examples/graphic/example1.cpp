#include "anvil/error/status.hpp"
#include "anvil/graphic/canvas.hpp"
#include <cstdio>

static constexpr anvil::u64 WIDTH  = 800;
static constexpr anvil::u64 HEIGHT = 400;

int                         main(void) {
        anvil::graphic::Canvas canvas;

        if (anvil::graphic::create(&canvas, WIDTH, HEIGHT, ' ') != OK) {
                return 1;
        }

        if (anvil::graphic::set(&canvas, 0, 'a') != OK) {
                printf("Failed\n");
        }

        printf("%lu\n", canvas.height);
        printf("%lu\n", canvas.width);
        printf("%c\n", canvas.buffer[0]);

        return 0;
}
