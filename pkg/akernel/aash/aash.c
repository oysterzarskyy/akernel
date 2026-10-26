#include <stddef.h>

#include "aash.h"

#include "../setup/include/setup.h"
#include "akernel/version.h"

static int akaash_span_matches(const char *text,
                               unsigned int length,
                               const char *expected,
                               unsigned int expected_length)
{
    unsigned int index;

    if (length != expected_length) {
        return 0;
    }

    for (index = 0; index < length; ++index) {
        if (text[index] != expected[index]) {
            return 0;
        }
    }

    return 1;
}

static unsigned int akaash_trim_end(const char *text,
                                    unsigned int start,
                                    unsigned int end)
{
    while (end > start && (text[end - 1u] == ' ' || text[end - 1u] == '\t')) {
        --end;
    }

    return end;
}

static int akaash_write(struct akernel_framebuffer_console *console,
                        const char *text,
                        unsigned int length)
{
    return akernel_framebuffer_console_write(text, length, console);
}

static int akaash_print_help(struct akernel_framebuffer_console *console)
{
    static const char help[] =
        "AASH commands: echo TEXT, color NAME, setup, version, help\n"
        "Colors: red green yellow blue magenta cyan white\n"
        "AASH 0.1 runs boot-embedded scripts; it is not Bash-compatible.\n";

    return akaash_write(console, help, (unsigned int)(sizeof(help) - 1u));
}

static int akaash_execute_line(const char *script,
                              unsigned int start,
                              unsigned int end,
                              struct akernel_framebuffer_console *console)
{
    unsigned int command_end;
    unsigned int argument_start;
    unsigned int argument_end;

    while (start < end && (script[start] == ' ' || script[start] == '\t')) {
        ++start;
    }
    end = akaash_trim_end(script, start, end);
    if (start == end || script[start] == '#') {
        return 0;
    }

    command_end = start;
    while (command_end < end &&
           script[command_end] != ' ' &&
           script[command_end] != '\t') {
        ++command_end;
    }
    argument_start = command_end;
    while (argument_start < end &&
           (script[argument_start] == ' ' || script[argument_start] == '\t')) {
        ++argument_start;
    }
    argument_end = end;

    if (akaash_span_matches(script + start,
                            command_end - start,
                            "echo",
                            4u)) {
        if (argument_start < argument_end &&
            akaash_write(console,
                         script + argument_start,
                         argument_end - argument_start) != 0) {
            return -1;
        }
        return akaash_write(console, "\n", 1u);
    }

    if (akaash_span_matches(script + start,
                            command_end - start,
                            "color",
                            5u)) {
        return argument_start < argument_end
                   ? akernel_framebuffer_console_set_color(
                         console,
                         script + argument_start,
                         argument_end - argument_start)
                   : -1;
    }

    if (akaash_span_matches(script + start,
                            command_end - start,
                            "setup",
                            5u)) {
        return argument_start == argument_end
                   ? akernel_setup_render(console)
                   : -1;
    }

    if (akaash_span_matches(script + start,
                            command_end - start,
                            "help",
                            4u)) {
        return argument_start == argument_end ? akaash_print_help(console)
                                              : -1;
    }

    if (akaash_span_matches(script + start,
                            command_end - start,
                            "version",
                            7u)) {
        static const char prefix[] = "akernel " AKERNEL_VERSION_STRING
                                     " / AASH " AKASH_VERSION "\n";
        return argument_start == argument_end
                   ? akaash_write(console,
                                  prefix,
                                  (unsigned int)(sizeof(prefix) - 1u))
                   : -1;
    }

    return -1;
}

int akaash_run(const char *script,
               struct akernel_framebuffer_console *console)
{
    static const char error_message[] =
        "AASH error: unknown command, invalid arguments, or output failure.\n";
    unsigned int line_start = 0u;

    if (script == NULL || console == NULL) {
        return -1;
    }

    while (script[line_start] != '\0') {
        unsigned int line_end = line_start;
        unsigned int content_end;
        int result;

        while (script[line_end] != '\0' && script[line_end] != '\n') {
            ++line_end;
        }
        content_end = line_end;
        if (content_end > line_start && script[content_end - 1u] == '\r') {
            --content_end;
        }

        result = akaash_execute_line(script,
                                     line_start,
                                     content_end,
                                     console);
        if (result != 0) {
            (void)akaash_write(
                console,
                error_message,
                (unsigned int)(sizeof(error_message) - 1u));
            return result;
        }

        line_start = script[line_end] == '\n' ? line_end + 1u : line_end;
    }

    return 0;
}
