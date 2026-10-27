#include "akernel/drivers/serial.h"
#include "akernel/arch/io.h"

int serial_init(uint16_t port) {
    outb(port + 1, 0x00);    // Disable all interrupts
    outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);    //                  (hi byte)
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(port + 0, 0xAE);    // Test character

    // Check if serial is faulty
    if (inb(port + 0) != 0xAE) {
        return -1;
    }

    // Set normal operation mode
    outb(port + 4, 0x0F);
    return 0;
}

static int is_transmit_empty(uint16_t port) {
    return inb(port + 5) & 0x20;
}

void serial_putchar(uint16_t port, char c) {
    while (is_transmit_empty(port) == 0);
    outb(port, c);
}

void serial_write(uint16_t port, const char *str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putchar(port, '\r');
        }
        serial_putchar(port, *str++);
    }
}

int serial_received(uint16_t port) {
    return inb(port + 5) & 1;
}

char serial_read(uint16_t port) {
    while (serial_received(port) == 0);
    return inb(port);
}
