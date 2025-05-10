#include "internal/memory_allocation.h"
#include "internal/error.h"
#include "internal/utility.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

void *safe_aligned_alloc(size_t size, size_t alignment) {
	INVARIANT(size != 0, ERR_ALLOC_SIZE_ZERO);
	INVARIANT(is_power_of_two(alignment), ERR_ALLOC_ALIGNMENT_NOT_POWER_OF_TWO, alignment);
	INVARIANT(alignment <= (1 << 16), ERR_ALLOC_ALIGNMENT_TOO_LARGE, (size_t)(1 << 16), alignment);

	size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
	size_t total_size = size + alignment + sizeof(Metadata);

	total_size = (total_size + page_size - 1) & ~(page_size - 1);

	void *base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	INVARIANT(base != MAP_FAILED, ERR_OUT_OF_MEMORY, total_size);

	uintptr_t addr = (uintptr_t)base + sizeof(Metadata);
	uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

	Metadata *metadata = (Metadata *)(aligned_addr - sizeof(Metadata));
	metadata->base = base;
	metadata->total_size = total_size;

	return (void *)aligned_addr;
}

void safe_free(void *ptr) {
	if (!ptr) {
		return;
	}
	free(*(void **)ptr);
}

void safe_aligned_free(void *ptr) {
	if (!ptr)
		return;

	Metadata *metadata = (Metadata *)((uintptr_t)ptr - sizeof(Metadata));

	INVARIANT(metadata->base != NULL, ERR_NULL_POINTER, "metadata->base");
	INVARIANT(metadata->total_size > 0, ERR_VALUE_MIN, "metadata->total_size", 1, metadata->total_size);

#ifdef DEBUG
	memset(metadata->base, MEMORY_POISON_PATTERN, metadata->total_size);
#else
	memset(metadata->base, 0, metadata->total_size);
#endif
	munmap(metadata->base, metadata->total_size);
}
