#include "internal/memory_allocation.h"
#include "internal/utility.h"
#include "memory/constants.h"
#include "memory/error.h"
#include "sys/mman.h"
#include <stdbool.h>
#include <unistd.h>

/**
 * @brief Encapsulates metadata for an aligned memory block allocation, storing information
 *        about the memory mapping and allocation state.
 *
 * @invariant base != NULL
 * @invariant page_size > 0
 * @invariant virtual_capacity > 0
 * @invariant capacity > 0
 * @invariant capacity <= virtual_capacity
 * @invariant page_count > 0
 *
 * @note This structure is typically prepended to the user-aligned memory block.
 *
 * Fields           | Type   | Size (Bytes)  | Description
 * ---------------- | ------ | ------------- | -------------------------------------------------
 * base             | void*  | sizeof(void*) | Pointer to the start of the originally allocated memory mapping
 * page_size        | size_t | sizeof(size_t)| System page size used for memory alignment
 * virtual_capacity | size_t | sizeof(size_t)| Total virtual memory capacity allocated
 * capacity         | size_t | sizeof(size_t)| Current accessible memory capacity
 * page_count       | size_t | sizeof(size_t)| Number of pages in the current capacity
 */
typedef struct metadata_t {
        void*  base;
        size_t page_size;
        size_t virtual_capacity;
        size_t capacity;
        size_t page_count;
} Metadata;
static_assert(sizeof(Metadata) == 40, "Metadata should be 40 bytes (5 * 8 bytes on 64-bit systems)");
static_assert(alignof(Metadata) == alignof(void*), "Metadata should have the natural alignment of a void pointer");

MALLOC WARN_UNSURED_RESULT void* anvil_memory_alloc_lazy(const size_t capacity, const size_t alignment) {
        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "%s = %zd", alignment, alignment);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const size_t page_size  = (size_t)sysconf(_SC_PAGESIZE);
        size_t       total_size = capacity + sizeof(Metadata);
        total_size              = (total_size + (page_size - 1)) & ~(page_size - 1);
        void* base              = mmap(NULL, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (CHECK(base != MAP_FAILED, ERR_OUT_OF_MEMORY)) {
                return NULL;
        }

        if (CHECK(mprotect(base, page_size, PROT_READ | PROT_WRITE) == 0, ERR_MEMORY_PERMISSION_CHANGE)) {
                munmap(base, total_size);
                return NULL;
        }

        uintptr_t addr             = (uintptr_t)base + sizeof(Metadata);
        uintptr_t aligned_addr     = (addr + (alignment - 1)) & ~(alignment - 1);

        Metadata* metadata         = (Metadata*)(aligned_addr - sizeof(Metadata));
        metadata->base             = base;
        metadata->virtual_capacity = total_size;
        metadata->capacity         = page_size;
        metadata->page_size        = page_size;
        metadata->page_count       = metadata->capacity >> __builtin_ctzl(page_size);

        return (void*)aligned_addr;
}

MALLOC WARN_UNSURED_RESULT void* anvil_memory_alloc_eager(const size_t capacity, const size_t alignment) {

        INVARIANT_POSITIVE(capacity);
        INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "%s = %zd", alignment, alignment);
        INVARIANT_RANGE(alignment, MIN_ALIGNMENT, MAX_ALIGNMENT);

        const size_t page_size  = (size_t)sysconf(_SC_PAGESIZE);
        size_t       total_size = capacity + sizeof(Metadata);
        total_size              = (total_size + (page_size - 1)) & ~(page_size - 1);
        void* base              = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (CHECK(base != MAP_FAILED, ERR_OUT_OF_MEMORY)) {
                return NULL;
        }

        uintptr_t addr             = (uintptr_t)base + sizeof(Metadata);
        uintptr_t aligned_addr     = (addr + (alignment - 1)) & ~(alignment - 1);

        Metadata* metadata         = (Metadata*)(aligned_addr - sizeof(Metadata));
        metadata->base             = base;

        /// NOTE: (UniquesKernel) since anvil_memory_dealloc relies on the virtual capacity for correct deallocation of
        /// memory the virtual capacity to total_size. In eager allocation the virtual capacity is rather meaningless.
        metadata->virtual_capacity = total_size;
        metadata->capacity         = total_size;
        metadata->page_size        = page_size;
        metadata->page_count       = metadata->capacity >> __builtin_ctzl(page_size);

        return (void*)aligned_addr;
}

WARN_UNSURED_RESULT Error anvil_memory_dealloc(void* ptr) {
        INVARIANT_NOT_NULL(ptr);

        Metadata* metadata = (Metadata*)((uintptr_t)ptr - sizeof(Metadata));

        INVARIANT_NOT_NULL(metadata->base);
        INVARIANT_POSITIVE(metadata->virtual_capacity);
        INVARIANT_POSITIVE(metadata->page_size);

        /// NOTE: (UniquesKernel) TRY_CHECK will return early using the provided Error.
        TRY_CHECK(munmap(metadata->base, metadata->virtual_capacity) == 0, ERR_MEMORY_DEALLOCATION);

        return ERR_SUCCESS;
}

WARN_UNSURED_RESULT Error anvil_memory_commit(void* ptr, const size_t commit_size) {
        INVARIANT_NOT_NULL(ptr);
        INVARIANT_POSITIVE(commit_size);

        Metadata*    metadata     = (Metadata*)((uintptr_t)ptr - sizeof(Metadata));
        const size_t page_size    = metadata->page_size;
        const size_t _commit_size = (commit_size + (page_size - 1)) & ~(page_size - 1);

        /// NOTE: (UniquesKernel) TRY_CHECK will return early using the provided Error.
        TRY_CHECK(_commit_size <= metadata->virtual_capacity - metadata->capacity, ERR_OUT_OF_MEMORY);
        TRY_CHECK(mprotect((void*)((uintptr_t)metadata->base + metadata->capacity), _commit_size,
                           PROT_READ | PROT_WRITE) == 0,
                  ERR_MEMORY_PERMISSION_CHANGE);

        metadata->capacity   += _commit_size;
        metadata->page_count  = metadata->capacity >> __builtin_ctzl(page_size);

        return ERR_SUCCESS;
}