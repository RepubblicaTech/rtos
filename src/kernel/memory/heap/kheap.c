#include <memory/heap/kheap.h>
#include <util/string.h>

#define MIN_ALLOC   16
#define MAX_ALLOC   4096
#define PAGE_SIZE   4096
#define CACHE_COUNT 9 // log2(4096) - log2(16) + 1

typedef struct slab {
    struct slab *next;
    uint8_t *bitmap;
    void *mem;
    size_t obj_size;
    size_t used;
    size_t capacity;
} slab_t;

typedef struct {
    size_t obj_size;
    slab_t *slabs;
} slab_cache_t;

static slab_cache_t slab_caches[CACHE_COUNT];

static inline size_t align_up(size_t x, size_t align) {
    return (x + align - 1) & ~(align - 1);
}

static inline int log2_floor(size_t x) {
    int r = 0;
    while (x >>= 1)
        r++;
    return r;
}

static inline int size_to_index(size_t size) {
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;
    if (size > MAX_ALLOC)
        return -1;
    return log2_floor(size) - 4;
}

static slab_t *slab_create(size_t obj_size) {
    slab_t *slab = (slab_t *)os_alloc_pages(1);
    if (!slab)
        return NULL;

    slab->obj_size = obj_size;
    slab->capacity = PAGE_SIZE / obj_size;
    slab->used     = 0;

    slab->mem = os_alloc_pages(1);
    if (!slab->mem) {
        os_free_pages(slab, 1);
        return NULL;
    }

    size_t bitmap_size = (slab->capacity + 7) / 8;
    slab->bitmap = os_alloc_pages((bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE);
    if (!slab->bitmap) {
        os_free_pages(slab->mem, 1);
        os_free_pages(slab, 1);
        return NULL;
    }

    memset(slab->bitmap, 0, bitmap_size);
    slab->next = NULL;
    return slab;
}

static void *slab_alloc(slab_t *slab) {
    for (size_t i = 0; i < slab->capacity; i++) {
        if (!(slab->bitmap[i / 8] & (1 << (i % 8)))) {
            slab->bitmap[i / 8] |= (1 << (i % 8));
            slab->used++;
            return (uint8_t *)slab->mem + i * slab->obj_size;
        }
    }
    return NULL;
}

static void slab_free(slab_t *slab, void *ptr) {
    uintptr_t base = (uintptr_t)slab->mem;
    uintptr_t p    = (uintptr_t)ptr;
    size_t index   = (p - base) / slab->obj_size;

    if (index < slab->capacity) {
        size_t byte = index / 8;
        size_t bit  = index % 8;
        if (slab->bitmap[byte] & (1 << bit)) {
            slab->bitmap[byte] &= ~(1 << bit);
            slab->used--;
        }
    }
}

void kmalloc_init() {
    for (int i = 0; i < CACHE_COUNT; i++) {
        slab_caches[i].obj_size = (size_t)1 << (i + 4); // 2^4 = 16
        slab_caches[i].slabs    = NULL;
    }
}

void *kmalloc(size_t size) {
    if (size == 0)
        return NULL;

    if (size > MAX_ALLOC) {
        size_t pages = (align_up(size, PAGE_SIZE)) / PAGE_SIZE;
        return os_alloc_pages(pages);
    }

    int index = size_to_index(size);
    if (index < 0 || index >= CACHE_COUNT)
        return NULL;

    slab_cache_t *cache = &slab_caches[index];
    slab_t *slab        = cache->slabs;

    while (slab) {
        if (slab->used < slab->capacity) {
            return slab_alloc(slab);
        }
        slab = slab->next;
    }

    slab = slab_create(cache->obj_size);
    if (!slab)
        return NULL;

    slab->next   = cache->slabs;
    cache->slabs = slab;

    return slab_alloc(slab);
}

void kfree(void *ptr) {
    if (!ptr)
        return;

    uintptr_t uptr = (uintptr_t)ptr;

    for (int i = 0; i < CACHE_COUNT; i++) {
        slab_t *slab = slab_caches[i].slabs;
        while (slab) {
            uintptr_t base = (uintptr_t)slab->mem;
            if (uptr >= base && uptr < base + PAGE_SIZE) {
                slab_free(slab, ptr);
                return;
            }
            slab = slab->next;
        }
    }

    // Large allocation
    // We don't track size; assume freeing single page
    os_free_pages(ptr, 1);
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr)
        return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, new_size); // over-copy, but simple
    kfree(ptr);
    return new_ptr;
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr    = kmalloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}
