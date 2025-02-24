#ifndef DEV_NULL_H
#define DEV_NULL_H

#include <dev/device.h>
#include <memory/heap/liballoc.h>
#include <stddef.h>
#include <util/string.h>

void dev_null_init();

int dev_null_read(struct device *dev, void *buffer, size_t size, size_t offset);
int dev_null_write(struct device *dev, const void *buffer, size_t size,
                   size_t offset);

#endif // DEV_NULL_H