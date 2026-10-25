#ifndef AKPKG_SYS_H
#define AKPKG_SYS_H

#include "../package.h"

#define AKPKG_SYS_MIN_KERNEL_MAJOR 1u
#define AKPKG_SYS_MIN_KERNEL_MINOR 1u
#define AKPKG_SYS_MIN_KERNEL_PATCH 0u
#define AKPKG_SYS_SUPPORTED_ARCHITECTURES AKPACKAGE_ARCH_SUPPORTED

const akpkg_descriptor *akpkg_sys_get_descriptor(void);
int akpkg_sys_is_compatible_with_kernel(void);

#endif
