#ifndef ANVIL_MEMORY_CONSTANTS_H
#define ANVIL_MEMORY_CONSTANTS_H

#include <stdalign.h>

#define EAGER (1 << 0)
#define LAZY  (1 << 1)
#define MAX_ALIGNMENT (1 << 11) // alignment is capped at half a page.
#define MIN_ALIGNMENT 1
#define MAX_STACK_DEPTH 64

#endif // ANVIL_MEMORY_CONSTANTS_H