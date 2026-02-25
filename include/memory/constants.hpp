#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

namespace anvil::memory {

static const size_t          PAGE_SIZE       = static_cast<size_t>(sysconf(_SC_PAGESIZE));

inline const std::size_t     MAX_ALIGNMENT   = PAGE_SIZE >> 1; // NOLINT alignment is capped at half a page.
inline const std::size_t     MIN_ALIGNMENT   = 1;
inline constexpr std::size_t MAX_STACK_DEPTH = 64;
inline constexpr std::size_t MAX_CAPACITY    = SIZE_MAX / 2;
static const size_t          PAGE_SHIFT      = static_cast<unsigned long>(__builtin_ctzl(PAGE_SIZE));

} // namespace anvil::memory
#endif // ANVIL_MEMORY_CONSTANTS_HPP
