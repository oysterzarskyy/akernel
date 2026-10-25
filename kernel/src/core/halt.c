#include "akernel/core/halt.h"

void akernel_halt_forever(void)
{
#if defined(__i386__) || defined(__x86_64__)
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
#else
#error "akernel_halt_forever requires an x86 target"
#endif
}
