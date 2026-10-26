#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "aash.h"
#include "akernel/version.h"
#include "include/setup.h"

static char output[4096];
static unsigned int output_length;
static unsigned int setup_calls;
static char selected_color[16];

int akernel_framebuffer_console_write(const char *text,
                                      unsigned int length,
                                      void *context)
{
    (void)context;
    if (text == NULL || length > sizeof(output) - output_length - 1u) {
        return -1;
    }

    memcpy(output + output_length, text, length);
    output_length += length;
    output[output_length] = '\0';
    return 0;
}

int akernel_framebuffer_console_set_color(
    struct akernel_framebuffer_console *console,
    const char *name,
    unsigned int length)
{
    (void)console;
    if (name == NULL || length == 0u || length >= sizeof(selected_color)) {
        return -1;
    }

    memcpy(selected_color, name, length);
    selected_color[length] = '\0';
    return strcmp(selected_color, "red") == 0 ||
                   strcmp(selected_color, "green") == 0 ||
                   strcmp(selected_color, "yellow") == 0 ||
                   strcmp(selected_color, "blue") == 0 ||
                   strcmp(selected_color, "magenta") == 0 ||
                   strcmp(selected_color, "cyan") == 0 ||
                   strcmp(selected_color, "white") == 0
               ? 0
               : -1;
}

int akernel_setup_render(struct akernel_framebuffer_console *console)
{
    static const char status[] = "setup completed\n";
    ++setup_calls;
    return akernel_framebuffer_console_write(
        status, (unsigned int)(sizeof(status) - 1u), console);
}

static int test_boot_script(void)
{
    static const char script[] =
        "# AASH comments work\r\n"
        "color cyan\r\n"
        "echo hello from aash\r\n"
        "setup\r\n"
        "version";
    struct akernel_framebuffer_console console = {0};

    output_length = 0u;
    output[0] = '\0';
    setup_calls = 0u;
    selected_color[0] = '\0';

    if (akaash_run(script, &console) != 0) {
        return 1;
    }
    if (strcmp(selected_color, "cyan") != 0 ||
        setup_calls != 1u ||
        strstr(output, "hello from aash\n") == NULL ||
        strstr(output, "setup completed\n") == NULL ||
        strstr(output, "akernel " AKERNEL_VERSION_STRING
                       " / AASH " AKASH_VERSION "\n") == NULL) {
        return 1;
    }
    return 0;
}

static int test_invalid_commands(void)
{
    struct akernel_framebuffer_console console = {0};

    output_length = 0u;
    output[0] = '\0';
    if (akaash_run("unknown\n", &console) == 0 ||
        strstr(output, "AASH error:") == NULL ||
        akaash_run("color ultraviolet\n", &console) == 0 ||
        akaash_run("setup unexpected\n", &console) == 0 ||
        akaash_run(NULL, &console) == 0 ||
        akaash_run("help\n", NULL) == 0) {
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_boot_script() != 0) {
        fputs("AASH boot script test failed\n", stderr);
        return 1;
    }
    if (test_invalid_commands() != 0) {
        fputs("AASH invalid command test failed\n", stderr);
        return 1;
    }

    puts("AASH interpreter tests passed.");
    return 0;
}
