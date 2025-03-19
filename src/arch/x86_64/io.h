#ifndef IO_H
#define IO_H 1

#include <stdint.h>

extern void _outb(uint16_t port, uint8_t val);
extern uint8_t _inb(uint16_t port);

extern void _outw(uint16_t port, uint16_t val);
extern uint16_t _inw(uint16_t port);

extern void _outd(uint16_t port, uint32_t val);
extern uint32_t _ind(uint16_t port);

extern void _io_wait(void);

#endif
