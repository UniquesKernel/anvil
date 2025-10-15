#include "internal/memory_allocation.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include "memory/error.hpp"
#include "sys/mman.h"
#include <cstdint>
#include <unistd.h>

using std::size_t;

/**
 * @brief Encapsulates metadata for an aligned memory block allocation, storing information
 *        about the memory mapping and allocation state.
 *
 * @invariant base != nullptr
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
struct Metadata {
        void*  base;
        size_t page_size;
        size_t virtual_capacity;
        size_t capacity;
        size_t page_count;
};
static_assert(sizeof(Metadata) == 40, "Metadata should be 40 bytes (5 * 8 bytes on 64-bit systems)");
static_assert(alignof(Metadata) == alignof(void*), "Metadata should have the natural alignment of a void pointer");

ANVIL_ATTR_ALLOCATOR void* anvil_memory_alloc_lazy(const size_t capacity, const size_t alignment) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "%s = %zd", alignment, alignment);
        ANVIL_INVARIANT_RANGE(alignment, anvil::memory::MIN_ALIGNMENT, anvil::memory::MAX_ALIGNMENT);

        const size_t page_size  = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        size_t       total_size = capacity + sizeof(Metadata);
        total_size              = (total_size + (page_size - 1)) & ~(page_size - 1);
        void* base              = mmap(nullptr, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (base == MAP_FAILED) {
                return nullptr;
        }

        madvise(base, total_size, MADV_HUGEPAGE);

        if (anvil::error::check(mprotect(base, page_size, PROT_READ | PROT_WRITE) == 0, ERR_MEMORY_PERMISSION_CHANGE) !=
            ERR_SUCCESS) {
                munmap(base, total_size);
                return nullptr;
        }

        uintptr_t addr             = reinterpret_cast<uintptr_t>(base) + sizeof(Metadata);
        uintptr_t aligned_addr     = (addr + (alignment - 1)) & ~(alignment - 1);

        Metadata* metadata         = reinterpret_cast<Metadata*>(aligned_addr - sizeof(Metadata));
        metadata->base             = base;
        metadata->virtual_capacity = total_size;
        metadata->capacity         = page_size;
        metadata->page_size        = page_size;
        metadata->page_count       = metadata->capacity >> __builtin_ctzl(page_size);

        return reinterpret_cast<void*>(aligned_addr);
}

ANVIL_ATTR_ALLOCATOR void* anvil_memory_alloc_eager(const size_t capacity, const size_t alignment) {
        ANVIL_INVARIANT_POSITIVE(capacity);
        ANVIL_INVARIANT(is_power_of_two(alignment), INV_BAD_ALIGNMENT, "%s = %zd", alignment, alignment);
        ANVIL_INVARIANT_RANGE(alignment, anvil::memory::MIN_ALIGNMENT, anvil::memory::MAX_ALIGNMENT);

        const size_t page_size  = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        size_t       total_size = capacity + sizeof(Metadata) + page_size;
        total_size              = (total_size + (page_size - 1)) & ~(page_size - 1);
        void* base              = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (base == MAP_FAILED) {
                return nullptr;
        }

        madvise(base, total_size, MADV_HUGEPAGE);

        uintptr_t addr             = reinterpret_cast<uintptr_t>(base) + sizeof(Metadata);
        uintptr_t aligned_addr     = (addr + (alignment - 1)) & ~(alignment - 1);

        Metadata* metadata         = reinterpret_cast<Metadata*>(aligned_addr - sizeof(Metadata));
        metadata->base             = base;

        /// NOTE: (UniquesKernel) since anvil_memory_dealloc relies on the virtual capacity for correct deallocation of
        /// memory the virtual capacity to total_size. In eager allocation the virtual capacity is rather meaningless.
        metadata->virtual_capacity = total_size;
        metadata->capacity         = total_size;
        metadata->page_size        = page_size;
        metadata->page_count       = metadata->capacity >> __builtin_ctzl(page_size);

        return reinterpret_cast<void*>(aligned_addr);
}

Error anvil_memory_dealloc(void* ptr) {
        ANVIL_INVARIANT_NOT_NULL(ptr);

        Metadata* metadata = reinterpret_cast<Metadata*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(Metadata));

        ANVIL_INVARIANT_NOT_NULL(metadata->base);
        ANVIL_INVARIANT_POSITIVE(metadata->virtual_capacity);
        ANVIL_INVARIANT_POSITIVE(metadata->page_size);

        const Error unmap_result =
            ::anvil::error::check(munmap(metadata->base, metadata->virtual_capacity) == 0, ERR_MEMORY_DEALLOCATION);
        if (::anvil::error::is_error(unmap_result)) [[unlikely]] {
                return unmap_result;
        }

        return ERR_SUCCESS;
}

Error anvil_memory_commit(void* ptr, const size_t commit_size) {
        ANVIL_INVARIANT_NOT_NULL(ptr);
        ANVIL_INVARIANT_POSITIVE(commit_size);

        Metadata*    metadata     = reinterpret_cast<Metadata*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(Metadata));
        const size_t page_size    = metadata->page_size;
        const size_t _commit_size = (commit_size + (page_size - 1)) & ~(page_size - 1);
        const Error capacity_result = ::anvil::error::check(
            _commit_size <= metadata->virtual_capacity - metadata->capacity, ERR_OUT_OF_MEMORY);
        if (::anvil::error::is_error(capacity_result)) [[unlikely]] {
                return capacity_result;
        }
        const Error protect_result = ::anvil::error::check(
            mprotect(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(metadata->base) + metadata->capacity),
                     _commit_size, PROT_READ | PROT_WRITE) == 0,
            ERR_MEMORY_PERMISSION_CHANGE);
        if (::anvil::error::is_error(protect_result)) [[unlikely]] {
                return protect_result;
        }

        metadata->capacity   += _commit_size;
        metadata->page_count  = metadata->capacity >> __builtin_ctzl(page_size);

        return ERR_SUCCESS;
}