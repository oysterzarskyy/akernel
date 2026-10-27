#ifndef AKERNEL_DRIVERS_SERIAL_H
#define AKERNEL_DRIVERS_SERIAL_H

#include <stdint.h>

#define COM1_PORT 0x3F8

int serial_init(uint16_t port);
void serial_putchar(uint16_t port, char c);
void serial_write(uint16_t port, const char *str);
int serial_received(uint16_t port);
char serial_read(uint16_t port);

#endif
