#ifndef DEV_E9_H
#define DEV_E9_H

#define PORT 0xe9

#include <../arch/x86_64/io.h>
#include <dev/device.h>
#include <memory/heap/liballoc.h>
#include <util/string.h>

void dev_e9_init();

int dev_e9_read(struct device *dev, void *buffer, size_t size, size_t offset);
int dev_e9_write(struct device *dev, const void *buffer, size_t size,
                 size_t offset);

#endif // DEV_E9_H