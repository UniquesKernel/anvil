#include "math/linear_algebra/vector.hpp"
#include <cstdio>

using namespace anvil::math;

namespace {
void sum_example() {
        const Vector2 VEC{1.0F, 2};
        const Vector2 VEC2{3.0F, 4.0F};
        const Vector2 VEC_SUM = VEC + VEC2;

        printf("VEC: (%.2f, %.2f)\n", VEC.x, VEC.y);
        printf("VEC2: (%.2f, %.2f)\n", VEC2.x, VEC2.y);
        printf("VEC_SUM: (%.2f, %.2f)\n", VEC_SUM.x, VEC_SUM.y);
        printf("\n");
}

void difference_example() {
        const Vector2 VEC{1.0F, 2};
        const Vector2 VEC2{3.0F, 4.0F};
        const Vector2 VEC_DIFF = VEC - VEC2;

        printf("VEC: (%.2f, %.2f)\n", VEC.x, VEC.y);
        printf("VEC2: (%.2f, %.2f)\n", VEC2.x, VEC2.y);
        printf("VEC_DIFF: (%.2f, %.2f)\n", VEC_DIFF.x, VEC_DIFF.y);
        printf("\n");
}

void negation_example() {
        const Vector2 VEC{1.0F, 2};
        const Vector2 NEGATED_VEC = -VEC;

        printf("VEC: (%.2f, %.2f)\n", VEC.x, VEC.y);
        printf("NEGATED_VEC: (%.2f, %.2f)\n", NEGATED_VEC.x, NEGATED_VEC.y);
        printf("\n");
}

void dot_product_example() {
        const Vector2 VEC{1.0F, 2};
        const Vector2 VEC2{3.0F, 4.0F};
        const float   DOT_PRODUCT = VEC * VEC2;

        printf("VEC: (%.2f, %.2f)\n", VEC.x, VEC.y);
        printf("VEC2: (%.2f, %.2f)\n", VEC2.x, VEC2.y);
        printf("DOT_PRODUCT: %.2f\n", DOT_PRODUCT);
        printf("\n");
}

void scalar_product_example() {
        const Vector2 VEC{1.0F, 2};
        const float   SCALAR         = 3.0F;
        const Vector2 SCALAR_PRODUCT = SCALAR * VEC;

        printf("VEC: (%.2f, %.2f)\n", VEC.x, VEC.y);
        printf("SCALAR: %.2f\n", SCALAR);
        printf("SCALAR_PRODUCT: (%.2f, %.2f)\n", SCALAR_PRODUCT.x, SCALAR_PRODUCT.y);
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
        return 0;
}