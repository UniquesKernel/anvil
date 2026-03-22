#ifndef ANVIL_MEMORY_POLICY_HPP
#define ANVIL_MEMORY_POLICY_HPP

#include <cstdlib>

/**
 * Creates an error if `malloc` and related functions are used in
 * the Anvil codebase. Use Anvil's custom allocators instead. Add
 * translation units above this header if they still need to use
 * `malloc` internally before it is poisoned.
 */
#pragma GCC poison malloc calloc realloc free

#endif // !ANVIL_MEMORY_POLICY_HPP
