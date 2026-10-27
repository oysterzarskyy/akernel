/*
 * PS/2 Keyboard Driver – scancode set 1
 * Returns ASCII character or 0 for non-printable scancodes.
 * keyboard_read_char() is NON-BLOCKING: returns 0 immediately if nothing ready.
 */
#include "akernel/drivers/keyboard.h"
#include "akernel/arch/io.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

/* Unshifted scancode → ASCII */
static const char sc_normal[128] = {
/*00*/  0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
/*0F*/ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
/*1D*/  0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
/*2A*/  0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
/*37*/ '*', 0,  ' ', 0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*46*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*55*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*64*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*73*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*82*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* Shifted scancode → ASCII */
static const char sc_shift[128] = {
/*00*/  0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
/*0F*/ '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
/*1D*/  0,  'A','S','D','F','G','H','J','K','L',':','"','~',
/*2A*/  0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
/*37*/ '*', 0,  ' ', 0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*46*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*55*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*64*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*73*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*82*/  0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static int shift_held  = 0;
static int caps_lock   = 0;

int keyboard_has_char(void) {
    /* bit 0 of status = output buffer full */
    return inb(KBD_STATUS_PORT) & 0x01;
}

/*
 * Non-blocking: if the keyboard output buffer is empty returns 0 immediately.
 * Modifier keys (Shift, Caps Lock) update internal state; key releases are
 * discarded.  Returns the ASCII character or 0 for non-printable keys.
 */
char keyboard_read_char(void) {
    if (!keyboard_has_char()) {
        return 0;
    }

    uint8_t sc = inb(KBD_DATA_PORT);

    /* ---- Key release (bit 7 set) ---- */
    if (sc & 0x80) {
        uint8_t make = sc & 0x7F;
        if (make == 0x2A || make == 0x36) {
            shift_held = 0;
        }
        return 0;
    }

    /* ---- Modifier make codes ---- */
    if (sc == 0x2A || sc == 0x36) { shift_held = 1;  return 0; }
    if (sc == 0x3A)               { caps_lock ^= 1; return 0; } /* CapsLk toggle */
    if (sc == 0x1D || sc == 0x38) { return 0; }                 /* Ctrl / Alt – ignore */

    if (sc >= 128) return 0;

    char c = shift_held ? sc_shift[sc] : sc_normal[sc];

    /* CapsLock only flips letters */
    if (caps_lock && c >= 'a' && c <= 'z') c -= 32;
    if (caps_lock && c >= 'A' && c <= 'Z') c += 32;

    return c;
}
