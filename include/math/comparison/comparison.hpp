/**
 * @file minmax.hpp
 */
#ifndef ANVIL_MATH_COMPARISON_MINMAX_HPP
#define ANVIL_MATH_COMPARISON_MINMAX_HPP

#include "anvil/types.hpp"

namespace anvil::math::comparison {

/**
 * @brief returns the smaller of two 64 bit integers
 *
 * @param[in] a
 * @param[in] b
 */
u64       min(u64 a, u64 b);

/**
 * @brief returns the larger of two 64 bit integers
 *
 * @param[in] a
 * @param[in] b
 */
u64       max(u64 a, u64 b);

/**
 * @brief returns the smaller of two 32 bit floats
 *
 * @param[in] a
 * @param[in] b
 */
f32       min(f32 a, f32 b);

/**
 * @brief returns the larger of two 32 bit floats
 *
 * @param[in] a
 * @param[in] b
 */
f32       max(f32 a, f32 b);

/**
 */
FloatType classify(anvil::f32 num) noexcept;

/**
 */
bool      is_power_of_two(anvil::u64 x) noexcept;

} // namespace anvil::math::comparison

#endif // !ANVIL_MATH_COMPARISON_MINMAX_HPP
