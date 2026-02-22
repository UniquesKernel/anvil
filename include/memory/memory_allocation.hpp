/**
 * @file memory_allocation.hpp
 * @brief Virtual memory allocation, deallocation, and management interface
 *
 * This header defines an interface for the systematic manipulation of virtual
 * memory address spaces, encompassing the fundamental operations of allocation,
 * deallocation, and the binding of physical memory resources to virtual pages.
 * The interface further provides mechanisms for the bidirectional transformation
 * between virtual and physical memory addresses. The virtual address spaces
 * allocated through this interface exist in an uncommitted state until physical
 * memory resources are explicitly bound to the corresponding virtual pages.
 *
 * @note The computational model employed herein adheres to the principle of
 *       fail-fast semantics, wherein erroneous program states precipitate
 *       immediate termination with diagnostic output rather than the propagation
 *       of error conditions through the call stack.
 *
 * @note The memory regions allocated through this interface do not possess
 *       inherent thread-safety properties and require explicit synchronization
 *       primitives to ensure correctness under concurrent access patterns.
 */

#ifndef ANVIL_MEMORY_ALLOCATION_HPP
#define ANVIL_MEMORY_ALLOCATION_HPP

#include "error/status.hpp"

namespace anvil::memory {

/**
 * @brief Encapsulates metadata for an aligned memory block allocation, storing information
 *        about the memory mapping and allocation state.
 *
 * This structure is prepended to user-aligned memory blocks to track allocation details.
 * It enables deallocation and lazy commitment operations by storing the original mapping
 * base address and capacity information.
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
 * virtual_capacity | size_t | sizeof(size_t) | Total virtual memory capacity allocated (lazy) or total size (eager)
 * capacity         | size_t | sizeof(size_t) | Current committed/writable byte capacity from mapping base
 * page_count       | size_t | sizeof(size_t) | Number of committed pages (`capacity / PAGE_SIZE`)
 *
 * @note On 64-bit systems: sizeof(Metadata) = 4 * 8 = 32 bytes
 */
struct Metadata {
        void*       base;             ///< Original mmap base address
        std::size_t virtual_capacity; ///< Total reserved virtual memory
        std::size_t capacity;         ///< Current committed capacity
        std::size_t page_count;       ///< Number of committed pages
};
static_assert(sizeof(Metadata) == 32, "Metadata should be 32 bytes (4 * 8 bytes on 64-bit systems)"); // NOLINT
static_assert(alignof(Metadata) == alignof(void*), "Metadata should have the natural alignment of a void pointer");

/**
 * @brief Reserves virtual memory pages without eagerly committing all physical pages.
 *
 * Reserves a virtual address range and returns an aligned user pointer through
 * `mem_out`. Only the first bookkeeping page is made writable for allocator
 * metadata; the remaining reserved pages are initially inaccessible until
 * committed with `anvil_memory_commit`.
 *
 * @pre `mem_out != nullptr`
 * @pre `*mem_out == nullptr`
 * @pre `capacity > 0`
 * @pre `capacity <= MAX_CAPACITY`
 * @pre sufficient virtual address space exists for allocation
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`
 *
 * @post On success, `*mem_out` points to an address aligned to `alignment`.
 * @post On success, only the first page of the mapping is writable; remaining pages are inaccessible until committed.
 *
 * @param[out] mem_out   Receives the aligned base address of the user-visible allocation.
 * @param[in]  capacity  Requested user capacity in bytes.
 * @param[in]  alignment Alignment of the returned initial address point.
 * @return Error         `OK` on success, otherwise an error code.
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_alloc_lazy(void** mem_out, std::size_t capacity, std::size_t alignment) noexcept;

/**
 * @brief Allocates writable memory eagerly.
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
 * @post On success, `*mem_out` points to an address aligned to `alignment`.
 * @post On success, the mapped region is readable and writable immediately.
 *
 * @param[out] mem_out   Receives the aligned base address of the user-visible allocation.
 * @param[in]  capacity  Requested user capacity in bytes.
 * @param[in]  alignment Alignment of the returned initial address point.
 * @return Error         `OK` on success, otherwise an error code.
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_alloc_eager(void** mem_out, std::size_t capacity, std::size_t alignment) noexcept;

/**
 * @brief Releases an allocation created by this module.
 *
 * Unmaps the full backing range associated with `ptr`, including reserved and
 * committed pages, and returns it to the operating system.
 *
 * @pre ptr != nullptr
 * @pre ptr must reference memory allocated by anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 *
 * @param[in] ptr        Address denoting the commencement of the memory region
 *                       to be returned to the computational environment.
 * @return Error         Error code indicating success or failure of the deallocation operation
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_dealloc(void* ptr) noexcept;

/**
 * @brief Commits additional pages in a lazily allocated virtual region.
 *
 * Expands the writable/accessible committed prefix of the allocation by
 * rounding `commit_size` up to the system page size and changing page
 * protections accordingly.
 *
 * @pre `ptr != nullptr`
 * @pre ptr must reference memory allocated with anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 * @pre `commit_size > 0`
 * @pre `commit_size <= MAX_CAPACITY`
 *
 * @post On success, committed capacity increases by `ceil(commit_size / PAGE_SIZE) * PAGE_SIZE`.
 * @post On success, `page_count` is recomputed to match committed capacity.
 *
 * @param[in] ptr           Address denoting the commencement of the memory region
 *                          to which additional physical memory resources should be committed.
 * @param[in] commit_size   Size in bytes of additional physical memory to commit.
 *
 * @return Error            Error code indicating success or failure of committing additional physical memory.
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_commit(void* ptr, std::size_t commit_size) noexcept;

} // namespace anvil::memory

#endif // ANVIL_MEMORY_ALLOCATION_HPP
