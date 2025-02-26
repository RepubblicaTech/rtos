#include "device.h"

#include <stdio.h>
#include <util/string.h>

static device_t *device_table[DEVICES_MAX];
static int device_count = 0;

int register_device(device_t *dev) {
    if (device_count >= DEVICES_MAX) {
        kprintf_warn("Device table full!\n");
        return -1;
    }
    device_table[device_count++] = dev;
    debugf("Device %s registered with type %s\n", dev->name,
           dev->type == DEVICE_TYPE_BLOCK ? "BLK" : "CHR");

    return 0;
}

device_t *get_device(const char *name) {
    for (int i = 0; i < device_count; i++) {
        if (strcmp(device_table[i]->name, name) == 0) {
            return device_table[i];
        }
    }
    return NULL;
}
