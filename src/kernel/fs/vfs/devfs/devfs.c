#include "devfs.h"
#include "dev/device.h"
#include "fs/vfs/vfs.h"
#include "stdio.h"
#include "util/assert.h"
#include <memory/heap/liballoc.h>

mount_t *devfs_root = NULL;

int devfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset) {
    debugf("devfs recived read request for device: '%s'\n",
           vfs_get_full_path(vnode));
    device_t *dev = (device_t *)vnode->data;
    dev->read(dev, buf, size, offset);
    return 0;
}

int devfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset) {
    debugf("devfs recived write request for device: '%s'\n",
           vfs_get_full_path(vnode));
    device_t *dev = (device_t *)vnode->data;
    dev->write(dev, buf, size, offset);
    return 0;
}

int devfs_add_dev(device_t *dev) {
    vnode_t *dev_vnode =
        vfs_create_vnode(devfs_root->root, dev->name, VNODE_FILE);
    if (!dev_vnode) {
        kprintf_warn("Failed to create vnode for device '%s'\n", dev->name);
        return -1;
    }

    dev_vnode->data = dev;
    dev_vnode->ops  = kmalloc(sizeof(vnode_ops_t));
    if (!dev_vnode->ops) {
        kprintf_warn("Failed to allocate memory for vnode ops\n");
        return -1;
    }

    dev_vnode->ops->read  = devfs_read;
    dev_vnode->ops->write = devfs_write;

    dev_vnode->size = 0;
    dev_vnode->data = (void *)dev;

    debugf_debug("Added device '%s' to devfs\n", dev->name);

    return 0;
}

void devfs_init() {
    vnode_t *devfs_dir = vfs_create_vnode(root_mount->root, "dev", VNODE_DIR);
    assert(devfs_dir);
    devfs_dir->flags = VNODE_FLAG_MOUNTPOINT;

    mount_t *mount = vfs_mount("/dev", "devfs");
    if (!mount) {
        debugf_warn("Failed to mount devfs at '/dev'.\n");
        return;
    }

    devfs_root       = mount;
    devfs_root->root = devfs_dir;
    devfs_dir->mount = mount;

    debugf_debug("DevFS initialized at /dev\n");
}