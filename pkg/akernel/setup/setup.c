#include <stddef.h>
#include "../../kernel/include/akernel/version.h"
#include "include/setup.h"

static const char setup_script[] =
    "# AASH boot setup preview\n"
    "color yellow\n"
    "echo      +--------+\n"
    "echo      |########|\n"
    "echo      || o  o ||\n"
    "echo      ||  ..  ||\n"
    "echo      ||  __  ||\n"
    "echo      |########|\n"
    "echo      +--------+\n"
    "echo        " AKERNEL_VERSION_NAME "\n"
    "color cyan\n"
    "echo " AKERNEL_VERSION_NAME " " AKERNEL_VERSION "-nightly - akernel shell-like script interpreter\n"
    "version\n"
    "setup\n"
    "help\n"
    "color green\n"
    "echo " AKERNEL_VERSION_NAME " preview complete; the kernel will halt safely.\n";

const char *akernel_setup_boot_script(void)
{
    return setup_script;
}

static int setup_write(struct akernel_framebuffer_console *console,
                       const char *text,
                       unsigned int length)
{
    return akernel_framebuffer_console_write(text, length, console);
}

int akernel_setup_render(struct akernel_framebuffer_console *console)
{
    static const char title[] = "AASH SETUP / FIRST-BOOT CHECK\n";
    static const char warning[] =
        "Experimental preview: this does not install an operating system.\n";
    static const char steps[] =
        "[OK] Limine boot and framebuffer console are available.\n"
        "[--] Keyboard input and persistent storage are not implemented.\n"
        "[--] No disk, partition, or filesystem changes were made.\n"
        "Next: add keyboard and filesystem drivers before an installer.\n";
    int result;

    if (console == NULL) {
        return -1;
    }

    result = akernel_framebuffer_console_set_color(console, "cyan", 4u);
    if (result != 0) {
        return result;
    }
    result = setup_write(console, title, (unsigned int)(sizeof(title) - 1u));
    if (result != 0) {
        return result;
    }

    result = akernel_framebuffer_console_set_color(console, "yellow", 6u);
    if (result != 0) {
        return result;
    }
    result =
        setup_write(console, warning, (unsigned int)(sizeof(warning) - 1u));
    if (result != 0) {
        return result;
    }

    result = akernel_framebuffer_console_set_color(console, "green", 5u);
    if (result != 0) {
        return result;
    }
    return setup_write(console, steps, (unsigned int)(sizeof(steps) - 1u));
}