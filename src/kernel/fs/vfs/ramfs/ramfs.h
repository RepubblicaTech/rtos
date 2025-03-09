#ifndef VFS_RAMFS_H
#define VFS_RAMFS_H

#include <fs/vfs/vfs.h>

extern vfs_ops_t ramfs_vfs_ops;
extern vnode_ops_t ramfs_vnode_ops;

typedef struct ramfs_file_data {
    void *data;
    size_t size;
    size_t capacity;
} ramfs_file_data_t;

typedef struct ramfs_data {
    vnode_t *root_node;
} ramfs_data_t;

mount_t *ramfs_new();

int ramfs_mount(struct mount *vfs, struct vnode *parent, const char *name);
int ramfs_umount(struct mount *vfs);
int ramfs_lookup(struct mount *vfs, const char *path, struct vnode **res_vnodes,
                 size_t res_vnodes_size);
int ramfs_statfs(struct mount *vfs, struct statfs *buf);

int ramfs_read(struct vnode *vnode, void *buf, size_t size, size_t offset);
int ramfs_write(struct vnode *vnode, const void *buf, size_t size,
                size_t offset);
int ramfs_mkdir(struct vnode *parent, const char *name, int mode);
int ramfs_create(struct vnode *parent, const char *name, int mode);
int ramfs_remove(struct vnode *vnode);

#endif // VFS_RAMFS_H