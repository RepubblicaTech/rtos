#ifndef FREELIST_H
#define FREELIST_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct freelist_node {
    size_t length; // length is in bytes

    struct freelist_node *next;
} freelist_node;

#endif
