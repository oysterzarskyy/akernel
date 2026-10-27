#!/usr/bin/env bash
# ==============================================================================
# akernel Build Script
# Builds 32-bit x86 / 64-bit x86_64 ELF kernels, Limine ELF, and bootable ISOs.
# ==============================================================================

set -eo pipefail

# Determine repository root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# Version information matching Makefile / release
LIMINE_VERSION="12.9.4"
LIMINE_SHA256="847334bc4bc142de996fbd132806b89ed73ac9ab34dbc1e90cfebf1fcc68c2b1"
ISO_NAME="akernel-v1.2.1.067-x86_64.iso"
ZIP_NAME="akernel-v1.2.1.067-x86_64.zip"

# Color support
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    COLOR_RESET="\033[0m"
    COLOR_BOLD="\033[1m"
    COLOR_GREEN="\033[32m"
    COLOR_YELLOW="\033[33m"
    COLOR_BLUE="\033[34m"
    COLOR_CYAN="\033[36m"
    COLOR_RED="\033[31m"
else
    COLOR_RESET=""
    COLOR_BOLD=""
    COLOR_GREEN=""
    COLOR_YELLOW=""
    COLOR_BLUE=""
    COLOR_CYAN=""
    COLOR_RED=""
fi

log_info()    { echo -e "${COLOR_CYAN}[INFO]${COLOR_RESET} $*"; }
log_step()    { echo -e "${COLOR_BLUE}==>${COLOR_RESET} ${COLOR_BOLD}$*${COLOR_RESET}"; }
log_success() { echo -e "${COLOR_GREEN}[SUCCESS]${COLOR_RESET} $*"; }
log_warn()    { echo -e "${COLOR_YELLOW}[WARN]${COLOR_RESET} $*"; }
log_error()   { echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} $*" >&2; }

# Toolchain configuration (respect existing environment variables)
CC="${CC:-gcc}"
LD="${LD:-ld}"
NASM="${NASM:-nasm}"
ZIP="${ZIP:-zip}"
QEMU="${QEMU:-qemu-system-x86_64}"
MAKE_CMD="${MAKE:-make}"

# Compiler flags
CFLAGS=(-std=c11 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -Werror -O2 -Ikernel/include)
LIMINE_CFLAGS=("${CFLAGS[@]}" -m64 -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -DAKPACKAGE_BOOT_PROTOCOL="\"Limine\"")

print_usage() {
    cat << EOF
akernel build manager

Usage:
  ./scripts/build.sh [target] [options]

Targets:
  all             Build both x86 (32-bit) and x86_64 (64-bit) ELF kernels (default)
  x86             Build 32-bit legacy x86 kernel (build/x86/kernel.elf)
  x86_64          Build 64-bit legacy x86_64 kernel (build/x86_64/kernel.elf)
  limine          Build 64-bit Limine-protocol kernel (build/x86_64/akernel-limine.elf)
  iso             Build bootable Limine ISO image (build/${ISO_NAME})
  iso-zip         Build bootable ISO, archive ZIP and SHA256 checksum
  test            Build and run the AASH interpreter test suite
  run             Build ISO and run in QEMU virtual machine
  clean           Remove build directory and all compiled artifacts
  check           Check if required development tools and dependencies are installed

Options:
  --direct        Force direct compilation instead of using GNU Make
  --cc <compiler> Specify C compiler (default: ${CC})
  -j, --jobs <N>  Pass parallel jobs flag to Make
  -v, --verbose   Verbose output
  -h, --help      Show this help message

Environment Variables:
  CC, LD, NASM, QEMU, CC_X86, LD_X86, CC_X86_64, LD_X86_64

Examples:
  ./scripts/build.sh                # Build all default ELF targets
  ./scripts/build.sh iso            # Build bootable ISO
  ./scripts/build.sh run            # Build ISO and launch QEMU
  ./scripts/build.sh test           # Run tests
  ./scripts/build.sh clean          # Clean build artifacts
EOF
}

check_tool() {
    local tool="$1"
    local hint="$2"
    if ! command -v "${tool}" >/dev/null 2>&1; then
        log_error "Missing required tool: ${tool}"
        if [ -n "${hint}" ]; then
            log_info "Tip: ${hint}"
        fi
        return 1
    fi
    return 0
}

detect_toolchains() {
    local os
    os="$(uname -s 2>/dev/null || echo "Unknown")"

    # Auto-detect macOS cross-compilers if host gcc produces Mach-O
    if [ "${os}" = "Darwin" ]; then
        if [ -z "${CC_X86:-}" ] && command -v i686-elf-gcc >/dev/null 2>&1; then
            CC_X86="i686-elf-gcc"
        fi
        if [ -z "${LD_X86:-}" ] && command -v i686-elf-ld >/dev/null 2>&1; then
            LD_X86="i686-elf-ld"
        fi
        if [ -z "${CC_X86_64:-}" ] && command -v x86_64-elf-gcc >/dev/null 2>&1; then
            CC_X86_64="x86_64-elf-gcc"
        fi
        if [ -z "${LD_X86_64:-}" ] && command -v x86_64-elf-ld >/dev/null 2>&1; then
            LD_X86_64="x86_64-elf-ld"
        fi
    fi

    CC_X86="${CC_X86:-${CC}}"
    LD_X86="${LD_X86:-${LD}}"
    CC_X86_64="${CC_X86_64:-${CC}}"
    LD_X86_64="${LD_X86_64:-${LD}}"
}

run_check_deps() {
    log_step "Checking development tools and dependencies..."
    local ok=0
    local missing=0

    check_dep() {
        local name="$1"
        local cmd="$2"
        local purpose="$3"
        local pkg="$4"
        if command -v "${cmd}" >/dev/null 2>&1; then
            echo -e "  [${COLOR_GREEN}OK${COLOR_RESET}] ${name} (${cmd}): found $(${cmd} --version 2>&1 | head -n 1)"
            ok=$((ok + 1))
        else
            echo -e "  [${COLOR_RED}MISSING${COLOR_RESET}] ${name} (${cmd}) - needed for ${purpose} (install: ${pkg})"
            missing=$((missing + 1))
        fi
    }

    check_dep "C Compiler" "${CC}" "Compiling C kernel code and tests" "gcc or gcc-multilib"
    check_dep "Linker" "${LD}" "Linking kernel ELF binaries" "binutils"
    check_dep "NASM Assembler" "${NASM}" "Assembling x86/x86_64 entry points" "nasm"
    check_dep "GNU Make" "${MAKE_CMD}" "Build automation" "make or build-essential"
    check_dep "cURL" "curl" "Downloading Limine bootloader assets" "curl"
    check_dep "SHA-256 Checksum" "sha256sum" "Verifying downloaded bootloader checksums" "coreutils"
    check_dep "Tar" "tar" "Extracting Limine binaries" "tar"
    check_dep "Xorriso" "xorriso" "Creating bootable hybrid ISO images" "xorriso"
    check_dep "Zip" "${ZIP}" "Creating distribution archives" "zip"
    check_dep "QEMU Virtual Machine" "${QEMU}" "Emulating and running the OS" "qemu-system-x86"

    echo ""
    if [ ${missing} -eq 0 ]; then
        log_success "All tools are present! Your environment is ready to build akernel."
    else
        log_warn "${missing} tool(s) are missing. Some targets may fail to build."
        echo "Quick install commands:"
        echo "  Ubuntu/Debian: sudo apt-get update && sudo apt-get install -y build-essential gcc-multilib nasm curl xorriso zip unzip qemu-system-x86"
        echo "  Arch Linux:    sudo pacman -S base-devel nasm curl xorriso zip unzip qemu-system-x86"
        echo "  Fedora:        sudo dnf install gcc gcc-c++ nasm curl xorriso zip unzip qemu-system-x86 make"
        echo "  macOS:         brew install nasm xorriso qemu i686-elf-gcc x86_64-elf-gcc i686-elf-binutils x86_64-elf-binutils"
        echo "  Windows:       Install WSL 2 (wsl --install) or use MSYS2 with x86_64-elf toolchain."
    fi
}

# --- Direct Compilation Functions ---

direct_build_x86() {
    log_step "Building x86 (32-bit) legacy kernel (direct)..."
    mkdir -p build/x86
    "${NASM}" -f elf32 kernel/arch/x86/entry.asm -o build/x86/entry.o
    "${CC_X86}" "${CFLAGS[@]}" -m32 -c kernel/src/legacy/main.c -o build/x86/main.o
    "${CC_X86}" "${CFLAGS[@]}" -m32 -c kernel/src/core/halt.c -o build/x86/halt.o
    "${LD_X86}" -m elf_i386 -T kernel/arch/x86/linker.ld -o build/x86/kernel.elf \
        build/x86/entry.o build/x86/main.o build/x86/halt.o
    log_success "Created build/x86/kernel.elf"
}

direct_build_x86_64() {
    log_step "Building x86_64 (64-bit) legacy kernel (direct)..."
    mkdir -p build/x86_64
    "${NASM}" -f elf64 kernel/arch/x86_64/entry.asm -o build/x86_64/entry.o
    "${CC_X86_64}" "${CFLAGS[@]}" -m64 -c kernel/src/legacy/main.c -o build/x86_64/main.o
    "${CC_X86_64}" "${CFLAGS[@]}" -m64 -c kernel/src/core/halt.c -o build/x86_64/halt.o
    "${LD_X86_64}" -m elf_x86_64 -T kernel/arch/x86_64/linker.ld -o build/x86_64/kernel.elf \
        build/x86_64/entry.o build/x86_64/main.o build/x86_64/halt.o
    log_success "Created build/x86_64/kernel.elf"
}

direct_build_limine_elf() {
    log_step "Building x86_64 Limine kernel ELF (direct)..."
    mkdir -p build/x86_64
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ithird_party/limine-protocol -c kernel/src/boot/limine.c -o build/x86_64/limine.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ipkg/akernel/aash -Ipkg/akernel/fetch -Ipkg/akernel/setup -c kernel/src/kernel.c -o build/x86_64/kernel.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -c kernel/src/console/framebuffer.c -o build/x86_64/framebuffer.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -c kernel/src/core/halt.c -o build/x86_64/limine-halt.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ipkg/akernel/fetch -c pkg/akernel/fetch/fetch.c -o build/x86_64/limine-fetch.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ipkg/akernel/aash -Ipkg/akernel/setup -c pkg/akernel/aash/aash.c -o build/x86_64/limine-aash.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ipkg/akernel/setup -c pkg/akernel/setup/setup.c -o build/x86_64/limine-setup.o \
        build/x86_64/serial.o build/x86_64/keyboard.o build/x86_64/shell.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -c kernel/src/drivers/serial.c -o build/x86_64/serial.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -c kernel/src/drivers/keyboard.c -o build/x86_64/keyboard.o
    "${CC_X86_64}" "${LIMINE_CFLAGS[@]}" -Ipkg/akernel/fetch -c kernel/src/shell.c -o build/x86_64/shell.o

    "${LD_X86_64}" -m elf_x86_64 -z max-page-size=0x1000 \
        -T kernel/arch/x86_64/limine.ld -o build/x86_64/akernel-limine.elf \
        build/x86_64/kernel.o build/x86_64/limine.o \
        build/x86_64/framebuffer.o build/x86_64/limine-halt.o \
        build/x86_64/limine-fetch.o build/x86_64/limine-aash.o \
        build/x86_64/limine-setup.o
    log_success "Created build/x86_64/akernel-limine.elf"
}

fetch_limine_bootloader() {
    local limine_archive="build/limine-binary-${LIMINE_VERSION}.tar.gz"
    local limine_dir="build/limine-binary"
    local limine_url="https://github.com/Limine-Bootloader/limine/releases/download/v${LIMINE_VERSION}/limine-binary.tar.gz"

    mkdir -p build
    if [ ! -f "${limine_archive}" ]; then
        log_info "Downloading Limine bootloader v${LIMINE_VERSION}..."
        curl -fL --retry 3 -o "${limine_archive}.part" "${limine_url}"
        printf '%s  %s\n' "${LIMINE_SHA256}" "${limine_archive}.part" | sha256sum -c -
        mv "${limine_archive}.part" "${limine_archive}"
    fi

    if [ ! -f "${limine_dir}/.ready" ]; then
        log_info "Verifying and extracting Limine binaries..."
        printf '%s  %s\n' "${LIMINE_SHA256}" "${limine_archive}" | sha256sum -c -
        rm -rf "${limine_dir}"
        tar -xzf "${limine_archive}" -C build
        touch "${limine_dir}/.ready"
    fi

    if [ ! -f "${limine_dir}/limine" ]; then
        log_info "Compiling limine host installer tool..."
        if command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" -C "${limine_dir}"
        else
            ${CC} -O2 "${limine_dir}/limine.c" -o "${limine_dir}/limine"
        fi
    fi
}

direct_build_iso() {
    direct_build_limine_elf
    fetch_limine_bootloader

    local limine_dir="build/limine-binary"
    local iso_image="build/${ISO_NAME}"
    local iso_root="build/iso-root"

    log_step "Constructing bootable ISO image (${iso_image})..."
    rm -rf "${iso_root}"
    mkdir -p "${iso_root}/boot" "${iso_root}/EFI/BOOT"
    mkdir -p "${iso_root}/boot/limine"

    cp limine.conf "${iso_root}/boot/limine/limine.conf"
    cp build/x86_64/akernel-limine.elf "${iso_root}/boot/akernel.elf"
    cp "${limine_dir}/limine-bios-cd.bin" "${limine_dir}/limine-bios.sys" \
       "${limine_dir}/limine-uefi-cd.bin" "${iso_root}/boot/limine/"
    cp "${limine_dir}/LICENSE" "${limine_dir}/3RDPARTY.md" \
       "${iso_root}/boot/limine/"
    cp "${limine_dir}/BOOTX64.EFI" "${iso_root}/EFI/BOOT/"

    xorriso -as mkisofs -R -r -J \
        -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
        -hfsplus -apm-block-size 2048 \
        --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part \
        --efi-boot-image --protective-msdos-label \
        "${iso_root}" -o "${iso_image}"

    "${limine_dir}/limine" bios-install "${iso_image}"
    log_success "Bootable ISO created at ${iso_image}"
}

direct_build_iso_zip() {
    direct_build_iso
    local iso_image="build/${ISO_NAME}"
    local iso_zip="build/${ZIP_NAME}"

    log_step "Compressing ISO into ${iso_zip}..."
    rm -f "${iso_zip}"
    ${ZIP} -j -9 "${iso_zip}" "${iso_image}"
    sha256sum "${iso_zip}" > "${iso_zip}.sha256"
    log_success "Created ${iso_zip} and ${iso_zip}.sha256"
}

direct_test_aash() {
    log_step "Building and running AASH interpreter tests..."
    mkdir -p build
    ${CC} -std=c11 -Wall -Wextra -Werror -Ikernel/include \
        -Ipkg/akernel/aash -Ipkg/akernel/setup \
        tests/aash_test.c pkg/akernel/aash/aash.c -o build/test-aash
    ./build/test-aash
    log_success "All AASH unit tests passed!"
}

# --- Execution Dispatcher ---

USE_DIRECT=0
JOBS_ARG=""
VERBOSE=0
TARGET=""

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help|help)
            print_usage
            exit 0
            ;;
        --direct)
            USE_DIRECT=1
            shift
            ;;
        --cc)
            CC="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS_ARG="-j$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        all|x86|x86_64|limine|iso|iso-zip|test|run|clean|check|check-deps|deps|setup)
            TARGET="$1"
            shift
            ;;
        *)
            log_error "Unknown argument or target: $1"
            print_usage
            exit 1
            ;;
    esac
done

detect_toolchains

# Default target is "all" if none provided
TARGET="${TARGET:-all}"

START_TIME=$(date +%s)

case "${TARGET}" in
    check|check-deps)
        run_check_deps
        exit 0
        ;;
    clean)
        log_step "Cleaning build artifacts..."
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" clean
        else
            rm -rf build
        fi
        log_success "Clean completed."
        exit 0
        ;;
    test)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} test-aash
        else
            check_tool "${CC}" "Install GCC or build-essential"
            direct_test_aash
        fi
        ;;
    x86)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC_X86="${CC_X86}" LD_X86="${LD_X86}" NASM="${NASM}" check-x86
        else
            check_tool "${CC_X86}" "Install GCC with 32-bit multilib support"
            check_tool "${NASM}" "Install nasm"
            check_tool "${LD_X86}" "Install binutils"
            direct_build_x86
        fi
        ;;
    x86_64)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC_X86_64="${CC_X86_64}" LD_X86_64="${LD_X86_64}" NASM="${NASM}" check-x86_64
        else
            check_tool "${CC_X86_64}" "Install GCC"
            check_tool "${NASM}" "Install nasm"
            check_tool "${LD_X86_64}" "Install binutils"
            direct_build_x86_64
        fi
        ;;
    limine)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC_X86_64="${CC_X86_64}" LD_X86_64="${LD_X86_64}" build/x86_64/akernel-limine.elf
        else
            check_tool "${CC_X86_64}" "Install GCC"
            check_tool "${LD_X86_64}" "Install binutils"
            direct_build_limine_elf
        fi
        ;;
    iso)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC="${CC}" iso
        else
            check_tool "${CC}" "Install GCC"
            check_tool "curl" "Install curl"
            check_tool "sha256sum" "Install coreutils"
            check_tool "tar" "Install tar"
            check_tool "xorriso" "Install xorriso"
            direct_build_iso
        fi
        ;;
    iso-zip)
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC="${CC}" iso-zip
        else
            check_tool "${CC}" "Install GCC"
            check_tool "${ZIP}" "Install zip"
            direct_build_iso_zip
        fi
        ;;
    run)
        if [ ! -f "build/${ISO_NAME}" ]; then
            log_info "ISO not found. Building it first..."
            "$0" iso
        fi
        log_step "Launching QEMU with build/${ISO_NAME}..."
        check_tool "${QEMU}" "Install qemu-system-x86"
        "${QEMU}" -m 2G -cdrom "build/${ISO_NAME}" -no-reboot -no-shutdown
        ;;
    all)
        log_step "Building all default kernel targets..."
        if [ "${USE_DIRECT}" -eq 0 ] && command -v "${MAKE_CMD}" >/dev/null 2>&1; then
            "${MAKE_CMD}" ${JOBS_ARG} CC_X86="${CC_X86}" LD_X86="${LD_X86}" CC_X86_64="${CC_X86_64}" LD_X86_64="${LD_X86_64}" NASM="${NASM}" all
        else
            check_tool "${CC_X86}" "Install GCC multilib"
            check_tool "${CC_X86_64}" "Install GCC"
            check_tool "${NASM}" "Install nasm"
            check_tool "${LD_X86}" "Install binutils"
            check_tool "${LD_X86_64}" "Install binutils"
            direct_build_x86
            direct_build_x86_64
        fi
        log_success "Built targets in build/x86/ and build/x86_64/"
        ;;
    deps|setup)
        "${SCRIPT_DIR}/setup.sh"
        exit 0
        ;;
esac

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
log_success "Task completed in ${DURATION}s."
