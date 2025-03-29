#ifndef PCIE_H
#define PCIE_H

#include <fs/ustar/ustar.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_VENDOR_NAME 128
#define MAX_DEVICE_NAME 128

#define PCI_ADDR(s, b, d, f) (((b << 20) | (d << 15) | (f << 12)) + s)

typedef struct {
    uint8_t class_code;
    const char *class_name;
} pci_class_t;

typedef struct {
    uint8_t class_code;
    uint8_t subclass;
    const char *subclass_name;
} pci_subclass_t;

typedef struct {
    char vendor[MAX_VENDOR_NAME];
    char device[MAX_DEVICE_NAME];
} pci_vendor_specific_t;

typedef struct pci_device {
    uint8_t bus, device, function;
    uint16_t vendor_id, device_id;

    pci_vendor_specific_t vendor_specific;
    pci_subclass_t subclass_info;
    pci_class_t class;

    uint8_t class_code, subclass, prog_if, header_type;
    struct pci_device *next;
} pci_device_t;

extern pci_device_t *pci_devices_head;

void pci_scan();
void pci_print_devices(ustar_file_tree_t *pci_ids);
void pci_free_list();
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function,
                         uint8_t offset);
const char *pci_get_class_name(uint8_t class_code);
const char *pci_get_subclass_name(uint8_t class_code, uint8_t subclass);

pci_vendor_specific_t get_pci_device_info(const char *pci_ids, size_t length,
                                          uint16_t vendor_id,
                                          uint16_t device_id);

#endif
