#include "stdio.h"

#include <io.h>

#include <limine.h>

#include <spinlock.h>

#include <util/va_list.h>

extern struct limine_framebuffer *framebuffer;

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS     1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS           0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS           1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS       0
#define NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW 1
typedef long ssize_t;

#define NANOPRINTF_IMPLEMENTATION
#include <nanoprintf.h>

atomic_flag STDIO_FB_LOCK;
atomic_flag STDIO_E9_LOCK;

// unlocks spinlocks by force
// interrupts are disabled to avoid other ints to be fired
void stdio_panic_init() {
    asm("cli");

    spinlock_release(&STDIO_FB_LOCK);
    spinlock_release(&STDIO_E9_LOCK);
}

void set_screen_bg_fg(uint32_t bg_rgb, uint32_t fg_rgb) {
    fb_set_bg(bg_rgb);
    fb_set_fg(fg_rgb);
}

void putc(char c) {
    spinlock_acquire(&STDIO_FB_LOCK);
    fb_putc(c);
    spinlock_release(&STDIO_FB_LOCK);
}

void dputc(char c) {
    spinlock_acquire(&STDIO_E9_LOCK);
    _outb(0xE9, c);
    spinlock_release(&STDIO_E9_LOCK);
}

void mputc(char c) {
    putc(c);
    dputc(c);
}

void rsod_init() {
    set_screen_bg_fg(0xff0000, 0xffffff);
    fb_clearscreen();
}

int printf(void (*putc_function)(char), const char *fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length < 0 || length >= (int)sizeof(buffer)) {
        return -1;
    }

    for (int i = 0; i < length; ++i) {
        (*putc_function)(buffer[i]);
    }

    return length;
}
