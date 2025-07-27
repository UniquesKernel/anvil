#include "internal/utility.h"
#include "memory/error.h"

bool __attribute__((pure)) is_power_of_two(const size_t x) {
        return ((x & (x - 1)) == 0);
}