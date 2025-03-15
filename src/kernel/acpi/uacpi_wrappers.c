#include <acpi/uacpi/types.h>

#include <kernel.h>
#include <memory/paging/paging.h>
#include <memory/vma.h>

#include <util/util.h>

#include <stdio.h>
#include <util/string.h>

// --- fun fact: `uacpi_phys_addr` is uint64_t btw ---

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    bootloader_data *limine_data = get_bootloader_data();
    if (!limine_data)
        return UACPI_STATUS_NOT_FOUND;

    if (!limine_data->rsdp_table_address)
        return UACPI_STATUS_NOT_FOUND;

    uint64_t rsdp_phys = VIRT_TO_PHYSICAL(limine_data->rsdp_table_address);
    *out_rsdp_address  = rsdp_phys;

    return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    uint64_t aligned  = ROUND_DOWN(addr, PFRAME_SIZE);
    size_t actual_len = len + (size_t)(addr - aligned);

    void *virt = vma_alloc(get_current_ctx(), actual_len, true);

    uint64_t phys = pg_virtual_to_phys(_get_pml4(), (uint64_t)virt);
    pmm_free((void *)phys, ROUND_UP(actual_len, PFRAME_SIZE));

    // we'll overwrite the physical mapping :3
    map_region_to_page(_get_pml4(), addr, (uint64_t)virt, actual_len,
                       PMLE_KERNEL_READ_WRITE);

    // re-align the pointer to original addr offset
    aligned += addr - aligned;

    return (void *)aligned;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    uint64_t aligned  = ROUND_DOWN((uint64_t)addr, PFRAME_SIZE);
    size_t actual_len = len + (size_t)((uint64_t)addr - aligned);

    unmap_region(_get_pml4(), aligned, actual_len);
}

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level log_level, const uacpi_char *fmt) {
    uint32_t prev_bg = fb_get_bg();
    uint32_t prev_fg = fb_get_fg();

    switch (log_level) {
    case UACPI_LOG_DEBUG:
        debugf("[ uacpi::DEBUG ] %s\n", fmt);
        break;
    case UACPI_LOG_TRACE:
        debugf(ANSI_COLOR_GRAY "[ uacpi::TRACE ] %s\n", fmt);
        break;

    case UACPI_LOG_INFO:
        debugf(ANSI_COLOR_GRAY);
        fb_set_fg(INFO_FG);
        mprintf("[ uacpi::INFO ] %s\n", fmt);
        break;
    case UACPI_LOG_WARN:
        fb_set_fg(WARNING_FG);
        debugf(ANSI_COLOR_ORANGE);
        mprintf("[ uacpi::WARNING ] %s\n", fmt);
        break;
    case UACPI_LOG_ERROR:
        fb_set_fg(PANIC_FG);
        debugf(ANSI_COLOR_RED);
        mprintf("[ uacpi::CRITICAL ] %s\n", fmt);

    default:
        break;
    }

    debugf(ANSI_COLOR_RESET);
    set_screen_bg_fg(prev_bg, prev_fg);
}
#else
void uacpi_kernel_log(uacpi_log_level log_level, const uacpi_char *fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length < 0 || length >= (int)sizeof(buffer)) {
        return;
    }

    uacpi_kernel_log(log_level, buffer);
}

void uacpi_kernel_vlog(uacpi_log_level log_level, const uacpi_char *fmt,
                       uacpi_va_list va_args) {

    char buffer[1024];

    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, va_args);

    if (length < 0 || length >= (int)sizeof(buffer)) {
        return;
    }

    uacpi_kernel_log(log_level, buffer);
}
#endif