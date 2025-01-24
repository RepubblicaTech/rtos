#ifndef VMM_H
#define VMM_H 1

#include <stddef.h>
#include <stdint.h>

#include <limine.h>

#include <memory/paging/paging.h>

typedef struct virtmem_object_t {
    uint64_t base;
    size_t len;
    uint64_t flags;

    struct virtmem_object_t *next;
    struct virtmem_object_t *prev;
} virtmem_object_t;

typedef struct vmm_context_t {
    uint64_t *pml4_table;

    virtmem_object_t *root_vmo;
} vmm_context_t;

// VMO flags
#define VMO_PRESENT (1 << 0)
#define VMO_RW      (1 << 1)
#define VMO_USER    (1 << 2)
// this flag get's set when the VMO gets allocated
#define VMO_ALLOCATED (1 << 8)

// VMO "macro"flags
#define VMO_KERNEL    VMO_PRESENT
#define VMO_KERNEL_RW VMO_RW | VMO_KERNEL
#define VMO_USER_RW   VMO_PRESENT | VMO_RW | VMO_USER

<<<<<<< HEAD
void vmm_switch_ctx(vmm_context_t* new_ctx);
=======
void vmo_dump(virtmem_object_t *vmo);
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

<<<<<<< Updated upstream
virtmem_object_t *vmo_init(uint64_t base, size_t length, uint64_t flags);
void vmo_destroy(virtmem_object_t *vmo);

vmm_context_t *vmm_ctx_init(uint64_t *pml4, uint64_t flags);
=======
<<<<<<< Updated upstream
virtmem_object_t *vmo_init(uint64_t base, size_t length, uint64_t flags);
void vmo_destroy(virtmem_object_t *vmo);

vmm_context_t *vmm_ctx_init(uint64_t *pml4, uint64_t flags);
=======
virtmem_object_t* vmo_init(uint64_t base, size_t length, uint64_t flags);
void vmo_dump(virtmem_object_t* vmo);
void vmo_destroy(virtmem_object_t* vmo);
virtmem_object_t* split_vmo_at(virtmem_object_t* src_vmo, size_t len);

vmm_context_t* vmm_ctx_init(uint64_t* pml4, uint64_t flags);
void vmm_ctx_destroy(vmm_context_t* ctx);
>>>>>>> Stashed changes
>>>>>>> Stashed changes

uint64_t vmo_to_page_flags(uint64_t vmo_flags);
uint64_t page_to_vmo_flags(uint64_t pg_flags);

<<<<<<< Updated upstream
virtmem_object_t *split_vmo_at(virtmem_object_t *src_vmo, size_t len);

void vmm_init(vmm_context_t *ctx);
=======
<<<<<<< Updated upstream
virtmem_object_t *split_vmo_at(virtmem_object_t *src_vmo, size_t len);

void vmm_init(vmm_context_t *ctx);
=======
void vmm_init(vmm_context_t* ctx);
>>>>>>> Stashed changes
>>>>>>> Stashed changes

#endif
