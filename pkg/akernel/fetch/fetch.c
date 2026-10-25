#include "fetch.h"
#include "include/internals.h"

static const akpkg_descriptor akpkg_fetch_descriptor = {
    .name = "fetch",
    .package_version = {0u, 1u, 0u},
    .version_string = "0.1.0",
    .summary = "text system information display with ASCII artwork",
    .minimum_kernel_version = {
        AKPKG_FETCH_MIN_KERNEL_MAJOR,
        AKPKG_FETCH_MIN_KERNEL_MINOR,
        AKPKG_FETCH_MIN_KERNEL_PATCH
    },
    .supported_architectures = AKPKG_FETCH_SUPPORTED_ARCHITECTURES,
    .flags = AKPKG_FLAG_FREESTANDING_ONLY
};

static const char *const akpkg_fetch_art[] = {
    "                   ###-",
    "          .---##+++#+#.",
    "          .-#####--+-+-+",
    "          .-#-++#++# +-#####+-",
    "   .#######-###+## ####-+--.--",
    "   -#####-#-+--#-+#+  +#####+-",
    "   .####+##-   +#-.#.  ----",
    "    +##-.        +#+-+- + +",
    "            +#+   .--.+---++",
    "   .###    .#.#     -+--####",
    "###-#.#++--###+---   .-#.#.#.",
    "#-#-###- +-#+#.- - +#- +####.         ...",
    "###-.--++--##+---- # #   +##+##.     ###.",
    "             .+.+++.#+      ######+ -#-#.",
    "              -+-+.+        +##-###.####",
    "                .++-         +####-.#-#.",
    "                               -+- .###",
    "                                 ..."
};

static const char *const akpkg_fetch_info[] = {
    "akernel",
    "-----------------------------",
    "OS: akernel",
    "Version: " AKPACKAGE_VERSION_STRING,
    "Architecture: " AKPACKAGE_TARGET_ARCH_NAME,
    "Build: freestanding ELF",
    "Status: beta scaffold",
    "Packages: metadata and stubs",
    "Boot: external loader required"
};

static unsigned int akpkg_fetch_string_length(const char *text)
{
    unsigned int length = 0u;

    while (text[length] != '\0') {
        ++length;
    }

    return length;
}

static int akpkg_fetch_write_text(akpkg_fetch_write_fn write,
                                  const char *text,
                                  unsigned int length,
                                  void *context)
{
    if (length == 0u) {
        return 0;
    }

    return write(text, length, context);
}

const akpkg_descriptor *akpkg_fetch_get_descriptor(void)
{
    return &akpkg_fetch_descriptor;
}

int akpkg_fetch_is_compatible_with_kernel(void)
{
    return akpkg_is_compatible(&akpkg_fetch_descriptor);
}

int akpkg_fetch_render(akpkg_fetch_write_fn write, void *context)
{
    unsigned int row;
    unsigned int art_rows =
        (unsigned int)(sizeof(akpkg_fetch_art) / sizeof(akpkg_fetch_art[0]));
    unsigned int info_rows =
        (unsigned int)(sizeof(akpkg_fetch_info) / sizeof(akpkg_fetch_info[0]));

    if (write == (akpkg_fetch_write_fn)0) {
        return -1;
    }

    for (row = 0u; row < art_rows; ++row) {
        unsigned int art_length =
            akpkg_fetch_string_length(akpkg_fetch_art[row]);
        unsigned int info_length;
        unsigned int column;
        int result;

        result = akpkg_fetch_write_text(
            write, akpkg_fetch_art[row], art_length, context);
        if (result != 0) {
            return result;
        }

        for (column = art_length; column < 44u; ++column) {
            result = akpkg_fetch_write_text(write, " ", 1u, context);
            if (result != 0) {
                return result;
            }
        }

        info_length = row < info_rows
                          ? akpkg_fetch_string_length(akpkg_fetch_info[row])
                          : 0u;
        if (info_length != 0u) {
            result = akpkg_fetch_write_text(write, "  ", 2u, context);
            if (result != 0) {
                return result;
            }

            result = akpkg_fetch_write_text(
                write, akpkg_fetch_info[row], info_length, context);
            if (result != 0) {
                return result;
            }
        }

        result = akpkg_fetch_write_text(write, "\n", 1u, context);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

const char *akpkg_fetch_internal_state(void)
{
    return AKPKG_FETCH_INTERNAL_STATE;
}
