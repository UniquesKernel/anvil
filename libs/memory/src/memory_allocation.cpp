#include "memory/memory_allocation.hpp"
#include "error/assert.hpp"
#include "error/status.hpp"
#include "internal/utility.hpp"
#include "memory/constants.hpp"
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

using std::size_t;
namespace anvil::memory {
Error anvil_memory_alloc_lazy(void** mem_out, const size_t capacity, const size_t alignment) noexcept {
        INVARIANT(mem_out != nullptr, NULL_PARAMETER);
        INVARIANT(*mem_out == nullptr, NULL_PARAMETER);
        INVARIANT(capacity > 0, INVALID_ARGUMENTS);
        INVARIANT(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        INVARIANT(is_power_of_two(alignment), INVALID_ARGUMENTS);
        INVARIANT(anvil::memory::MIN_ALIGNMENT <= alignment, INVALID_ARGUMENTS);
        INVARIANT(alignment <= anvil::memory::MAX_ALIGNMENT, INVALID_ARGUMENTS);

        size_t total_size = capacity + sizeof(Metadata);
        total_size        = (total_size + (PAGE_SIZE - 1U)) & ~(PAGE_SIZE - 1U);
        void* base        = mmap(nullptr, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (base == MAP_FAILED) {
                return OUT_OF_MEMORY;
        }

        // NOTE: (UniquesKernel) On failure madvise will default to Normal paging of ~4KB
        // Since this is a generalized memory allocator mechanism, the
        // return value can safely be ignored
        (void)madvise(base, total_size, MADV_HUGEPAGE);

        if (mprotect(base, PAGE_SIZE, PROT_READ | PROT_WRITE) != 0) {
                munmap(base, total_size);
                return OUT_OF_MEMORY;
        }

        const uintptr_t ADDR         = reinterpret_cast<uintptr_t>(base) + sizeof(Metadata);
        const uintptr_t ALIGNED_ADDR = (ADDR + (alignment - 1)) & ~(alignment - 1);

        Metadata*       metadata     = reinterpret_cast<Metadata*>(ALIGNED_ADDR - sizeof(Metadata));
        metadata->base               = base;
        metadata->virtual_capacity   = total_size;
        metadata->capacity           = PAGE_SIZE;
        metadata->page_count         = metadata->capacity >> PAGE_SHIFT;

        *mem_out                     = reinterpret_cast<void*>(ALIGNED_ADDR);
        return OK;
}

Error anvil_memory_alloc_eager(void** mem_out, const size_t capacity, const size_t alignment) noexcept {
        INVARIANT(mem_out != nullptr, NULL_PARAMETER);
        INVARIANT(*mem_out == nullptr, NULL_PARAMETER);
        INVARIANT(capacity > 0, INVALID_ARGUMENTS);
        INVARIANT(capacity <= MAX_CAPACITY, INVALID_ARGUMENTS);
        INVARIANT(is_power_of_two(alignment), INVALID_ARGUMENTS);
        INVARIANT(anvil::memory::MIN_ALIGNMENT <= alignment, INVALID_ARGUMENTS);
        INVARIANT(alignment <= anvil::memory::MAX_ALIGNMENT, INVALID_ARGUMENTS);

        size_t total_size = capacity + sizeof(Metadata) + PAGE_SIZE;
        total_size        = (total_size + (PAGE_SIZE - 1U)) & ~(PAGE_SIZE - 1U);
        void* base        = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (base == MAP_FAILED) {
                return OUT_OF_MEMORY;
        }

        // NOTE: (UniquesKernel) On failure madvise will default to Normal paging of ~4KB
        // Since this is a generalized memory allocator mechanism, the
        // return value can safely be ignored
        (void)madvise(base, total_size, MADV_HUGEPAGE);

        const uintptr_t ADDR         = reinterpret_cast<uintptr_t>(base) + sizeof(Metadata);
        const uintptr_t ALIGNED_ADDR = (ADDR + (alignment - 1U)) & ~(alignment - 1U);

        // NOTE: (UniquesKernel) since anvil_memory_dealloc relies on the virtual capacity for correct deallocation of
        // memory the virtual capacity to total_size. In eager allocation the virtual capacity is rather meaningless.
        Metadata*       metadata     = reinterpret_cast<Metadata*>(ALIGNED_ADDR - sizeof(Metadata));
        metadata->base               = base;
        metadata->virtual_capacity   = total_size;
        metadata->capacity           = capacity;
        metadata->page_count         = total_size >> PAGE_SHIFT;

        *mem_out                     = reinterpret_cast<void*>(ALIGNED_ADDR);
        return OK;
}

Error anvil_memory_dealloc(void* ptr) noexcept {
        INVARIANT(ptr != nullptr, NULL_PARAMETER);

        Metadata* metadata = reinterpret_cast<Metadata*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(Metadata));
        GUARANTEE(metadata->base != nullptr);
        GUARANTEE(metadata->virtual_capacity > 0);
        GUARANTEE(munmap(metadata->base, metadata->virtual_capacity) == 0);

        return OK;
}

Error anvil_memory_commit(void* ptr, const size_t commit_size) noexcept {
        INVARIANT(ptr != nullptr, NULL_PARAMETER);
        INVARIANT(commit_size > 0, INVALID_ARGUMENTS);
        INVARIANT(commit_size <= MAX_CAPACITY, INVALID_ARGUMENTS);

        Metadata* metadata = reinterpret_cast<Metadata*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(Metadata));
        GUARANTEE(metadata->base != nullptr);
        GUARANTEE(metadata->virtual_capacity > 0);
        GUARANTEE(metadata->capacity <= metadata->virtual_capacity);

        const size_t COMMIT_SIZE = (commit_size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
        if (COMMIT_SIZE > metadata->virtual_capacity - metadata->capacity) [[unlikely]] {
                return OUT_OF_MEMORY;
        }

        void* commit_base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(metadata->base) + metadata->capacity);
        if (mprotect(commit_base, COMMIT_SIZE, PROT_READ | PROT_WRITE) != 0) [[unlikely]] {
                return OUT_OF_MEMORY;
        }

        metadata->capacity   += COMMIT_SIZE;
        metadata->page_count  = metadata->capacity >> PAGE_SHIFT;

        return OK;
}
} // namespace anvil::memory
