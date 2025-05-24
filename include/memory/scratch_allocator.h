/**
 * @file scratch_allocator.h
 * @brief Defines the interface for a scratch allocator.
 *
 * A scratch allocator is a memory allocation strategy that provides fast,
 * typically linear, allocation from a pre-allocated buffer. It is designed
 * for temporary allocations where all allocated memory can be freed at once
 * by resetting the allocator. This makes it suitable for short-lived data.
 */
#ifndef ANVIL_MEMORY_SCRATCH_ALLOCATOR_H
#define ANVIL_MEMORY_SCRATCH_ALLOCATOR_H

#include <stddef.h>

#if defined(__GNUC___) || defined(__clang__)
#define MALLOC_ATTRIBUTE       __attribute__((malloc))
#define FREE_SCRATCH_ATTRIBUTE __attribute__((malloc(scratch_allocator_destroy)))
#define WARN_IF_NOT_USED       __attribute__((warn_unused_result))
#else
#define MALLOC_ATTRIBUTE
#define FREE_SCRATCH_ATTRIBUTE
#define WARN_IF_NOT_USED
#endif

typedef struct ScratchAllocator ScratchAllocator;

/**
 * @brief Creates a scratch allocator, with an capacity and memory alignment.
 *
 * @invariant `alignment` is a power of two.
 * @invariant `capacity` is larger than zero.
 *
 * @param[in] capacity is the size of the allocators memory.
 * @param[in] alignment is the alignment of the memory in the allocator.
 * @returns ScratchAllocator*
 */
ScratchAllocator*               scratch_allocator_create(const size_t capacity,
							 const size_t alignment) FREE_SCRATCH_ATTRIBUTE WARN_IF_NOT_USED;

/**
 * @brief allocates memory from a scratch allocator and return the memory to the caller.
 *
 * @invariant `size` must be larger than zero.
 * @invariant `count` must be larger than zero.
 * @invariant `allocator` must be non Null.
 *
 * @param[out] allocator that you allocate memory from
 * @param[in] size is the size of the data type you want to store
 * @param[in] count is the number of entities you store in the memory
 *
 * @note The allocator may pad the allocated memory if necessary, thus returning
 * slightly more memory than asked for, if necessary to maintain alignment.
 */
void*                           scratch_allocator_alloc(ScratchAllocator* restrict allocator, const size_t size,
							const size_t count) MALLOC_ATTRIBUTE WARN_IF_NOT_USED;

/**
 * @brief Reset a scratch allocator allowing existing allocation to be overwritten
 *
 * @invariant `allocator` is non null.
 * @invariant `size` is larger than 0
 * @invariant `count` is larger than 0
 *
 * @param[out] allocator to reset.
 *
 * @note Any allocated memory, allocated before calling the method should be considered invalid
 * after calling this function.
 */
void                            scratch_allocator_reset(ScratchAllocator* restrict allocator);

/**
 * @brief Destroy a scratch allocator, deallocating all memory it holds
 *
 * @invariant allocator is non null
 * @invariant *allocator is non null
 *
 * @param[out] allocator to destroy
 *
 * @note All allocations made using the allocator should be considered invalid after this function is called
 */
void                            scratch_allocator_destroy(ScratchAllocator** allocator);

#undef MALLOC_ATTRIBUTE
#undef FREE_SCRATCH_ATTRIBUTE
#undef WARN_IF_NOT_USED

#endif // ANVIL_MEMORY_SCRATCH_ALLOCATOR_H