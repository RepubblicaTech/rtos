#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

#include <stdio.h>

#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>

#include <cpu.h>
#include <util/memory.h>

#include <kernel.h>

/*
    Set the base revision to 2, this is recommended as this is the latest
    base revision described by the Limine boot protocol specification.
    See specification for further info.

    (Note for Omar) 
    --- DON'T EVEN DARE TO PUT THIS ANYWHERE ELSE OTHER THAN HERE. ---
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

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#memory-map-feature
__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#paging-mode-feature
__attribute__((used, section(".requests")))
static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0
};

// Define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

extern void crash_test();

void timer(registers* regs) {
    return;
}

// Halt and catch fire function.
static void hcf(void) {
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}

struct limine_framebuffer *framebuffer;
struct flanterm_context *ft_ctx;
struct flanterm_fb_context *ft_fb_ctx;

struct limine_memmap_entry *entry;
struct limine_hhdm_response *hhdm_response;
struct limine_paging_mode_response *paging_mode_response;

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

    kprintf("Hello!\n");

    debugf("Hello from the E9 port!\n");
    debugf("Current video mode is: %dx%d address: 0x%x\n\n", framebuffer->width, framebuffer->height, (uint64_t *)framebuffer->address);

    gdt_init();
    kprintf("[ INFO ]    GDT init done\n");

    idt_init();
    kprintf("[ INFO ]    IDT init done\n");

    isr_init();
    kprintf("[ INFO ]    ISR init done\n");

    irq_init();
    kprintf("[ INFO ]    IRQ and PIC init done\n");

    // crash_test();

    irq_registerHandler(0, timer);

    if (memmap_request.response == NULL || memmap_request.response->entry_count < 1) {
        // ERROR
        kprintf("--- PANIC! --- No memory map received\n");
        panic();
    }

    struct limine_memmap_response *memmap_response = memmap_request.response;

    parsed_limine_data.memory_entries = memmap_response->entries;
    parsed_limine_data.entry_count = memmap_response->entry_count;

    // Load limine's memory map into OUR struct
    for (uint64_t i = 0; i < parsed_limine_data.entry_count; i++)
    {
        entry = parsed_limine_data.memory_entries[i];
        kprintf("Region start: 0x%lX; length: %lu; type: %s\n", entry->base, entry->length, memory_block_type[entry->type]);

        entry = NULL;           // "non si sa mai" :)
    }

    if (paging_mode_request.response == NULL) {
        debugf("--- PANIC! ---  We've got no paging!\n");
        panic();
    }
    paging_mode_response = paging_mode_request.response;

    if (paging_mode_response->mode < paging_mode_request.min_mode || paging_mode_response->mode > paging_mode_request.max_mode) {
        debugf("--- PANIC --- %luth paging mode is not supported!\n");
        panic();
    }
    kprintf("%luth level paging is enabled\n", 4 + paging_mode_response->mode);

    // crash_test();

    for (;;);

    // We're done, just hang...
    // hcf();
}
