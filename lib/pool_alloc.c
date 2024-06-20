#include <inc/pool_alloc.h>

#include <inc/assert.h>

void
pool_allocator_init(PoolAllocator* allocator) {
    assert(allocator->element_size >= sizeof(struct List));

    list_init(&allocator->free_list);
    char* pool = allocator->pool;
    const size_t elem_size = allocator->element_size;
    for (size_t i = 0; i < allocator->pool_element_count; ++i) {
        struct List* cur_entry = (struct List*)(pool + i * elem_size);
        list_append(&allocator->free_list, cur_entry);
    }
}

void*
pool_allocator_alloc_object(PoolAllocator* allocator) {
    if (list_is_empty(&allocator->free_list)) {
        return NULL;
    }

    return list_del(allocator->free_list.next);
}

void
pool_allocator_free_object(PoolAllocator* allocator, void* object) {
    const size_t elem_size = allocator->element_size;
    const size_t pool_size = allocator->pool_element_count * elem_size;
    const uintptr_t pool_start = (uintptr_t)allocator->pool;
    const uintptr_t pool_end = pool_start + pool_size;
    const uintptr_t addr = (uintptr_t)object;

    assert(pool_start <= addr);
    assert(addr < pool_end);

    assert((addr - pool_start) % elem_size == 0);

    assert(object != NULL);
    list_append(&allocator->free_list, object);
}
