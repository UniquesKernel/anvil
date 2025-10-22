#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include <cstdint>

#if defined(__clang__) || defined(__GNUC__)
#define ANVIL_ATTR_CLEANUP(func) [[gnu::cleanup(func)]]
#define ANVIL_ATTR_COLD          [[gnu::cold]]
#define ANVIL_ATTR_HOT           [[gnu::hot]]
#define ANVIL_ATTR_ALWAYS_INLINE [[gnu::always_inline]]
#define ANVIL_ATTR_NOINLINE      [[gnu::noinline]]
#define ANVIL_ATTR_ALLOCATOR     [[gnu::malloc]]
#define ANVIL_ATTR_PURE          [[gnu::pure]]
#else
#define ANVIL_ATTR_CLEANUP(func)
#define ANVIL_ATTR_COLD
#define ANVIL_ATTR_HOT
#define ANVIL_ATTR_ALWAYS_INLINE
#define ANVIL_ATTR_NOINLINE
#define ANVIL_ATTR_ALLOCATOR
#define ANVIL_ATTR_PURE
#endif

#ifndef DEFER
#define DEFER(clean_up_func) ANVIL_ATTR_CLEANUP(clean_up_func)
#endif

namespace anvil::memory {

enum class AllocationStrategy : std::size_t {
        Eager = 1u << 0,
        Lazy  = 1u << 1,
};

inline constexpr std::size_t MAX_ALIGNMENT   = 1 << 11; // alignment is capped at half a page.
inline constexpr std::size_t MIN_ALIGNMENT   = 1;
inline constexpr std::size_t MAX_STACK_DEPTH = 64;

} // namespace anvil::memory

#endif // ANVIL_MEMORY_CONSTANTS_HPP