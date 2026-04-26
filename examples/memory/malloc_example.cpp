#include "anvil/types.hpp"
#include <cstdio>
#include <cstdlib>
#include <immintrin.h>
#include <vector>

int main(void) {
        int total = 100;
        while (--total) {
                const anvil::i64         NUM_ALLOCS = (anvil::i64)1024 * (anvil::i64)1024;
                const anvil::i64         ALLOC_SIZE = (anvil::i64)64;

                std::vector<anvil::i64*> ptr_arr;
                ptr_arr.reserve(NUM_ALLOCS);
                unsigned long long starting_count = __rdtsc();

                anvil::i64         sum            = (anvil::i64)0;
                for (anvil::i64 i = 0; i < NUM_ALLOCS; i++) {
                        anvil::i64* const ptr = (anvil::i64*)(malloc(ALLOC_SIZE));
                        if (ptr == nullptr) [[unlikely]] {
                                return 1;
                        }
                        *ptr = i;
                        ptr_arr.push_back(ptr);
                }
                for (const anvil::i64* const& ptr : ptr_arr) {
                        sum += *ptr;
                }

                for (anvil::i64 i = 0; i < ptr_arr.size(); i++) {
                        free(ptr_arr[i]);
                }

                unsigned long long end_count = __rdtsc();
                printf("SUM: %li\n", sum);
                printf("CYCLES: %llu\n", end_count - starting_count);
        }
        return 0;
}
