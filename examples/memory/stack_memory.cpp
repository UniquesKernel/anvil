#include "error/status.hpp"
#include "memory/stack_allocator.hpp"
#include <cstddef>
#include <cstdio>

namespace {
void cleanup_allocator(anvil::memory::stack_allocator::StackAllocator** allocator) {
        (void)anvil::memory::stack_allocator::destroy(allocator);
}
} // namespace

int main(void) {
        const int                                   NUM_ALLOCS = 10000000;
        const size_t                                ALLOC_SIZE = 8; // tiny allocations

        // Create allocator with enough space
        __attribute__((cleanup(cleanup_allocator))) anvil::memory::stack_allocator::StackAllocator* allocator = nullptr;
        const Error ERR = anvil::memory::stack_allocator::create(&allocator, ALLOC_SIZE * NUM_ALLOCS, alignof(int));

        if (ERR != OK) {
                printf("ERROR: %i\n", ERR);
                return 1;
        }

        // Benchmark: many tiny allocations
        int sum = 0;
        for (int i = 0; i < NUM_ALLOCS; i++) {
                                int* ptr = (int*)(anvil::memory::stack_allocator::alloc(allocator, ALLOC_SIZE, alignof(int)));

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
