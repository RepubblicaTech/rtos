#include "pcie.h"
#include "fs/ustar/ustar.h"
#include "io.h"
#include "memory/heap/liballoc.h"
#include "stdio.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

pci_device_t *pci_devices_head = NULL;

static const pci_class_t pci_classes[] = {
    {0x0, "Unclassified"},
    {0x1, "Mass Storage Controller"},
    {0x2, "Network Controller"},
    {0x3, "Display Controller"},
    {0x4, "Multimedia Controller"},
    {0x5, "Memory Controller"},
    {0x6, "Bridge"},
    {0x7, "Simple Communication Controller"},
    {0x8, "Base System Peripheral"},
    {0x9, "Input Device Controller"},
    {0xA, "Docking Station"},
    {0xB, "Processor"},
    {0xC, "Serial Bus Controller"},
    {0xD, "Wireless Controller"},
    {0xE, "Intelligent Controller"},
    {0xF, "Satellite Communications Controller"},
    {0x10, "Encryption Controller"},
    {0x11, "Signal Processing Controller"},
    {0x12, "Processing Accelerator"},
    {0x13, "Non-Essential Instrumentation"},
    {0xFF, "Unknown Class"}};

static const pci_subclass_t pci_subclasses[] = {
    {0x1, 0x6, "SATA Controller"},
    {0x1, 0x7, "Serial Attached SCSI Controller"},
    {0x2, 0x0, "Ethernet Controller"},
    {0x2, 0x1, "Token Ring Controller"},
    {0x2, 0x2, "FDDI Controller"},
    {0x2, 0x3, "ATM Controller"},
    {0x3, 0x0, "VGA-Compatible Controller"},
    {0x3, 0x1, "XGA Controller"},
    {0x3, 0x2, "3D Controller"},
    {0x6, 0x0, "Host Bridge"},
    {0x6, 0x1, "ISA Bridge"},
    {0x6, 0x2, "EISA Bridge"},
    {0x6, 0x3, "Microchannel Bridge"},
    {0x6, 0x4, "PCI-to-PCI Bridge"},
    {0x6, 0x5, "PCMCIA Bridge"},
    {0xC, 0x0, "FireWire (IEEE 1394) Controller"},
    {0xC, 0x1, "ACCESS Bus Controller"},
    {0xC, 0x2, "SSA Controller"},
    {0xC, 0x3, "USB Controller"},
    {0xC, 0x4, "Fibre Channel Controller"},
    {0xC, 0x5, "SMBus Controller"},
    {0xC, 0x6, "InfiniBand Controller"},
    {0xC, 0x7, "IPMI Interface"},
    {0xC, 0x8, "SERCOS Interface (IEC 61491)"},
    {0xC, 0x9, "CANbus Controller"},
    {0xFF, 0xFF, "Unknown Subclass"}};

const char *pci_get_class_name(uint8_t class_code) {
    for (size_t i = 0; i < sizeof(pci_classes) / sizeof(pci_classes[0]); i++) {
        if (pci_classes[i].class_code == class_code)
            return pci_classes[i].class_name;
    }
    return "Unknown Class";
}

const char *pci_get_subclass_name(uint8_t class_code, uint8_t subclass) {
    for (size_t i = 0; i < sizeof(pci_subclasses) / sizeof(pci_subclasses[0]);
         i++) {
        if (pci_subclasses[i].class_code == class_code &&
            pci_subclasses[i].subclass == subclass)
            return pci_subclasses[i].subclass_name;
    }
    return "Unknown Subclass";
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function,
                         uint8_t offset) {
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) | ((uint32_t)function << 8) |
                       (offset & 0xFC);
    _outl(PCI_CONFIG_ADDRESS, address);
    return _inl(PCI_CONFIG_DATA);
}

void pci_add_device(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t vendor_device = pci_config_read(bus, device, function, 0);
    if ((vendor_device & 0xFFFF) == 0xFFFF)
        return;

    pci_device_t *new_dev = (pci_device_t *)kmalloc(sizeof(pci_device_t));
    new_dev->bus          = bus;
    new_dev->device       = device;
    new_dev->function     = function;
    new_dev->vendor_id    = vendor_device & 0xFFFF;
    new_dev->device_id    = (vendor_device >> 16) & 0xFFFF;
    uint32_t class_info   = pci_config_read(bus, device, function, 8);
    new_dev->class_code   = (class_info >> 24) & 0xFF;
    new_dev->subclass     = (class_info >> 16) & 0xFF;
    new_dev->prog_if      = (class_info >> 8) & 0xFF;
    new_dev->header_type  = pci_config_read(bus, device, function, 0x0C) & 0xFF;
    new_dev->next         = pci_devices_head;
    pci_devices_head      = new_dev;
}

void pci_scan() {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                pci_add_device(bus, device, function);
            }
        }
    }
}

void pci_print_devices(ustar_file_tree_t *pci_ids) {
    if (!pci_devices_head) {
        kprintf("No PCI devices found.\n");
        return;
    }
    pci_device_t *dev = pci_devices_head;
    while (dev) {
        kprintf("PCI Device: Bus %u, Device %u, Function %u\n", dev->bus,
                dev->device, dev->function);

        {
            pci_vendor_specific_t info = get_pci_device_info(
                pci_ids->found_files[0]->start, pci_ids->found_files[0]->size,
                dev->vendor_id, dev->device_id);
            if (info.vendor[0] != '\0') {
                kprintf("  Vendor: %s (0x%04X)\n  Device: %s (0x%04X)\n",
                        info.vendor, dev->vendor_id, info.device,
                        dev->device_id);
            } else {
                kprintf("  Vendor: Unknown (0x%04X)\n  Device: Unknown "
                        "(0x%04X)\n",
                        dev->vendor_id, dev->device_id);
            }
        }
        kprintf("  Class: %s (0x%02X)\n  Subclass: %s (0x%02X)\n  Prog IF: "
                "0x%02X\n",
                pci_get_class_name(dev->class_code), dev->class_code,
                pci_get_subclass_name(dev->class_code, dev->subclass),
                dev->subclass, dev->prog_if);
        dev = dev->next;
    }
}

void pci_free_list() {
    while (pci_devices_head) {
        pci_device_t *temp = pci_devices_head;
        pci_devices_head   = pci_devices_head->next;
        kfree(temp);
    }
}

pci_vendor_specific_t get_pci_device_info(const char *pci_ids, size_t length,
                                          uint16_t vendor_id,
                                          uint16_t device_id) {
    pci_vendor_specific_t info = {"Unknown Vendor", "Unknown Device"};

    size_t i                          = 0;
    char vendor_name[MAX_VENDOR_NAME] = "";
    int found_vendor                  = 0;

    while (i < length) {
        if (pci_ids[i] == '#') {
            while (i < length && pci_ids[i] != '\n')
                i++;
            i++;
            continue;
        }

        if (pci_ids[i] != '\t') { // Vendor line
            uint16_t v_id = 0;
            size_t j      = 0;
            while (i < length && pci_ids[i] != ' ' && j < 4) {
                v_id = (v_id << 4) | (pci_ids[i] >= '0' && pci_ids[i] <= '9'
                                          ? pci_ids[i] - '0'
                                          : (pci_ids[i] | 32) - 'a' + 10);
                i++;
                j++;
            }
            while (i < length && pci_ids[i] == ' ')
                i++;

            size_t k = 0;
            while (i < length && pci_ids[i] != '\n' &&
                   k < MAX_VENDOR_NAME - 1) {
                vendor_name[k++] = pci_ids[i++];
            }
            vendor_name[k] = '\0';

            if (v_id == vendor_id) {
                for (size_t m = 0; m < MAX_VENDOR_NAME; m++) {
                    info.vendor[m] = vendor_name[m];
                }
                found_vendor = 1;
            }
        } else if (found_vendor) { // Device line (indented)
            i++;                   // Skip tab
            uint16_t d_id = 0;
            size_t j      = 0;
            while (i < length && pci_ids[i] != ' ' && j < 4) {
                d_id = (d_id << 4) | (pci_ids[i] >= '0' && pci_ids[i] <= '9'
                                          ? pci_ids[i] - '0'
                                          : (pci_ids[i] | 32) - 'a' + 10);
                i++;
                j++;
            }
            while (i < length && pci_ids[i] == ' ')
                i++;

            char device_name[MAX_DEVICE_NAME];
            size_t k = 0;
            while (i < length && pci_ids[i] != '\n' &&
                   k < MAX_DEVICE_NAME - 1) {
                device_name[k++] = pci_ids[i++];
            }
            device_name[k] = '\0';

            if (d_id == device_id) {
                for (size_t m = 0; m < MAX_DEVICE_NAME; m++) {
                    info.device[m] = device_name[m];
                }
                break;
            }
        }

        while (i < length && pci_ids[i] != '\n')
            i++;
        i++;
    }

    return info;
}
