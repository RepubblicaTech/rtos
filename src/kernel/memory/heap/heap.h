#ifndef HEAP_H
#define HEAP_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROUND_DOWN(n, a) ((n) & ~((a) - 1))
#define ROUND_UP(n, a)   (((n) + (a) - 1) & ~((a) - 1))

#define MIN(a, b) (a < b ? a : b)

#define PREFIX(fun) k##fun

#define HEAPMAGIC_AVAIL 0xC00DC00B // GOOD GOOB
#define HEAPMAGIC_UNAV  0xDEADD00D // DEAD DOOD

#define HEAPVER_MAJOR 0
#define HEAPVER_MINOR 1

typedef struct heapmem_node_t {
    uint32_t magic;

    size_t len;     // usable data length (in bytes)
    bool allocated; // to be used with magic bytes

    struct heapmem_node_t *next;
} heapmem_node_t;

#define HEAP_ALIGN(p)   p + sizeof(heapmem_node_t)
#define HEAP_UNALIGN(p) p - sizeof(heapmem_node_t)

#define HEAP_PAGES 2 // default pages to allocate for root node

// NO WAY your fancy malloc stuff
void *PREFIX(malloc)(size_t);
void PREFIX(free)(void *);
void *PREFIX(calloc)(size_t times, size_t size);
void *PREFIX(realloc)(void *p_old, size_t size);

/*   the following needs to be implemented by the kernel    */

extern unsigned long long page_size; // page frame size (sounds hella illegal)

// asks the bottom-level MM <size> bytes (not page-aligned)
// save the actually allocated size to <size_out> (might be NULL, and you
// shouldn't save it in that case)
void *heap_alloc(size_t size, size_t *size_out);
// releases a pointer to the same bottom-level MM
void heap_dealloc(void *);

// Locks/releases a spinlock/mutex/whatever you want
void heap_lock();
void heap_unlock();

// prints debug info if (HEAP_DEBUG is defined)
#ifdef HEAP_DEBUG
void heap_debug(const char *, ...);
#endif

// your usual mem*** functions, if you're a sane person you should have these
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif // HEAP_H