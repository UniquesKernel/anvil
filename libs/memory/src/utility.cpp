#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"

bool __attribute__((pure)) is_power_of_two(const size_t x) {
        return x != 0 && ((x & (x - 1)) == 0);
}

template <typename Allocator, typename T> struct Transfer {
        Allocator* allocator;
};

template <typename Allocator, typename T>
Transfer<Allocator*, T> transfer(Allocator* allocator, T* src, const size_t data_size, const size_t alignment) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_RANGE(data_size, 1, allocator->capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "alignment was not a power two but was %zu",
                        alignment);

        void* transfer                                                            = reinterpret_cast<void*>(allocator);
        *reinterpret_cast<size_t*>(transfer)                                      = anvil::memory::TRANSFER_MAGIC;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + sizeof(size_t)) = data_size;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer) + 2 * sizeof(size_t)) = alignment;
        memcpy(static_cast<char*>(transfer) + 3 * sizeof(size_t), src, data_size);

        return Transfer<Allocator*, T>{reinterpret_cast<Allocator*>(transfer)};
}

template <typename DestAllocator, typename SrcAllocator, typename T>
T* absorb(DestAllocator* allocator, SrcAllocator* src, Error (*destroy_fn)(SrcAllocator**)) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_NOT_NULL(destroy_fn);

        if (*reinterpret_cast<size_t*>(src) != anvil::memory::TRANSFER_MAGIC) {
                return NULL;
        }

        size_t data_size = *reinterpret_cast<size_t*>(static_cast<char*>(src) + sizeof(size_t));
        size_t alignment = *reinterpret_cast<size_t*>(static_cast<char*>(src) + 2 * sizeof(size_t));
        T*     dest      = reinterpret_cast<T*>(alloc(allocator, data_size, alignment));

        if (!dest) {
                destroy_fn(&src);
                return NULL;
        }

        *reinterpret_cast<size_t*>(src) = 0x0;
        memcpy(dest, static_cast<char*>(src) + 3 * sizeof(size_t), data_size);

        destroy_fn(&src);
        return dest;
}