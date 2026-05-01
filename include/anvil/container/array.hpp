#ifndef ANVIL_CONTAINER_ARRAY_HPP
#define ANVIL_CONTAINER_ARRAY_HPP

#include "anvil/error/status.hpp"
#include "anvil/memory/resizeable_buffer.hpp"
#include "anvil/types.hpp"

namespace anvil::container {

/**
 *
 */
struct Array {
        memory::resizeable_buffer::ResizeableBuffer* data      = nullptr;
        u64                                          size      = 0;
        u64                                          capacity  = 0;
        u64                                          type_size = 0;
};
static_assert(sizeof(Array) == 32, "Arrays must be 24 bytes in size");
static_assert(alignof(Array) == alignof(void*), "Arrays are pointer aligned");

Error create(Array* array, u64 type_size, u64 element_count);

} // namespace anvil::container
#endif // ANVIL_CONTAINER_ARRAY_HPP
