#ifndef AKPKG_AASH_H
#define AKPKG_AASH_H

#include "akernel/console/framebuffer.h"

#define AKASH_VERSION "0.1.0-nightly"

int akaash_run(const char *script,
               struct akernel_framebuffer_console *console);

#endif
