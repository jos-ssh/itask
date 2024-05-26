/**
 * @file list.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-26
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_LIST_H
#define __INC_LIST_H

#include <inc/assert.h>
#include <inc/types.h>

struct List {
    struct List *prev, *next;
};

static inline void __attribute__((always_inline))
list_init(struct List* list) {
    list->next = list->prev = list;
}

static inline bool __attribute__((always_inline))
list_is_empty(struct List* list) {
    return list == list->next;
}

static inline void __attribute__((always_inline))
list_append(struct List* list, struct List* elem) {
    if (list == NULL || elem == NULL) {
        panic("Invalid call to list_append!");
    }
    if (list->next == NULL || list->prev == NULL) {
        panic("list_append called for corrupted list!");
    }
    elem->next = list->next;
    elem->next->prev = elem;
    elem->prev = list;
    list->next = elem;
}

static inline struct List* __attribute__((always_inline))
list_del(struct List* list) {
    if (list == NULL) {
        panic("Invalid call to list_del!");
    }
    if (list->next == NULL || list->prev == NULL) {
        panic("list_del called for corrupted list");
    }

    if (list->next == list->prev)
        return NULL;

    struct List* prev = list->prev;
    struct List* next = list->next;

    prev->next = next;
    next->prev = prev;

    list_init(list);
    return list;
}

#endif /* list.h */
