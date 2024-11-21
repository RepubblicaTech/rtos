#ifndef FREELIST_H
#define FREELIST_H 1

#include <stddef.h>

typedef struct freelist_entry {
	struct freelist_entry *next;
	struct freelist_entry *prev;
} freelist_entry;

#endif
