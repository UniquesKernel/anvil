/**
 * @file dynamic_allocator.h
 * @brief Defines the interface for a dynamic allocator.
 *
 * A dynamic allocator is a memory allocation strategy that manages multiple memory blocks
 * and can grow by allocating new blocks when needed. It provides more flexibility than
 * a scratch allocator by dynamically expanding its capacity, making it suitable for
 * longer-lived data with unpredictable memory requirements.
 *
 * @warning These allocators are not thread-safe. Concurrent access from multiple threads
 * may lead to race conditions and undefined behavior.
 */
#ifndef ANVIL_MEMORY_DYNAMIC_ALLOCATOR_H
#define ANVIL_MEMORY_DYNAMIC_ALLOCATOR_H

#if defined(__GNUC___) || defined(__clang__)
#define MALLOC_ATTRIBUTE       __attribute__((malloc))
#define FREE_DYNAMIC_ATTRIBUTE __attribute__((malloc(dynamic_allocator_destroy)))
#define WARN_IF_NOT_USED       __attribute__((warn_unused_result))
#else
#define MALLOC_ATTRIBUTE
#define FREE_DYNAMIC_ATTRIBUTE
#define WARN_IF_NOT_USED
#endif

#include <stddef.h>

typedef struct dynamic_allocator_t                        DynamicAllocator;

/**
 * @brief Creates a dynamic allocator, with an initial capacity and memory alignment.
 *
 * @invariant `alignment` is a power of two.
 * @invariant `initial_capacity` is larger than zero.
 *
 * @param[in] initial_capacity is the size of the allocator's memory.
 * @param[in] alignment is the alignment of the memory in the allocator.
 * @returns DynamicAllocator*
 *
 * @note Breaking invariants will cause the program to crash.
 */
FREE_DYNAMIC_ATTRIBUTE WARN_IF_NOT_USED DynamicAllocator* dynamic_allocator_create(const size_t initial_capacity, const size_t alignment);

/**
 * @brief allocates memory from a dynamic allocator and return the memory to the caller.
 *
 * @invariant `size` must be larger than zero.
 * @invariant `count` must be larger than zero.
 * @invariant `allocator` must be non-NULL.
 *
 * @param[out] allocator that you allocate memory from
 * @param[in] size is the size of the data type you want to store
 * @param[in] count is the number of entities you store in the memory
 * @returns void* a pointer to the allocated memory
 *
 * @note The allocator may automatically allocate new memory blocks if the current
 * ones don't have enough space for the requested allocation.
 * @note Breaking invariants will cause the program to crash.
 */
MALLOC_ATTRIBUTE WARN_IF_NOT_USED void*                   dynamic_allocator_alloc(DynamicAllocator* allocator, const size_t size, const size_t count);

/**
 * @brief Reset a dynamic allocator allowing existing allocation to be overwritten
 *
 * @invariant `allocator` is non-NULL.
 *
 * @param[out] allocator to reset.
 *
 * @note Any allocated memory, allocated before calling the method should be considered invalid
 * after calling this function. All memory blocks except the first one will be deallocated.
 * @note Breaking invariants will cause the program to crash.
 */
void                                                      dynamic_allocator_reset(DynamicAllocator* restrict allocator);

/**
 * @brief Destroy a dynamic allocator, deallocating all memory blocks it holds
 *
 * @invariant allocator is non-NULL
 * @invariant *allocator is non-NULL
 *
 * @param[out] allocator to destroy
 *
 * @note All allocations made using the allocator should be considered invalid after this function is called
 * @note Breaking invariants will cause the program to crash.
 */
void                                                      dynamic_allocator_destroy(DynamicAllocator** allocator);

#endif // ANVIL_MEMORY_DYNAMIC_ALLOCATOR_H