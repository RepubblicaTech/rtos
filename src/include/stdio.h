#ifndef STDIO_H
#define STDIO_H 1

#define PRINTF_MIRROR

#include <stdint.h>

void clearscreen();

void putc(char c);
void dputc(char c);

void mputc(char c);

int kprintf(void (*putc_function)(char), const char* fmt, ...);

#ifndef PRINTF_MIRROR
    #define printf(fmt, ...) kprintf(putc, fmt, ##__VA_ARGS__)
#else
    #define printf(fmt, ...) kprintf(mputc, fmt, ##__VA_ARGS__)
#endif

#define debugf(fmt, ...) kprintf(dputc, fmt, ##__VA_ARGS__)

#endif
