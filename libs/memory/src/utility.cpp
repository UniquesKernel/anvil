#include "internal/utility.hpp"
#include "memory/constants.hpp"

ANVIL_ATTR_PURE bool is_power_of_two(const std::size_t x) {
        return x != 0 && ((x & (x - 1)) == 0);
}