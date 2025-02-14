#ifndef STDIO_H
#define STDIO_H 1

// #define PRINTF_MIRROR            // printf() will be mirrored to the E9 port
#define BETTER_DEBUG // better debug info (with function names and line)

#include <stdint.h>

#include <flanterm/backends/fb.h>
#include <flanterm/flanterm.h>

#define DEFAULT_FG 0xeeeeee
#define DEFAULT_BG 0x050505

#define INFO_FG    0xa4a5a4
#define SUCCESS_FG 0x00e826
#define WARNING_FG 0xea7500
#define PANIC_FG   0xea0000

uint32_t fb_get_bg();
void fb_set_bg(uint32_t bg_rgb);

uint32_t fb_get_fb();
void fb_set_fg(uint32_t fg_rgb);

void set_screen_bg_fg(uint32_t bg_rgb, uint32_t fg_rgb);

void stdio_panic_init();
void clearscreen();
void rsod_init();

void putc(char c);
void dputc(char c);

void mputc(char c);

int printf(void (*putc_function)(char), const char *fmt, ...);

#ifdef PRINTF_MIRROR
#define kprintf(fmt, ...) printf(mputc, fmt, ##__VA_ARGS__)
#else
#define kprintf(fmt, ...) printf(putc, fmt, ##__VA_ARGS__)
#endif

#define mprintf(fmt, ...) printf(mputc, fmt, ##__VA_ARGS__)

#define kprintf_ok(fmt, ...)                                                   \
    ({                                                                         \
        uint32_t prev_fg = fb_get_fb();                                        \
        fb_set_fg(0x00e826);                                                   \
        kprintf("[ %s():%d::SUCCESS ] " fmt, __FUNCTION__, __LINE__,           \
                ##__VA_ARGS__);                                                \
        fb_set_fg(prev_fg);                                                    \
    })

#define kprintf_info(fmt, ...)                                                 \
    ({                                                                         \
        uint32_t prev_fg = fb_get_fb();                                        \
        fb_set_fg(INFO_FG);                                                    \
        kprintf("[ %s():%d::INFO ] " fmt, __FUNCTION__, __LINE__,              \
                ##__VA_ARGS__);                                                \
        fb_set_fg(prev_fg);                                                    \
    })

#define kprintf_warn(fmt, ...)                                                 \
    ({                                                                         \
        uint32_t prev_fg = fb_get_fb();                                        \
        fb_set_fg(WARNING_FG);                                                 \
        kprintf("--- [ WARNING @ s():%d ] --- " fmt, __FUNCTION__, __LINE__,   \
                ##__VA_ARGS__);                                                \
        fb_set_fg(prev_fg);                                                    \
    })

#define kprintf_panic(fmt, ...)                                                \
    ({                                                                         \
        uint32_t prev_fg = fb_get_fb();                                        \
        fb_set_fg(PANIC_FG);                                                   \
        kprintf("--- [ PANIC @ %s():%d ] --- " fmt " Halting...",              \
                __FUNCTION__, __LINE__, ##__VA_ARGS__);                        \
        fb_set_fg(prev_fg);                                                    \
    })

#define debugf(fmt, ...) printf(dputc, fmt, ##__VA_ARGS__)

#define ANSI_COLOR_RED    "\33[31m"
#define ANSI_COLOR_GREEN  "\33[32m"
#define ANSI_COLOR_ORANGE "\33[33m"
#define ANSI_COLOR_GRAY   "\33[90m"
#define ANSI_COLOR_RESET  "\33[0m"

#define COLOR(color, str) color str ANSI_COLOR_RESET

#define debugf_debug(fmt, ...)                                                 \
    debugf(COLOR(ANSI_COLOR_GRAY, "[ %s()::DEBUG ] " fmt), __FUNCTION__,       \
           ##__VA_ARGS__)
#define debugf_ok(fmt, ...)                                                    \
    debugf(COLOR(ANSI_COLOR_GREEN, "[ %s()::SUCCESS ] " fmt), __FUNCTION__,    \
           ##__VA_ARGS__)

#define debugf_warn(fmt, ...)                                                  \
    debugf(COLOR(ANSI_COLOR_ORANGE, "[ %s()::PANIC ] " fmt), __FUNCTION__,     \
           ##__VA_ARGS__)

#define debugf_panic(fmt, ...)                                                 \
    debugf(COLOR(ANSI_COLOR_RED, "[ %s()::PANIC ] " fmt), __FUNCTION__,        \
           ##__VA_ARGS__)

#endif
