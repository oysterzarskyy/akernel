#ifndef AKERNEL_DRIVERS_KEYBOARD_H
#define AKERNEL_DRIVERS_KEYBOARD_H

#include <stdint.h>

char keyboard_read_char(void);
int keyboard_has_char(void);

#endif
