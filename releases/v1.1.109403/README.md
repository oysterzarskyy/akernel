# akernel beta 1.1.109403

This directory records the beta release status and intended artifacts.

## Image status

The repository includes an opt-in x86-64 Limine boot path and can build
`build/akernel-1.1.109403-x86_64.iso` locally with `make iso`. This source
release does not include a prebuilt ISO; the filename in `manifest.json` is a
build output path, not a download link. The image is intended for disposable
VM testing, not installation.

The ISO target downloads the pinned Limine v12.9.4 binary package, verifies its
SHA-256 before extraction, and creates BIOS/UEFI boot media. Only the x86-64
Limine target is bootable; the separate legacy x86 and x86-64 ELF files are not.
Do not write the ISO or ELF files to a physical disk or replace a Linux
installation.

## Recommended beta test: build inside an Ubuntu VM

Create a fresh Ubuntu 24.04 LTS virtual machine in VirtualBox, VMware, or an
equivalent desktop hypervisor. Allocate at least 2 CPU cores, 2 GB RAM, and a
20 GB virtual disk. Install Ubuntu only onto that virtual disk, do not attach a
physical host disk, and take a VM snapshot before testing.

Run these commands in the Ubuntu guest:

```sh
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils nasm git file curl xorriso qemu-system-x86
git clone https://github.com/oysterzarskyy/akernel.git
cd akernel
make
make iso
make run-qemu
file build/x86/kernel.elf build/x86_64/kernel.elf
readelf -h build/x86/kernel.elf
readelf -h build/x86_64/kernel.elf
```

The repository currently has no published version tag, so this builds the
default branch. After the version tag is published, use `git fetch --tags` and
`git checkout v1.1.109403` before building to test that exact source snapshot.
`make run-qemu` starts an isolated VM that boots the generated ISO as a virtual
CD. The Limine boot display uses the framebuffer to show the akernel fetch
information and compact ASCII art. The guest and host operating systems remain
unchanged.

## Package status

- `pkg/akernel/package.h` defines common package descriptors and compatibility
  checks.
- `pkg/akernel/fetch/` renders system/build information next to compact ASCII
  artwork through a caller-provided writer; it is not a network download
  utility.
- `pkg/akernel/sys/` contains system package metadata only.
- `make` builds the two kernel ELF targets; it does not compile or install
  packages.
- There is no package repository, installer, dependency resolver, or stable
  package ABI in this beta.

Package compatibility metadata is not a promise that a package is installable
or provides a working service.
