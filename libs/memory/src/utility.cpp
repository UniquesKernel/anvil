#include "internal/utility.hpp"
#include <cstddef>

[[gnu::pure]]
bool is_power_of_two(const std::size_t x) noexcept {
        return x != 0 && ((x & (x - 1)) == 0);
}
