#include "math/linear_algebra/vector.hpp"

namespace anvil::math {

Vector3 cross_product(const Vector3 a, const Vector3 b) {
        return {(a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x)};
}

} // namespace anvil::math