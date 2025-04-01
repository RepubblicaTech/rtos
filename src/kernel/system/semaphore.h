#ifndef SEMAPHORE_H
#define SEMAPHORE_H 1

#include <stdbool.h>

typedef struct {
    bool lock;
} semaphore_t;

#endif