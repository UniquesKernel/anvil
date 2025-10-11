#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"

bool __attribute__((pure)) is_power_of_two(const size_t x) {
        return x != 0 && ((x & (x - 1)) == 0);
}