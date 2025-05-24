#include "memory/scratch_allocator.h"
#include "internal/error.h"
#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include <assert.h>
#include <stdalign.h>
#include <string.h>

/**
 * @struct ScratchAllocator
 * @brief A simple linear scratch memory allocator.
 *
 * | Variable         | Type         | Description                                   | Size (bytes)         |
 * |------------------|--------------|-----------------------------------------------|----------------------|
 * | base             | void*        | Pointer to start of memory region             | 8 (on 64-bit) / 4 (on 32-bit) |
 * | capacity         | const size_t | Total size (in bytes) of memory region        | 8 (on 64-bit) / 4 (on 32-bit) |
 * | allocated        | size_t       | Number of bytes currently allocated           | 8 (on 64-bit) / 4 (on 32-bit) |
 * | alignment        | const size_t | Alignment requirement for allocations         | 8 (on 64-bit) / 4 (on 32-bit) |
 *
 * @invariant base is a valid, non-NULL pointer to a memory region of capacity bytes.
 * @invariant capacity > 0 and capacity is a multiple of alignment.
 * @invariant alignment is a power of two and greater than 0.
 * @invariant allocated <= capacity.
 * @invariant allocated is always aligned to alignment.
 *
 * @note The struct size is asserted to be either 16 or 32 bytes, depending on architecture.
 * @note The struct alignment is asserted to match the alignment of a void pointer.
 */
typedef struct ScratchAllocator {
	void*        base;
	const size_t capacity;
	size_t       allocated;
	const size_t alignment;
} ScratchAllocator;

static_assert(sizeof(ScratchAllocator) == 32 || sizeof(ScratchAllocator) == 16,
	      "Scratch Allocator should be 32 bytes or 16 bytes depending on architecture");
static_assert(alignof(ScratchAllocator) == alignof(void*), "Scratch Allocator should be aligned to void*");

FREE_SCRATCH_ATTRIBUTE WARN_IF_NOT_USED ScratchAllocator* scratch_allocator_create(const size_t capacity,
									   const size_t alignment) {
	INVARIANT(capacity > 0, ERR_ZERO_CAPACITY, capacity);
	INVARIANT(capacity >= alignment, ERR_GREATER_EQUAL, "capacity", "alignment", capacity, alignment);
	INVARIANT(is_power_of_two(alignment), ERR_ALLOC_ALIGNMENT_NOT_POWER_OF_TWO, alignment);

	ScratchAllocator* allocator = safe_aligned_alloc(sizeof(*allocator), alignof(max_align_t));
	if (!allocator) {
		return NULL;
	}

	allocator->base = safe_aligned_alloc(capacity, alignment);
	if (!allocator->base) {
		safe_aligned_free(allocator);
		return NULL;
	}

	allocator->allocated = 0;
	memcpy(&(allocator->alignment), &alignment, sizeof(size_t));
	memcpy(&(allocator->capacity), &capacity, sizeof(size_t));

	return allocator;
} 

MALLOC_ATTRIBUTE WARN_IF_NOT_USED void* scratch_allocator_alloc(ScratchAllocator* restrict allocator, const size_t size,
								const size_t count) {
	INVARIANT(allocator, ERR_NULL_POINTER, "allocator");
	INVARIANT(is_power_of_two(allocator->alignment), ERR_ALLOC_ALIGNMENT_NOT_POWER_OF_TWO, allocator->alignment);
	INVARIANT(size > 0, ERR_GREATER_THAN, "size", "0", size, 0);
	INVARIANT(count > 0, ERR_GREATER_THAN, "count", 0, count, 0);

	const uintptr_t base    = (uintptr_t)allocator->base;
	const uintptr_t current = base + allocator->allocated;
	const uintptr_t aligned = (current + (allocator->alignment - 1)) & ~(uintptr_t)(allocator->alignment - 1);
	const size_t    offset  = aligned - current;

	size_t          total_size;
	if (UNLIKELY(__builtin_umull_overflow(count, size, &total_size))) {
		return NULL;
	}

	if (UNLIKELY(__builtin_uaddl_overflow(total_size, offset, &total_size))) {
		return NULL;
	}

	if (total_size > allocator->capacity - allocator->allocated) {
		return NULL;
	}

	allocator->allocated += total_size;
	return (void*)aligned;
}

void scratch_allocator_reset(ScratchAllocator* restrict allocator) {
	INVARIANT(allocator, ERR_NULL_POINTER, "allocator");
	allocator->allocated = 0;
}
void scratch_allocator_destroy(ScratchAllocator** allocator) {
	INVARIANT(allocator && *allocator, ERR_NULL_POINTER, "allocator");
	safe_aligned_free((*allocator)->base);
	safe_aligned_free((*allocator));
	*allocator = NULL;
}