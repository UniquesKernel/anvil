#ifndef ANVIL_MEMORY_RESIZEABLE_BUFFER_HPP
#define ANVIL_MEMORY_RESIZEABLE_BUFFER_HPP

#include "anvil/error/status.hpp"
#include "anvil/types.hpp"

namespace anvil::memory::resizeable_buffer {

struct ResizeableBuffer {
        void* base      = nullptr; ///< Start of usable memory region
        u64   capacity  = 0;       ///< Total usable capacity in bytes
        u64   alignment = 0;       ///< Alignment of the memory region
};

[[nodiscard]] Error            create(ResizeableBuffer** buffer_out, u64 capacity, u64 alignment) noexcept;
[[gnu::pure, nodiscard]] void* data(const ResizeableBuffer* buffer) noexcept;
[[nodiscard]] Error            destroy(ResizeableBuffer** buffer_out) noexcept;
[[nodiscard]] void*            resize(ResizeableBuffer** buffer, u64 new_size) noexcept;

} // namespace anvil::memory::resizeable_buffer

#endif // !ANVIL_MEMORY_RESIZEABLE_BUFFER_HPP
