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

#include <memory/page.h>

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
    asm ("cli");
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

    set_screen_bg_fg(0x0f0f0f, 0xffffff);
    for (size_t i = 0; i < ft_ctx->rows; i++)
    {
        for (size_t i = 0; i < ft_ctx->cols; i++)  kprintf(" ");
    }
    clearscreen();
    

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
    isr_registerHandler(14, pf_handler);

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
        kprintf("Entry n. %ld; Region start: 0x%lX; length: 0x%lX; type: %s\n", i, entry->base, entry->length, memory_block_type[entry->type]);
    }

    entry = parsed_limine_data.memory_entries[parsed_limine_data.entry_count - 1];
    parsed_limine_data.memory_total = entry->base + entry->length;
    
    for (uint64_t i = 0; i < parsed_limine_data.entry_count; i++)
    {
        entry = parsed_limine_data.memory_entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) parsed_limine_data.memory_usable_total += entry->length;
    }


    kprintf("Total Memory size: %lu KBytes\n", parsed_limine_data.memory_total / 2 / 8 / 1024 / 1024);
    
    kprintf("Totale usable memory: %lu KBytes\n", parsed_limine_data.memory_usable_total / 2 / 8 / 1024 / 1024);

    if (paging_mode_request.response == NULL) {
        debugf("--- PANIC! ---  We've got no paging!\n");
        panic();
    }
    paging_mode_response = paging_mode_request.response;

    if (paging_mode_response->mode < paging_mode_request.min_mode || paging_mode_response->mode > paging_mode_request.max_mode) {
        debugf("--- PANIC --- %luth paging mode is not supported!\n", paging_mode_response->mode);
        panic();
    }
    kprintf("%luth level paging is enabled\n", 4 + paging_mode_response->mode);

    // crash_test();

    for (;;);

    // We're done, just hang...
    // hcf();
}
