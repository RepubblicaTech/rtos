#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define VNODE_FLAG_MOUNTPOINT 0x1
#define VNODE_FLAG_READONLY   0x2
#define VNODE_FLAG_EXECUTABLE 0x4

typedef enum vnode_type {
    VNODE_DIR,
    VNODE_FILE,
    VNODE_DEVICE,
    VNODE_UNKNOWN
} vnode_type_t;

struct vnode;
struct mount;
struct statfs;

typedef struct vfs_ops {
    int (*mount)(struct mount *vfs, struct vnode *parent, const char *name);
    int (*umount)(struct mount *vfs);
    int (*lookup)(struct mount *vfs, const char *path,
                  struct vnode **res_vnodes, size_t res_vnodes_size);
    int (*statfs)(struct mount *vfs, struct statfs *buf);
} vfs_ops_t;

typedef struct vnode_ops {
    int (*read)(struct vnode *vnode, void *buf, size_t size, size_t offset);
    int (*write)(struct vnode *vnode, const void *buf, size_t size,
                 size_t offset);

    int (*mkdir)(struct vnode *parent, const char *name, int mode);
    int (*create)(struct vnode *parent, const char *name, int mode);
    int (*remove)(struct vnode *vnode);
} vnode_ops_t;

typedef struct statfs {
    uint64_t f_bsize;
    char f_fstypename[16];
} statfs_t;

typedef struct vnode {
    struct vnode *parent;
    struct vnode *next;
    struct vnode *child;
    struct mount *mount;

    vnode_type_t type;
    char name[256];
    uint64_t size;
    void *data;

    int flags;

    vnode_ops_t *ops;
} vnode_t;

typedef struct mount {
    vnode_t *root;
    struct mount *next;
    struct mount *prev;

    vfs_ops_t *ops;

    void *data;
} mount_t;

extern mount_t *root_mount;

void vfs_init(void);

vnode_t *create_vnode(struct vnode *parent, const char *name, vnode_type_t type,
                      int flags, vnode_ops_t *ops);

int vfs_mount(struct mount *mount, struct vnode *parent, const char *type);
int vfs_umount(struct mount *mount);
int vfs_lookup(struct mount *mount, const char *path, vnode_t **res_vnode,
               size_t res_vnode_size);
int vfs_statfs(struct mount *mount, struct statfs *buf);

int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset);
int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset);

#define VFS_ROOT() (root_mount->root)

int vfs_mkdir(struct vnode *parent, const char *name, int mode);
int vfs_create(struct vnode *parent, const char *name, int mode);
int vfs_remove(struct vnode *vnode);

#endif // VFS_H