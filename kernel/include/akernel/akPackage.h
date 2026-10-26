#ifndef AKERNEL_AKPACKAGE_H
#define AKERNEL_AKPACKAGE_H

#include "version.h"

#define AKPACKAGE_NAME "akernel"
#define AKPACKAGE_VERSION_MAJOR AKERNEL_VERSION_MAJOR
#define AKPACKAGE_VERSION_MINOR AKERNEL_VERSION_MINOR
#define AKPACKAGE_VERSION_PATCH AKERNEL_VERSION_PATCH
#define AKPACKAGE_VERSION_STRING AKERNEL_VERSION_STRING
#define AKPACKAGE_VERSION AKERNEL_VERSION
#define AKPACKAGE_VERSION_NAME AKERNEL_VERSION_NAME

#define AKPACKAGE_ARCH_X86 0x01u
#define AKPACKAGE_ARCH_X86_64 0x02u
#define AKPACKAGE_ARCH_SUPPORTED \
    (AKPACKAGE_ARCH_X86 | AKPACKAGE_ARCH_X86_64)

#if defined(__x86_64__) || defined(_M_X64)
#define AKPACKAGE_TARGET_ARCH AKPACKAGE_ARCH_X86_64
#define AKPACKAGE_TARGET_POINTER_BITS 64u
#define AKPACKAGE_TARGET_ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
#define AKPACKAGE_TARGET_ARCH AKPACKAGE_ARCH_X86
#define AKPACKAGE_TARGET_POINTER_BITS 32u
#define AKPACKAGE_TARGET_ARCH_NAME "x86"
#else
#define AKPACKAGE_TARGET_ARCH 0u
#define AKPACKAGE_TARGET_POINTER_BITS 0u
#define AKPACKAGE_TARGET_ARCH_NAME "unsupported"
#endif

#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0)
#define AKPACKAGE_TARGET_FREESTANDING 1u
#else
#define AKPACKAGE_TARGET_FREESTANDING 0u
#endif

#if defined(__ELF__)
#define AKPACKAGE_TARGET_ELF 1u
#else
#define AKPACKAGE_TARGET_ELF 0u
#endif

#define AKPACKAGE_TARGET_IS_SUPPORTED \
    ((AKPACKAGE_TARGET_ARCH != 0u) && \
     (AKPACKAGE_TARGET_FREESTANDING != 0u) && \
     (AKPACKAGE_TARGET_ELF != 0u))
#define AKPACKAGE_OS_BUILD_COMPATIBLE AKPACKAGE_TARGET_IS_SUPPORTED
#define AKPACKAGE_IS_BOOTABLE_IMAGE 0u
#define AKPACKAGE_REQUIRES_EXTERNAL_BOOTLOADER 1u
#ifndef AKPACKAGE_BOOT_PROTOCOL
#define AKPACKAGE_BOOT_PROTOCOL "loader-defined"
#endif
#define AKPACKAGE_OUTPUT_FORMAT "ELF"
#define AKPACKAGE_OS_STATUS "freestanding kernel scaffold"

#define AKPACKAGE_IS_AT_LEAST(major, minor, patch) \
    (((AKPACKAGE_VERSION_MAJOR) > (major)) || \
     ((AKPACKAGE_VERSION_MAJOR) == (major) && \
      (AKPACKAGE_VERSION_MINOR) > (minor)) || \
     ((AKPACKAGE_VERSION_MAJOR) == (major) && \
      (AKPACKAGE_VERSION_MINOR) == (minor) && \
      (AKPACKAGE_VERSION_PATCH) >= (patch)))

typedef struct akpackage_version {
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
} akpackage_version;

typedef struct akpackage_info {
    const char *name;
    akpackage_version version;
    const char *version_string;
    unsigned int supported_architectures;
    unsigned int target_architecture;
    const char *target_architecture_name;
    unsigned int target_pointer_bits;
    unsigned int target_freestanding;
    unsigned int target_elf;
    const char *output_format;
    const char *boot_protocol;
    const char *os_status;
    unsigned int requires_external_bootloader;
    unsigned int is_bootable_image;
} akpackage_info;

static inline int akpackage_version_at_least(unsigned int major,
                                             unsigned int minor,
                                             unsigned int patch)
{
    if (AKPACKAGE_VERSION_MAJOR != major) {
        return AKPACKAGE_VERSION_MAJOR > major;
    }

    if (AKPACKAGE_VERSION_MINOR != minor) {
        return AKPACKAGE_VERSION_MINOR > minor;
    }

    return AKPACKAGE_VERSION_PATCH >= patch;
}

static inline akpackage_info akpackage_get_info(void)
{
    akpackage_info info;

    info.name = AKPACKAGE_NAME;
    info.version.major = AKPACKAGE_VERSION_MAJOR;
    info.version.minor = AKPACKAGE_VERSION_MINOR;
    info.version.patch = AKPACKAGE_VERSION_PATCH;
    info.version_string = AKPACKAGE_VERSION_STRING;
    info.supported_architectures = AKPACKAGE_ARCH_SUPPORTED;
    info.target_architecture = AKPACKAGE_TARGET_ARCH;
    info.target_architecture_name = AKPACKAGE_TARGET_ARCH_NAME;
    info.target_pointer_bits = AKPACKAGE_TARGET_POINTER_BITS;
    info.target_freestanding = AKPACKAGE_TARGET_FREESTANDING;
    info.target_elf = AKPACKAGE_TARGET_ELF;
    info.output_format = AKPACKAGE_OUTPUT_FORMAT;
    info.boot_protocol = AKPACKAGE_BOOT_PROTOCOL;
    info.os_status = AKPACKAGE_OS_STATUS;
    info.requires_external_bootloader = AKPACKAGE_REQUIRES_EXTERNAL_BOOTLOADER;
    info.is_bootable_image = AKPACKAGE_IS_BOOTABLE_IMAGE;

    return info;
}

#endif
