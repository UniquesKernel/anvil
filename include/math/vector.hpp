/**
 * @file vector.hpp
 * @brief Lightweight 2D/3D vector types and basic arithmetic operators.
 *
 * Provides `Vector2` and `Vector3` structs with inline operators for
 * addition, subtraction, scalar multiplication, dot product, negation,
 * and approximate equality. Equality uses a relative tolerance to reduce
 * sensitivity to floating-point rounding.
 */
#ifndef ANVIL_MATH_VECTOR_HPP
#define ANVIL_MATH_VECTOR_HPP

#include <algorithm>
#include <cstdlib>
namespace anvil::math {

/**
 * @brief Two-dimensional vector
 *
 * `Vector2` represents a mathematical vector in 2D space with components,
 * `x` and `y`, approximated by single-precision (32-bit) floating-point arithmetic.
 *
 * Supported operations:
 *      - Vector addition and subtraction
 *      - Scalar multiplication (Using the `*` operator)
 *      - Dot Product (using the `*` operator)
 *      - Negation
 *
 * @note The multiplication operator (`*`) is overloaded:
 *      - Vector2 * Vector2 → `float` (dot product)
 *      - Vector2 * `float` → Vector2 (scalar multiplication)
 *
 * @note Equality uses relative tolerance (ε = 1e-4) to account for
 *       floating-point rounding errors.
 *
 * ### Fields
 * | Name | Type      | Size    |
 * |------|-----------|---------|
 * | x    | `float`   | 4 bytes |
 * | y    | `float`   | 4 bytes |
 *
 * **Total Size:** 8 bytes
 */
struct Vector2 {
        float x = 0.0F;
        float y = 0.0F;
};
static_assert(sizeof(Vector2) == 8, "Vector2 holds two floating-point values of 8 bytes total"); // NOLINT

/**
 * @brief Three-dimensional vector
 *
 * `Vector3` represents a mathematical vector in 3D space with components,
 * `x`, `y`, and `z`, approximated by single-precision (32-bit) floating-point arithmetic.
 *
 * Supported operations:
 *      - Vector addition and subtraction
 *      - Scalar multiplication (Using the `*` operator)
 *      - Dot Product (using the `*` operator)
 *      - Negation
 *
 * @note The multiplication operator (`*`) is overloaded:
 *      - Vector3 * Vector3 → `float` (dot product)
 *      - Vector3 * `float` → Vector3 (scalar multiplication)
 *
 * @note Equality uses relative tolerance (ε = 1e-4) to account for
 *       floating-point rounding errors.
 *
 * ### Fields
 * | Name | Type      | Size    |
 * |------|-----------|---------|
 * | x    | `float`   | 4 bytes |
 * | y    | `float`   | 4 bytes |
 * | z    | `float`   | 4 bytes |
 *
 * **Total Size:** 12 bytes
 */
struct Vector3 {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
};
static_assert(sizeof(Vector3) == 12, "Vector3 holds three floating-point values of 12 bytes total"); // NOLINT

/* ================================================================================
 *                                      VECTOR 2
 *  ================================================================================ */

/**
 * @brief Vector addition in two dimensions
 */
inline Vector2 operator+(const Vector2& A, const Vector2& B) {
        return {A.x + B.x, A.y + B.y};
}

/**
 * @brief Vector subtraction in two dimentsions
 */
inline Vector2 operator-(const Vector2& A, const Vector2& B) {
        return {A.x - B.x, A.y - B.y};
}

/**
 * @brief Vector negation in two dimensions
 */
inline Vector2 operator-(const Vector2& A) {
        return {-A.x, -A.y};
}

/**
 * @brief Dot product over a two-dimensional space
 */
inline float operator*(const Vector2& A, const Vector2& B) {
        return (A.x * B.x) + (A.y * B.y);
}

/**
 * @brief Scalar product over a two-dimensional space
 */
inline Vector2 operator*(const float SCALAR, const Vector2& A) {
        return {A.x * SCALAR, A.y * SCALAR};
}

/**
 * @brief Scalar product over a two-dimensional space
 */
inline Vector2 operator*(const Vector2& A, const float SCALAR) {
        return SCALAR * A;
}

/**
 * @brief equality between two-dimensional vectors, using scaling approximations
 */
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector2& A, const Vector2& B) {
        const float EPSILON = 1e-4F;

        const float DIFF_X  = std::abs((A - B).x);
        const float DIFF_Y  = std::abs((A - B).y);

        const float SCALE_X = std::max({std::abs(A.x), std::abs(B.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(A.y), std::abs(B.y), 1.0F});

        return ((A.x == B.x) && (A.y == B.y)) || ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y));
}

/* ================================================================================
 *                                      VECTOR 3
 *  ================================================================================ */

/**
 * @brief Vector addition in three dimensions
 */
inline Vector3 operator+(const Vector3& A, const Vector3& B) {
        return {A.x + B.x, A.y + B.y, A.z + B.z};
}

/**
 * @brief Vector subtraction in three dimentsions
 */
inline Vector3 operator-(const Vector3& A, const Vector3& B) {
        return {A.x - B.x, A.y - B.y, A.z - B.z};
}

/**
 * @brief Vector negation in three dimensions
 */
inline Vector3 operator-(const Vector3& A) {
        return {-A.x, -A.y, -A.z};
}

/**
 * @brief Dot product over a three-dimensional space
 */
inline float operator*(const Vector3& A, const Vector3& B) {
        return (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
}

/**
 * @brief Scalar product over a three-dimensional space
 */
inline Vector3 operator*(const float SCALAR, const Vector3& A) {
        return {A.x * SCALAR, A.y * SCALAR, A.z * SCALAR};
}

/**
 * @brief Scalar product over a three-dimensional space
 */
inline Vector3 operator*(const Vector3& A, const float SCALAR) {
        return SCALAR * A;
}

/**
 * @brief equality between three-dimensional vectors, using scaling approximations
 */
#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector3& A, const Vector3& B) {
        const float EPSILON = 1e-4F;

        const float DIFF_X  = std::abs((A - B).x);
        const float DIFF_Y  = std::abs((A - B).y);
        const float DIFF_Z  = std::abs((A - B).z);

        const float SCALE_X = std::max({std::abs(A.x), std::abs(B.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(A.y), std::abs(B.y), 1.0F});
        const float SCALE_Z = std::max({std::abs(A.z), std::abs(B.z), 1.0F});

        return ((A.x == B.x) && (A.y == B.y) && (A.z == B.z)) ||
               ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y) && DIFF_Z <= EPSILON * SCALE_Z);
}

/**
 * @brief Cross product takes two Vector3 and produces a new vector, which is perpendicular to both.
 *
 * @param[in] A         The first vector in the cross product
 * @param[in] B         The second vector in the cross product
 *
 * @return Vector3 perpendicular to both A and B
 *
 * @note The cross product is only defined in three dimensions.
 */
Vector3 cross_product(Vector3 A, Vector3 B);

} // namespace anvil::math

#endif // !ANVIL_MATH_VECTOR_HPP
