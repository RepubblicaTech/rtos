#ifndef STDIO_H
#define STDIO_H 1

#include <stdint.h>

void clearscreen();

void putc(char c);
void dputc(char c);

int kprintf(void (*putc_function)(char), const char* fmt, ...);

#define printf(fmt, ...) kprintf(putc, fmt, ##__VA_ARGS__)

#define debugf(fmt, ...) kprintf(dputc, fmt, ##__VA_ARGS__)

#endif
