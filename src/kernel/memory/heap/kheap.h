#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void kmalloc_init(void *(*alloc_pages)(size_t pages),
                  void (*free_pages)(void *ptr, size_t pages));

void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t new_size);
void *kcalloc(size_t num, size_t size);

#ifdef __cplusplus
}
#endif

#endif // KMALLOC_H
