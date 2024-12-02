#include "kernel.h"

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

#include <memory/pmm.h>
#include <memory/paging/paging.h>
#include <memory/vmm.h>

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

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#hhdm-higher-half-direct-map-feature
__attribute__((used, section(".requests"))) 
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#kernel-address-feature
__attribute__((used, section(".requests"))) 
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#rsdp-feature
__attribute__((used, section(".requests"))) 
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

// Define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

extern void _crash_test();

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
struct limine_kernel_address_response *kernel_address_response;
struct limine_rsdp_response *rsdp_response;

struct bootloader_data limine_parsed_data;

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
        _panic();
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

    set_screen_bg_fg(0x000116, 0xeeeeee);           // dark blue, white-ish

    for (size_t i = 0; i < ft_ctx->rows; i++)
    {
        for (size_t i = 0; i < ft_ctx->cols; i++)  kprintf(" ");
    }
    clearscreen();
    
    kprintf("Hello!\n");

    debugf("Kernel built on %s\n", __DATE__);

    debugf("Hello from the E9 port!\n");
    debugf("Current video mode is: %dx%d address: %p\n\n", framebuffer->width, framebuffer->height, framebuffer->address);
    
    if (kernel_address_request.response == NULL) {
        kprintf_panic("Couldn't get kernel address!\n");
        _panic();
    }
    kernel_address_response = kernel_address_request.response;
    limine_parsed_data.kernel_base_physical = kernel_address_response->physical_base;
    limine_parsed_data.kernel_base_virtual = kernel_address_response->virtual_base;
    debugf("Kernel address: (phys)%#llx (virt)%#llx\n\n", limine_parsed_data.kernel_base_physical, limine_parsed_data.kernel_base_virtual);

    debugf("\tlimine_requests_start: %llp; limine_requests_end: %llp\n", &__limine_reqs_start, &__limine_reqs_end);
    debugf("Kernel sections:\n");
    debugf("\tkernel_start: %#llp\n", &__kernel_start);
    debugf("\tkernel_text_start: %llp; kernel_text_end: %llp\n", &__kernel_text_start, &__kernel_text_end);
    debugf("\tkernel_rodata_start: %llp; kernel_rodata_end: %llp\n", &__kernel_rodata_start, &__kernel_rodata_end);
    debugf("\tkernel_data_start: %llp; kernel_data_end: %llp\n", &__kernel_data_start, &__kernel_data_end);
    debugf("\tkernel_end: %llp\n", &__kernel_end);

    gdt_init();
    kprintf_info("GDT init done\n");

    idt_init();
    kprintf_info("IDT init done\n");

    isr_init();
    kprintf_info("ISR init done\n");

    irq_init();
    kprintf_info("IRQ and PIC init done\n");

    // _crash_test();

    irq_registerHandler(0, timer);
    isr_registerHandler(14, pf_handler);

    if (memmap_request.response == NULL || memmap_request.response->entry_count < 1) {
        // ERROR
        kprintf_panic("No memory map received!\n");
        _panic();
    }

    struct limine_memmap_response *memmap_response = memmap_request.response;

    limine_parsed_data.memory_entries = memmap_response->entries;
    limine_parsed_data.entry_count = memmap_response->entry_count;

    // Load limine's memory map into OUR struct
    for (uint64_t i = 0; i < limine_parsed_data.entry_count; i++)
    {
        entry = limine_parsed_data.memory_entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) limine_parsed_data.memory_usable_total += entry->length;

        kprintf("Entry n. %lld; Region start: %#llx; length: %#llx; type: %s\n", i, entry->base, entry->length, memory_block_type[entry->type]);
    }
    entry = limine_parsed_data.memory_entries[limine_parsed_data.entry_count - 1];
    limine_parsed_data.memory_total = entry->base + entry->length;

    kprintf("Total Memory size: %#llx (%llu KBytes)\n", limine_parsed_data.memory_total, limine_parsed_data.memory_total / 0x1000000);
    
    kprintf("Total usable memory: %#llx (%llu KBytes)\n", limine_parsed_data.memory_usable_total, limine_parsed_data.memory_usable_total / 0x1000000);

    if (hhdm_request.response == NULL) {
        kprintf_panic("Couldn't get Higher Half Map!\n");
        _panic();
    }

    hhdm_response = hhdm_request.response;

    limine_parsed_data.hhdm_offset = hhdm_response->offset;
    debugf("Higher Half Direct Map offset: %#llx\n", limine_parsed_data.hhdm_offset);

    pmm_init();
    kprintf_info("PMM init done\n");

    if (!check_pae()) {
        kprintf_info("PAE is disabled\n");
    } else {
        kprintf_info("PAE is enabled\n");
    }

    if (check_apic()) {
        kprintf_info("APIC device is supported\n");
        // TODO: enable the APIC and map it's virtual address
    }

    if (paging_mode_request.response == NULL) {
        kprintf_panic("We've got no paging!\n");
        _panic();
    }
    paging_mode_response = paging_mode_request.response;

    if (paging_mode_response->mode < paging_mode_request.min_mode || paging_mode_response->mode > paging_mode_request.max_mode) {
        kprintf_panic("%lluth paging mode is not supported!\n", paging_mode_response->mode);
        _panic();
    }
    kprintf("%lluth level paging is enabled\n", 4 + paging_mode_response->mode);

    vmm_init();
    kprintf_info("VMM init done\n");
    
    // _crash_test();

    for (;;);

    // We're done, just hang...
    // hcf();
}
