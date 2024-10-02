#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

#include <limine.h>

#include <util/va_list.h>

#include <io/io.h>

extern struct flanterm_context *ft_ctx;
extern struct limine_framebuffer *framebuffer;

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW 1
typedef long ssize_t;

#define NANOPRINTF_IMPLEMENTATION
#include <nanoprintf.h>

void clearscreen() {
    ft_ctx->clear(ft_ctx, true);
}

void putc(char c) {
    flanterm_write(ft_ctx, &c, sizeof(c));
}

void dputc(char c) {
    outb(0xE9, c);
}

void mputc(char c) {
    putc(c);
    dputc(c);
}

int kprintf(void (*putc_function)(char), const char* fmt, ...) {
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
