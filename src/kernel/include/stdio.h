#ifndef STDIO_H
#define STDIO_H 1

// #define PRINTF_MIRROR

#include <stdint.h>

void set_screen_bg_fg(uint32_t bg_rgb, uint32_t fg_rgb);

void rsod_init();

void clearscreen();

void putc(char c);
void dputc(char c);

void mputc(char c);

int printf(void (*putc_function)(char), const char* fmt, ...);

#ifndef PRINTF_MIRROR
    #define kprintf(fmt, ...) printf(putc, fmt, ##__VA_ARGS__)
#else
    #define kprintf(fmt, ...) printf(mputc, fmt, ##__VA_ARGS__)
#endif

#define debugf(fmt, ...) printf(dputc, fmt, ##__VA_ARGS__)

#endif
