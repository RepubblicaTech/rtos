#ifndef STDIO_H
#define STDIO_H 1

#include <base.h>

void putc(char c);
int kprintf(const char* fmt, ...);

#define printf(fmt, ...) kprintf(fmt, ##__VA_ARGS__)

#endif
