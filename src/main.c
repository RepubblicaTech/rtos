#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

#include <util/memory.h>

#include <stdio.h>

#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>

#include <limine.h>

#include <util/cpu.h>

/*
    Set the base revision to 2, this is recommended as this is the latest
    base revision described by the Limine boot protocol specification.
    See specification for further info.

    (Note for Omar) --- DON'T EVEN DARE TO PUT THIS ANYWHERE ELSE OTHER THAN HERE. ---
*/

__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};


// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

struct limine_framebuffer *framebuffer;
struct flanterm_context *ft_ctx;
struct flanterm_fb_context *ft_fb_ctx;

struct limine_memmap_entry *memmap_entry;

extern void crash_test();

void timer(registers* regs) {
    printf(".");
}

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
void kstart(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        panic();
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
    debugf("Current video mode is: %dx%d address: 0x%x\n\n", framebuffer->width, framebuffer->height, (uint64_t *)framebuffer->address);

    gdt_init();
    printf("[ INFO ]    GDT init done\n");

    idt_init();
    printf("[ INFO ]    IDT init done\n");

    isr_init();
    printf("[ INFO ]    ISR init done\n");

    irq_init();
    printf("[ INFO ]    IRQ and PIC init done\n");

    // crash_test();

    irq_registerHandler(0, timer);

    if (memmap_request.response == NULL || memmap_request.response->entry_count < 1) {
        // ERROR
        debugf("ERROR: No memory map received\n");
        panic();
    }
    

    for (int i = 0; i < memmap_request.response->entry_count; i++)
    {
        memmap_entry = memmap_request.response->entries[i];
        debugf("Found memory at address: 0x%lX; length: %lu KBytes; type: %s\n", memmap_entry->base, memmap_entry->length / 1024, memory_block_type[memmap_entry->type]);
    }

    for (;;);

    // We're done, just hang...
    // hcf();
}
