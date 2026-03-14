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
        float x = 0.0F;
        float y = 0.0F;
};
static_assert(sizeof(Vector2) == 8, "Vector2 holds two floating-point values of 8 bytes total"); // NOLINT

/**
 * @brief Three-dimensional vector
 *
 * `Vector3` represents a mathematical vector in 3D space with components
 * `x`, `y`, and `z`, stored as single-precision (32-bit) floating-point values.
 */
struct Vector3 {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
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

inline float operator*(const Vector2& a, const Vector2& b) {
        return (a.x * b.x) + (a.y * b.y);
}

inline Vector2 operator*(const float scalar, const Vector2& a) {
        return {a.x * scalar, a.y * scalar};
}

inline Vector2 operator*(const Vector2& a, const float scalar) {
        return scalar * a;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector2& a, const Vector2& b) {
        const float EPSILON = 1e-4F;

        const float DIFF_X = std::abs((a - b).x);
        const float DIFF_Y = std::abs((a - b).y);

        const float SCALE_X = std::max({std::abs(a.x), std::abs(b.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(a.y), std::abs(b.y), 1.0F});

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

inline float operator*(const Vector3& a, const Vector3& b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline Vector3 operator*(const float scalar, const Vector3& a) {
        return {a.x * scalar, a.y * scalar, a.z * scalar};
}

inline Vector3 operator*(const Vector3& a, const float scalar) {
        return scalar * a;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector3& a, const Vector3& b) {
        const float EPSILON = 1e-4F;

        const float DIFF_X = std::abs((a - b).x);
        const float DIFF_Y = std::abs((a - b).y);
        const float DIFF_Z = std::abs((a - b).z);

        const float SCALE_X = std::max({std::abs(a.x), std::abs(b.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(a.y), std::abs(b.y), 1.0F});
        const float SCALE_Z = std::max({std::abs(a.z), std::abs(b.z), 1.0F});

        return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z)) ||
               ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y) && (DIFF_Z <= EPSILON * SCALE_Z));
}
#pragma GCC diagnostic pop

Vector3 cross_product(Vector3 a, Vector3 b);

} // namespace anvil::math

#endif // !ANVIL_MATH_LINEAR_ALGEBRA_VECTOR_HPP