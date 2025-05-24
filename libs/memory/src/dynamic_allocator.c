#include "memory/dynamic_allocator.h"
#include <stddef.h>

/**
 * @struct MemoryBlock
 * @brief Represents a block of memory within the dynamic allocator.
 *
 * | Variable  | Type                  | Description                                   | Size (bytes)                  |
 * |-----------|-----------------------|-----------------------------------------------|-------------------------------|
 * | base      | void*                 | Pointer to the start of the memory block      | 8 (on 64-bit) / 4 (on 32-bit) |
 * | next      | struct memory_block_t*| Pointer to the next memory block in the list  | 8 (on 64-bit) / 4 (on 32-bit) |
 * | capacity  | const size_t          | Total size (in bytes) of the memory block     | 8 (on 64-bit) / 4 (on 32-bit) |
 * | allocated | size_t                | Number of bytes currently allocated in this block | 8 (on 64-bit) / 4 (on 32-bit) |
 *
 * @invariant base is a valid, non-NULL pointer to a memory region of capacity bytes.
 * @invariant capacity > 0.
 * @invariant allocated <= capacity.
 */
typedef struct memory_block_t {
	void*                  base;
	struct memory_block_t* next;
	const size_t           capacity;
	size_t                 allocated;
} MemoryBlock;

/**
 * @struct DynamicAllocator
 * @brief A dynamic memory allocator that manages a list of memory blocks.
 *
 * | Variable     | Type                  | Description                                      | Size (bytes)                  |
 * |--------------|-----------------------|--------------------------------------------------|-------------------------------|
 * | memory_block | MemoryBlock*          | Pointer to the first memory block in the list    | 8 (on 64-bit) / 4 (on 32-bit) |
 * | alignment    | const size_t          | Alignment requirement for allocations            | 8 (on 64-bit) / 4 (on 32-bit) |
 *
 * @invariant alignment is a power of two and greater than 0.
 */
typedef struct dynamic_allocator_t {
	MemoryBlock* memory_block;
	const size_t alignment;
} DynamicAllocator;
