#ifndef STDIO_H
#define STDIO_H 1

#include <base.h>

void clearscreen();

void putc(char c);
int kprintf(const char* fmt, ...);

void dputc(char c);
int kdprintf(const char* fmt, ...);

#define printf(fmt, ...) kprintf(fmt, ##__VA_ARGS__)

#define debugf(fmt, ...) kdprintf(fmt, ##__VA_ARGS__)

#endif
