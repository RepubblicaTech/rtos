#ifndef FB_H
#define FB_H 1

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *fb_base;

    uint64_t screen_width;
    uint64_t screen_height;

    uint64_t pitch;

    uint64_t cols;
    uint64_t rows;

    // current "cursor" position
    size_t current_x;
    size_t current_y;

    // all the (<< shifts) and (& mask) stuff for RGB
    uint8_t red_mask;
    uint8_t red_shift;
    uint8_t green_mask;
    uint8_t green_shift;
    uint8_t blue_mask;
    uint8_t blue_shift;

    uint32_t current_fg;
    uint32_t current_bg;

    // saved "cursor" position
    size_t saved_x;
    size_t saved_y;

    uint32_t saved_fg;
    uint32_t saved_bg;

} framebuffer_t;

framebuffer_t *framebuffer_init(uint32_t *fb_base, uint64_t width,
                                uint64_t height, uint64_t pitch, uint8_t *font,
                                uint8_t red_mask, uint8_t red_shift,
                                uint8_t green_mask, uint8_t green_shift,
                                uint8_t blue_mask, uint8_t blue_shift);

void fb_scroll(int rows);

void fb_set_fg(uint32_t rgb);
void fb_set_bg(uint32_t rgb);

uint32_t fb_get_fg();
uint32_t fb_get_bg();

void plot_pixel(uint64_t x, uint64_t y, uint32_t value);
void putpixel(uint32_t rgb);

void fb_put_glyph(char c);
void fb_putc(char c);

void fb_clearscreen();

#endif