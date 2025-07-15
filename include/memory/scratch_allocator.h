#ifndef ANVIL_MEMORY_SCRATCH_ALLOCATOR_H
#define ANVIL_MEMORY_SCRATCH_ALLOCATOR_H

#include "error.h"
#include <stddef.h>

typedef struct scratch_allocator_t ScratchAllocator;

#define EAGER 0b00000001
#define LAZY  0b00000010

/**
 * @brief Establishes an allocator, of type ScratchAllocator, with a static memory mapping.
 *
 * @invariant capacity > 0.
 * @invariant alignment is power of two.
 * @invariant alignment >= alignof(void*).
 * @invariant alloc_mode is set to either EAGER (0b00000001) or LAZY (0b00000010) but not both.
 *
 * @param[in] capacity              Total available memory for the allocator.
 * @param[in] alignment             Alignment of the memory the addresses the allocator returns.
 * @param[in] alloc_mode            Allocation mode (Eager|Lazy) that allocator uses when allocating memory.
 * @return ScratchAllocator*        Pointer to the created ScratchAllocator.
 */
ScratchAllocator* anvil_memory_scratch_allocator_create(const size_t capacity, const size_t alignment,
                                                        const size_t alloc_mode);

Error             anvil_memory_scratch_allocator_destroy(ScratchAllocator** allocator);
#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_H