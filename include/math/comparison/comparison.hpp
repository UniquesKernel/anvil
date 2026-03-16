/**
 * @file minmax.hpp
 */
#ifndef ANVIL_MATH_COMPARISON_MINMAX_HPP
#define ANVIL_MATH_COMPARISON_MINMAX_HPP

#include "anvil/types.hpp"

namespace anvil::math::comparison {

u64  min(u64 left, u64 right);
u64  max(u64 left, u64 right);

f32  min(f32 left, f32 right);
f32  max(f32 left, f32 right);

bool is_nan(f32 num);
bool is_inf(f32 num);

bool is_power_of_two(anvil::u64 x) noexcept;
} // namespace anvil::math::comparison

#endif // !ANVIL_MATH_COMPARISON_MINMAX_HPP
