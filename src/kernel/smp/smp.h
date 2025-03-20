#ifndef SMP_H
#define SMP_H 1

#define LAPIC_ICR_LOW  0x300
#define LAPIC_ICR_HIGH 0x310

#define LAPIC_DELIVERY_MODE_FIXED 0x000
#define LAPIC_DELIVERY_MODE_NMI   0x400
#define LAPIC_DELIVERY_MODE_INIT  0x500

#define LAPIC_DEST_SELF          0x40000
#define LAPIC_DEST_ALL_INCLUDING 0x80000
#define LAPIC_DEST_ALL_EXCLUDING 0xC0000

#define IPI_VECTOR_TEST       0xF0
#define IPI_VECTOR_RESCHEDULE 0xF1
#define IPI_VECTOR_TLB_SHOOT  0xF2
#define IPI_VECTOR_HALT       0xF3

#include <limine.h>

int smp_init(LIMINE_PTR(struct limine_smp_response *) cpus);
void mp_trampoline(struct limine_smp_info *cpu);

// ipi
void send_ipi(uint8_t cpu_id, uint8_t vector);
void send_ipi_all(uint8_t vector);
void send_ipi_all_excluding_self(uint8_t vector);
void send_ipi_self(uint8_t vector);

void register_ipi_handlers(void);

extern uint64_t cpu_count;

#endif