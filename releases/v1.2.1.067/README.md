# akernel 1.2.1.067 nightly — AASH preview

This is an experimental prerelease based on stable akernel 1.2.1 "Slim Jims".
It is not a stable release or a complete operating system. Download the
[nightly prerelease](https://github.com/oysterzarskyy/akernel/releases/tag/v1.2.1.067)
and verify the ISO and ZIP with its `SHA256SUMS.txt` asset.

## Experimental features

- Colored framebuffer text and ASCII output.
- AASH 0.1, a small freestanding command interpreter implemented in C.
- A built-in AASH boot script that displays version/help text and launches a
  non-destructive setup preflight screen.

AASH currently supports one command per line: `echo`, `color`, `setup`,
`version`, and `help`, plus full-line `#` comments. Scripts are embedded in the
kernel; there is no keyboard input or filesystem from which to load scripts.
It is Bash-like in spirit, but not Bash-, Zsh-, or Fish-compatible.

The setup screen only reports the current boot/framebuffer state. It does not
partition disks, write files, install an OS, or change persistent storage.
Keyboard input, storage drivers, and filesystems are not implemented.

## Build and test

On Linux or WSL:

```sh
make CC=gcc iso-zip
make test-aash
make run-qemu
```

MSYS2 UCRT64 requires the ELF-targeting cross compiler rather than native
UCRT64 GCC:

```sh
make CC_X86_64=x86_64-elf-gcc LD_X86_64=x86_64-elf-ld iso-zip
make test-aash
```

The ISO is for disposable VM testing only. Do not write it to a physical disk
or use it as an installer.

The built ISO was tested with QEMU 11.1.1 using BIOS firmware. It reached the
kernel halt loop with no guest errors; the framebuffer showed the experimental
magenta, cyan, yellow, and green text colors. UEFI boot has not been tested for
this nightly.
