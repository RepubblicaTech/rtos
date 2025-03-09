#include "vfs.h"
#include "fs/vfs/ramfs/ramfs.h"
#include "stdio.h"

#include <memory/heap/liballoc.h>
#include <nanoprintf.h>
#include <stdint.h>
#include <util/string.h>

mount_t *root_mount = NULL;

void vfs_init(void) {
    mount_t *root = ramfs_new();
    if (!root) {
        debugf_panic("Failed to create root mount\n");
    }

    root->root        = kmalloc(sizeof(vnode_t));
    root->root->child = NULL;
    root->root->type  = VNODE_DIR;
    root->root->flags = VNODE_FLAG_MOUNTPOINT;
    strncpy(root->root->name, "/", sizeof(root->root->name));

    ramfs_mount(root, NULL, "/");

    root_mount = root;

    debugf_debug("VFS initialized. Addr: %.16llx\n", (uint64_t)root);
}

vnode_t *create_vnode(struct vnode *parent, const char *name, vnode_type_t type,
                      int flags, vnode_ops_t *ops) {

    vnode_t *new_vnode = kmalloc(sizeof(vnode_t));
    if (!new_vnode) {
        return NULL;
    }

    strncpy(new_vnode->name, name, sizeof(new_vnode->name));
    new_vnode->type  = type;
    new_vnode->child = NULL;
    new_vnode->next  = NULL;

    vnode_t *current = parent->child;
    if (current == NULL) {
        parent->child = new_vnode;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_vnode;
    }
    new_vnode->parent = parent;
    new_vnode->mount  = parent->mount;
    new_vnode->size   = 0;
    new_vnode->data   = NULL;
    new_vnode->ops    = ops;
    new_vnode->flags  = flags;

    debugf_debug("Created new vnode '%s' of type '%s', parent '%s'\n", name,
                 (type == VNODE_DIR) ? "directory" : "file",
                 new_vnode->parent->name);

    return new_vnode;
}

int vfs_mount(struct mount *mount, struct vnode *parent, const char *type) {
    if (!parent || !type || !mount) {
        return -1;
    }

    if (mount->ops->mount) {
        return mount->ops->mount(mount, parent, type);
    } else {
        return -1;
    }
}

int vfs_umount(struct mount *mount) {
    if (!mount) {
        return -1;
    }

    if (mount->ops->umount) {
        return mount->ops->umount(mount);
    } else {
        return -1;
    }
}

int vfs_lookup(struct mount *mount, const char *path, vnode_t **res_vnode,
               size_t res_vnode_size) {

    if (!mount || !path) {
        return -1;
    }

    if (mount->ops->lookup) {
        return mount->ops->lookup(mount, path, res_vnode, res_vnode_size);
    } else {
        return -1;
    }
}

int vfs_statfs(struct mount *mount, struct statfs *buf) {
    if (!mount || !buf) {
        return -1;
    }

    if (mount->ops->statfs) {
        return mount->ops->statfs(mount, buf);
    } else {
        return -1;
    }
}

int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset) {
    if (!vnode || !buf) {
        return -1;
    }

    if (vnode->ops->read) {
        return vnode->ops->read(vnode, buf, size, offset);
    } else {
        return -1;
    }
}

int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset) {
    if (!vnode || !buf) {
        return -1;
    }

    if (vnode->ops->write) {
        return vnode->ops->write(vnode, buf, size, offset);
    } else {
        return -1;
    }
}

int vfs_mkdir(struct vnode *parent, const char *name, int mode) {
    if (!parent || !name) {
        return -1;
    }

    if (parent->ops->mkdir) {
        return parent->ops->mkdir(parent, name, mode);
    } else {
        return -1;
    }
}

int vfs_create(struct vnode *parent, const char *name, int mode) {
    if (!parent || !name) {
        return -1;
    }

    if (parent->ops && parent->ops->create) {
        return parent->ops->create(parent, name, mode);
    } else {
        return -1;
    }
}

int vfs_remove(struct vnode *vnode) {
    if (!vnode) {
        return -1;
    }

    if (vnode->ops->remove) {
        return vnode->ops->remove(vnode);
    } else {
        return -1;
    }
}