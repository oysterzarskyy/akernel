#!/usr/bin/env bash
# ==============================================================================
# akernel QEMU Launcher (scripts/qemu-run.sh)
# Builds the kernel (via build.sh) and then launches it in QEMU.
# ==============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# Default configurations (matching Makefile)
ISO_IMAGE="build/akernel-v1.2.1.067-x86_64.iso"
QEMU="${QEMU:-qemu-system-x86_64}"
MEMORY="2G"
BOOT_MODE="bios"
GDB_DEBUG=0
ENABLE_KVM=0
SERIAL_CONSOLE=0
EXTRA_QEMU_ARGS=()
TARGET_TYPE="iso"
KERNEL_ELF=""
DO_BUILD=1
BUILD_TARGET="iso"
BUILD_ARGS=()

# Color formatting
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    C_RESET="\033[0m"
    C_BOLD="\033[1m"
    C_GREEN="\033[32m"
    C_YELLOW="\033[33m"
    C_BLUE="\033[34m"
    C_CYAN="\033[36m"
    C_RED="\033[31m"
else
    C_RESET=""
    C_BOLD=""
    C_GREEN=""
    C_YELLOW=""
    C_BLUE=""
    C_CYAN=""
    C_RED=""
fi

log_info()  { echo -e "${C_BLUE}[INFO]${C_RESET} $*"; }
log_step()  { echo -e "${C_CYAN}==>${C_RESET} ${C_BOLD}$*${C_RESET}"; }
log_warn()  { echo -e "${C_YELLOW}[WARN]${C_RESET} $*"; }
log_error() { echo -e "${C_RED}[ERROR]${C_RESET} $*" >&2; }

print_usage() {
    cat << EOF
Usage:
  ./scripts/qemu-run.sh [options] [-- extra qemu args]

Default behavior:
  Automatically runs build.sh first to build the latest kernel ISO,
  then immediately launches QEMU.

Options:
  --no-build         Skip running build.sh and boot existing build artifact directly
  --direct           Pass --direct to build.sh (compile directly without GNU Make)
  --iso <path>       Specify custom ISO image to boot (default: ${ISO_IMAGE})
  --kernel <path>    Directly boot an ELF kernel binary (QEMU -kernel)
  --bios             Boot using standard BIOS (default)
  --uefi             Boot using UEFI firmware (searches OVMF / edk2)
  --mem <size>       Virtual machine RAM size (default: ${MEMORY})
  --debug, -d        Freeze CPU at reset and open GDB stub on :1234 (-s -S)
  --kvm              Enable KVM hardware acceleration (if supported by host)
  --serial           Redirect virtual serial port to stdio
  --nographic        Disable graphical display, redirect console to terminal
  -h, --help         Show this help message

Examples:
  ./scripts/qemu-run.sh                       # Build ISO then launch QEMU with BIOS
  ./scripts/qemu-run.sh --uefi                # Build ISO then launch QEMU with UEFI
  ./scripts/qemu-run.sh --no-build            # Launch QEMU immediately without rebuilding
  ./scripts/qemu-run.sh --debug               # Build and start QEMU waiting for GDB on :1234
  ./scripts/qemu-run.sh --kernel build/x86_64/kernel.elf  # Direct multiboot ELF boot
  ./scripts/qemu-run.sh --serial              # See kernel shell output in this terminal

VirtualBox (no script needed):
  1. Build the ISO:  ./scripts/build.sh iso
  2. In VirtualBox -> New VM -> Type: Other / Version: Other/Unknown (64-bit)
  3. Settings -> Storage -> add the built ISO as a virtual optical drive
  4. Settings -> System -> Firmware: BIOS  (or EFI for UEFI boot)
  5. Boot the VM - Limine menu appears, select akernel and press Enter
  Note: VirtualBox 6.1+ supports both BIOS and EFI. Either works.
EOF
}

# Parse command line options
while [ $# -gt 0 ]; do
    case "$1" in
        --no-build)
            DO_BUILD=0
            shift
            ;;
        --build|-b)
            DO_BUILD=1
            shift
            ;;
        --direct)
            BUILD_ARGS+=("--direct")
            shift
            ;;
        --iso)
            ISO_IMAGE="$2"
            TARGET_TYPE="iso"
            BUILD_TARGET="iso"
            shift 2
            ;;
        --kernel)
            KERNEL_ELF="$2"
            TARGET_TYPE="kernel"
            BUILD_TARGET="all"
            shift 2
            ;;
        --uefi)
            BOOT_MODE="uefi"
            shift
            ;;
        --bios)
            BOOT_MODE="bios"
            shift
            ;;
        --mem)
            MEMORY="$2"
            shift 2
            ;;
        -d|--debug)
            GDB_DEBUG=1
            shift
            ;;
        --kvm)
            ENABLE_KVM=1
            shift
            ;;
        --serial)
            SERIAL_CONSOLE=1
            shift
            ;;
        --nographic)
            EXTRA_QEMU_ARGS+=("-nographic")
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        --)
            shift
            EXTRA_QEMU_ARGS+=("$@")
            break
            ;;
        *)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Step 1: Build the kernel / ISO (unless --no-build was passed)
if [ "${DO_BUILD}" -eq 1 ]; then
    log_step "Step 1/2: Building kernel (${BUILD_TARGET})..."
    "${SCRIPT_DIR}/build.sh" "${BUILD_TARGET}" "${BUILD_ARGS[@]}"
else
    log_info "Skipping build step (--no-build specified)."
fi

# Step 2: Verify QEMU is installed
if ! command -v "${QEMU}" >/dev/null 2>&1; then
    log_error "QEMU executable '${QEMU}' was not found in PATH."
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get update && sudo apt-get install -y qemu-system-x86 ovmf"
    echo "  Arch Linux:    sudo pacman -S qemu-system-x86 edk2-ovmf"
    echo "  Fedora:        sudo dnf install -y qemu-system-x86 edk2-ovmf"
    echo "  macOS:         brew install qemu"
    exit 1
fi

# Verify artifact existence
if [ "${TARGET_TYPE}" = "iso" ] && [ ! -f "${ISO_IMAGE}" ]; then
    log_error "ISO image '${ISO_IMAGE}' not found."
    exit 1
elif [ "${TARGET_TYPE}" = "kernel" ] && [ ! -f "${KERNEL_ELF}" ]; then
    log_error "Kernel binary '${KERNEL_ELF}' not found."
    exit 1
fi

# Base QEMU flags (matching Makefile: $(QEMU) -m 2G -cdrom $(ISO_IMAGE) -no-reboot -no-shutdown)
CMD_ARGS=("-m" "${MEMORY}" "-no-reboot" "-no-shutdown")

if [ "${TARGET_TYPE}" = "iso" ]; then
    CMD_ARGS+=("-cdrom" "${ISO_IMAGE}")
else
    CMD_ARGS+=("-kernel" "${KERNEL_ELF}")
fi

# KVM acceleration
if [ "${ENABLE_KVM}" -eq 1 ]; then
    if [ -w /dev/kvm ]; then
        log_info "KVM hardware acceleration enabled."
        CMD_ARGS+=("-enable-kvm")
    else
        log_warn "KVM requested but /dev/kvm is not accessible. Running in software emulation mode."
    fi
fi

# UEFI firmware setup
if [ "${BOOT_MODE}" = "uefi" ]; then
    OVMF_PATH=""
    for candidate in \
        "/usr/share/OVMF/OVMF_CODE.fd" \
        "/usr/share/ovmf/OVMF.fd" \
        "/usr/share/ovmf/x64/OVMF_CODE.fd" \
        "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd" \
        "/usr/share/edk2/ovmf/OVMF_CODE.fd" \
        "/usr/share/qemu/OVMF.fd" \
        "/opt/homebrew/share/qemu/edk2-x86_64-code.fd" \
        "/usr/local/share/qemu/edk2-x86_64-code.fd"; do
        if [ -f "${candidate}" ]; then
            OVMF_PATH="${candidate}"
            break
        fi
    done

    if [ -n "${OVMF_PATH}" ]; then
        log_info "Booting with UEFI (${OVMF_PATH})..."
        CMD_ARGS+=("-bios" "${OVMF_PATH}")
    else
        log_warn "OVMF UEFI firmware file not found. Falling back to standard BIOS mode."
        log_warn "To enable UEFI, install ovmf (e.g. sudo apt install ovmf)."
    fi
else
    log_info "Booting with BIOS..."
fi

# GDB debug flags
if [ "${GDB_DEBUG}" -eq 1 ]; then
    log_info "GDB debugging enabled: CPU stopped at reset. Listening on tcp::1234"
    log_info "In another terminal, connect with: gdb build/x86_64/akernel-limine.elf -ex 'target remote :1234'"
    CMD_ARGS+=("-s" "-S")
fi

# Serial port forwarding
if [ "${SERIAL_CONSOLE}" -eq 1 ]; then
    CMD_ARGS+=("-serial" "stdio")
fi

# Append any extra flags
if [ ${#EXTRA_QEMU_ARGS[@]} -gt 0 ]; then
    CMD_ARGS+=("${EXTRA_QEMU_ARGS[@]}")
fi

log_step "Step 2/2: Launching QEMU..."
log_info "Command: ${QEMU} ${CMD_ARGS[*]}"
exec "${QEMU}" "${CMD_ARGS[@]}"
