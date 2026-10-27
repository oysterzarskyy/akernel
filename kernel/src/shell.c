#include <stddef.h>
#include <stdint.h>

#include "akernel/shell.h"
#include "akernel/console/framebuffer.h"
#include "akernel/drivers/keyboard.h"
#include "akernel/drivers/serial.h"
#include "akernel/arch/io.h"
#include "akernel/version.h"
#include "fetch.h"

/* ------------------------------------------------------------------ */
/*  Tiny string helpers (no libc in freestanding)                      */
/* ------------------------------------------------------------------ */

static unsigned int kstrlen(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

static int kstreq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int kstarts(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Dual-output print helpers                                           */
/* ------------------------------------------------------------------ */

static void kprint(struct akernel_framebuffer_console *con, const char *s) {
    if (!s) return;
    unsigned int n = kstrlen(s);
    if (con) akernel_framebuffer_console_write(s, n, con);
    serial_write(COM1_PORT, s);
}

static void kprint_color(struct akernel_framebuffer_console *con,
                         const char *color, const char *s) {
    if (con) akernel_framebuffer_console_set_color(con, color, kstrlen(color));
    kprint(con, s);
}

/* ------------------------------------------------------------------ */
/*  Simple busy-wait delay (spin loop – no PIT configured yet)         */
/* ------------------------------------------------------------------ */

static void delay_spin(unsigned int count) {
    for (volatile unsigned int i = 0; i < count; i++) {
        __asm__ volatile ("pause");
    }
}

/* ------------------------------------------------------------------ */
/*  Blinking text cursor drawn on the framebuffer                      */
/*  Cursor is a 2×8 white rectangle at the current write position.     */
/* ------------------------------------------------------------------ */

static void cursor_draw(struct akernel_framebuffer_console *con, int visible) {
    if (!con) return;
    /* Use the existing putchar slot – draw a space with inverted colour */
    if (visible) {
        akernel_framebuffer_console_set_color(con, "white", 5u);
        akernel_framebuffer_console_putchar(con, '_');
        /* Step back so next typed char overwrites it */
        akernel_framebuffer_console_backspace(con);
    } else {
        /* Erase cursor by drawing a space-width background block */
        akernel_framebuffer_console_backspace(con);
    }
}

/* ------------------------------------------------------------------ */
/*  Prompt                                                              */
/* ------------------------------------------------------------------ */

static void print_prompt(struct akernel_framebuffer_console *con) {
    kprint_color(con, "green",   "\n@user");
    kprint_color(con, "white",   "/");
    kprint_color(con, "magenta", "akernel");
    kprint_color(con, "yellow",  " $ ");
    kprint_color(con, "white",   "");
    /* mirror to serial */
    serial_write(COM1_PORT, "\n@user/akernel $ ");
}

/* ------------------------------------------------------------------ */
/*  Hardware reboot via 8042                                            */
/* ------------------------------------------------------------------ */

static void reboot_system(void) {
    uint8_t tmp;
    do {
        tmp = inb(0x64);
        if (tmp & 1) inb(0x60);
    } while (tmp & 2);
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile ("hlt");
}

/* ------------------------------------------------------------------ */
/*  CPUID – returns CPU vendor string in vendor[13]                     */
/* ------------------------------------------------------------------ */

static void cpuid_vendor(char vendor[13]) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(0));
    ((uint32_t *)vendor)[0] = ebx;
    ((uint32_t *)vendor)[1] = edx;
    ((uint32_t *)vendor)[2] = ecx;
    vendor[12] = '\0';
}

/* ------------------------------------------------------------------ */
/*  Command execution                                                   */
/* ------------------------------------------------------------------ */

static void execute(char *line, struct akernel_framebuffer_console *con) {
    /* trim leading spaces */
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    /* ---- help ---- */
    if (kstreq(line, "help")) {
        kprint_color(con, "cyan",
            "akernel " AKERNEL_VERSION " (" AKERNEL_VERSION_NAME ") shell commands:\n");
        kprint(con,
            "  help             list available commands\n"
            "  clear            clear the screen\n"
            "  fetch / neofetch system info & ASCII art\n"
            "  uname [-a]       kernel release info\n"
            "  version          kernel version & codename\n"
            "  echo <text>      print text\n"
            "  color <name>     change text colour\n"
            "                   (red green yellow blue magenta cyan white)\n"
            "  cpuid            CPU vendor string\n"
            "  reboot           hardware reset via 8042\n"
            "  halt / poweroff  stop the CPU\n");

    /* ---- clear ---- */
    } else if (kstreq(line, "clear")) {
        if (con) akernel_framebuffer_console_clear(con);

    /* ---- fetch / neofetch ---- */
    } else if (kstreq(line, "fetch") || kstreq(line, "neofetch")) {
        akpkg_fetch_render(akernel_framebuffer_console_write, con);

    /* ---- uname ---- */
    } else if (kstreq(line, "uname") || kstreq(line, "uname -a")) {
        kprint(con, "akernel " AKERNEL_VERSION " #1 SMP x86_64 GNU/akernel\n");

    /* ---- version ---- */
    } else if (kstreq(line, "version")) {
        kprint_color(con, "cyan",
            "akernel v" AKERNEL_VERSION " (" AKERNEL_VERSION_NAME ")\n");

    /* ---- echo ---- */
    } else if (kstarts(line, "echo ")) {
        kprint(con, line + 5);
        kprint(con, "\n");

    /* ---- color ---- */
    } else if (kstarts(line, "color ")) {
        char *col = line + 6;
        while (*col == ' ') col++;
        unsigned int len = kstrlen(col);
        /* strip trailing spaces */
        while (len > 0 && col[len-1] == ' ') len--;
        if (!con || akernel_framebuffer_console_set_color(con, col, len) != 0) {
            kprint_color(con, "red",
                "error: unknown colour; try red green yellow blue magenta cyan white\n");
        }

    /* ---- cpuid ---- */
    } else if (kstreq(line, "cpuid")) {
        char vendor[13];
        cpuid_vendor(vendor);
        kprint_color(con, "cyan", "CPU vendor: ");
        kprint(con, vendor);
        kprint(con, "\n");

    /* ---- reboot ---- */
    } else if (kstreq(line, "reboot")) {
        kprint_color(con, "yellow", "Rebooting…\n");
        delay_spin(5000000u);
        reboot_system();

    /* ---- halt / poweroff ---- */
    } else if (kstreq(line, "halt") || kstreq(line, "poweroff")) {
        kprint_color(con, "yellow",
            "System halted. Safe to close QEMU.\n");
        for (;;) __asm__ volatile ("cli; hlt");

    /* ---- unknown ---- */
    } else {
        kprint_color(con, "red", "command not found: ");
        kprint(con, line);
        kprint(con, "\n");
    }
}

/* ------------------------------------------------------------------ */
/*  Main shell entry point                                              */
/* ------------------------------------------------------------------ */

void akernel_shell_start(struct akernel_framebuffer_console *con) {
    /* 256-byte line buffer */
    char buf[256];
    unsigned int len = 0;

    /* Blink state */
    unsigned int blink_counter  = 0;
    int          cursor_visible = 0;

    /* ~10M spins ≈ 1 blink period on typical QEMU TCG */
#define BLINK_HALF 4000000u

    kprint_color(con, "green",
        "akernel " AKERNEL_VERSION " (" AKERNEL_VERSION_NAME ") – type help\n");
    print_prompt(con);
    if (con) cursor_draw(con, 1);

    for (;;) {
        /* ---- blink cursor ---- */
        blink_counter++;
        if (blink_counter >= BLINK_HALF) {
            blink_counter = 0;
            cursor_visible ^= 1;
            if (con) cursor_draw(con, cursor_visible);
        }

        /* ---- poll keyboard (non-blocking) ---- */
        char c = keyboard_read_char();

        /* ---- poll serial (QEMU -serial stdio) ---- */
        if (!c && serial_received(COM1_PORT)) {
            c = serial_read(COM1_PORT);
        }

        if (!c) continue;

        /* erase cursor before editing */
        if (con && cursor_visible) {
            cursor_draw(con, 0);
            cursor_visible = 0;
        }

        if (c == '\r') c = '\n';

        if (c == '\n') {
            /* execute line */
            kprint(con, "\n");
            buf[len] = '\0';
            execute(buf, con);
            len = 0;
            print_prompt(con);
            if (con) { cursor_draw(con, 1); cursor_visible = 1; }
            blink_counter = 0;

        } else if (c == '\b' || c == 127) {
            /* backspace */
            if (len > 0) {
                len--;
                if (con) akernel_framebuffer_console_backspace(con);
                serial_write(COM1_PORT, "\b \b");
            }

        } else if (c >= 32 && c <= 126) {
            /* printable character */
            if (len < sizeof(buf) - 1) {
                buf[len++] = c;
                if (con) akernel_framebuffer_console_putchar(con, c);
                serial_putchar(COM1_PORT, c);
            }
        }

        /* redraw cursor after every event */
        if (con && !cursor_visible) {
            cursor_draw(con, 1);
            cursor_visible = 1;
            blink_counter  = 0;
        }
    }
}
