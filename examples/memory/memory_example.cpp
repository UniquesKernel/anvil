#include "anvil/error/status.hpp"
#include "anvil/memory/scratch_allocator.hpp"
#include "anvil/types.hpp"
#include <cstdio>
#include <cstring>
#include <immintrin.h>

namespace {
void cleanup_allocator(anvil::memory::ScratchAllocator** allocator) {
        (void)anvil::memory::destroy(allocator);
}
} // namespace

int main(void) {
        int total = 100;
        while (--total) {
                const anvil::i64 NUM_ALLOCS = (anvil::i64)1024 * (anvil::i64)1024;
                const anvil::i64 ALLOC_SIZE = 64;

                // Total size = (Data blocks) + (The array of pointers to those blocks) + Padding
                const size_t     TOTAL_NEEDED =
                    ((size_t)NUM_ALLOCS * ALLOC_SIZE) + (sizeof(anvil::i64*) * NUM_ALLOCS) + 1024;

                __attribute__((cleanup(cleanup_allocator))) anvil::memory::ScratchAllocator* allocator = nullptr;
                const anvil::Error ERR = anvil::memory::create(&allocator, TOTAL_NEEDED, 64);

                if (ERR != anvil::OK || allocator == nullptr) {
                        return 1;
                }

                // Phase 1: Storage Allocation (Mirroring std::vector reserve)
                anvil::i64** ptr_arr =
                    (anvil::i64**)(anvil::memory::alloc(allocator, sizeof(anvil::i64*) * NUM_ALLOCS, 64));

                anvil::i64         sum            = 0;
                unsigned long long starting_count = __rdtsc();

                // Phase 2: Allocation & Storing (Mirroring the first loop)
                for (anvil::i64 i = 0; i < NUM_ALLOCS; i++) {
                        anvil::i64* ptr = (anvil::i64*)(anvil::memory::alloc(allocator, ALLOC_SIZE, 64));
                        if (ptr == nullptr) [[unlikely]] {
                                return 1;
                        }

                        *ptr       = i;   // Set the value
                        ptr_arr[i] = ptr; // Store the address
                }

                // Phase 3: Accessing (Mirroring the 'sum' loop)
                // This is what tests your cache locality!
                for (anvil::i64 i = 0; i < NUM_ALLOCS; i++) {
                        sum += *ptr_arr[i];
                }

                unsigned long long end_count = __rdtsc();
                // Phase 4: Freeing (Handled by __attribute__ cleanup)
                printf("SUM: %li\n", sum);
                printf("CYCLES: %llu\n", end_count - starting_count);
        }
        return 0;
}
