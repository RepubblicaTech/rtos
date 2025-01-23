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
#include <pic.h>
#include <pit.h>
#include <mmio/apic/apic.h>
#include <mmio/apic/io_apic.h>

#include <time.h>

#include <cpu.h>
#include <util/string.h>

#include <memory/pmm.h>
#include <memory/paging/paging.h>
#include <memory/vmm.h>
#include <memory/vma.h>
#include <memory/heap/liballoc.h>

#include <acpi/acpi.h>
#include <acpi/rsdp.h>

#define PIT_TICKS   100

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

struct limine_framebuffer *framebuffer;
struct flanterm_context *ft_ctx;
struct flanterm_fb_context *ft_fb_ctx;

struct limine_memmap_entry *entry;
struct limine_hhdm_response *hhdm_response;
struct limine_paging_mode_response *paging_mode_response;
struct limine_kernel_address_response *kernel_address_response;
struct limine_rsdp_response *rsdp_response;

struct bootloader_data limine_parsed_data;

struct bootloader_data get_bootloader_data() {
    return limine_parsed_data;
}

vmm_context_t* kernel_vmm_ctx;

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void kstart(void) {
    asm ("cli");
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        _hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        _hcf();
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

    set_screen_bg_fg(DEFAULT_BG, DEFAULT_FG);           // black-ish, white-ish

    for (size_t i = 0; i < ft_ctx->rows; i++)
    {
        for (size_t i = 0; i < ft_ctx->cols; i++)  kprintf(" ");
    }
    clearscreen();

    kprintf("Hello!\n");

    debugf_debug("Kernel built on %s\n", __DATE__);

    debugf_debug("Hello from the E9 port!\n");
    debugf_debug("Current video mode is: %dx%d address: %p\n\n", framebuffer->width, framebuffer->height, framebuffer->address);

    if (kernel_address_request.response == NULL) {
        kprintf_panic("Couldn't get kernel address!\n");
        _hcf();
    }
    kernel_address_response = kernel_address_request.response;
    limine_parsed_data.kernel_base_physical = kernel_address_response->physical_base;
    limine_parsed_data.kernel_base_virtual = kernel_address_response->virtual_base;
    debugf_debug("Kernel address: (phys)%#llx (virt)%#llx\n\n", limine_parsed_data.kernel_base_physical, limine_parsed_data.kernel_base_virtual);

    debugf_debug("\tlimine_requests_start: %llp; limine_requests_end: %llp\n", &__limine_reqs_start, &__limine_reqs_end);
    debugf_debug("Kernel sections:\n");
    debugf_debug("\tkernel_start: %#llp\n", &__kernel_start);
    debugf_debug("\tkernel_text_start: %llp; kernel_text_end: %llp\n", &__kernel_text_start, &__kernel_text_end);
    debugf_debug("\tkernel_rodata_start: %llp; kernel_rodata_end: %llp\n", &__kernel_rodata_start, &__kernel_rodata_end);
    debugf_debug("\tkernel_data_start: %llp; kernel_data_end: %llp\n", &__kernel_data_start, &__kernel_data_end);
    debugf_debug("\tkernel_end: %llp\n", &__kernel_end);

    gdt_init();
    kprintf_ok("GDT init done\n");

    idt_init();
    kprintf_ok("IDT init done\n");

    isr_init();
    kprintf_ok("ISR init done\n");

    // _crash_test();

    irq_init();
    kprintf_ok("PIC init done\n");

    pit_init(PIT_TICKS);
    kprintf_ok("PIT init done\n");

    isr_registerHandler(14, pf_handler);

    if (memmap_request.response == NULL || memmap_request.response->entry_count < 1) {
        // ERROR
        kprintf_panic("No memory map received!\n");
        _hcf();
    }

    struct limine_memmap_response *memmap_response = memmap_request.response;

    limine_parsed_data.limine_memory_map = memmap_response->entries;
    limine_parsed_data.memmap_entry_count = memmap_response->entry_count;

    // Load limine's memory map into OUR struct
    limine_parsed_data.usable_entry_count = 0;
    for (uint64_t i = 0; i < limine_parsed_data.memmap_entry_count; i++)
    {
        entry = limine_parsed_data.limine_memory_map[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            limine_parsed_data.memory_usable_total += entry->length;
            limine_parsed_data.usable_entry_count++;
        }

        debugf_debug("Entry n. %lld; Region start: %#llx; length: %#llx; type: %s\n", i, entry->base, entry->length, memory_block_type[entry->type]);
    }
    limine_parsed_data.memory_total = limine_parsed_data.limine_memory_map[limine_parsed_data.memmap_entry_count - 1]->base + 
                                      limine_parsed_data.limine_memory_map[limine_parsed_data.memmap_entry_count - 1]->length;

    kprintf("Total Memory size: %#lx (%lu MBytes)\n", limine_parsed_data.memory_total, limine_parsed_data.memory_total / 0x100000);

    kprintf("Total usable memory: %#lx (%lu MBytes)\n", limine_parsed_data.memory_usable_total, limine_parsed_data.memory_usable_total / 0x100000);

    if (hhdm_request.response == NULL) {
        kprintf_panic("Couldn't get Higher Half Map!\n");
        _hcf();
    }

    hhdm_response = hhdm_request.response;

    limine_parsed_data.hhdm_offset = hhdm_response->offset;
    debugf_debug("Higher Half Direct Map offset: %#llx\n", limine_parsed_data.hhdm_offset);

    pmm_init();
    kprintf_ok("PMM init done\n");

    if (rsdp_request.response == NULL) {
        kprintf_panic("Couldn't get RSDP address!\n");
        _hcf();
    }
    rsdp_response = rsdp_request.response;
    limine_parsed_data.rsdp_table_address = (uint64_t*)rsdp_response->address;
    debugf_debug("Address of RSDP: %#llp\n", limine_parsed_data.rsdp_table_address);
    acpi_init();
    kprintf_ok("ACPI tables parsing done\n");

    if (paging_mode_request.response == NULL) {
        kprintf_panic("We've got no paging!\n");
        _hcf();
    }
    paging_mode_response = paging_mode_request.response;

    if (paging_mode_response->mode < paging_mode_request.min_mode || paging_mode_response->mode > paging_mode_request.max_mode) {
        kprintf_panic("%lluth paging mode is not supported!\n", paging_mode_response->mode);
        _hcf();
    }
    kprintf("%lluth level paging is enabled\n", 4 + paging_mode_response->mode);

    if (!check_pae()) {
        kprintf_info("PAE is disabled\n");
    } else {
        kprintf_info("PAE is enabled\n");
    }

    kprintf_info("Initializing paging\n");
    // just checking if the PIT works :)
    uint64_t start = get_current_ticks();
    // kernel PML4 table
    uint64_t* kernel_pml4 = (uint64_t*)PHYS_TO_VIRTUAL(pmm_alloc(PMLT_SIZE));
    paging_init(kernel_pml4);
    uint64_t time = get_current_ticks();
    time -= start;
    time /= PIT_TICKS;
    kprintf_ok("Paging init done\n\t\tTime taken: ~%llu seconds\n", time);

    kernel_vmm_ctx = vmm_ctx_init(kernel_pml4, VMO_KERNEL_RW);
    vmm_init(kernel_vmm_ctx);
    void *ptr = (void*)PHYS_TO_VIRTUAL(pmm_alloc(PMLT_SIZE));
    kernel_vmm_ctx->root_vmo = vmo_init((uint64_t)ptr, PMLT_SIZE, VMO_KERNEL_RW);

    kprintf_ok("Kernel VMM init done\n");

    kprintf_info("Lil malloc test :3\n");
    void* ptr1 = kmalloc(0xA0);
    kprintf_info("Uuuh kmalloc(0xA0) -> %p\n", ptr1);
    void* ptr2 = kmalloc(0xA3B0);
    kprintf_info("Uuuh kmalloc(0xA3B0) -> %p\n", ptr2);

    kfree(ptr1);
    kfree(ptr2);
    kprintf_info("We've freed both pointers :D\n");
    ptr1 = kmalloc(0xF00);
    kprintf_info("Uuuh kmalloc(0xF00) -> %p\n", ptr1);
    kfree(ptr1);

    if (check_apic()) {
        kprintf_info("APIC device is supported\n");
        apic_init();
        ioapic_init();
        kprintf_ok("APIC init done\n");
    }

    sdt_pointer* rsdp = get_rsdp();

	kprintf("--- %s INFO ---\n", rsdp->revision > 0 ? "XSDP" : "RSDP");

	kprintf("Signature: %.8s\n", rsdp->signature);
	kprintf("Checksum: %hhu\n\n", rsdp->checksum);

	kprintf("OEM ID: %.6s\n", rsdp->oem_id);
	kprintf("Revision: %s\n", rsdp->revision > 0 ? "2.0 - 6.1" : "1.0");

	if (rsdp->revision > 0) {
		kprintf("Length: %lu\n", rsdp->length);
		kprintf("XSDT address: %#llp\n", rsdp->p_xsdt_address);
		kprintf("Extended checksum: %hhu\n", rsdp->extended_checksum);
	} else {
		kprintf("RSDT address: %#lp\n", rsdp->p_rsdt_address);
	}

	kprintf("--- %s END ---\n", rsdp->revision > 0 ? "XSDP" : "RSDP");
    
    for (;;);
}
