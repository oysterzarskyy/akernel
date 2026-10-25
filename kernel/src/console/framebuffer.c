#include <stddef.h>
#include <stdint.h>

#include "akernel/console/framebuffer.h"

static const uint8_t font[62][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
    {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30},
    {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17},
    {14, 4, 4, 4, 4, 4, 14}, {7, 2, 2, 2, 18, 18, 12},
    {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
    {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16},
    {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30}, {31, 4, 4, 4, 4, 4, 4},
    {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},
    {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4}, {31, 1, 2, 4, 8, 16, 31},
    {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},
    {14, 17, 1, 2, 4, 8, 31}, {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2}, {31, 16, 16, 30, 1, 1, 30},
    {14, 16, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},
    {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14},
    {0, 0, 14, 1, 15, 17, 15}, {16, 16, 30, 17, 17, 17, 30},
    {0, 0, 14, 16, 16, 17, 14}, {1, 1, 15, 17, 17, 17, 15},
    {0, 0, 14, 17, 31, 16, 14}, {6, 8, 8, 30, 8, 8, 8},
    {0, 15, 17, 17, 15, 1, 14}, {16, 16, 30, 17, 17, 17, 17},
    {4, 0, 12, 4, 4, 4, 14}, {2, 0, 6, 2, 2, 18, 12},
    {16, 16, 18, 20, 24, 20, 18}, {12, 4, 4, 4, 4, 4, 14},
    {0, 0, 26, 21, 21, 21, 21}, {0, 0, 30, 17, 17, 17, 17},
    {0, 0, 14, 17, 17, 17, 14}, {0, 30, 17, 17, 30, 16, 16},
    {0, 15, 17, 17, 15, 1, 1}, {0, 0, 22, 25, 16, 16, 16},
    {0, 0, 15, 16, 14, 1, 30}, {8, 8, 30, 8, 8, 9, 6},
    {0, 0, 17, 17, 17, 19, 13}, {0, 0, 17, 17, 17, 10, 4},
    {0, 0, 17, 17, 21, 21, 10}, {0, 0, 17, 10, 4, 10, 17},
    {0, 17, 17, 17, 15, 1, 14}, {0, 0, 31, 2, 4, 8, 31}
};

static uint32_t framebuffer_channel(uint8_t value,
                                   uint8_t mask_size,
                                   uint8_t mask_shift)
{
    uint32_t maximum = (1u << mask_size) - 1u;
    return ((uint32_t)value * maximum / 255u) << mask_shift;
}

static uint32_t framebuffer_colour(
    const struct akernel_framebuffer_info *framebuffer,
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    return framebuffer_channel(red, framebuffer->red_mask_size,
                               framebuffer->red_mask_shift) |
           framebuffer_channel(green, framebuffer->green_mask_size,
                               framebuffer->green_mask_shift) |
           framebuffer_channel(blue, framebuffer->blue_mask_size,
                               framebuffer->blue_mask_shift);
}

static int framebuffer_is_valid(
    const struct akernel_framebuffer_info *framebuffer)
{
    return framebuffer != NULL &&
           framebuffer->address != NULL &&
           framebuffer->width != 0u &&
           framebuffer->height != 0u &&
           framebuffer->pitch_pixels >= framebuffer->width &&
           framebuffer->height <= UINT64_MAX / framebuffer->pitch_pixels &&
           framebuffer->red_mask_size != 0u &&
           framebuffer->red_mask_size <= 8u &&
           framebuffer->green_mask_size != 0u &&
           framebuffer->green_mask_size <= 8u &&
           framebuffer->blue_mask_size != 0u &&
           framebuffer->blue_mask_size <= 8u &&
           (unsigned int)framebuffer->red_mask_shift +
                   framebuffer->red_mask_size <= 32u &&
           (unsigned int)framebuffer->green_mask_shift +
                   framebuffer->green_mask_size <= 32u &&
           (unsigned int)framebuffer->blue_mask_shift +
                   framebuffer->blue_mask_size <= 32u;
}

static void framebuffer_clear(struct akernel_framebuffer_console *console,
                              uint32_t colour)
{
    uint64_t y;

    for (y = 0; y < console->framebuffer.height; ++y) {
        uint64_t x;
        volatile uint32_t *row =
            console->framebuffer.address +
            y * console->framebuffer.pitch_pixels;

        for (x = 0; x < console->framebuffer.width; ++x) {
            row[x] = colour;
        }
    }
}

static void framebuffer_draw_pixel(
    struct akernel_framebuffer_console *console,
    uint64_t x,
    uint64_t y)
{
    uint32_t row;
    uint32_t column;

    for (row = 0; row < console->scale; ++row) {
        for (column = 0; column < console->scale; ++column) {
            uint64_t pixel_x = x + column;
            uint64_t pixel_y = y + row;

            if (pixel_x < console->framebuffer.width &&
                pixel_y < console->framebuffer.height) {
                console->framebuffer.address[
                    pixel_y * console->framebuffer.pitch_pixels + pixel_x] =
                    console->foreground;
            }
        }
    }
}

static const uint8_t *glyph_rows(char character)
{
    static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t exclamation[7] = {4, 4, 4, 4, 4, 0, 4};
    static const uint8_t question[7] = {14, 17, 1, 2, 4, 0, 4};
    static const uint8_t apostrophe[7] = {4, 4, 8, 0, 0, 0, 0};
    static const uint8_t quotation[7] = {10, 10, 0, 0, 0, 0, 0};
    static const uint8_t comma[7] = {0, 0, 0, 0, 4, 4, 8};
    static const uint8_t period[7] = {0, 0, 0, 0, 0, 12, 12};
    static const uint8_t colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const uint8_t semicolon[7] = {0, 4, 4, 0, 4, 4, 8};
    static const uint8_t hyphen[7] = {0, 0, 0, 31, 0, 0, 0};
    static const uint8_t plus[7] = {0, 4, 4, 31, 4, 4, 0};
    static const uint8_t hash[7] = {10, 31, 10, 10, 31, 10, 0};
    static const uint8_t slash[7] = {1, 2, 2, 4, 8, 8, 16};
    static const uint8_t backslash[7] = {16, 8, 8, 4, 2, 2, 1};
    static const uint8_t left_parenthesis[7] = {2, 4, 8, 8, 8, 4, 2};
    static const uint8_t right_parenthesis[7] = {8, 4, 2, 2, 2, 4, 8};
    static const uint8_t less_than[7] = {2, 4, 8, 16, 8, 4, 2};
    static const uint8_t greater_than[7] = {8, 4, 2, 1, 2, 4, 8};
    static const uint8_t equals[7] = {0, 31, 0, 31, 0, 0, 0};
    static const uint8_t left_bracket[7] = {14, 8, 8, 8, 8, 8, 14};
    static const uint8_t right_bracket[7] = {14, 2, 2, 2, 2, 2, 14};
    static const uint8_t caret[7] = {4, 10, 17, 0, 0, 0, 0};
    static const uint8_t underscore[7] = {0, 0, 0, 0, 0, 0, 31};
    static const uint8_t pipe[7] = {4, 4, 4, 4, 4, 4, 4};
    static const uint8_t asterisk[7] = {0, 21, 14, 31, 14, 21, 0};
    static const uint8_t dollar[7] = {4, 15, 20, 14, 5, 30, 4};
    static const uint8_t percent[7] = {24, 25, 2, 4, 8, 19, 3};
    static const uint8_t ampersand[7] = {12, 18, 20, 8, 21, 18, 13};
    static const uint8_t tilde[7] = {0, 0, 9, 22, 0, 0, 0};

    if (character >= 'A' && character <= 'Z') {
        return font[(unsigned int)(character - 'A')];
    }
    if (character >= '0' && character <= '9') {
        return font[26u + (unsigned int)(character - '0')];
    }
    if (character >= 'a' && character <= 'z') {
        return font[36u + (unsigned int)(character - 'a')];
    }

    switch (character) {
    case '!': return exclamation;
    case '?': return question;
    case '\'': return apostrophe;
    case '"': return quotation;
    case ',': return comma;
    case '.': return period;
    case ':': return colon;
    case ';': return semicolon;
    case '-': return hyphen;
    case '+': return plus;
    case '#': return hash;
    case '/': return slash;
    case '\\': return backslash;
    case '(': return left_parenthesis;
    case ')': return right_parenthesis;
    case '<': return less_than;
    case '>': return greater_than;
    case '=': return equals;
    case '[': return left_bracket;
    case ']': return right_bracket;
    case '^': return caret;
    case '_': return underscore;
    case '|': return pipe;
    case '*': return asterisk;
    case '$': return dollar;
    case '%': return percent;
    case '&': return ampersand;
    case '~': return tilde;
    default: return blank;
    }
}

int akernel_framebuffer_console_init(
    struct akernel_framebuffer_console *console,
    const struct akernel_framebuffer_info *framebuffer)
{
    if (console == NULL || !framebuffer_is_valid(framebuffer)) {
        return -1;
    }

    console->framebuffer = *framebuffer;
    console->scale = framebuffer->width >= 1000u &&
                             framebuffer->height >= 420u
                         ? 2u
                         : 1u;
    console->start_x = 24u;
    console->cursor_x = console->start_x;
    console->cursor_y = 24u;
    console->foreground =
        framebuffer_colour(framebuffer, 165u, 220u, 140u);

    framebuffer_clear(console,
                      framebuffer_colour(framebuffer, 16u, 24u, 32u));
    return 0;
}

int akernel_framebuffer_console_write(const char *text,
                                      unsigned int length,
                                      void *context)
{
    struct akernel_framebuffer_console *console = context;
    unsigned int index;

    if (text == NULL || console == NULL) {
        return -1;
    }

    for (index = 0; index < length; ++index) {
        const uint8_t *rows;
        uint32_t row;

        if (text[index] == '\n') {
            console->cursor_x = console->start_x;
            console->cursor_y += 8u * console->scale;
            continue;
        }

        rows = glyph_rows(text[index]);
        for (row = 0; row < 7u; ++row) {
            uint32_t column;

            for (column = 0; column < 5u; ++column) {
                if ((rows[row] & (1u << (4u - column))) != 0u) {
                    framebuffer_draw_pixel(
                        console,
                        console->cursor_x + column * console->scale,
                        console->cursor_y + row * console->scale);
                }
            }
        }
        console->cursor_x += 6u * console->scale;
    }

    return 0;
}
