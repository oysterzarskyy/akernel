#include <stddef.h>
#include <stdint.h>

#include "limine.h"
#include "akernel/boot/limine.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

int akernel_limine_get_framebuffer(
    struct akernel_framebuffer_info *output)
{
    struct limine_framebuffer *framebuffer;
    uint64_t width_bytes;

    if (output == NULL ||
        !LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) ||
        framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count == 0 ||
        framebuffer_request.response->framebuffers == NULL) {
        return -1;
    }

    framebuffer = framebuffer_request.response->framebuffers[0];
    if (framebuffer == NULL ||
        framebuffer->address == NULL ||
        framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB ||
        framebuffer->bpp != 32 ||
        framebuffer->width == 0 ||
        framebuffer->height == 0 ||
        framebuffer->width > UINT64_MAX / 4u ||
        framebuffer->pitch % 4u != 0u ||
        framebuffer->red_mask_size == 0 ||
        framebuffer->green_mask_size == 0 ||
        framebuffer->blue_mask_size == 0 ||
        framebuffer->red_mask_size > 8 ||
        framebuffer->green_mask_size > 8 ||
        framebuffer->blue_mask_size > 8 ||
        (unsigned int)framebuffer->red_mask_shift +
                framebuffer->red_mask_size > 32u ||
        (unsigned int)framebuffer->green_mask_shift +
                framebuffer->green_mask_size > 32u ||
        (unsigned int)framebuffer->blue_mask_shift +
                framebuffer->blue_mask_size > 32u) {
        return -1;
    }

    width_bytes = framebuffer->width * 4u;
    if (framebuffer->pitch < width_bytes ||
        framebuffer->height >
            UINT64_MAX / (framebuffer->pitch / 4u)) {
        return -1;
    }

    output->address = framebuffer->address;
    output->width = framebuffer->width;
    output->height = framebuffer->height;
    output->pitch_pixels = framebuffer->pitch / 4u;
    output->red_mask_size = framebuffer->red_mask_size;
    output->red_mask_shift = framebuffer->red_mask_shift;
    output->green_mask_size = framebuffer->green_mask_size;
    output->green_mask_shift = framebuffer->green_mask_shift;
    output->blue_mask_size = framebuffer->blue_mask_size;
    output->blue_mask_shift = framebuffer->blue_mask_shift;

    return 0;
}
