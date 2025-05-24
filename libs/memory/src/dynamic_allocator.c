#include "memory/dynamic_allocator.h"
#include "internal/error.h"
#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include <stdalign.h>
#include <stddef.h>
#include <string.h>

/**
 * @struct MemoryBlock
 * @brief Represents a block of memory within the dynamic allocator.
 *
 * This structure is used to manage dynamically allocated memory blocks,
 * supporting efficient allocation and tracking of memory usage within a custom allocator.
 *
 * | Variable  | Type                   | Description                                   | Size (bytes)                  |
 * |-----------|------------------------|-----------------------------------------------|-------------------------------|
 * | base      | void*                  | Pointer to the start of the memory block      | 8 (on 64-bit) / 4 (on 32-bit) |
 * | next      | struct memory_block_t* | Pointer to the next memory block in the list  | 8 (on 64-bit) / 4 (on 32-bit) |
 * | capacity  | const size_t           | Total size (in bytes) of the memory block     | 8 (on 64-bit) / 4 (on 32-bit) |
 * | allocated | size_t                 | Number of bytes currently allocated in block  | 8 (on 64-bit) / 4 (on 32-bit) |
 *
 * @invariant base is a valid, non-NULL pointer to a memory region of capacity bytes.
 * @invariant capacity > 0.
 * @invariant allocated <= capacity.
 *
 * @note The 'capacity' field is constant after initialization, ensuring the block size does not change.
 * @note The 'allocated' field tracks the number of bytes currently in use within the block.
 */
typedef struct memory_block_t {
	void*                  base;
	struct memory_block_t* next;
	const size_t           capacity;
	size_t                 allocated;
} MemoryBlock;

static_assert(sizeof(MemoryBlock) == 32 || sizeof(MemoryBlock) == 16, "MemoryBlock size must be 32 or 16 bytes depending on architecture");
static_assert(alignof(MemoryBlock) == alignof(void*), "MemoryBlock alignment must match pointer alignment");

/**
 * @struct DynamicAllocator
 * @brief A dynamic memory allocator that manages a list of memory blocks.
 *
 * This structure encapsulates the state of a dynamic memory allocator,
 * which manages a linked list of memory blocks to fulfill allocation requests.
 *
 * | Variable     | Type         | Description                                   | Size (bytes)                  |
 * |--------------|--------------|-----------------------------------------------|-------------------------------|
 * | memory_block | MemoryBlock* | Pointer to the first memory block in the list | 8 (on 64-bit) / 4 (on 32-bit) |
 * | alignment    | const size_t | Alignment requirement for allocations         | 8 (on 64-bit) / 4 (on 32-bit) |
 *
 * @invariant alignment is a power of two and greater than 0.
 * @invariant memory_block is either NULL or points to a valid MemoryBlock.
 *
 * @note The 'alignment' field is constant after initialization, ensuring all allocations meet the specified alignment.
 */
typedef struct dynamic_allocator_t {
	MemoryBlock* memory_block;
	const size_t alignment;
} DynamicAllocator;

static_assert(sizeof(DynamicAllocator) == (sizeof(void*) + sizeof(size_t)), "DynamicAllocator size must match pointer + size_t");
static_assert(alignof(DynamicAllocator) == alignof(void*), "DynamicAllocator alignment must match pointer alignment");

FREE_DYNAMIC_ATTRIBUTE WARN_IF_NOT_USED DynamicAllocator* dynamic_allocator_create(const size_t capacity, const size_t alignment) {
	INVARIANT(capacity > 0, ERR_GREATER_THAN, "capacity", "0", capacity, 0);
	INVARIANT(capacity >= alignment, ERR_GREATER_EQUAL, "capacity", "alignment", capacity, alignment);
	INVARIANT(is_power_of_two(alignment), ERR_ALLOC_ALIGNMENT_NOT_POWER_OF_TWO, alignment);

	DynamicAllocator* allocator = safe_aligned_alloc(sizeof(*allocator), alignof(DynamicAllocator));

	if (!allocator) {
		return NULL;
	}
	memcpy(&allocator->alignment, &alignment, sizeof(size_t));

	allocator->memory_block = safe_aligned_alloc(sizeof(*allocator->memory_block), alignof(MemoryBlock));
	if (!allocator->memory_block) {
		safe_aligned_free(allocator);
		return NULL;
	}

	allocator->memory_block->allocated = 0;
	memcpy(&allocator->memory_block->capacity, &capacity, sizeof(size_t));
	allocator->memory_block->next = NULL;
	allocator->memory_block->base = safe_aligned_alloc(capacity, alignment);

	if (!allocator->memory_block->base) {
		safe_aligned_free(allocator->memory_block);
		safe_aligned_free(allocator);
		return NULL;
	}

	return allocator;
}

MALLOC_ATTRIBUTE WARN_IF_NOT_USED void* dynamic_allocator_alloc(DynamicAllocator* restrict allocator, const size_t size, const size_t count) {
	return NULL;
}

void dynamic_allocator_reset(DynamicAllocator* restrict allocator) {
	INVARIANT(allocator, ERR_NULL_POINTER, "allocator");
	INVARIANT(allocator->memory_block, ERR_NULL_MEMORY_BLOCK);

	for (MemoryBlock *current = allocator->memory_block->next, *n; current != NULL && (n = current->next, 1); current = n) {
		safe_aligned_free(current->base);
		safe_aligned_free(current);
	}
	allocator->memory_block->allocated = 0;
	allocator->memory_block->next      = NULL;
}

void dynamic_allocator_destroy(DynamicAllocator** allocator) {
	INVARIANT(allocator && *allocator, ERR_NULL_POINTER, "allocator");
	INVARIANT((*allocator)->memory_block, ERR_NULL_MEMORY_BLOCK);

	for (MemoryBlock *current = (*allocator)->memory_block, *n; current != NULL && (n = current->next, 1); current = n) {
		safe_aligned_free(current->base);
		safe_aligned_free(current);
	}
	safe_aligned_free((*allocator));
	(*allocator) = NULL;
}