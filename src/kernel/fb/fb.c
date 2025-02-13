/*
    Flanterm drove me crazy so i'm making my own framefuffer driver :3

    "i swear i'd rather spend the rest of my life making a framebuffer "driver"
   than trying to debug someone else's" - Omar, 12/02/2025
*/

#include "fb.h"
#include "font.h"

#include <stdbool.h>
#include <stdio.h>

// the single character on the screen
// according to .F16 VGA font file
#define GLYPH_HEIGHT 16
#define GLYPH_WIDTH  8

framebuffer_t fb_ctx;

#define FB_RGB(r, g, b) (r & 0xff) << 16 | (g & 0xff) << 8 | (b & 0xff)

extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);

uint8_t *fb_font;

/* function prototypes */
framebuffer_t *framebuffer_init(uint32_t *fb_base, uint64_t width,
                                uint64_t height, uint64_t pitch, uint8_t *font,
                                uint8_t red_mask, uint8_t red_shift,
                                uint8_t green_mask, uint8_t green_shift,
                                uint8_t blue_mask, uint8_t blue_shift) {
    if (!fb_base)
        return NULL;

    if (!font)
        fb_font = (uint8_t *)&builtin_font;

    memset(&fb_ctx, 0, sizeof(framebuffer_t));

    fb_ctx.fb_base = fb_base;

    fb_ctx.screen_width  = width;
    fb_ctx.screen_height = height;
    fb_ctx.pitch         = pitch;

    fb_ctx.current_x = 0;
    fb_ctx.current_y = 0;

    fb_ctx.red_mask    = red_mask;
    fb_ctx.red_shift   = red_shift;
    fb_ctx.green_mask  = green_mask;
    fb_ctx.green_shift = green_shift;
    fb_ctx.blue_mask   = blue_mask;
    fb_ctx.blue_shift  = blue_shift;

    fb_ctx.current_fg = FB_RGB(0xff, 0xff, 0xff);
    fb_ctx.current_bg = FB_RGB(0x23, 0x23, 0x23);

    fb_ctx.cols = fb_ctx.screen_width / GLYPH_WIDTH;
    fb_ctx.rows = fb_ctx.screen_height / GLYPH_HEIGHT;

    return &fb_ctx;
}

// scroll one pixel row upwards
// TODO: pixel rows parameter
void fb_scroll(int rows) {
    for (int i = 0; i < rows; i++) {
        for (uint64_t y = 1; y < fb_ctx.screen_height; y++) {
            for (uint64_t x = 0; x < fb_ctx.screen_width; x++) {
                volatile uint32_t *cur_row =
                    &fb_ctx.fb_base[y * (fb_ctx.pitch / 4)];
                volatile uint32_t *prev_row =
                    &fb_ctx.fb_base[(y - 1) * (fb_ctx.pitch / 4)];

                prev_row[x] = cur_row[x];
            }
        }
    }
}

// this is for internal use
void plot_pixel(uint64_t x, uint64_t y, uint32_t rgb) {
    if (x > fb_ctx.screen_width || y > fb_ctx.screen_height)
        return;

    fb_ctx.fb_base[(y * (fb_ctx.pitch / 4)) + x] = rgb;
}

void putpixel(uint32_t rgb) {
    plot_pixel(fb_ctx.current_x, fb_ctx.current_y, rgb);

    // first increment x
    if (++fb_ctx.current_x > fb_ctx.screen_width) {
        fb_ctx.current_x = 0;
        // then y if needed
        fb_ctx.current_y++;
    }
}

void fb_save_state() {
    fb_ctx.saved_x = fb_ctx.current_x;
    fb_ctx.saved_y = fb_ctx.current_y;

    fb_ctx.saved_fg = fb_ctx.current_fg;
    fb_ctx.saved_bg = fb_ctx.current_bg;
}

void fb_restore_state() {
    fb_ctx.current_x = fb_ctx.saved_x;
    fb_ctx.current_y = fb_ctx.saved_y;

    fb_ctx.current_fg = fb_ctx.saved_fg;
    fb_ctx.current_bg = fb_ctx.saved_bg;
}

void fb_set_fg(uint32_t rgb) {
    fb_ctx.current_fg = rgb;
}

void fb_set_bg(uint32_t rgb) {
    fb_ctx.current_bg = rgb;
}

uint32_t fb_get_fg() {
    return fb_ctx.current_fg;
}

uint32_t fb_get_bg() {
    return fb_ctx.current_bg;
}

// parameter `c` gives us the ASCII code of the letter
void fb_put_glyph(char c) {
    uint8_t *glyph_start = &fb_font[c * GLYPH_HEIGHT];

    fb_save_state();

    for (int i = 0; i < GLYPH_HEIGHT; i++) {
        for (int j = 0; j < GLYPH_WIDTH; j++) {
            putpixel(glyph_start[i] & (1 << (8 - j)) ? fb_ctx.current_fg
                                                     : fb_ctx.current_bg);
        }
        fb_ctx.current_y++;
        fb_ctx.current_x -= GLYPH_WIDTH;
    }

    fb_restore_state();

    fb_ctx.current_x += GLYPH_WIDTH;
}

void fb_putc(char c) {
    switch (c) {
    case '\n':
        while (fb_ctx.current_x < fb_ctx.screen_width) {
            fb_put_glyph(' ');
        }
        fb_ctx.current_x  = 0;
        fb_ctx.current_y += GLYPH_HEIGHT;
        if (fb_ctx.current_y >= fb_ctx.screen_height) {
            fb_scroll(GLYPH_HEIGHT);
            fb_ctx.current_y -= GLYPH_HEIGHT;
        }
        break;

    case '\t':
        for (int i = 0; i < 8; i++) {
            fb_put_glyph(' ');
        }
        break;
    case '\b':
        fb_ctx.current_x -= GLYPH_WIDTH;
        break;
    default:
        fb_put_glyph(c);
        break;
    }
}

void fb_clearscreen() {
    for (int c = 0; c < fb_ctx.rows; c++) {
        for (int r = 0; r < fb_ctx.cols; r++) {
            fb_putc(' ');
        }
    }
    fb_ctx.current_x = 0;
    fb_ctx.current_y = 0;
}
