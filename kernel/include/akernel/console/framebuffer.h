#ifndef AKERNEL_CONSOLE_FRAMEBUFFER_H
#define AKERNEL_CONSOLE_FRAMEBUFFER_H

#include <stdint.h>

struct akernel_framebuffer_info {
    volatile uint32_t *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch_pixels;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

struct akernel_framebuffer_console {
    struct akernel_framebuffer_info framebuffer;
    uint64_t cursor_x;
    uint64_t cursor_y;
    uint64_t start_x;
    uint32_t scale;
    uint32_t foreground;
};

int akernel_framebuffer_console_init(
    struct akernel_framebuffer_console *console,
    const struct akernel_framebuffer_info *framebuffer);

int akernel_framebuffer_console_write(const char *text,
                                      unsigned int length,
                                      void *context);

int akernel_framebuffer_console_set_color(
    struct akernel_framebuffer_console *console,
    const char *name,
    unsigned int length);

void akernel_framebuffer_console_clear(struct akernel_framebuffer_console *console);
void akernel_framebuffer_console_putchar(struct akernel_framebuffer_console *console, char c);
void akernel_framebuffer_console_backspace(struct akernel_framebuffer_console *console);

#endif
