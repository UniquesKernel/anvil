#ifndef ANVIL_TYPES_HPP
#define ANVIL_TYPES_HPP

#include <cstdint>

namespace anvil {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8  = int8_t;

using f64 = double;
using f32 = float;

static_assert(sizeof(f64) == 8, "Anvil requires double to be 64 bits");
static_assert(sizeof(f32) == 4, "Anvil requires float to be 32 bits");

enum FloatType : u8 {
        POS_FINITE = 0, // signed = 0, special = 0, is_nan = 0
        POS_INF    = 2, // signed = 0, special = 1, is_nan = 0
        POS_NAN    = 3, // signed = 0, special = 1, is_nan = 1
        NEG_FINITE = 4, // signed = 1, special = 0, is_nan = 0
        NEG_INF    = 6, // signed = 1, special = 1, is_nan = 0
        NEG_NAN    = 7, // signed = 1, special = 1, is_nan = 1
};
static_assert(sizeof(FloatType) == sizeof(u8), "Anvil requires FloatType to be 8 bits");

} // namespace anvil

#endif // ANVIL_TYPES_HPP
