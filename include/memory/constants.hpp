#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>
#include <cstdalign>

namespace anvil {
namespace memory {

inline constexpr std::size_t EAGER           = std::size_t{1} << 0;
inline constexpr std::size_t LAZY            = std::size_t{1} << 1;
inline constexpr std::size_t MAX_ALIGNMENT   = std::size_t{1} << 11; // alignment is capped at half a page.
inline constexpr std::size_t MIN_ALIGNMENT   = std::size_t{1};
inline constexpr std::size_t MAX_STACK_DEPTH = std::size_t{64};

#if SIZE_MAX == UINT32_MAX
// 32-bit platforms
inline constexpr std::size_t TRANSFER_MAGIC = static_cast<std::size_t>(0xDEADC0DE);
#elif SIZE_MAX == UINT64_MAX
// 64-bit platforms
inline constexpr std::size_t TRANSFER_MAGIC = static_cast<std::size_t>(0xFFFFFFFFDEADC0DE);
#else
#error "Unsupported platform size_t width"
#endif

} // namespace memory
} // namespace anvil

#endif // ANVIL_MEMORY_CONSTANTS_HPP