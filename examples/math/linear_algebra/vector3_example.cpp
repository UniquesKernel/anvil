#include "anvil/types.hpp"
#include "math/linear_algebra/vector.hpp"
#include <cstdio>

using namespace anvil::math;

namespace {
void sum_example() {
        const Vector3 VEC{1.0F, 2.0F, 3.0F};
        const Vector3 VEC2{4.0F, 5.0F, 6.0F};
        const Vector3 VEC_SUM = VEC + VEC2;

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("VEC2: (%.2f, %.2f, %.2f)\n", VEC2.x, VEC2.y, VEC2.z);
        printf("VEC_SUM: (%.2f, %.2f, %.2f)\n", VEC_SUM.x, VEC_SUM.y, VEC_SUM.z);
        printf("\n");
}

void difference_example() {
        const Vector3 VEC{1.0F, 2.0F, 3.0F};
        const Vector3 VEC2{4.0F, 5.0F, 6.0F};
        const Vector3 VEC_DIFF = VEC - VEC2;

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("VEC2: (%.2f, %.2f, %.2f)\n", VEC2.x, VEC2.y, VEC2.z);
        printf("VEC_DIFF: (%.2f, %.2f, %.2f)\n", VEC_DIFF.x, VEC_DIFF.y, VEC_DIFF.z);
        printf("\n");
}

void negation_example() {
        const Vector3 VEC{1.0F, 2.0F, 3.0F};
        const Vector3 NEGATED_VEC = -VEC;

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("NEGATED_VEC: (%.2f, %.2f, %.2f)\n", NEGATED_VEC.x, NEGATED_VEC.y, NEGATED_VEC.z);
        printf("\n");
}

void dot_product_example() {
        const Vector3    VEC{1.0F, 2.0F, 3.0F};
        const Vector3    VEC2{4.0F, 5.0F, 6.0F};
        const anvil::f32 DOT_PRODUCT = VEC * VEC2;

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("VEC2: (%.2f, %.2f, %.2f)\n", VEC2.x, VEC2.y, VEC2.z);
        printf("DOT_PRODUCT: %.2f\n", DOT_PRODUCT);
        printf("\n");
}

void scalar_product_example() {
        const Vector3    VEC{1.0F, 2.0F, 3.0F};
        const anvil::f32 SCALAR         = 3.0F;
        const Vector3    SCALAR_PRODUCT = SCALAR * VEC;

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("SCALAR: %.2f\n", SCALAR);
        printf("SCALAR_PRODUCT: (%.2f, %.2f, %.2f)\n", SCALAR_PRODUCT.x, SCALAR_PRODUCT.y, SCALAR_PRODUCT.z);
        printf("\n");
}

void cross_product_example() {
        const Vector3 VEC{1.0F, 0.0F, 0.0F};
        const Vector3 VEC2{0.0F, 1.0F, 0.0F};
        const Vector3 CROSS_PRODUCT = cross_product(VEC, VEC2);

        printf("VEC: (%.2f, %.2f, %.2f)\n", VEC.x, VEC.y, VEC.z);
        printf("VEC2: (%.2f, %.2f, %.2f)\n", VEC2.x, VEC2.y, VEC2.z);
        printf("CROSS_PRODUCT: (%.2f, %.2f, %.2f)\n", CROSS_PRODUCT.x, CROSS_PRODUCT.y, CROSS_PRODUCT.z);
        printf("\n");
}
} // namespace

int main(void) {
        printf("%s", "======================================\n");
        sum_example();
        printf("%s", "======================================\n");
        difference_example();
        printf("%s", "======================================\n");
        negation_example();
        printf("%s", "======================================\n");
        dot_product_example();
        printf("%s", "======================================\n");
        scalar_product_example();
        printf("%s", "======================================\n");
        cross_product_example();
        return 0;
}