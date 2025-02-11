#ifndef FLANTERM_WRAPPERS_H
#define FLANTERM_WRAPPERS_H 1

#include <stddef.h>

void *flanterm_alloc(size_t size);
void flanterm_free(void *ptr, size_t size);

#endif