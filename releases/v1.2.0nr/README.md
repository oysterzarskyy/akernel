# akernel nightly 1.2.0nr

This directory records the current nightly source snapshot. Its numeric kernel
version is `1.2.0nr`. A nightly is not a stable release and does not have a
release tag or a published ISO.

## ISO image

The x86-64 Limine BIOS/UEFI test image and ZIP archive can be built with GCC:

```sh
make CC=gcc iso-zip
```

The output is `build/akernel-v1.2.0nr-x86_64.iso`. The Makefile downloads the
pinned Limine v12.9.4 binary package and verifies its SHA-256 before building
the image. The ZIP archive and its checksum are written alongside the ISO.
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
