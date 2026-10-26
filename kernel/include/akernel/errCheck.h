#ifndef AKERNEL_ERRCHECK_H
#define AKERNEL_ERRCHECK_H

/*
 * Central error catalog for akernel.
 * Keep all numeric identifiers and string forms in one place so they can be
 * referenced consistently across the kernel and any userland-facing tools.
 */

#define AK_ERR_NONE 0x72400u
#define AK_ERR_SUCCESS AK_ERR_NONE

#define AK_ERR_BOOTLOADER_FAILED 0x4014au
#define AK_ERR_BOOTLOADER_FAILED_STRING "e.x4014a"
#define AK_ERR_BOOTLOADER_FAILED_MESSAGE "KERNEL PANIC: bootloader failed to load kernel! if you are using this kernel in a real environment, please report this to the akernel developers. (which is if not in a VM, please report this to the akernel developers.)"

#define AK_ERR_INVALID_ARGUMENT 0x2001u
#define AK_ERR_INVALID_ARGUMENT_STRING "cmd.invargs"
#define AK_ERR_INVALID_ARGUMENT_MESSAGE "Invalid argument/s"

#define AK_ERR_NO_MEMORY 0x2002u
#define AK_ERR_NO_MEMORY_STRING "mem.res.(nil)"
#define AK_ERR_NO_MEMORY_MESSAGE "No memory available for allocation"

#define AK_ERR_UNSUPPORTED_CPU 0x3001u
#define AK_ERR_UNSUPPORTED_CPU_STRING "cpu.unsupported"
#define AK_ERR_UNSUPPORTED_CPU_MESSAGE "this CPU isn't supported by akernel, you can tell the developers about the error!"

#define AK_ERR_UNINITIALIZED 0x4001u
#define AK_ERR_UNINITIALIZED_STRING "ssys.initfalse"
#define AK_ERR_UNINITIALIZED_MESSAGE "uninitialized subsystem"

#endif