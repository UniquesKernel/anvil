#include <cstddef>
#include <cstdio>
#include <cstdlib>

int main(void) {
        const int NUM_ALLOCS        = 10000000;
        const int ALLOC_SIZE        = 8; // tiny allocations

        int**     ALLOCATIONS_TRACK = (int**)(malloc((size_t)(ALLOC_SIZE * NUM_ALLOCS)));
        // Benchmark: many tiny allocations (no tracking, no cleanup)
        int       sum               = 0;
        for (int i = 0; i < NUM_ALLOCS; i++) {
                int* ptr             = (int*)(malloc(ALLOC_SIZE));
                ALLOCATIONS_TRACK[i] = ptr;

                if (ptr == nullptr) {
                        printf("ALLOCATION FAILURE at %i\n", i);
                        return 1;
                }

                *ptr  = i;
                sum  += *ptr;
        }

        printf("SUM: %i\n", sum);

        for (int i = 0; i < NUM_ALLOCS; i++) {
                free(ALLOCATIONS_TRACK[i]);
        }
        free((void*)(ALLOCATIONS_TRACK));
        // Note: Memory leaked intentionally for pure malloc benchmark
        return 0;
}
