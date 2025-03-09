#include "ramfs.h"
#include "fs/vfs/vfs.h"
#include "memory/heap/liballoc.h"
#include "stdio.h"

#include "util/string.h"
#include "util/util.h"

vfs_ops_t ramfs_vfs_ops = {.mount  = ramfs_mount,
                           .umount = ramfs_umount,
                           .lookup = ramfs_lookup,
                           .statfs = ramfs_statfs};

vnode_ops_t ramfs_vnode_ops = {.read   = ramfs_read,
                               .write  = ramfs_write,
                               .create = ramfs_create,
                               .mkdir  = ramfs_mkdir,
                               .remove = ramfs_remove};

mount_t *ramfs_new() {
    mount_t *mount = kmalloc(sizeof(mount_t));
    if (!mount) {
        debugf_warn("Failed to allocate memory for RAMFS mount");
        return NULL;
    }

    ramfs_data_t *data = kmalloc(sizeof(ramfs_data_t));
    memset(data, 0x00, sizeof(ramfs_data_t));
    if (!data) {
        debugf_warn("Failed to allocate memory for RAMFS data");
        kfree(mount);
        return NULL;
    }
    memset(data, 0x00, sizeof(ramfs_data_t));

    mount->data = data;
    mount->ops  = kmalloc(sizeof(vfs_ops_t));
    mount->ops  = &ramfs_vfs_ops;
    mount->next = NULL;
    mount->prev = NULL;

    return mount;
}

int ramfs_mount(struct mount *vfs, struct vnode *parent, const char *name) {
    if (!vfs || !name) {
        return -1;
    }

    ramfs_data_t *data = vfs->data;

    if (parent) {
        data->root_node = create_vnode(parent, name, VNODE_DIR,
                                       VNODE_FLAG_MOUNTPOINT, &ramfs_vnode_ops);
    } else {
        data->root_node        = kmalloc(sizeof(vnode_t));
        data->root_node->child = NULL;
        data->root_node->type  = VNODE_DIR;
        data->root_node->flags = VNODE_FLAG_MOUNTPOINT;
        strncpy(data->root_node->name, "/", sizeof(data->root_node->name));
    }

    if (!data->root_node) {
        return -1;
    }

    if (parent) {
        vnode_t *current = parent->child;
        if (current == NULL) {
            parent->child = data->root_node;
        } else {
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = data->root_node;
        }
    }

    // tell the parent that it has a new child
    data->root_node->parent = parent;
    data->root_node->mount  = vfs;

    return 0;
}

int ramfs_umount(struct mount *vfs) {
    if (!vfs) {
        return -1;
    }

    ramfs_data_t *data = vfs->data;
    if (data->root_node) {
        kfree(data->root_node);
    }

    kfree(data);
    kfree(vfs);

    return 0;
}

int ramfs_lookup(struct mount *vfs, const char *path, struct vnode **res_vnodes,
                 size_t res_vnodes_size) {
    if (!vfs || !path || !res_vnodes || !res_vnodes_size) {
        return -5; // Invalid arguments
    }

    vnode_t *current_vnode = vfs->root;
    if (!current_vnode) {
        return -1; // No root node
    }

    // Handle root path
    if (path[0] == '/') {
        path++;
    }

    // Special case: empty path means root
    if (*path == '\0') {
        res_vnodes[0] = current_vnode;
        return 1;
    }

    // Make a copy of the path that we can modify
    char path_copy[256]; // Adjust size as needed
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);
    size_t i    = 0;

    // Store the root node as the first result
    if (i < res_vnodes_size) {
        res_vnodes[i++] = current_vnode;
    }

    // Process each path component
    while (token != NULL) {
        if (current_vnode->type != VNODE_DIR) {
            return -4; // Not a directory
        }

        vnode_t *found = NULL;
        vnode_t *child = current_vnode->child;

        while (child) {
            if (strcmp(child->name, token) == 0) {
                found = child;
                break;
            }
            child = child->next;
        }

        if (!found) {
            return -1; // No such file or directory
        }

        current_vnode = found;

        if (i < res_vnodes_size) {
            res_vnodes[i] = current_vnode;
        }
        i++;

        token = strtok_r(NULL, "/", &saveptr);
    }

    return i;
}

int ramfs_statfs(struct mount *vfs, struct statfs *buf) {
    if (!vfs || !buf) {
        return -1;
    }

    buf->f_bsize = 4096;
    strncpy(buf->f_fstypename, "ramfs", 16);

    return 0;
}

int ramfs_read(struct vnode *vnode, void *buf, size_t size, size_t offset) {
    if (!vnode || !buf) {
        return -1;
    }

    ramfs_file_data_t *data = vnode->data;
    if (!data) {
        return -1;
    }

    if (offset >= data->size) {
        return 0;
    }

    size_t remaining = data->size - offset;
    size_t to_read   = size < remaining ? size : remaining;

    memcpy(buf, data->data + offset, to_read);

    return to_read;
}
int ramfs_write(struct vnode *vnode, const void *buf, size_t size,
                size_t offset) {
    if (!vnode || !buf) {
        return -1;
    }

    ramfs_file_data_t *data = vnode->data;
    if (!data) {
        return -1;
    }

    if (offset + size > data->capacity) {
        size_t new_capacity = offset + size;
        void *new_data      = krealloc(data->data, new_capacity);
        if (!new_data) {
            return -1;
        }

        data->data     = new_data;
        data->capacity = new_capacity;
    }

    memcpy(data->data + offset, buf, size);

    if (offset + size > data->size) {
        data->size = offset + size;
    }

    return size;
}

int ramfs_mkdir(struct vnode *parent, const char *name, int mode) {
    (void)mode;
    vnode_t *new_vnode =
        create_vnode(parent, name, VNODE_DIR, 0, &ramfs_vnode_ops);
    if (!new_vnode) {
        return -1;
    }

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

    return 0;
}

int ramfs_create(struct vnode *parent, const char *name, int mode) {
    (void)mode;
    vnode_t *new_vnode =
        create_vnode(parent, name, VNODE_FILE, 0, &ramfs_vnode_ops);
    if (!new_vnode) {
        return -1;
    }
    return 0;
}

int ramfs_remove(struct vnode *vnode) {

    debugf_debug("Removed vnode '%s'\n", vnode->name);
    if (!vnode) {
        return -1;
    }

    vnode_t *parent = vnode->parent;
    if (!parent) {
        return -1;
    }

    vnode_t *current = parent->child;
    if (current == vnode) {
        parent->child = vnode->next;
    } else {
        while (current->next != vnode) {
            current = current->next;
        }
        current->next = vnode->next;
    }

    kfree(vnode);

    return 0;
}