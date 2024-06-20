/**
 * @file pool_alloc.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-26
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_POOL_ALLOC_H
#define __INC_POOL_ALLOC_H

#include <inc/types.h>
#include <inc/env.h>
#include <inc/list.h>

struct PoolAllocator;

typedef void pool_init(struct PoolAllocator* allocator);
typedef void* pool_alloc_object(struct PoolAllocator* allocator);
typedef void pool_free_object(struct PoolAllocator* allocator,
                              void* object);

typedef struct PoolAllocator {
    void* pool;
    size_t element_size;
    size_t pool_element_count;

    struct List free_list;

    pool_init* init;
    pool_alloc_object* alloc;
    pool_free_object* free;

} PoolAllocator;

void pool_allocator_init(PoolAllocator* allocator);

void* pool_allocator_alloc_object(PoolAllocator* allocator);

void pool_allocator_free_object(PoolAllocator* allocator, void* object);

#define MAKE_ALLOCATOR(alloc_pool)                                          \
    (PoolAllocator) {                                                       \
        .pool = (alloc_pool),                                               \
        .element_size = sizeof(*(alloc_pool)),                              \
        .pool_element_count = sizeof((alloc_pool)) / sizeof(*(alloc_pool)), \
        .free_list = {.prev = NULL, .next = NULL},                          \
        .init = pool_allocator_init,                                        \
        .alloc = pool_allocator_alloc_object,                               \
        .free = pool_allocator_free_object                                  \
    }

#endif /* pool_alloc.h */
