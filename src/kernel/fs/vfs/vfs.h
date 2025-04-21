#ifndef VFS_H
#define VFS_H

#include "types.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VNODE_DIR  = 0x0001,
    VNODE_FILE = 0x0002,
    VNODE_DEV  = 0x0003,
} vnode_type_t;

#define VNODE_FLAG_MOUNTPOINT 0x0001

struct vnode;
struct mount;

typedef struct vnode_ops {
    int (*read)(struct vnode *vnode, void *buf, size_t size, size_t offset);
    int (*write)(struct vnode *vnode, const void *buf, size_t size,
                 size_t offset);
} vnode_ops_t;

typedef struct vnode {
    struct vnode *parent;
    struct vnode *next;
    struct vnode *child;
    struct mount *mount;

    vnode_type_t type;
    char name[256];
    uint64_t size;
    void *data;

    vnode_ops_t *ops;
    uint32_t flags; // some flags ored together

    lock_t lock;
} vnode_t;

typedef struct mount {
    vnode_t *root;      // Root vnode of the filesystem
    struct mount *next; // Next mount point
    struct mount *prev; // Previous mount point
    char *mountpoint;   // Path to the mount point, e.g. /mnt/
    char *type;         // File system type, e.g. "ramfs"
    void *data;         // Private data for the file system
} mount_t;

extern mount_t *root_mount;

void vfs_init(void);
vnode_t *vfs_lookup(vnode_t *parent, const char *name);
mount_t *vfs_mount(const char *path, const char *type);
vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type);
void vfs_umount(mount_t *mount);
int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset);
int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset);
vnode_t *vfs_lazy_lookup(mount_t *mount, const char *path);
char *vfs_get_full_path(vnode_t *vnode);
void vfs_debug_print(mount_t *mount);
char *vfs_type_to_str(vnode_type_t type);
void vfs_delete_node(vnode_t *vnode);

#define VFS_ROOT()    (root_mount->root)
#define VFS_GET(path) (vfs_lazy_lookup(root_mount, path))
#define VFS_READ(path)                                                         \
    ({                                                                         \
        vnode_t *node = VFS_GET(path);                                         \
        assert(node);                                                          \
        char *buf       = kmalloc(node->size + 1);                             \
        buf[node->size] = 0;                                                   \
        vfs_read(node, buf, node->size, 0);                                    \
        buf;                                                                   \
    })
#define VFS_WRITE(path, data)                                                  \
    ({                                                                         \
        vnode_t *node = VFS_GET(path);                                         \
        assert(node);                                                          \
        vfs_write(node, data, strlen(data), 0);                                \
    })

#endif // VFS_H