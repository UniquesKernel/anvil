#include "anvil/types.hpp"
#include "error/status.hpp"
#include "memory/resizable_buffer.hpp"
#include "memory/scratch_allocator.hpp"
#include <cstdio>
#include <immintrin.h>

namespace {
void cleanup_allocator(anvil::memory::resizable_buffer::ResizableBuffer** allocator) {
        (void)anvil::memory::resizable_buffer::destroy(allocator);
}
} // namespace

int main(void) {
        const anvil::i64                            NUM_ALLOCS = 10000000;
        // Now requesting 64 bytes (16 integers) per allocation
        const anvil::u64                            ALLOC_SIZE = 64;

        __attribute__((cleanup(cleanup_allocator))) anvil::memory::resizable_buffer::ResizableBuffer* allocator =
            nullptr;

        // 1/16th the allocations! We align to 64 bytes (the exact size of a CPU Cache Line)
        const Error ERR = anvil::memory::resizable_buffer::create(&allocator, ALLOC_SIZE, 64);

        if (ERR != OK) {
                printf("ERROR: %i\n", ERR);
                return 1;
        }

        // --- ILP SETUP (Two parallel tracks) ---
        __m256i     v_sum1 = _mm256_setzero_si256();
        __m256i     v_sum2 = _mm256_setzero_si256();

        // Track 1: [0, 1, 2, 3, 4, 5, 6, 7]
        __m256i     v_i1   = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
        // Track 2: [8, 9, 10, 11, 12, 13, 14, 15]
        __m256i     v_i2   = _mm256_set_epi32(15, 14, 13, 12, 11, 10, 9, 8);

        // Step is now 16
        __m256i     v_step = _mm256_set1_epi32(16);

        // --- UNROLLED LOOP ---
        // Total iterations: 625,000
        anvil::i32* ptr    = (anvil::i32*)(anvil::memory::resizable_buffer::data(allocator));
        if (ptr == nullptr) [[unlikely]] {
                return 1;
        }

        for (anvil::i64 i = 0; i < NUM_ALLOCS; i += 16) {
                // Allocate 64 bytes, aligned to a 64-byte cache line

                // Write 16 integers. CPU will pipeline these stores!
                _mm256_store_si256((__m256i*)ptr, v_i1);
                _mm256_store_si256((__m256i*)(ptr + 8), v_i2); // ptr + 8 ints = 32 bytes forward

                // CPU will route these to different execution ports simultaneously
                v_sum1 = _mm256_add_epi32(v_sum1, v_i1);
                v_sum2 = _mm256_add_epi32(v_sum2, v_i2);

                // Increment both tracks
                v_i1   = _mm256_add_epi32(v_i1, v_step);
                v_i2   = _mm256_add_epi32(v_i2, v_step);
        }

        // --- HORIZONTAL ADDITION ---
        // Combine track 2 into track 1
        v_sum1 = _mm256_add_epi32(v_sum1, v_sum2);

        anvil::i32 sums[8];
        _mm256_storeu_si256((__m256i*)sums, v_sum1);

        anvil::i64 final_sum = 0;
        for (anvil::i32 j = 0; j < 8; ++j) {
                final_sum += sums[j];
        }

        printf("SUM: %ld\n", final_sum);

        return 0;
}
