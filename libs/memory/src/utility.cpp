#include "internal/utility.hpp"
#include "memory/error.hpp"

bool __attribute__((pure)) is_power_of_two(const size_t x) {
        return ((x & (x - 1)) == 0);
}