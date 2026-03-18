/**
 * @file minmax.hpp
 */
#ifndef ANVIL_MATH_COMPARISON_MINMAX_HPP
#define ANVIL_MATH_COMPARISON_MINMAX_HPP

#include "anvil/types.hpp"

namespace anvil::math::comparison {

/**
 * @brief Return the smaller of two 64-bit unsigned integers.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 * @return The smaller value of `a` and `b`.
 */
[[gnu::const, nodiscard]] u64       min(u64 a, u64 b) noexcept;

/**
 * @brief Return the larger of two 64-bit unsigned integers.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 * @return The larger value of `a` and `b`.
 */
[[gnu::const, nodiscard]] u64       max(u64 a, u64 b) noexcept;

/**
 * @brief Return the smaller of two 32-bit floats.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return The smaller value of `a` and `b`.
 */
[[gnu::const, nodiscard]] f32       min(f32 a, f32 b) noexcept;

/**
 * @brief Return the larger of two 32-bit floats.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return The larger value of `a` and `b`.
 */
[[gnu::const, nodiscard]] f32       max(f32 a, f32 b) noexcept;

/**
 * @brief Returns a FloatType enumeration value to indicate floating point category
 *
 * @param[in] num       32-bit number to be evaluated
 * @return An integer value corresponding to the floating point categories:
 *         -NaN, -Inf, -Finite, Finite, Inf, NaN
 */
[[gnu::const, nodiscard]] FloatType classify(anvil::f32 num) noexcept;

/**
 * @brief determines if a u64 value is a power of two
 */
[[gnu::const, nodiscard]] bool      is_power_of_two(anvil::u64 x) noexcept;

} // namespace anvil::math::comparison

#endif // !ANVIL_MATH_COMPARISON_MINMAX_HPP
