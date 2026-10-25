# akernel

A small freestanding kernel source scaffold with separate 32-bit x86 and
x86-64 entry points.

## Build checks

On Linux, install GCC with multilib support, GNU binutils, NASM, and Make, then
run:

```sh
make
```

The `check-x86` and `check-x86_64` targets compile and link separate ELF
artifacts under `build/`. The GitHub Actions workflow runs both checks and
prints the GCC and NASM versions used.

## Boot status

These files are kernel entry-point scaffolds, not complete bootloaders or
bootable disk images. A loader must load the matching ELF, provide a valid
32-bit protected-mode or 64-bit long-mode environment, and jump to `_start`.
The sample kernel writes to VGA text memory at `0xB8000`, so a compatible
text-mode mapping is also required. No mode switch, disk loading, or GRUB
configuration is included yet.

## License

GNU General Public License v3.0 or any later version. See [LICENSE](LICENSE)
for the SPDX identifier and link to the canonical license text.