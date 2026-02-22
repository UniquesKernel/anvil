#include "error/status.hpp"
#include "memory/scratch_allocator.hpp"
#include <cstdio>

#define DEFER(func) __attribute__((cleanup(func)))

namespace {
void cleanup_allocator(anvil::memory::scratch_allocator::ScratchAllocator** a) noexcept {
        (void)anvil::memory::scratch_allocator::destroy(a);
}
} // namespace

int main(void) {
        const int                                                                    ARRAYSIZE = 100;

        DEFER(cleanup_allocator) anvil::memory::scratch_allocator::ScratchAllocator* allocator = nullptr;
        if (anvil::memory::scratch_allocator::create(&allocator, sizeof(int) * ARRAYSIZE, alignof(int)) != OK) {
                printf("MEMORY ERROR\n");
                return 1;
        }

        int* arr = reinterpret_cast<int*>(anvil::memory::scratch_allocator::alloc(allocator, sizeof(int) * (ARRAYSIZE),
                                                                                  alignof(int)));

        if (arr == nullptr) {
                printf("ALLOCATION FAILURE");
                return 1;
        }

        for (int i = 0; i < ARRAYSIZE; i++) {
                arr[i] = i;
        }

        for (int i = 0; i < ARRAYSIZE; i++) {
                printf("%i\n", arr[i]);
        }

        int sum = 0;
        for (int i = 0; i < ARRAYSIZE; i++) {
                sum += arr[i];
        }

        printf("SUM: %i", sum);
        return 0;
}
