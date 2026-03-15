#include "math/comparison/minmax.hpp"

namespace anvil::math::comparison {

u64 min(const u64 left, const u64 right) {
        return (left < right) ? left : right;
}

u64 max(const u64 left, const u64 right) {
        return (left <= right) ? right : left;
}
} // namespace anvil::math::comparison
