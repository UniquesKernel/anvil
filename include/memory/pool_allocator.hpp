#ifndef ANVIL_MEMORY_POOL_ALLOCATOR_HPP
#define ANVIL_MEMORY_POOL_ALLOCATOR_HPP

#include "error.hpp"
#include <cstddef>

typedef struct pool_allocator_t PoolAllocator;

namespace anvil {
namespace memory {
namespace pool_allocator {

using std::size_t;

PoolAllocator* anvil_memory_pool_allocator_create(const size_t object_size, const size_t object_count,
                                                  const size_t alignment);
Error          anvil_memory_pool_allocator_destroy(PoolAllocator** allocator);
Error          anvil_memory_pool_allocator_reset(PoolAllocator* const allocator);
void*          anvil_memory_pool_allocator_alloc(PoolAllocator* const allocator);

} // namespace pool_allocator
} // namespace memory
} // namespace anvil

#endif // ANVIL_MEMORY_POOL_ALLOCATOR_HPP