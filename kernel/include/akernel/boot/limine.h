#ifndef AKERNEL_BOOT_LIMINE_H
#define AKERNEL_BOOT_LIMINE_H

#include "akernel/console/framebuffer.h"

int akernel_limine_get_framebuffer(
    struct akernel_framebuffer_info *framebuffer);

#endif
