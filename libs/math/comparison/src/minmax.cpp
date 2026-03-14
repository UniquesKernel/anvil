#include "math/comparison/minmax.hpp"

namespace anvil::math::comparison {

unsigned long min(const unsigned long left, const unsigned long right) {
        return (left < right) ? left : right;
}

unsigned long max(const unsigned long left, const unsigned long right) {
        return (left <= right) ? right : left;
}
} // namespace anvil::math::comparison
