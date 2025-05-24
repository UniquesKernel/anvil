/**
 * @file dynamic_allocator.h
 * @brief Defines the interface for a dynamic allocator.
 *
 * A dynamic allocator is a memory allocation strategy that manages multiple memory blocks
 * and can grow by allocating new blocks when needed. It provides more flexibility than
 * a scratch allocator by dynamically expanding its capacity, making it suitable for
 * longer-lived data with unpredictable memory requirements.
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

FREE_DYNAMIC_ATTRIBUTE WARN_IF_NOT_USED DynamicAllocator* dynamic_allocator_create(const size_t capacity, const size_t alignment);

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
 */
void                                                      dynamic_allocator_destroy(DynamicAllocator** allocator);

#endif // ANVIL_MEMORY_DYNAMIC_ALLOCATOR_H