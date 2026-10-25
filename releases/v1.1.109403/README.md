# akernel beta 1.1.109403

This directory records the beta release status and intended artifacts.

## Image status

**No ISO is available for this version.** The repository builds x86 and x86-64
ELF kernel files, but it does not provide a bootloader, boot protocol, disk
image target, or the CPU-mode and memory setup required to start the kernel.
The ISO filename in `manifest.json` is a planned name only; it is not a file
and must not be treated as a download.

Do not replace a Linux installation, write an akernel artifact to a disk, or
try to boot this ELF with a generic VM kernel option. Use the disposable Linux
VM workflow below to build and inspect the outputs safely.

## Recommended beta test: build inside an Ubuntu VM

Create a fresh Ubuntu 24.04 LTS virtual machine in VirtualBox, VMware, or an
equivalent desktop hypervisor. Allocate at least 2 CPU cores, 2 GB RAM, and a
20 GB virtual disk. Install Ubuntu only onto that virtual disk, do not attach a
physical host disk, and take a VM snapshot before testing.

Run these commands in the Ubuntu guest:

```sh
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils nasm git
git clone https://github.com/oysterzarskyy/akernel.git
cd akernel
make
file build/x86/kernel.elf build/x86_64/kernel.elf
readelf -h build/x86/kernel.elf
readelf -h build/x86_64/kernel.elf
```

The repository currently has no published version tag, so this builds the
default branch. After the version tag is published, use `git fetch --tags` and
`git checkout v1.1.109403` before `make` to test that exact source snapshot.
Successful ELF inspection verifies the build format, not bootability. The VM
remains a Linux guest; this beta does not replace its operating system.

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
