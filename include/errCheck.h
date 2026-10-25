#ifndef AK_ERRCHECK_H
#define AK_ERRCHECK_H

/*
 * Central error catalog for akernel.
 * Keep all numeric identifiers and string forms in one place so they can be
 * referenced consistently across the kernel and any userland-facing tools.
 */

#define AK_ERR_NONE 0x0000u
#define AK_ERR_SUCCESS AK_ERR_NONE

#define AK_ERR_BOOTLOADER_FAILED 0x101au
#define AK_ERR_BOOTLOADER_FAILED_STRING "e.x101a"
#define AK_ERR_BOOTLOADER_FAILED_MESSAGE "bootloader failed"

#define AK_ERR_INVALID_ARGUMENT 0x2001u
#define AK_ERR_INVALID_ARGUMENT_STRING "e.x2001"
#define AK_ERR_INVALID_ARGUMENT_MESSAGE "invalid argument"

#define AK_ERR_NO_MEMORY 0x2002u
#define AK_ERR_NO_MEMORY_STRING "e.x2002"
#define AK_ERR_NO_MEMORY_MESSAGE "out of memory"

#define AK_ERR_UNSUPPORTED_CPU 0x3001u
#define AK_ERR_UNSUPPORTED_CPU_STRING "e.x3001"
#define AK_ERR_UNSUPPORTED_CPU_MESSAGE "unsupported CPU mode"

#define AK_ERR_UNINITIALIZED 0x4001u
#define AK_ERR_UNINITIALIZED_STRING "e.x4001"
#define AK_ERR_UNINITIALIZED_MESSAGE "uninitialized subsystem"

#endif