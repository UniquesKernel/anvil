#include <cstddef>
#include <cstdio>
#include <cstdlib>

int main(void) {
        const int NUM_ALLOCS        = 10000000;
        const int ALLOC_SIZE        = 8; // tiny allocations

        int**     ALLOCATIONS_TRACK = static_cast<int**>(malloc(static_cast<size_t>(ALLOC_SIZE * NUM_ALLOCS)));
        // Benchmark: many tiny allocations (no tracking, no cleanup)
        int       sum               = 0;
        for (int i = 0; i < NUM_ALLOCS; i++) {
                int* ptr             = static_cast<int*>(malloc(ALLOC_SIZE));
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
        free(reinterpret_cast<void*>(ALLOCATIONS_TRACK));
        // Note: Memory leaked intentionally for pure malloc benchmark
        return 0;
}
