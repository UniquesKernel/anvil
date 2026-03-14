#ifndef ANVIL_MEMORY_RESIZABLE_BUFFER_HPP
#define ANVIL_MEMORY_RESIZABLE_BUFFER_HPP

#include "error/status.hpp"

namespace anvil::memory::resizable_buffer {

struct ResizableBuffer {
        void*         base;      ///< Start of usable memory region
        unsigned long capacity;  ///< Total usable capacity in bytes
        unsigned long alignment; ///< Alignment of the memory region
};

Error create(ResizableBuffer** buffer_out, unsigned long capacity, unsigned long alignment) noexcept;
void* data(const ResizableBuffer* buffer) noexcept;
Error destroy(ResizableBuffer** buffer_out) noexcept;
void* resize(ResizableBuffer** buffer, unsigned long new_size) noexcept;

} // namespace anvil::memory::resizable_buffer

#endif // !ANVIL_MEMORY_RESIZABLE_BUFFER_HPP