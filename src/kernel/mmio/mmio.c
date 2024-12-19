#include <mmio/mmio.h>

#include <stdio.h>
#include <util/string.h>

extern void _hcf();

mmio_device mmio_devices[MMIO_MAX_DEVICES];

// used for appending devices to the list
int mmio_dev_index = 0;

mmio_device* get_mmio_devices() {
	return mmio_devices;
}

mmio_device find_mmio(const char* sig) {
	mmio_device* mmios = get_mmio_devices();

	for (int i = 0; i < MMIO_MAX_DEVICES; i++)
	{
		if (memcmp(mmios[i].name, sig, strlen(sig)) == 0) {
			return mmios[i];
		}
	}

	kprintf_warn("MMIO \"%s\" not found!\n", sig);
	_hcf();
}

void append_mmio(mmio_device device) {
	if (mmio_dev_index >= MMIO_MAX_DEVICES) {
		kprintf_warn("Attempt to append more devices than the maximum amount. Quitting task...\n");
		kprintf_info("\tdevice index: %d; maximum supported devices: %d", mmio_dev_index, MMIO_MAX_DEVICES);
		return;
	}

	mmio_devices[mmio_dev_index] = device;
	mmio_dev_index++;						// increments the index for the next append task
}
