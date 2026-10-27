#include "akernel/boot/limine.h"
#include "akernel/console/framebuffer.h"
#include "akernel/drivers/serial.h"
#include "akernel/shell.h"
#include "fetch.h"

void kmain(void)
{
    struct akernel_framebuffer_info framebuffer;
    struct akernel_framebuffer_console console;

    // Initialize serial console (COM1 0x3F8) for QEMU -serial stdio logging
    serial_init(COM1_PORT);
    serial_write(COM1_PORT, "\n[akernel] Initializing kernel subsystems...\n");

    if (akernel_limine_get_framebuffer(&framebuffer) != 0 ||
        akernel_framebuffer_console_init(&console, &framebuffer) != 0) {
        serial_write(COM1_PORT, "[akernel ERROR] Failed to initialize framebuffer console!\n");
        // Fallback: stay alive and listen on serial console even if graphics fails
        akernel_shell_start(0);
        return;
    }

    // Display ASCII fetch info initially
    akernel_framebuffer_console_set_color(&console, "magenta", 7u);
    akpkg_fetch_render(akernel_framebuffer_console_write, &console);

    // Launch interactive Linux-like terminal session (stays alive and loops continuously)
    akernel_shell_start(&console);
}
