#ifndef VMM_H
#define VMM_H 1

// PAT
#define PAT_UNCACHEABLE			0
#define PAT_WRITE_COMBINING		1
#define PAT_WRITE_THROUGH		4
#define PAT_WRITE_PROTECTED		5
#define PAT_WRITEBACK			6
#define PAT_UNCACHED			7

void vmm_init();

#endif
