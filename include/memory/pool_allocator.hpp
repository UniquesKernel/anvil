#ifndef ANVIL_MEMORY_POOL_ALLOCATOR_HPP
#define ANVIL_MEMORY_POOL_ALLOCATOR_HPP

#include "error.hpp"
#include <cstddef>


namespace anvil::memory::pool_allocator {
struct PoolAllocator;

PoolAllocator* anvil_memory_pool_allocator_create(const std::size_t object_size, const std::size_t object_count,
                                                  const std::size_t alignment);
Error          anvil_memory_pool_allocator_destroy(PoolAllocator** allocator);
Error          anvil_memory_pool_allocator_reset(PoolAllocator* const allocator);
void*          anvil_memory_pool_allocator_alloc(PoolAllocator* const allocator);

} // namespace anvil::memory::pool_allocator

#endif // ANVIL_MEMORY_POOL_ALLOCATOR_HPP: