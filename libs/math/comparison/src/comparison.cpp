#include "math/comparison/comparison.hpp"
#include <cstring>

namespace anvil::math::comparison {

[[gnu::const]]
u64 max(const u64 left, const u64 right) {
        return (left <= right) ? right : left;
}

[[gnu::const]]
f32 max(const f32 left, const f32 right) {
        return (left <= right) ? right : left;
}

[[gnu::const]]
u64 min(const u64 left, const u64 right) {
        return (left < right) ? left : right;
}

[[gnu::const]]
f32 min(const f32 left, const f32 right) {
        return (left < right) ? left : right;
}

[[gnu::const]]
bool is_nan(f32 num) {
        u32       bits;
        const u32 EXPONENT_BIT_MASK = 0x7F800000;
        const u32 MANTISSA_BIT_MASK = 0x7FFFFF;
        memcpy(&bits, &num, sizeof(num));
        return (bits & EXPONENT_BIT_MASK) == EXPONENT_BIT_MASK && (bits & MANTISSA_BIT_MASK) != 0;
}

[[gnu::pure]]
bool is_power_of_two(const anvil::u64 x) noexcept {
        return x != 0 && ((x & (x - 1)) == 0);
}

} // namespace anvil::math::comparison
