# akernel v1.1

akernel is a small freestanding kernel scaffold with separate 32-bit x86 and
x86-64 ELF targets. Version 1.1 adds a project site, clearer host build
instructions, and a versioned kernel startup banner.

> **Status:** This is not a bootable operating system yet. The build produces
> kernel ELF files, not a bootloader, disk image, or installable OS. A loader
> must provide the required CPU mode and memory mapping before entering `_start`.

## Build

GNU Make, GCC (with 32-bit support), GNU binutils, and NASM are required.

### Linux

On Debian or Ubuntu:

```sh
sudo apt-get update
sudo apt-get install --yes build-essential gcc-multilib nasm
make
```

`make check-x86` and `make check-x86_64` build individual targets. Outputs are
written to `build/x86/kernel.elf` and `build/x86_64/kernel.elf`.

### Windows

Use WSL 2 with an Ubuntu distribution, then follow the Linux instructions inside
the WSL terminal. The kernel uses ELF cross-compilation tools and is not a
native Windows executable.

### macOS

Install GNU Make, NASM, and ELF-targeting GCC/binutils cross-toolchains for
i686 and x86_64 using your preferred package manager. Apple's default compiler
and linker produce Mach-O binaries, so they cannot directly create these ELF
kernel targets. If GNU Make is installed as `gmake`, use `gmake` in the
commands below.

Pass the cross-tool names to Make, for example:

```sh
gmake CC_X86=i686-elf-gcc LD_X86=i686-elf-ld \
      CC_X86_64=x86_64-elf-gcc LD_X86_64=x86_64-elf-ld
```

The x86 and x86-64 toolchains must also provide NASM and support the ELF
emulations selected in the Makefile.

## Website and community forum

The community forum and handbook are in [`web/`](web/). LESS sources are
compiled into browser-ready CSS with `npm ci` and `npm run build` from `web/`.
GitHub Pages runs this build before deployment. The forum uses the cookie-backed
JSON API in [`api/server.py`](api/server.py) for anonymous sessions, discussions,
and replies. Run the API and local site together with Python 3:

```sh
python api/server.py
```

Set `AKERNEL_API_PORT` or `AKERNEL_DATABASE` to change the API port or SQLite
database path. GitHub Pages serves only the static site, so the API must be
hosted separately or placed behind a same-origin reverse proxy. Set
`AKERNEL_ALLOWED_ORIGIN` to the exact site origin. For a cross-site HTTPS API,
also set `AKERNEL_COOKIE_SAMESITE=None` and `AKERNEL_COOKIE_SECURE=1`. The
browser keeps its anonymous identity in a persistent HttpOnly cookie; clearing
cookies loses access to that guest identity. See the [forum API guide](web/docs/#forum-api)
for deployment details.

GitHub Pages deployment is configured in
`.github/workflows/pages.yml`; enable **Settings → Pages → Build and deployment
→ GitHub Actions** in the repository to publish the static site.

## Beta release 1.1.109403

Versioned beta notes and the artifact manifest are in
[`releases/v1.1.109403/`](releases/v1.1.109403/). This beta has no ISO: the
project has no bootloader, boot protocol, or disk-image target yet. Do not
replace a Linux installation or try to boot these ELF outputs. The recommended
test is to build them inside a disposable Ubuntu virtual machine; see the
[step-by-step build guides for each host OS](web/docs/steps/).

The `pkg/akernel/` tree contains package metadata, a text system-information
renderer (`fetch`, output through a caller-provided writer), and a system
interface stub; it is not an installer or complete package manager. See the
[package status guide](web/docs/#packages) before using those headers.

## Fork, contribute, and publish

1. Fork `oysterzarskyy/akernel` on GitHub, then clone your fork:

   ```sh
   git clone https://github.com/YOUR-USERNAME/akernel.git
   cd akernel
   ```

2. Create a branch, make a change, and run the build checks:

   ```sh
   git switch -c my-change
   make
   git add path/to/changed-files
   git commit -m "Describe the change"
   git push -u origin my-change
   ```

3. Open a pull request from your branch to `oysterzarskyy/akernel`.

To publish a source release, push a version tag and create a GitHub Release for
that tag:

```sh
git tag v(whatever you are work)
git push origin v1.1
```

Attach the built ELF files if you want to distribute the compiled scaffold.
They are not bootable OS images; do not advertise them as an installable
operating system. A future bootloader, image format, and boot/test workflow are
needed before an OS image can be published.

## Boot status

The x86 and x86-64 entry points initialize a stack and call `kernel_main`. A
loader must load the matching ELF, provide a valid 32-bit protected-mode or
64-bit long-mode environment, and jump to `_start`. The sample kernel writes
to VGA text memory at `0xB8000`, so a compatible text-mode mapping is also
required. Mode switching, disk loading, and GRUB configuration are not
included.

## Package and target compatibility

`include/akPackage.h` exposes the kernel package version and build-target
metadata. Package code can check `AKPACKAGE_OS_BUILD_COMPATIBLE` to ensure it is
being compiled for a supported x86 or x86-64 freestanding ELF target. The
package descriptors under `pkg/akernel/` can additionally declare a minimum
kernel version and supported architecture mask.

These checks describe compiler/build compatibility only. The project does not
define a boot protocol or produce a bootable image; a compatible external
loader must provide the CPU mode and memory mappings described above.

## License

GNU General Public License v3.0 or any later version. See [LICENSE](LICENSE)
for the SPDX identifier and link to the canonical license text.
