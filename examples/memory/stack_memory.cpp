#include "error/status.hpp"
#include "memory/stack_allocator.hpp"
#include <cstddef>
#include <cstdio>

const size_t CAPACITY        = 1024;
const size_t ALIGNMENT       = 8;
const size_t ALLOCATION_SIZE = sizeof(int) * 10;
int          main() {
        anvil::memory::stack_allocator::StackAllocator* allocator = nullptr;

        if (anvil::memory::stack_allocator::create(&allocator, CAPACITY, ALIGNMENT) != OK) {
                return 1;
        }

        (void)anvil::memory::stack_allocator::record(allocator);
        int* const ARRAY =
            static_cast<int*>(anvil::memory::stack_allocator::alloc(allocator, ALLOCATION_SIZE, alignof(int)));

        for (int i = 0; i < ALLOCATION_SIZE / sizeof(int); i++) {
                ARRAY[i] = i;
        }

        for (int i = 0; i < ALLOCATION_SIZE / sizeof(int); i++) {
                printf("%i\n", ARRAY[i]);
        }

        (void)anvil::memory::stack_allocator::unwind(allocator);

        int* const ARRAY2 =
            static_cast<int*>(anvil::memory::stack_allocator::alloc(allocator, ALLOCATION_SIZE, alignof(int)));

        for (int i = 0; i < ALLOCATION_SIZE / sizeof(int); i++) {
                ARRAY2[i] = i * 2;
        }

        printf("\nAFTER UNWIND\n\n");

        for (int i = 0; i < ALLOCATION_SIZE / sizeof(int); i++) {
                printf("%i\n", ARRAY[i]);
        }

        return 0;
}
