#ifndef ANVIL_MEMORY_TRANSFER_HPP
#define ANVIL_MEMORY_TRANSFER_HPP

#include "memory/constants.hpp"
#include "memory/error.hpp"
#include <cstring>

namespace anvil::memory {

inline bool is_power_of_two(const size_t x) {
        return x != 0 && ((x & (x - 1)) == 0);
}

template <typename Allocator, typename T> 
struct Transfer {
        Allocator* allocator;
};

template <typename Allocator, typename T>
Transfer<Allocator, T> transfer(Allocator* allocator, T* src, const size_t data_size, const size_t alignment) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_POSITIVE(data_size);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, 
                        "alignment was not a power two but was %zu", alignment);

        void* transfer_ptr = reinterpret_cast<void*>(allocator);
        *reinterpret_cast<size_t*>(transfer_ptr) = TRANSFER_MAGIC;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer_ptr) + sizeof(size_t)) = data_size;
        *reinterpret_cast<size_t*>(static_cast<char*>(transfer_ptr) + 2 * sizeof(size_t)) = alignment;
        std::memcpy(static_cast<char*>(transfer_ptr) + 3 * sizeof(size_t), src, data_size);

        return Transfer<Allocator, T>{reinterpret_cast<Allocator*>(transfer_ptr)};
}

template <typename DestAllocator, typename SrcAllocator, typename T>
T* absorb(DestAllocator* allocator, SrcAllocator* src, Error (*destroy_fn)(SrcAllocator**)) {
        ANVIL_INVARIANT_NOT_NULL(allocator);
        ANVIL_INVARIANT_NOT_NULL(src);
        ANVIL_INVARIANT_NOT_NULL(destroy_fn);

        void* src_ptr = static_cast<void*>(src);
        
        if (*reinterpret_cast<size_t*>(src_ptr) != TRANSFER_MAGIC) {
                return nullptr;
        }

        size_t data_size = *reinterpret_cast<size_t*>(static_cast<char*>(src_ptr) + sizeof(size_t));
        size_t alignment = *reinterpret_cast<size_t*>(static_cast<char*>(src_ptr) + 2 * sizeof(size_t));
        
        T* dest = reinterpret_cast<T*>(alloc(allocator, data_size, alignment));

        if (!dest) {
                destroy_fn(&src);
                return nullptr;
        }

        *reinterpret_cast<size_t*>(src_ptr) = 0x0;
        std::memcpy(dest, static_cast<char*>(src_ptr) + 3 * sizeof(size_t), data_size);

        destroy_fn(&src);
        return dest;
}

} // namespace anvil::memory

#endif // ANVIL_MEMORY_TRANSFER_HPP