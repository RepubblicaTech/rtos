#include <base.h>
#include <memory.h>

#include <stdio.h>

struct limine_framebuffer *framebuffer;
struct flanterm_context *ft_ctx;
struct flanterm_fb_context *ft_fb_ctx;


// Halt and catch fire function.
static void hcf(void) {
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];

    ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        (uint32_t *)framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
        framebuffer->red_mask_size, framebuffer->red_mask_shift,
        framebuffer->green_mask_size, framebuffer->green_mask_shift,
        framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );

    printf("Hello!\n");

    debugf("Hello from the E9 port!\n");
    debugf("Current video mode is: %dx%d address: 0x%x", framebuffer->width, framebuffer->height, (uint32_t *)framebuffer->address);

    // We're done, just hang...
    hcf();
}
