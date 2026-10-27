#!/usr/bin/env bash
# ==============================================================================
# akernel Environment Setup & Dependency Installer
# Installs toolchains and dependencies required to build akernel.
# ==============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "=========================================================="
echo " akernel Toolchain & Dependency Setup"
echo "=========================================================="

detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS_ID="${ID:-unknown}"
        OS_LIKE="${ID_LIKE:-}"
    elif [ "$(uname -s)" = "Darwin" ]; then
        OS_ID="darwin"
    elif uname -s | grep -qiE "mingw|msys|cygwin"; then
        OS_ID="windows"
    else
        OS_ID="unknown"
    fi
}

detect_os

case "${OS_ID}" in
    ubuntu|debian|linuxmint|pop)
        echo "[INFO] Detected Debian/Ubuntu derivative (${OS_ID})."
        INSTALL_CMD="sudo apt-get update && sudo apt-get install -y build-essential gcc-multilib nasm curl xorriso zip unzip qemu-system-x86 ovmf"
        ;;
    arch|manjaro|endeavouros)
        echo "[INFO] Detected Arch Linux derivative."
        INSTALL_CMD="sudo pacman -S --needed base-devel nasm curl xorriso zip unzip qemu-system-x86 edk2-ovmf"
        ;;
    fedora|rhel|centos)
        echo "[INFO] Detected Fedora/RHEL derivative."
        INSTALL_CMD="sudo dnf install -y gcc gcc-c++ glibc-devel.i686 nasm curl xorriso zip unzip qemu-system-x86 edk2-ovmf make"
        ;;
    alpine)
        echo "[INFO] Detected Alpine Linux."
        INSTALL_CMD="sudo apk add build-base nasm curl xorriso zip unzip qemu-system-x86_64 ovmf"
        ;;
    darwin)
        echo "[INFO] Detected macOS."
        INSTALL_CMD="brew install nasm xorriso qemu x86_64-elf-gcc x86_64-elf-binutils i686-elf-gcc i686-elf-binutils make"
        ;;
    windows)
        echo "[WARN] Windows host environment detected."
        echo "akernel targets ELF binaries and requires an ELF cross-toolchain or Linux environment."
        echo "Recommended steps on Windows:"
        echo "  1. Install WSL 2 by running: wsl --install"
        echo "  2. Open Ubuntu inside WSL and run: ./scripts/setup.sh --install"
        echo "  3. Build the kernel: ./scripts/build.sh all (or ./scripts/build.sh iso)"
        exit 0
        ;;
    *)
        echo "[WARN] Unrecognized distribution: ${OS_ID}"
        INSTALL_CMD=""
        ;;
esac

if [ "$1" = "--install" ] || [ "$1" = "-y" ]; then
    if [ -n "${INSTALL_CMD}" ]; then
        echo "[INFO] Running: ${INSTALL_CMD}"
        eval "${INSTALL_CMD}"
    else
        echo "[ERROR] Cannot automatically install dependencies on this platform."
        exit 1
    fi
else
    if [ -n "${INSTALL_CMD}" ]; then
        echo ""
        echo "To install all required build packages, run:"
        echo "  ${INSTALL_CMD}"
        echo ""
        echo "Or re-run this script with --install:"
        echo "  ./scripts/setup.sh --install"
    fi
fi

echo ""
echo "[INFO] Running dependency verification..."
"${SCRIPT_DIR}/build.sh" check-deps
