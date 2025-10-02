#ifndef ANVIL_MEMORY_CONSTANTS_H
#define ANVIL_MEMORY_CONSTANTS_H

#include <stdalign.h>

#define EAGER (1 << 0)
#define LAZY  (1 << 1)
#define MAX_ALIGNMENT (1 << 11) // alignment is capped at half a page.
#define MIN_ALIGNMENT 1
#define MAX_STACK_DEPTH 64

#if SIZE_MAX == UINT32_MAX
    // 32-bit platforms
    #define TRANSFER_MAGIC ((size_t)0xDEADC0DE)
#elif SIZE_MAX == UINT64_MAX
    // 64-bit platforms  
    #define TRANSFER_MAGIC ((size_t)0xFFFFFFFFDEADC0DE)
#else
    #error "Unsupported platform size_t width"
#endif

#endif // ANVIL_MEMORY_CONSTANTS_H