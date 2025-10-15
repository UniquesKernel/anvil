/**
 * @file memory_allocation.h
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

#include "memory/error.hpp"
#include <cstddef>

/**
 * @brief Allocation of virtual memory pages within the computational model
 *
 * This primitive operation establishes a mapping from virtual addresses to an
 * address space of specified extent, wherein the correspondence between virtual
 * and physical memory remains undefined until explicit commitment is enacted.
 * The allocation constitutes a reservation of virtual address space without
 * immediate physical memory binding.
 *
 * @pre `capacity > 0`
 * @pre sufficient virtual address space exists for allocation
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`
 *
 * The function yields a pointer to the allocated virtual address space upon
 * successful completion of the allocation operation.
 *
 * @param[in] capacity   Cardinality of the virtual address space measured in octets
 * @param[in] alignment  Alignment of the returned initial address point.
 * @return pointer       Element of the virtual address space denoting the base address
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] ANVIL_ATTR_ALLOCATOR void* anvil_memory_alloc_lazy(const size_t capacity, const size_t alignment);

/**
 * @brief Allocation of physical memory
 *
 * This primitive operation establishes an immediate physical binding of
 * physical memory. The provided memory is immediately available for both Read and Write
 * operations.
 *
 * @pre `capacity > 0`.
 * @pre address space exists for allocation
 * @pre `alignment` is a power of two.
 * @pre `MIN_ALIGNMENT <= alignment <= MAX_ALIGNMENT`
 *
 * The function yields a pointer to the allocated virtual address space upon
 * successful completion of the allocation operation.
 *
 * @param[in] capacity   Cardinality of the virtual address space measured in octets
 * @param[in] alignment  Alignment of the returned initial address point.
 * @return pointer       Element of the virtual address space denoting the base address
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] ANVIL_ATTR_ALLOCATOR void* anvil_memory_alloc_eager(const size_t capacity, const size_t alignment);

/**
 * @brief Reclamation of memory resources to the computational environment
 *
 * This operation effects the complete dissolution of the mapping established
 * between virtual and physical memory resources, returning all associated
 * memory to the domain of the operating system. The
 * operation terminates both the virtual address space reservation and any
 * committed physical memory bindings.
 *
 * @pre ptr != nullptr
 * @pre ptr must reference memory allocated by anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 *
 * @param[out] ptr       Address denoting the commencement of the memory region
 *                       to be returned to the computational environment
 * @return Error         Error code indicating success or failure of the deallocation operation
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_dealloc(void* ptr);

/**
 * @brief On demand commital of memory resources from virtual memory to physical memory
 *
 * This operation establishes read and write permission to an extension of the already
 * established mapping between virtual and physical memory resources.
 *
 * @pre ptr != nullptr
 * @pre ptr must reference memory allocated with anvil_memory_alloc_lazy or anvil_memory_alloc_eager
 * @pre commit_size must be positive and non zero
 *
 * @param[out] ptr          Address denoting the commencement of the memory region
 *                          to which additional physical memory resources should be commited.
 * @param[out] commit_size  the size (bytes) of additional physical resource to be commited.
 *
 * @return Error            Error code indicating success or failure of commiting additional physical memory.
 *
 * @note The compiler will express a warning if the return result is unused.
 */
[[nodiscard]] Error anvil_memory_commit(void* ptr, const std::size_t commit_size);

#endif // ANVIL_MEMORY_ALLOCATION_HPP