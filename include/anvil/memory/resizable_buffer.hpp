#ifndef ANVIL_MEMORY_RESIZABLE_BUFFER_HPP
#define ANVIL_MEMORY_RESIZABLE_BUFFER_HPP

#include "anvil/error/status.hpp"
#include "anvil/types.hpp"

namespace anvil::memory::resizable_buffer {

struct ResizableBuffer {
        void* base;      ///< Start of usable memory region
        u64   capacity;  ///< Total usable capacity in bytes
        u64   alignment; ///< Alignment of the memory region
};

[[nodiscard]] Error            create(ResizableBuffer** buffer_out, u64 capacity, u64 alignment) noexcept;
[[gnu::pure, nodiscard]] void* data(const ResizableBuffer* buffer) noexcept;
[[nodiscard]] Error            destroy(ResizableBuffer** buffer_out) noexcept;
[[nodiscard]] void*            resize(ResizableBuffer** buffer, u64 new_size) noexcept;

} // namespace anvil::memory::resizable_buffer

#endif // !ANVIL_MEMORY_RESIZABLE_BUFFER_HPP