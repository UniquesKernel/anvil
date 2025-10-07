#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>
#include <cstdalign>

using std::size_t;

namespace anvil {
namespace memory {

inline constexpr size_t EAGER           = 1 << 0;
inline constexpr size_t LAZY            = 1 << 1;
inline constexpr size_t MAX_ALIGNMENT   = 1 << 11; // alignment is capped at half a page.
inline constexpr size_t MIN_ALIGNMENT   = 1;
inline constexpr size_t MAX_STACK_DEPTH = 64;

#if SIZE_MAX == UINT32_MAX
// 32-bit platforms
inline constexpr size_t TRANSFER_MAGIC = static_cast<size_t>(0xDEADC0DE);
#elif SIZE_MAX == UINT64_MAX
// 64-bit platforms
inline constexpr size_t TRANSFER_MAGIC = static_cast<size_t>(0xFFFFFFFFDEADC0DE);
#else
#error "Unsupported platform size_t width"
#endif

} // namespace memory
} // namespace anvil

#endif // ANVIL_MEMORY_CONSTANTS_HPP