#ifndef AKPKG_SETUP_H
#define AKPKG_SETUP_H

#include "akernel/console/framebuffer.h"

int akernel_setup_render(struct akernel_framebuffer_console *console);
const char *akernel_setup_boot_script(void);

#endif