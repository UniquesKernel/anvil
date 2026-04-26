#include "anvil/types.hpp"
#include "anvil/error/assert.hpp"
#include "anvil/error/status.hpp"
#include "anvil/memory/stack_allocator.hpp"
#include <cstdio>

namespace {
void cleanup_allocator(anvil::memory::StackAllocator** allocator) {
        (void)anvil::memory::destroy(allocator);
}
} // namespace

int main(void) {
        const anvil::i32                            NUM_ALLOCS                               = 10000000;
        const anvil::u64                            ALLOC_SIZE                               = 8; // tiny allocations

        __attribute__((cleanup(cleanup_allocator))) anvil::memory::StackAllocator* allocator = nullptr;
        const anvil::Error ERR = anvil::memory::create(&allocator, ALLOC_SIZE * NUM_ALLOCS, alignof(anvil::i32));

        if (ERR != anvil::OK) {
                printf("ERROR: %i\n", ERR);
                return 1;
        }

        anvil::i32 sum = 0;
        for (anvil::i32 i = 0; i < NUM_ALLOCS; i++) {
                anvil::i32* ptr = (anvil::i32*)(anvil::memory::alloc(allocator, ALLOC_SIZE, alignof(anvil::i32)));

                if (ptr == nullptr) {
                        printf("ALLOCATION FAILURE at %i\n", i);
                        return 1;
                }

                *ptr  = i;
                sum  += *ptr;
        }

        printf("SUM: %i\n", sum);

        return 0;
}
