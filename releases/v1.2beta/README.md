# akernel beta v1.2

This directory records the v1.2 beta status and intended build artifacts.
The numeric kernel version is `1.2.0-beta`; the release tag is `v1.2beta`.

## Image status

The repository can build an x86-64 Limine BIOS/UEFI ISO with `make iso`.
The generated file is `build/akernel-v1.2beta-x86_64.iso`. No prebuilt ISO
is published with this source snapshot; see `manifest.json` for artifact
status. The image is for disposable VM testing only, not installation.

The build downloads the pinned Limine v12.9.4 binary package and verifies its
SHA-256 before use. The legacy x86 and x86-64 ELF outputs are not bootable
images. Do not write the ISO or ELF files to a physical disk or replace an
existing operating system.

## Build and test

Use a disposable Ubuntu 24.04 LTS virtual machine. Do not attach a physical
host disk. In the guest, install the build tools and compile:

```sh
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils nasm git file curl xorriso qemu-system-x86
git clone https://github.com/oysterzarskyy/akernel.git
cd akernel
git fetch --tags
git checkout v1.2beta
make
make iso
make run-qemu
file build/x86/kernel.elf build/x86_64/kernel.elf build/akernel-v1.2beta-x86_64.iso
```

`make run-qemu` boots the ISO as virtual CD media. The Limine display uses the
framebuffer to show the akernel fetch information and ASCII art. This is a
kernel scaffold beta, not a complete operating system or installer.

## Package status

Package descriptors and fetch/system interfaces remain scaffolding. This beta
does not provide a package repository, installer, dependency resolver, or
stable package ABI.
