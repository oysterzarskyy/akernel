#include "akernel/boot/limine.h"
#include "akernel/console/framebuffer.h"
#include "akernel/core/halt.h"
#include "aash.h"
#include "fetch.h"
#include "include/setup.h"

void kmain(void)
{
    struct akernel_framebuffer_info framebuffer;
    struct akernel_framebuffer_console console;

    if (akernel_limine_get_framebuffer(&framebuffer) != 0 ||
        akernel_framebuffer_console_init(&console, &framebuffer) != 0 ||
        akernel_framebuffer_console_set_color(&console, "magenta", 7u) != 0 ||
        akpkg_fetch_render(akernel_framebuffer_console_write, &console) != 0) {
        akernel_halt_forever();
    }
     // this doesnt need to be in the release,
     //if (akaash_run(akernel_setup_boot_script(), &console) != 0) {
    //    akernel_halt_forever();
   // }

    //akernel_halt_forever();
}
