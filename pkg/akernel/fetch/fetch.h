#ifndef AKPKG_FETCH_H
#define AKPKG_FETCH_H

#include "../package.h"

#define AKPKG_FETCH_MIN_KERNEL_MAJOR 1u
#define AKPKG_FETCH_MIN_KERNEL_MINOR 1u
#define AKPKG_FETCH_MIN_KERNEL_PATCH 0u
#define AKPKG_FETCH_SUPPORTED_ARCHITECTURES AKPACKAGE_ARCH_SUPPORTED

typedef int (*akpkg_fetch_write_fn)(const char *text,
                                    unsigned int length,
                                    void *context);

const akpkg_descriptor *akpkg_fetch_get_descriptor(void);
int akpkg_fetch_is_compatible_with_kernel(void);
/* A nonzero writer result stops rendering and is returned. */
int akpkg_fetch_render(akpkg_fetch_write_fn write, void *context);

#endif
