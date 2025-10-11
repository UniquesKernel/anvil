#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include <cstdalign>
#include <cstddef>
#include <cstdint>

namespace anvil::memory {

inline constexpr std::size_t EAGER           = 1 << 0;
inline constexpr std::size_t LAZY            = 1 << 1;
inline constexpr std::size_t MAX_ALIGNMENT   = 1 << 11; // alignment is capped at half a page.
inline constexpr std::size_t MIN_ALIGNMENT   = 1;
inline constexpr std::size_t MAX_STACK_DEPTH = 64;

#if SIZE_MAX == UINT32_MAX
// 32-bit platforms
inline constexpr std::size_t TRANSFER_MAGIC = static_cast<std::size_t>(0xDEADC0DE);
#elif SIZE_MAX == UINT64_MAX
// 64-bit platforms
inline constexpr std::size_t TRANSFER_MAGIC = static_cast<std::size_t>(0xFFFFFFFFDEADC0DE);
#else
#error "Unsupported platform std::size_t width"
#endif

} // namespace anvil::memory

#endif // ANVIL_MEMORY_CONSTANTS_HPP