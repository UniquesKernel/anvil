/**
 * @file memory_allocation_internal.h
 * @brief Internal memory allocation functions for the Anvil Memory system.
 *
 * This header file defines the core low-level memory allocation functions used by
 * the Anvil Memory allocators. It provides aligned memory allocation with proper
 * metadata tracking to support safe deallocation.
 */

#ifndef MEMORY_ALLOCATION_H
#define MEMORY_ALLOCATION_H

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Encapsulates metadata for an aligned memory block, primarily storing the base pointer
 *        and total size of the original allocation to facilitate deallocation.
 *
 * @invariant base != NULL
 * @invariant total_size > 0
 *
 * @note This structure is typically prepended to the user-aligned memory block.
 *
 * Fields     | Type   | Size (Bytes)  | Description
 * ---------- | ------ | ------------- | -------------------------------------------------
 * base       | void*  | sizeof(void*) | Pointer to the start of the originally allocated (potentially unaligned) memory
 * block. total_size | size_t | sizeof(size_t)| Total size of the originally allocated memory block in bytes.
 */
typedef struct Metadata {
	void*  base;
	size_t total_size;
} Metadata;
static_assert(sizeof(Metadata) == 16 || sizeof(Metadata) == 8,
	      "Metadata should be 16 or 8 bytes depending on architecture");
static_assert(alignof(Metadata) == alignof(void*), "should have the natural alignment of a void pointer");

/**
 * @brief Allocates a memory segment of specified `size` with `alignment` constraints.
 *
 * @invariant size > 0
 * @invariant alignment is a power of two (i.e., 1, 2, 4, 8, 16, ..., 65536)
 * @invariant alignment <= 65536
 * @invariant System possesses sufficient memory for the requested allocation plus metadata.
 *
 * @param[in] size The quantum of memory to allocate (in bytes).
 * @param[in] alignment The required memory alignment. Must be a power of two and <= 2^16.
 * @return Pointer to the beginning of the aligned, user-accessible memory segment.
 *
 * @note This function employs a fail-fast paradigm; violations of preconditions result in program termination.
 * @note The actual physical allocation size will exceed `size` to accommodate alignment requirements and internal metadata.
 * @note Memory allocated by this function must be deallocated using `safe_aligned_free()`.
 */
void* __attribute__((malloc)) safe_aligned_alloc(const size_t size, const size_t alignment);

/**
 * @brief Determines if a given unsigned integer `x` is a power of two.
 *
 * @param[in] x The unsigned integer to evaluate.
 * @return `true` if `x` is a power of two (i.e., x = 2^n for some integer n >= 0, and x != 0); `false` otherwise.
 *
 * @note Zero (0) is not considered a power of two by this function.
 */
bool __attribute__((pure))    is_power_of_two(const size_t x);

/**
 * @brief Deallocates a memory segment previously allocated by standard, non-aligning allocation mechanisms.
 *
 * @param[in] ptr Pointer to the memory segment to deallocate. May be `NULL`.
 *
 * @note If `ptr` is `NULL`, no operation is performed, ensuring idempotent behavior with null pointers.
 * @note This function should not be utilized for memory segments allocated via `safe_aligned_alloc()`.
 */
void                          safe_free(void* ptr);

/**
 * @brief Deallocates a memory segment previously allocated by `safe_aligned_alloc()`.
 *
 * @param[in] ptr Pointer to the aligned, user-accessible memory segment to deallocate. May be `NULL`.
 *
 * @note If `ptr` is `NULL`, no operation is performed.
 * @note This function correctly interprets internal metadata, prepended to the user block, to free the entire originally allocated physical memory block.
 * @note Employing standard `free()` or `safe_free()` on memory allocated by `safe_aligned_alloc()` will lead to undefined behavior or resource leaks.
 */
void                          safe_aligned_free(void* ptr);

#endif // !MEMORY_ALLOCATION_H
