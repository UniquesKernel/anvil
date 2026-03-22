/**
 * @file memory_allocation.hpp
 * @brief Provides primitive memory allocation and deallocation functionality
 *
 * This header defined an API for systematically allocating and deallocating
 * memory using both eager and lazy allocation methodologies. Lazy allocation
 * is achieved through the allocation of virtual memory with the benefit of
 * keeping memory contigous as it grows, leading to better cache locality as
 * memory usage grows.
 *
 * @note Follows fail-fast design; programmer errors cause immediate abort.
 *
 * @note The memory regions allocated through this interface are not inherently
 *       thread-safe and require explicit synchronization primitives to ensure
 *       correctness under concurrent access patterns.
 */

#ifndef ANVIL_MEMORY_ALLOCATION_HPP
#define ANVIL_MEMORY_ALLOCATION_HPP

#include "anvil/types.hpp"
#include "error/status.hpp"

namespace anvil::memory {

/**
 * @brief Encapsulates metadata for memory allocation
 *
 * Metadata is a struct that is prepended to user allocated memory, to
 * facilitate deallocation and lazy allocation patterns.
 *
 * @invariant base != nullptr (after successful allocation)
 * @invariant virtual_capacity > 0
 * @invariant capacity > 0
 * @invariant capacity <= virtual_capacity
 * @invariant page_count > 0
 *
 * Memory layout: [base][padding for alignment][Metadata][user-accessible memory]
 *
 * Field            | Type   | Size (Bytes)   | Description
 * ---------------- | ------ | -------------- | -------------------------------------------------
 * base             | void*  | sizeof(void*)  | Pointer to the start of the originally allocated memory mapping
 * virtual_capacity | u64    | sizeof(u64)    | Total virtual memory capacity allocated (lazy) or total size (eager)
 * capacity         | u64    | sizeof(u64)    | Current committed/writable byte capacity from mapping base
 * page_count       | u64    | sizeof(u64)    | Number of committed pages (`capacity / PAGE_SIZE`)
 *
 * @note On 64-bit systems: sizeof(Metadata) = 4 * 8 = 32 bytes
 */
struct Metadata {
        void* base;             ///< Original mmap base address
        u64   virtual_capacity; ///< Total reserved virtual memory
        u64   capacity;         ///< Current committed capacity
        u64   page_count;       ///< Number of committed pages
};
static_assert(sizeof(Metadata) == 32, "Metadata should be 32 bytes (4 * 8 bytes on 64-bit systems)"); // NOLINT
static_assert(alignof(Metadata) == alignof(void*), "Metadata should have the natural alignment of a void pointer");

/**
 * @brief Reserves virtual memory pages.
 *
 * Reserves a virtual address range and returns an aligned user pointer through
 * `mem_out`. Only the first page is made available for use, until the rest is committed.
 *
 * @pre `mem_out != nullptr`
 * @pre `*mem_out == nullptr`
 * @pre `capacity > 0`
 * @pre `capacity <= MAX_CAPACITY`
 * @pre sufficient virtual address space exists for allocation
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`
 *
 * @param[out] mem_out          Receives the aligned base address of the user-visible allocation.
 * @param[in]  capacity         Requested user capacity in bytes.
 * @param[in]  alignment        Alignment of the returned initial address point.
 * @return Error enumerated code where `OK` indicate success and other values indicate
 *         an error.
 */
[[nodiscard]] Error anvil_memory_alloc_lazy(void** mem_out, u64 capacity, u64 alignment) noexcept;

/**
 * @brief Allocates writable memory.
 *
 * Reserves and commits pages in one operation. The resulting memory region is
 * immediately readable and writable.
 *
 * @pre `mem_out != nullptr`
 * @pre `*mem_out == nullptr`
 * @pre `capacity > 0`.
 * @pre `capacity <= MAX_CAPACITY`
 * @pre address space exists for allocation
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`
 *
 * @param[out] mem_out          Receives the aligned base address of the user-visible allocation.
 * @param[in]  capacity         Requested user capacity in bytes.
 * @param[in]  alignment        Alignment of the returned initial address point.
 * @return Error enumerated code where `OK` indicate success and other values indicate
 *               errors.
 */
[[nodiscard]] Error anvil_memory_alloc_eager(void** mem_out, u64 capacity, u64 alignment) noexcept;

/**
 * @brief Deallocate memory, allocated by both the eager and lazy allocation methods.
 *
 * Unmaps the full backing range associated with `ptr`, including reserved and
 * committed pages, and return it to the operating system.
 *
 * @pre ptr != nullptr
 * @pre ptr must reference memory allocated by anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 *
 * @param[in] ptr        Pointer to the memory region to deallocate.
 * @return Error enumerated code where `OK` indicate success and other values indicate
 *         failure.
 */
[[nodiscard]] Error anvil_memory_dealloc(void* ptr) noexcept;

/**
 * @brief Commits reserved virtual memory to physical memory pages
 *
 * Expands the writable/accessibly committed prefix of a reserved vitual memory region.
 *
 * @pre `ptr != nullptr`
 * @pre ptr must reference memory allocated with anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 * @pre `commit_size > 0`
 * @pre `commit_size <= MAX_CAPACITY`
 *
 * @param[in] ptr           Pointer to the memory region to expand.
 * @param[in] commit_size   Size in bytes of additional physical memory to commit.
 *
 * @return Error enumerated code where `OK` indicate success and other values indicate
 *         failure.
 */
[[nodiscard]] Error anvil_memory_commit(void* ptr, u64 commit_size) noexcept;

} // namespace anvil::memory

#endif // ANVIL_MEMORY_ALLOCATION_HPP
