#include "kernel.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <flanterm/backends/fb.h>
#include <flanterm/flanterm.h>

#include <stdio.h>

#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <isr.h>
#include <mmio/apic/apic.h>
#include <mmio/apic/io_apic.h>
#include <pic.h>
#include <pit.h>

#include <time.h>

#include <cpu.h>
#include <util/string.h>

#include <memory/heap/liballoc.h>
#include <memory/paging/paging.h>
#include <memory/pmm.h>
#include <memory/vma.h>
#include <memory/vmm.h>

#include <scheduler/scheduler.h>

#include <acpi/acpi.h>
#include <acpi/rsdp.h>

#include <fs/ustar/ustar.h>

#define PIT_TICKS 1000 / 1 // 1 ms

#define INITRD_FILE "initrd.img"
#define INITRD_PATH "/" INITRD_FILE

/*
    Set the base revision to 2, this is recommended as this is the latest
    base revision described by the Limine boot protocol specification.
    See specification for further info.

    (Note for Omar)
    --- DON'T EVEN DARE TO PUT THIS ANYWHERE ELSE OTHER THAN HERE. ---
*/
__attribute__((used,
               section(".requests"))) static volatile LIMINE_BASE_REVISION(2);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.
__attribute__((
    used,
    section(".requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#memory-map-feature
__attribute__((
    used, section(".requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#paging-mode-feature
__attribute__((
    used,
    section(".requests"))) static volatile struct limine_paging_mode_request
    paging_mode_request = {.id = LIMINE_PAGING_MODE_REQUEST, .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#hhdm-higher-half-direct-map-feature
__attribute__((used,
               section(".requests"))) static volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#kernel-address-feature
__attribute__((
    used,
    section(".requests"))) static volatile struct limine_kernel_address_request
    kernel_address_request = {.id       = LIMINE_KERNEL_ADDRESS_REQUEST,
                              .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#rsdp-feature
__attribute__((used,
               section(".requests"))) static volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 0};

// https://github.com/limine-bootloader/limine/blob/v8.x/PROTOCOL.md#module-feature
__attribute__((
    used, section(".requests"))) static volatile struct limine_module_request
    module_request = {.id = LIMINE_MODULE_REQUEST, .revision = 0};

// Define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used,
               section(".requests_start_"
                       "marker"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".requests_end_marker"))) static volatile LIMINE_REQUESTS_END_MARKER;

extern void _crash_test();

struct limine_framebuffer *framebuffer;
struct flanterm_context *ft_ctx;
struct flanterm_fb_context *ft_fb_ctx;

struct limine_memmap_response *memmap_response;
struct limine_memmap_entry *memmap_entry;
struct limine_hhdm_response *hhdm_response;
struct limine_paging_mode_response *paging_mode_response;
struct limine_kernel_address_response *kernel_address_response;
struct limine_rsdp_response *rsdp_response;

struct limine_module_response *module_response;

struct bootloader_data limine_parsed_data;

struct bootloader_data *get_bootloader_data() {
    return &limine_parsed_data;
}

vmm_context_t *kernel_vmm_ctx;

// kernel main function
void kstart(void) {
    asm("cli");
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        _hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        _hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];

    ft_ctx = flanterm_fb_init(
        NULL, NULL, (uint32_t *)framebuffer->address, framebuffer->width,
        framebuffer->height, framebuffer->pitch, framebuffer->red_mask_size,
        framebuffer->red_mask_shift, framebuffer->green_mask_size,
        framebuffer->green_mask_shift, framebuffer->blue_mask_size,
        framebuffer->blue_mask_shift, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, 0, 0, 1, 0, 0, 0);

    set_screen_bg_fg(DEFAULT_BG, DEFAULT_FG); // black-ish, white-ish

    for (size_t i = 0; i < ft_ctx->rows; i++) {
        for (size_t i = 0; i < ft_ctx->cols; i++)
            kprintf(" ");
    }
    clearscreen();

    kprintf("Welcome to RTOS!\n");

    debugf_debug("Kernel built on %s\n", __DATE__);

    debugf_debug("Current video mode is: %dx%d address: %p\n\n",
                 framebuffer->width, framebuffer->height, framebuffer->address);

    if (kernel_address_request.response == NULL) {
        kprintf_panic("Couldn't get kernel address!\n");
        _hcf();
    }
    kernel_address_response = kernel_address_request.response;
    limine_parsed_data.kernel_base_physical =
        kernel_address_response->physical_base;
    limine_parsed_data.kernel_base_virtual =
        kernel_address_response->virtual_base;
    debugf_debug("Kernel address: (phys)%#llx (virt)%#llx\n\n",
                 limine_parsed_data.kernel_base_physical,
                 limine_parsed_data.kernel_base_virtual);

    debugf_debug("Kernel sections:\n");
    debugf_debug("\tkernel_start: %#p\n", &__kernel_start);
    debugf_debug("\tkernel_text_start: %p; kernel_text_end: %p\n",
                 &__kernel_text_start, &__kernel_text_end);
    debugf_debug("\tkernel_rodata_start: %p; kernel_rodata_end: %p\n",
                 &__kernel_rodata_start, &__kernel_rodata_end);
    debugf_debug("\tkernel_data_start: %p; kernel_data_end: %p\n",
                 &__kernel_data_start, &__kernel_data_end);
    debugf_debug("\tlimine_requests_start: %p; limine_requests_end: %p\n",
                 &__limine_reqs_start, &__limine_reqs_end);
    debugf_debug("\tkernel_end: %p\n", &__kernel_end);

    gdt_init();
    kprintf_ok("Initialized GDT\n");
    idt_init();
    kprintf_ok("Initialized IDT\n");
    isr_init();
    kprintf_ok("Initialized ISRs\n");
    isr_registerHandler(14, pf_handler);

    // _crash_test();

    irq_init();
    kprintf_ok("Initialized PIC and IRQs\n");
    pic_registerHandler(0, timer_tick);
    pit_init(PIT_TICKS);
    kprintf_ok("Initialized PIT\n");

    size_t start_tick_after_pit_init = get_current_ticks();

    if (memmap_request.response == NULL ||
        memmap_request.response->entry_count < 1) {
        debugf_panic("No memory map received!\n");
        _hcf();
    }

    memmap_response = memmap_request.response;

    limine_parsed_data.limine_memory_map  = memmap_response->entries;
    limine_parsed_data.memmap_entry_count = memmap_response->entry_count;

    // Load limine's memory map into OUR struct
    limine_parsed_data.usable_entry_count = 0;
    for (uint64_t i = 0; i < limine_parsed_data.memmap_entry_count; i++) {
        memmap_entry = limine_parsed_data.limine_memory_map[i];

        if (memmap_entry->type == LIMINE_MEMMAP_USABLE) {
            limine_parsed_data.memory_usable_total += memmap_entry->length;
            limine_parsed_data.usable_entry_count++;
        }

        debugf_debug(
            "Entry n. %lld; Region start: %#llx; length: %#llx; type: %s\n", i,
            memmap_entry->base, memmap_entry->length,
            memory_block_type[memmap_entry->type]);
    }

    limine_parsed_data.memory_total =
        limine_parsed_data
            .limine_memory_map[limine_parsed_data.memmap_entry_count - 1]
            ->base +
        limine_parsed_data
            .limine_memory_map[limine_parsed_data.memmap_entry_count - 1]
            ->length;

    if (hhdm_request.response == NULL) {
        debugf_panic("Couldn't get Higher Half Direct Map Offset!\n");
        _hcf();
    }

    hhdm_response = hhdm_request.response;

    limine_parsed_data.hhdm_offset = hhdm_response->offset;
    debugf_debug("Higher Half Direct Map offset: %#llx\n",
                 limine_parsed_data.hhdm_offset);

    pmm_init();
    kprintf_ok("Initialized PMM\n");

    if (rsdp_request.response == NULL) {
        kprintf_panic("Couldn't get RSDP address!\n");
        _hcf();
    }
    rsdp_response                         = rsdp_request.response;
    limine_parsed_data.rsdp_table_address = (uint64_t *)rsdp_response->address;
    debugf_debug("Address of RSDP: %p\n",
                 limine_parsed_data.rsdp_table_address);
    acpi_init();
    kprintf_ok("ACPI tables parsing done\n");

    if (paging_mode_request.response == NULL) {
        kprintf_panic("We've got no paging!\n");
        _hcf();
    }
    paging_mode_response = paging_mode_request.response;

    if (paging_mode_response->mode < paging_mode_request.min_mode ||
        paging_mode_response->mode > paging_mode_request.max_mode) {
        kprintf_panic("%lluth paging mode is not supported!\n",
                      paging_mode_response->mode);
        _hcf();
    }
    debugf_debug("%lluth level paging is available\n",
                 4 + paging_mode_response->mode);

    if (!check_pae()) {
        debugf_debug("PAE is not available\n");
    } else {
        debugf_debug("PAE is available\n");
    }

    debugf_debug("Initializing paging\n");
    // just checking if the PIT works :)
    uint64_t start        = get_current_ticks();
    // kernel PML4 table
    uint64_t *kernel_pml4 = (uint64_t *)PHYS_TO_VIRTUAL(pmm_alloc_page());
    paging_init(kernel_pml4);
    uint64_t time  = get_current_ticks();
    time          -= start;
    kprintf_ok("Paging init done\n\t\tTime taken: %llu seconds %llums\n",
               time / PIT_TICKS, time % 1000);

    kernel_vmm_ctx = vmm_ctx_init(kernel_pml4, VMO_KERNEL_RW);
    vmm_init(kernel_vmm_ctx);
    vmm_switch_ctx(kernel_vmm_ctx);
    kprintf_ok("Initialized VMM\n");

    size_t malloc_test_start_timestamp = get_current_ticks();
    kprintf_info("Malloc Test:\n");
    void *ptr1 = kmalloc(0xA0);
    kprintf_info("[1] kmalloc(0xA0) @ %p\n", ptr1);
    void *ptr2 = kmalloc(0xA3B0);
    kprintf_info("[2] kmalloc(0xA3B0) @ %p\n", ptr2);
    kfree(ptr1);
    kfree(ptr2);
    ptr1 = kmalloc(0xF00);
    kprintf_info("[3] kmalloc(0xF00) @ %p\n", ptr1);
    kfree(ptr1);
    size_t malloc_test_end_timestamp  = get_current_ticks();
    malloc_test_end_timestamp        -= malloc_test_start_timestamp;
    kprintf_ok("Malloc test complete: Time taken: %dms\n",
               malloc_test_end_timestamp);

    if (check_apic()) {
        asm("cli");
        debugf_debug("APIC device is supported\n");
        apic_init();
        ioapic_init();
        apic_registerHandler(0, timer_tick);
        kprintf_ok("APIC init done\n");
        asm("sti");
    } else {
        debugf_debug("APIC is not supported. Going on with legacy PIC\n");
    }

    sdt_pointer *rsdp = get_rsdp();

    kprintf("--- ACPI INFO ---\n");
    kprintf("Type: %s (Revision %s)\n", rsdp->revision > 0 ? "XSDP" : "RSDP",
            rsdp->revision > 0 ? "2.0 - 6.1" : "1.0");
    kprintf("%s address: 0x%.16llx\n", rsdp->revision > 0 ? "XSDP" : "RSDP",
            limine_parsed_data.rsdp_table_address);
    kprintf("%s address: 0x%.16llx\n", rsdp->revision > 0 ? "XSDT" : "RSDT",
            rsdp->revision > 0 ? rsdp->p_xsdt_address : rsdp->p_rsdt_address);

    kprintf("Signature: %.8s\n", rsdp->signature);
    kprintf("Checksum: %hhu\n", rsdp->checksum);

    kprintf("OEM ID: %.6s\n", rsdp->oem_id);
    kprintf("Revision: %s\n\n", rsdp->revision > 0 ? "2.0 - 6.1" : "1.0");

    if (rsdp->revision > 0) {
        XSDT *xsdt = (XSDT *)get_root_sdt();
        kprintf("XSDT Length: %lu\n", rsdp->length);
        kprintf("XSDT Extended checksum: %hhu\n", rsdp->extended_checksum);
        kprintf("XSDT OEM ID: %.6s\n", xsdt->header.oem_id);
        kprintf("XSDT Signature: %.4s\n", xsdt->header.signature);
        kprintf("XSDT Creator ID: 0x%llx\n", xsdt->header.creator_id);
    } else {
        RSDT *rsdt = (RSDT *)get_root_sdt();
        kprintf("RSDT OEM ID: %.6s\n", rsdt->header.oem_id);
        kprintf("RSDT Signature: %.4s\n", rsdt->header.signature);
        kprintf("RSDT Creator ID: 0x%llx\n", rsdt->header.creator_id);
    }
    kprintf("--- ACPI INFO END ---\n\n");

    kprintf("--- SYSTEM INFO ---\n");

    {
        char *cpu_name = kmalloc(49);

        unsigned int data[4];
        memset(cpu_name, 0, 49);

        for (int i = 0; i < 3; i++) {
            __asm__("cpuid"
                    : "=a"(data[0]), "=b"(data[1]), "=c"(data[2]), "=d"(data[3])
                    : "a"(0x80000002 + i));
            memcpy(cpu_name + i * 16, data, 16);
        }

        kprintf("CPU Name: %s\n", cpu_name);
        kfree(cpu_name);
    }

    kprintf("CPU Vendor: %s\n", get_cpu_vendor());
    char *hypervisor = kmalloc(49);

    {
        unsigned int data[4];
        memset(hypervisor, 0, 49);

        __asm__("cpuid"
                : "=a"(data[0]), "=b"(data[1]), "=c"(data[2]), "=d"(data[3])
                : "a"(0x40000000)); // Hypervisor info

        if (data[0] == 0x40000000) {
            // Hypervisor name is in EBX, ECX, and EDX
            memcpy(hypervisor, &data[1], 4);
            memcpy(hypervisor + 4, &data[2], 4);
            memcpy(hypervisor + 8, &data[3], 4);
            hypervisor[12] = '\0'; // Null-terminate the string
        } else {
            memcpy(hypervisor, "No Hypervisor", sizeof("No Hypervisor"));
        }

        kprintf("Hypervisor: %s\n\n", hypervisor);
        kfree(hypervisor);
    }

    kprintf("Total Memory: 0x%llx (%lu MBytes)\n",
            limine_parsed_data.memory_total,
            limine_parsed_data.memory_total / 0x100000);

    kprintf("Total available Memory: 0x%llx (%lu MBytes)\n\n",
            limine_parsed_data.memory_usable_total,
            limine_parsed_data.memory_usable_total / 0x100000);

    kprintf("--- SYSTEM INFO END ---\n");

    scheduler_init();
    kprintf_ok("Initialized scheduler\n");

    ft_ctx->full_refresh(ft_ctx);
    clearscreen();

    if (!module_request.response) {
        kprintf_warn("No modules loaded.\n");
    }

    module_response = module_request.response;

    struct limine_file *initrd;

    for (uint64_t module = 0; module < module_response->module_count;
         module++) {
        struct limine_file *limine_module = module_response->modules[module];
        kprintf_info("Module %s loaded\n", limine_module->path);

        if (strcmp(INITRD_PATH, limine_module->path) == 0) {
            kprintf_info("Found initrd image\n");
            initrd = limine_module;
        }
    }

    if (!initrd) {
        kprintf_panic("No initrd file found.");
        _hcf();
    }

    kprintf_info("Initrd loaded at address %p\n", initrd->address);

    ustar_fs *initramfs_disk = ramfs_init(initrd->address);

    kprintf_ok("initrd.img entries list:\n");
    for (size_t i = 0; i < initramfs_disk->file_count; i++) {
        kprintf_ok("File n.%zu: %s\n", i, initramfs_disk->files[i]->path);
    }

    size_t end_tick_after_init  = get_current_ticks();
    end_tick_after_init        -= start_tick_after_pit_init;
    kprintf("System started: Time took: %d seconds %d ms\n",
            end_tick_after_init / PIT_TICKS, end_tick_after_init % 1000);

    for (;;)
        ;
}
