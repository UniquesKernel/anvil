#include "math/vector.hpp"

namespace anvil::math {

Vector3 cross_product(const Vector3 A, const Vector3 B) {
        return {(A.y * B.z) - (A.z * B.y), (A.z * B.x) - (A.x * B.z), (A.x * B.y) - (A.y * B.x)};
}

} // namespace anvil::math
