/**
 * @file vector.hpp
 * @brief Lightweight 2D/3D vector types and basic arithmetic operators.
 *
 * Provides `Vector2` and `Vector3` structs with inline operators for
 * addition, subtraction, scalar multiplication, dot product, negation,
 * and approximate equality. Equality uses a relative tolerance to reduce
 * sensitivity to floating-point rounding.
 */
#ifndef ANVIL_MATH_LINEAR_ALGEBRA_VECTOR_HPP
#define ANVIL_MATH_LINEAR_ALGEBRA_VECTOR_HPP

#include "anvil/types.hpp"
#include <algorithm>
#include <cstdlib>

namespace anvil::math {

/**
 * @brief Two-dimensional vector
 *
 * `Vector2` represents a mathematical vector in 2D space with components
 * `x` and `y`, stored as single-precision (32-bit) floating-point values.
 */
struct Vector2 {
        f32 x = 0.0F;
        f32 y = 0.0F;
};
static_assert(sizeof(Vector2) == 8, "Vector2 holds two floating-point values of 8 bytes total"); // NOLINT

/**
 * @brief Three-dimensional vector
 *
 * `Vector3` represents a mathematical vector in 3D space with components
 * `x`, `y`, and `z`, stored as single-precision (32-bit) floating-point values.
 */
struct Vector3 {
        f32 x = 0.0F;
        f32 y = 0.0F;
        f32 z = 0.0F;
};
static_assert(sizeof(Vector3) == 12, "Vector3 holds three floating-point values of 12 bytes total"); // NOLINT

inline Vector2 operator+(const Vector2& a, const Vector2& b) {
        return {a.x + b.x, a.y + b.y};
}

inline Vector2 operator-(const Vector2& a, const Vector2& b) {
        return {a.x - b.x, a.y - b.y};
}

inline Vector2 operator-(const Vector2& a) {
        return {-a.x, -a.y};
}

inline f32 operator*(const Vector2& a, const Vector2& b) {
        return (a.x * b.x) + (a.y * b.y);
}

inline Vector2 operator*(const f32 scalar, const Vector2& a) {
        return {a.x * scalar, a.y * scalar};
}

inline Vector2 operator*(const Vector2& a, const f32 scalar) {
        return scalar * a;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector2& a, const Vector2& b) {
        const f32 EPSILON = 1e-4F;

        const f32 DIFF_X  = std::abs((a - b).x);
        const f32 DIFF_Y  = std::abs((a - b).y);

        const f32 SCALE_X = std::max({std::abs(a.x), std::abs(b.x), 1.0F});
        const f32 SCALE_Y = std::max({std::abs(a.y), std::abs(b.y), 1.0F});

        return ((a.x == b.x) && (a.y == b.y)) || ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y));
}
#pragma GCC diagnostic pop

inline Vector3 operator+(const Vector3& a, const Vector3& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vector3 operator-(const Vector3& a, const Vector3& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 operator-(const Vector3& a) {
        return {-a.x, -a.y, -a.z};
}

inline f32 operator*(const Vector3& a, const Vector3& b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline Vector3 operator*(const f32 scalar, const Vector3& a) {
        return {a.x * scalar, a.y * scalar, a.z * scalar};
}

inline Vector3 operator*(const Vector3& a, const f32 scalar) {
        return scalar * a;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector3& a, const Vector3& b) {
        const f32 EPSILON = 1e-4F;

        const f32 DIFF_X  = std::abs((a - b).x);
        const f32 DIFF_Y  = std::abs((a - b).y);
        const f32 DIFF_Z  = std::abs((a - b).z);

        const f32 SCALE_X = std::max({std::abs(a.x), std::abs(b.x), 1.0F});
        const f32 SCALE_Y = std::max({std::abs(a.y), std::abs(b.y), 1.0F});
        const f32 SCALE_Z = std::max({std::abs(a.z), std::abs(b.z), 1.0F});

        return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z)) ||
               ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y) && (DIFF_Z <= EPSILON * SCALE_Z));
}
#pragma GCC diagnostic pop

Vector3 cross_product(Vector3 a, Vector3 b);

} // namespace anvil::math

#endif // !ANVIL_MATH_LINEAR_ALGEBRA_VECTOR_HPP
