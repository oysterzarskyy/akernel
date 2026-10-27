#include "akernel/core/halt.h"

void akernel_halt_forever(void)
{
#if defined(__i386__) || defined(__x86_64__)
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
#else
    // Fallback for non-x86 targets or test environments
    for (;;) {
        // Infinite loop fallback
    }
#endif
}