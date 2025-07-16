#ifndef ANVIL_MEMORY_CONSTANTS_H
#define ANVIL_MEMORY_CONSTANTS_H

#include <stdalign.h>

#define EAGER 0b00000001
#define LAZY  0b00000010
#define MAX_ALIGNMENT (1 << 22)
#define MIN_ALIGNMENT (alignof(void*))

#endif // ANVIL_MEMORY_CONSTANTS_H