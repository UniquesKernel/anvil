#include "anvil/types.hpp"
#include <cstdio>
#include <cstdlib>
#include <immintrin.h>

int main(void) {
        const anvil::i32 NUM_ALLOCS = 10000000;
        const anvil::u64 ALLOC_SIZE = 64; // tiny allocations

        // --- ILP SETUP (Two parallel tracks) ---
        __m256i          v_sum1     = _mm256_setzero_si256();
        __m256i          v_sum2     = _mm256_setzero_si256();

        // Track 1: [0, 1, 2, 3, 4, 5, 6, 7]
        __m256i          v_i1       = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
        // Track 2: [8, 9, 10, 11, 12, 13, 14, 15]
        __m256i          v_i2       = _mm256_set_epi32(15, 14, 13, 12, 11, 10, 9, 8);

        // Step is now 16
        __m256i          v_step     = _mm256_set1_epi32(16);

        // --- UNROLLED LOOP ---
        // Total iterations: 625,000
        anvil::i32*      ptr        = (anvil::i32*)(malloc(ALLOC_SIZE));
        for (anvil::i32 i = 0; i < NUM_ALLOCS; i += 16) {
                // Allocate 64 bytes, aligned to a 64-byte cache line

                if (ptr == nullptr) [[unlikely]] {
                        return 1;
                }

                // Write 16 integers. CPU will pipeline these stores!
                _mm256_stream_si256((__m256i*)ptr, v_i1);
                _mm256_stream_si256((__m256i*)(ptr + 8), v_i2); // ptr + 8 ints = 32 bytes forward

                // CPU will route these to different execution ports simultaneously
                v_sum1 = _mm256_add_epi32(v_sum1, v_i1);
                v_sum2 = _mm256_add_epi32(v_sum2, v_i2);

                // Increment both tracks
                v_i1   = _mm256_add_epi32(v_i1, v_step);
                v_i2   = _mm256_add_epi32(v_i2, v_step);
        }
        free(ptr);

        // --- HORIZONTAL ADDITION ---
        // Combine track 2 into track 1
        v_sum1 = _mm256_add_epi32(v_sum1, v_sum2);

        anvil::i32 sums[8];
        _mm256_storeu_si256((__m256i*)sums, v_sum1);

        anvil::i32 final_sum = 0;
        for (anvil::i32 j = 0; j < 8; ++j) {
                final_sum += sums[j];
        }

        printf("SUM: %i\n", final_sum);

        return 0;
}
