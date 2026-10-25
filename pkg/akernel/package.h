#ifndef AKERNEL_PACKAGE_H
#define AKERNEL_PACKAGE_H

#include "../../kernel/include/akernel/akPackage.h"

typedef struct akpkg_descriptor {
    const char *name;
    akpackage_version package_version;
    const char *version_string;
    const char *summary;
    akpackage_version minimum_kernel_version;
    unsigned int supported_architectures;
    unsigned int flags;
} akpkg_descriptor;

#define AKPKG_FLAG_FREESTANDING_ONLY 0x01u

static inline int akpkg_version_at_least(akpackage_version actual,
                                         akpackage_version required)
{
    if (actual.major != required.major) {
        return actual.major > required.major;
    }

    if (actual.minor != required.minor) {
        return actual.minor > required.minor;
    }

    return actual.patch >= required.patch;
}

static inline int akpkg_is_compatible(const akpkg_descriptor *descriptor)
{
    if (descriptor == (const akpkg_descriptor *)0 ||
        (descriptor->supported_architectures & AKPACKAGE_TARGET_ARCH) == 0u ||
        AKPACKAGE_TARGET_ELF == 0u ||
        !akpkg_version_at_least(
            (akpackage_version){AKERNEL_VERSION_MAJOR,
                                AKERNEL_VERSION_MINOR,
                                AKERNEL_VERSION_PATCH},
            descriptor->minimum_kernel_version)) {
        return 0;
    }

    if ((descriptor->flags & AKPKG_FLAG_FREESTANDING_ONLY) != 0u &&
        AKPACKAGE_TARGET_FREESTANDING == 0u) {
        return 0;
    }

    return 1;
}

static inline akpkg_descriptor akpkg_kernel_descriptor(void)
{
    akpkg_descriptor descriptor;

    descriptor.name = AKPACKAGE_NAME;
    descriptor.package_version.major = AKPACKAGE_VERSION_MAJOR;
    descriptor.package_version.minor = AKPACKAGE_VERSION_MINOR;
    descriptor.package_version.patch = AKPACKAGE_VERSION_PATCH;
    descriptor.version_string = AKPACKAGE_VERSION_STRING;
    descriptor.summary = "core kernel package metadata";
    descriptor.minimum_kernel_version.major = AKPACKAGE_VERSION_MAJOR;
    descriptor.minimum_kernel_version.minor = AKPACKAGE_VERSION_MINOR;
    descriptor.minimum_kernel_version.patch = AKPACKAGE_VERSION_PATCH;
    descriptor.supported_architectures = AKPACKAGE_ARCH_SUPPORTED;
    descriptor.flags = AKPKG_FLAG_FREESTANDING_ONLY;

    return descriptor;
}

#endif
