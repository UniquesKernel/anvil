#include "anvil/types.hpp"
#include "anvil/error/status.hpp"
#include "anvil/memory/scratch_allocator.hpp"
#include <cstdio>
#include <cstring>
#include <immintrin.h>

namespace {
void cleanup_allocator(anvil::memory::ScratchAllocator** allocator) {
        (void)anvil::memory::destroy(allocator);
}
} // namespace

int main(void) {
        const anvil::i32                            NUM_ALLOCS                                 = 1024;
        const anvil::i32                            ALLOC_SIZE                                 = 64;

        __attribute__((cleanup(cleanup_allocator))) anvil::memory::ScratchAllocator* allocator = nullptr;

        const Error ERR = anvil::memory::create(&allocator, ((anvil::u64)NUM_ALLOCS * ALLOC_SIZE), 64);

        if (ERR != OK) {
                printf("ERROR: %i\n", ERR);
                return 1;
        }

        // --- ILP SETUP (Two parallel tracks) ---
        __m256i             v_sum1         = _mm256_setzero_si256();
        __m256i             v_sum2         = _mm256_setzero_si256();

        // Track 1: [0, 1, 2, 3, 4, 5, 6, 7]
        alignas(32) __m256i v_i1           = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
        // Track 2: [8, 9, 10, 11, 12, 13, 14, 15]
        alignas(32) __m256i v_i2           = _mm256_set_epi32(16, 15, 14, 13, 12, 11, 10, 9);

        __m256i             v_step         = _mm256_set1_epi32(16);
        int                 count          = NUM_ALLOCS / 16;
        unsigned long long  starting_count = __rdtsc();
        while (count--) {
                anvil::i32* ptr = (anvil::i32*)(anvil::memory::alloc(allocator, ALLOC_SIZE, 64));

                if (ptr == nullptr) [[unlikely]] {
                        return 1;
                }

                _mm256_store_si256((__m256i*)ptr, v_i1);
                _mm256_store_si256((__m256i*)(ptr + 8), v_i2); // ptr + 8 ints = 32 bytes forward

                v_sum1 = _mm256_add_epi32(v_sum1, v_i1);
                v_sum2 = _mm256_add_epi32(v_sum2, v_i2);

                v_i1   = _mm256_add_epi32(v_i1, v_step);
                v_i2   = _mm256_add_epi32(v_i2, v_step);
                // (void)anvil::memory::reset(allocator);
        }

        // --- HORIZONTAL ADDITION ---
        // Combine track 2 into track 1
        v_sum1 = _mm256_add_epi32(v_sum1, v_sum2);

        alignas(32) anvil::i32 sums[8];
        _mm256_storeu_si256((__m256i*)sums, v_sum1);

        anvil::i32 final_sum = 0;
        for (anvil::i32 j = 0; j < 8; ++j) {
                final_sum += sums[j];
        }
        unsigned long long end_count = __rdtsc();
        printf("SUM: %i\n", final_sum);
        printf("CYCLES: %llu\n", end_count - starting_count);

        return 0;
}
