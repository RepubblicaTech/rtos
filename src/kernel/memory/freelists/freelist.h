#ifndef FREELIST_H
#define FREELIST_H 1

#include <stddef.h>
#include <stdint.h>

typedef struct freelist_node {
    struct freelist_node *next;
    struct freelist_node *prev;

    size_t length;
} freelist_node;

#endif
