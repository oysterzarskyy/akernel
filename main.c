
#include "kernel.h"
#include "errCheck.h"
#include "version.h"

static const char *check_status(void)
{
    return "scaffold";
}

void kernel_main(void)
{
    static const char message[] = "akernel v" AKERNEL_VERSION;
    static const char status_message[] = "Kernel status: ";
    volatile unsigned short *const video_memory =
        (volatile unsigned short *)0xB8000;
    const char *status = check_status();

    for (unsigned int i = 0; message[i] != '\0'; ++i) {
        video_memory[i] =
            (unsigned short)(0x0F00u | (unsigned char)message[i]);
    }

    for (unsigned int i = 0; status_message[i] != '\0'; ++i) {
        video_memory[80u + i] =
            (unsigned short)(0x0F00u | (unsigned char)status_message[i]);
    }

    for (unsigned int i = 0; status[i] != '\0'; ++i) {
        video_memory[80u + sizeof(status_message) - 1u + i] =
            (unsigned short)(0x0F00u | (unsigned char)status[i]);
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}