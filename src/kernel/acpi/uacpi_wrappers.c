#include <acpi/uacpi/types.h>

#include <kernel.h>

#include <pit.h>

#include <memory/heap/liballoc.h>
#include <memory/paging/paging.h>
#include <memory/vma.h>

#include <util/assert.h>
#include <util/string.h>
#include <util/util.h>

#include <stdio.h>

#include <io.h>

#include <dev/pcie/pcie.h>

#include <spinlock.h>

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
    size_t offset     = (size_t)(addr - aligned);
    size_t actual_len = len + offset;

    void *virt = vma_alloc(get_current_ctx(), actual_len, true, (void *)addr);

    // re-align the pointer to original addr offset
    virt += offset;

    return virt;
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

    uint32_t prev_bg = fb_get_bg();
    uint32_t prev_fg = fb_get_fg();

    switch (log_level) {
    case UACPI_LOG_DEBUG:
        debugf(ANSI_COLOR_GRAY "[ uacpi::DEBUG ] %s", fmt);
        break;
    case UACPI_LOG_TRACE:
        debugf(ANSI_COLOR_GRAY "[ uacpi::TRACE ] %s", fmt);
        break;

    case UACPI_LOG_INFO:
        debugf(ANSI_COLOR_GRAY);
        fb_set_fg(INFO_FG);
        mprintf("[ uacpi::INFO ] %s", fmt);
        break;
    case UACPI_LOG_WARN:
        fb_set_fg(WARNING_FG);
        debugf(ANSI_COLOR_ORANGE);
        mprintf("[ uacpi::WARNING ] %s", fmt);
        break;
    case UACPI_LOG_ERROR:
        fb_set_fg(PANIC_FG);
        debugf(ANSI_COLOR_RED);
        mprintf("[ uacpi::CRITICAL ] %s", fmt);

    default:
        break;
    }

    debugf(ANSI_COLOR_RESET);
    set_screen_bg_fg(prev_bg, prev_fg);
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

#ifndef UACPI_BAREBONES_MODE

#ifdef UACPI_KERNEL_INITIALIZATION
// note for future Omar: other stuff calls this function to say:
// "Hey bro we need to init UACPI with this init level"
// and you initialize the proper components for that init level
uacpi_status uacpi_kernel_initialize(uacpi_init_level current_init_lvl) {
    if (current_init_lvl != UACPI_INIT_LEVEL_EARLY)
        return UACPI_STATUS_DENIED;

    return UACPI_STATUS_OK;
}

void uacpi_kernel_deinitialize(void) {
    return;
}
#endif

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address,
                                          uacpi_handle *out_handle) {
    uint64_t pci_address = PCI_ADDR(address.segment, address.bus,
                                    address.device, address.function);

    if (pci_address == 0)
        return UACPI_STATUS_DENIED;

    ((uint64_t **)out_handle)[0] = (uint64_t *)PHYS_TO_VIRTUAL(pci_address);

    return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle *handle) {
    assert(handle);

    handle[0] = NULL;
}

uacpi_status uacpi_kernel_pci_read_impl(uacpi_handle addr,
                                        uacpi_handle value_out,
                                        uacpi_size bytes) {
    switch (bytes) {
    case 1:
        ((uint8_t *)value_out)[0] = ((uint8_t *)PHYS_TO_VIRTUAL(addr))[0];
        break;

    case 2:
        ((uint16_t *)value_out)[0] = ((uint16_t *)PHYS_TO_VIRTUAL(addr))[0];
        ;
        break;
    case 4:
        ((uint32_t *)value_out)[0] = ((uint32_t *)PHYS_TO_VIRTUAL(addr))[0];
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset,
                                    uacpi_u8 *value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_read_impl(pci_addr, *(uacpi_size *)value, 1);
}
uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset,
                                     uacpi_u16 *value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_read_impl(pci_addr, *(uacpi_size *)value, 2);
}
uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset,
                                     uacpi_u32 *value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_read_impl(pci_addr, *(uacpi_size *)value, 4);
}

uacpi_status uacpi_kernel_pci_write_impl(uacpi_handle addr, uacpi_size value,
                                         uacpi_size bytes) {
    switch (bytes) {
    case 1:
        ((uint8_t *)PHYS_TO_VIRTUAL(addr))[0] = (uint8_t)value;
        break;

    case 2:
        ((uint16_t *)PHYS_TO_VIRTUAL(addr))[0] = (uint16_t)value;
        ;
        break;
    case 4:
        ((uint16_t *)PHYS_TO_VIRTUAL(addr))[0] = (uint16_t)value;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset,
                                     uacpi_u8 value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_write_impl(pci_addr, (uacpi_size)value, 1);
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset,
                                      uacpi_u16 value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_write_impl(pci_addr, (uacpi_size)value, 2);
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset,
                                      uacpi_u32 value) {
    uacpi_pci_address *pci_dev = (uacpi_pci_address *)PHYS_TO_VIRTUAL(device);

    void *pci_addr =
        (((pci_dev->bus << 8) + (pci_dev->device << 3) + pci_dev->function)
         << 12) +
        offset;

    return uacpi_kernel_pci_write_impl(pci_addr, (uacpi_size)value, 2);
}

typedef struct {
    uint16_t base;
    uint16_t max_offset;
} io_t;

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
                                 uacpi_handle *out_handle) {

    io_t *io = kmalloc(sizeof(io_t));
    if (!io)
        return UACPI_STATUS_OUT_OF_MEMORY;

    ((io_t **)out_handle)[0] = io;

    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {

    io_t *to_free = ((io_t **)handle)[0];
    if (!to_free)
        return;

    kfree(to_free);

    ((io_t **)handle)[0] = NULL;
}

uacpi_status uacpi_kernel_io_read8(uacpi_handle range, uacpi_size offset,
                                   uacpi_u8 *out_value) {
    io_t *io = range;
    if (!range)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    out_value[0]   = _inb(port);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle range, uacpi_size offset,
                                    uacpi_u16 *out_value) {
    io_t *io = range;
    if (!range)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    out_value[0]   = _inw(port);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle range, uacpi_size offset,
                                    uacpi_u32 *out_value) {
    io_t *io = range;
    if (!io)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    out_value[0]   = _ind(port);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle range, uacpi_size offset,
                                    uacpi_u8 in_value) {
    io_t *io = range;
    if (!io)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    _outb(port, in_value);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle range, uacpi_size offset,
                                     uacpi_u16 in_value) {
    io_t *io = range;
    if (!io)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    _outw(port, in_value);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle range, uacpi_size offset,
                                     uacpi_u32 in_value) {
    io_t *io = range;
    if (!io)
        return UACPI_STATUS_NOT_FOUND;

    uacpi_u16 port = (uacpi_u16)range->base + (uacpi_u16)offset;
    _outd(port, in_value);

    return UACPI_STATUS_OK;
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return kmalloc(size);
}

#ifdef UACPI_NATIVE_ALLOC_ZEROED
void *uacpi_kernel_alloc_zeroed(uacpi_size size) {
    void *base = kmalloc(size);
    memset(base, 0, size);
}
#endif

#ifndef UACPI_SIZED_FREES
void uacpi_kernel_free(void *mem) {
    if (!mem)
        return;

    kfree(mem);
}
#else
void uacpi_kernel_free(void *mem, uacpi_size size_hint) {
    UNUSED(size_hint);

    if (!mem)
        return;

    kfree(mem);
}
#endif

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    bootloader_data *limine_data = get_bootloader_data();

    uacpi_u64 boot_ns  = limine_data->boot_time;
    boot_ns           *= 1000; // 1 ms = 1000 ns

    return boot_ns;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    for (uacpi_u8 i = 0; i < usec; i++)
        ;
}

// TODO: timer-agnostic sleep
void uacpi_kernel_sleep(uacpi_u64 msec) {
    pit_sleep(msec);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    atomic_flag *atomic = kmalloc(sizeof(atomic_flag));

    return atomic;
}

void uacpi_kernel_free_mutex(uacpi_handle atomic) {
    kfree(atomic);
}

#endif // ifndef UACPI_BAREBONES_MODE
