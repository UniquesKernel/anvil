#ifndef ANVIL_MATH_VECTOR_HPP
#define ANVIL_MATH_VECTOR_HPP

#include <algorithm>
#include <cstdlib>
namespace anvil::math {
struct Vector2 {
        float x = 0.0F;
        float y = 0.0F;
};

struct Vector3 {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
};

// ================== Vector2 ===================
inline Vector2 operator+(const Vector2& A, const Vector2& B) {
        return {A.x + B.x, A.y + B.y};
}

inline Vector2 operator-(const Vector2& A, const Vector2& B) {
        return {A.x - B.x, A.y - B.y};
}

inline Vector2 operator-(const Vector2& A) {
        return {-A.x, -A.y};
}

inline float operator*(const Vector2& A, const Vector2& B) {
        return (A.x * B.x) + (A.y * B.y);
}

inline Vector2 operator*(const float SCALAR, const Vector2 A) {
        return {A.x * SCALAR, A.y * SCALAR};
}

inline Vector2 operator*(const Vector2& A, const float SCALAR) {
        return SCALAR * A;
}

#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector2& A, const Vector2& B) {
        const float EPSILON = 1e-4F;

        const float DIFF_X  = std::abs((A - B).x);
        const float DIFF_Y  = std::abs((A - B).y);

        const float SCALE_X = std::max({std::abs(A.x), std::abs(B.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(A.y), std::abs(B.y), 1.0F});

        return ((A.x == B.x) && (A.y == B.y)) || ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y));
}

// ================== Vector3 ===================

inline Vector3 operator+(const Vector3& A, const Vector3& B) {
        return {A.x + B.x, A.y + B.y, A.z + B.z};
}

inline Vector3 operator-(const Vector3& A, const Vector3& B) {
        return {A.x - B.x, A.y - B.y, A.z - B.z};
}

inline Vector3 operator-(const Vector3& A) {
        return {-A.x, -A.y, -A.z};
}

inline float operator*(const Vector3& A, const Vector3& B) {
        return (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
}

inline Vector3 operator*(const float SCALAR, const Vector3 A) {
        return {A.x * SCALAR, A.y * SCALAR, A.z * SCALAR};
}

inline Vector3 operator*(const Vector3& A, const float SCALAR) {
        return SCALAR * A;
}

#pragma GCC diagnostic ignored "-Wfloat-equal"
inline bool operator==(const Vector3& A, const Vector3& B) {
        const float EPSILON = 1e-4F;

        const float DIFF_X  = std::abs((A - B).x);
        const float DIFF_Y  = std::abs((A - B).y);
        const float DIFF_Z  = std::abs((A - B).z);

        const float SCALE_X = std::max({std::abs(A.x), std::abs(B.x), 1.0F});
        const float SCALE_Y = std::max({std::abs(A.y), std::abs(B.y), 1.0F});
        const float SCALE_Z = std::max({std::abs(A.z), std::abs(B.z), 1.0F});

        return ((A.x == B.x) && (A.y == B.y)) ||
               ((DIFF_X <= EPSILON * SCALE_X) && (DIFF_Y <= EPSILON * SCALE_Y) && DIFF_Z <= EPSILON * SCALE_Z);
}

} // namespace anvil::math

#endif // !ANVIL_MATH_VECTOR_HPP
