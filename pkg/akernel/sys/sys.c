#include "sys.h"

static const akpkg_descriptor akpkg_sys_descriptor = {
    .name = "sys",
    .package_version = {1u, 0u, 0u},
    .version_string = "1.0.0",
    .summary = "kernel system package stub",
    .minimum_kernel_version = {
        AKPKG_SYS_MIN_KERNEL_MAJOR,
        AKPKG_SYS_MIN_KERNEL_MINOR,
        AKPKG_SYS_MIN_KERNEL_PATCH
    },
    .supported_architectures = AKPKG_SYS_SUPPORTED_ARCHITECTURES,
    .flags = AKPKG_FLAG_FREESTANDING_ONLY
};

const akpkg_descriptor *akpkg_sys_get_descriptor(void)
{
    return &akpkg_sys_descriptor;
}

int akpkg_sys_is_compatible_with_kernel(void)
{
    return akpkg_is_compatible(&akpkg_sys_descriptor);
}
