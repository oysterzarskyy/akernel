# akernel 1.2.1 "Slim Jims"

This directory records the current versioned source snapshot. The kernel
version is `1.2.1` and its nickname is "Slim Jims". The official ISO and ZIP
are attached to the [v1.2.1 GitHub release](https://github.com/oysterzarskyy/akernel/releases/tag/v1.2.1).
The ISO was BIOS-boot-tested in QEMU 11.1.1 and reached the kernel halt loop
without guest errors.

## ISO image

On Linux or WSL, the x86-64 Limine BIOS/UEFI image and ZIP archive can be built
with GCC:

```sh
make CC=gcc iso-zip
```

For MSYS2 UCRT64, use an `x86_64-elf` GCC cross-toolchain. The UCRT64-native
GCC targets Windows PE and cannot link this ELF kernel:

```sh
make CC_X86_64=x86_64-elf-gcc LD_X86_64=x86_64-elf-ld iso-zip
```

The output is `build/akernel-v1.2.1-x86_64.iso`. The Makefile downloads the
pinned Limine v12.9.4 binary package and verifies its SHA-256 before building
the image. The ZIP archive and its checksum are written alongside the ISO.
The published ISO SHA-256 is
`520adbbdb77c8867e726255f6887a878c6add84d80a27d13a52f473f537f37ef`; the ZIP
SHA-256 is
`b70fff56a1b457c44a507a8f99c5c604970bb9c5e0b883f37bd3c0b538c1e042`.
Both hashes are also provided in the release's `SHA256SUMS.txt` asset.
Rebuilding may produce different hashes.
Test the ISO only as virtual CD media in a disposable VM. Do not write it to a
physical disk or use it as an installer.

## Build requirements

On Ubuntu or Debian, install the build tools with:

```sh
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils nasm curl xorriso zip unzip qemu-system-x86
```

The legacy ELF checks are available through `make check-x86` and
`make check-x86_64`. The ISO build and verification run automatically in the
GitHub Actions Bug checker workflow, which uploads the ISO, ZIP, and ZIP
checksum as a workflow artifact.

This remains a kernel scaffold. Package descriptors and fetch/system
interfaces are not a package repository, installer, or stable ABI.
