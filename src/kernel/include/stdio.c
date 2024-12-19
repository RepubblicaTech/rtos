#include "stdio.h"

#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

#include <limine.h>

#include <util/va_list.h>

#include <io/io.h>

extern struct flanterm_context *ft_ctx;
extern struct flanterm_fb_context *ft_fb_ctx;
extern struct limine_framebuffer *framebuffer;

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW 1
typedef long ssize_t;

#define NANOPRINTF_IMPLEMENTATION
#include <nanoprintf.h>

uint32_t current_bg;
uint32_t current_fg;

uint32_t get_screen_bg() {
    return current_bg;
}

uint32_t get_screen_fg() {
    return current_fg;
}

void set_screen_bg(uint32_t bg_rgb) {
    current_bg = bg_rgb;
    ft_ctx->set_text_bg_rgb(ft_ctx, bg_rgb);
}

void set_screen_fg(uint32_t fg_rgb) {
    current_fg = fg_rgb;
    ft_ctx->set_text_fg_rgb(ft_ctx, fg_rgb);
}

void set_screen_bg_fg(uint32_t bg_rgb, uint32_t fg_rgb) {
    set_screen_bg(bg_rgb);
    set_screen_fg(fg_rgb);
}

void clearscreen() {
    ft_ctx->clear(ft_ctx, true);
}

void putc(char c) {
    flanterm_write(ft_ctx, &c, sizeof(c));
}

void rsod_init() {
    set_screen_bg_fg(0xff0000, 0xffffff);

    ft_ctx->set_cursor_pos(ft_ctx, 0, 0);

    for (size_t i = 0; i < ft_ctx->rows; i++)
    {
        for (size_t i = 0; i < ft_ctx->cols; i++) putc(' ');
    }

    clearscreen();
}

void dputc(char c) {
    _outb(0xE9, c);
}

void mputc(char c) {
    putc(c);
    dputc(c);
}

int printf(void (*putc_function)(char), const char* fmt, ...) {
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
