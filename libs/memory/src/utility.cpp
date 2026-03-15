#include "internal/utility.hpp"

[[gnu::pure]]
bool is_power_of_two(const anvil::u64 x) noexcept {
        return x != 0 && ((x & (x - 1)) == 0);
}
