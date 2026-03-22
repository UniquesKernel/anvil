#include "anvil/math/comparison/comparison.hpp"
#include <cstring>

namespace anvil::math::comparison {

u64 max(const u64 a, const u64 b) noexcept {
        return (a <= b) ? b : a;
}

f32 max(const f32 a, const f32 b) noexcept {
        return (a <= b) ? b : a;
}

u64 min(const u64 a, const u64 b) noexcept {
        return (a < b) ? a : b;
}

f32 min(const f32 a, const f32 b) noexcept {
        return (a < b) ? a : b;
}

FloatType classify(const f32 num) noexcept {
        u32 bits;
        std::memcpy(&bits, &num, sizeof(bits));

        u32 is_signed  = bits >> 31U;
        u32 is_special = ((bits & 0x7F800000U) == 0x7F800000U);
        u32 is_nan     = ((bits & 0x007FFFFFU) != 0);

        return (FloatType)((is_signed << 2U) | (is_special << 1U) | (is_special & is_nan));
}

bool is_power_of_two(const anvil::u64 x) noexcept {
        return x != 0 && ((x & (x - 1)) == 0);
}

} // namespace anvil::math::comparison
