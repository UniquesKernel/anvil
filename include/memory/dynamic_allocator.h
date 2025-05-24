/**
 * @file
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

typedef struct dynamic_allocator_t DynamicAllocator;

DynamicAllocator*                  dynamic_allocator_create() FREE_DYNAMIC_ATTRIBUTE WARN_IF_NOT_USED;

void*                              dynamic_allocator_alloc() MALLOC_ATTRIBUTE WARN_IF_NOT_USED;

void                               dynamic_allocator_reset();
void                               dynamic_allocator_destroy();

#undef MALLOC_ATTRIBUTE
#undef FREE_DYNAMIC_ATTRIBUTE
#undef WARN_IF_NOT_USED

#endif // ANVIL_MEMORY_DYNAMIC_ALLOCATOR_H