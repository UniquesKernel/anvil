#ifndef ANVIL_MEMORY_RESIZABLE_BUFFER_HPP
#define ANVIL_MEMORY_RESIZABLE_BUFFER_HPP

#include "anvil/types.hpp"
#include "error/status.hpp"

namespace anvil::memory::resizable_buffer {

struct ResizableBuffer {
        void* base;      ///< Start of usable memory region
        u64   capacity;  ///< Total usable capacity in bytes
        u64   alignment; ///< Alignment of the memory region
};

Error create(ResizableBuffer** buffer_out, u64 capacity, u64 alignment) noexcept;
void* data(const ResizableBuffer* buffer) noexcept;
Error destroy(ResizableBuffer** buffer_out) noexcept;
void* resize(ResizableBuffer** buffer, u64 new_size) noexcept;

} // namespace anvil::memory::resizable_buffer

#endif // !ANVIL_MEMORY_RESIZABLE_BUFFER_HPP