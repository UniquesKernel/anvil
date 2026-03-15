/**
 * @file constants.hpp
 * @brief useful constants associated with memory management in anvil.
 *
 * The `constants.hpp` file contains frequently used constants, that are used
 * often by anvil in connection with memory management, whether it be to manage
 * allocations, deallocations or verification of input to allocators.
 */
#ifndef ANVIL_MEMORY_CONSTANTS_HPP
#define ANVIL_MEMORY_CONSTANTS_HPP

#include "anvil/types.hpp"
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

namespace anvil::memory {

/**
 * @brief The size of a memory page on any given architecture - evaluate once
 * at program initialization.
 */
static const u64 PAGE_SIZE       = (u64)(sysconf(_SC_PAGESIZE));

/**
 * @brief The maximum allowed alignment for any given memory allocation.
 */
inline const u64 MAX_ALIGNMENT   = PAGE_SIZE >> 1;

/**
 * @brief The minimum allowed alignment for any given memory allocation.
 */
inline const u64 MIN_ALIGNMENT   = 1;

/**
 * @brief The maximum allowed stack depth of a stack allocator of any given type.
 */
inline const u64 MAX_STACK_DEPTH = 64;

/**
 * @brief The maximum total memory allocation allowed by any given allocator.
 */
inline const u64 MAX_CAPACITY    = static_cast<u64>(-1) / 2;

/**
 * @brief The number of bites required to shift one memory page in any direction.
 */
inline const u64 PAGE_SHIFT      = (u64)(__builtin_ctzl(PAGE_SIZE));

} // namespace anvil::memory
#endif // ANVIL_MEMORY_CONSTANTS_HPP
