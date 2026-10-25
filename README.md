# akernel v1.1

akernel is a small freestanding kernel scaffold with separate 32-bit x86 and
x86-64 ELF targets. It also has an opt-in x86-64 bootable ISO target using the
Limine boot protocol.

> **Status:** `make iso` creates an x86-64 Limine boot image for VM testing.
> This remains a kernel scaffold, not a complete operating system or installer.
> The existing x86 and x86-64 ELF targets are separate legacy outputs.

## Build

GNU Make, GCC (with 32-bit support), GNU binutils, and NASM are required for
the legacy ELF targets. The optional ISO target additionally requires `curl`,
`sha256sum`, `tar`, and `xorriso`; its pinned Limine v12.9.4 binaries are
downloaded to `build/` and checked against a fixed SHA-256 before extraction.
The ISO also includes Limine's binary-package license and third-party notices.
Only the ISO target needs those additional tools.

### Linux

On Debian or Ubuntu:

```sh
sudo apt-get update
sudo apt-get install --yes build-essential gcc-multilib nasm curl xorriso
make
make iso
```

`make check-x86` and `make check-x86_64` build individual targets. Outputs are
written to `build/x86/kernel.elf` and `build/x86_64/kernel.elf`. `make iso`
builds `build/akernel-1.1.109403-x86_64.iso`, which supports x86-64 BIOS and
UEFI boot through Limine. To test it with QEMU, install `qemu-system-x86` and
run `make run-qemu`. The VM target boots only the ISO; it does not access a
physical disk.

### Windows

Use WSL 2 with an Ubuntu distribution, then follow the Linux instructions
inside the WSL terminal. The kernel uses ELF cross-compilation tools and is not
a native Windows executable. Run `make run-qemu` inside a Linux VM rather than
trying to boot the ISO directly on the Windows host.

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
emulations selected in the Makefile. For the ISO target, use a Linux
environment with `curl`, `sha256sum`, `tar`, and `xorriso`.

## Website and community forum

The community forum and handbook are in [`web/`](web/). LESS sources are
compiled into browser-ready CSS with `npm ci` and `npm run build` from `web/`.
The static site is published to GitHub Pages by
[`.github/workflows/pages.yml`](.github/workflows/pages.yml). The workflow
installs the web dependencies, compiles LESS, and deploys the static output.
In **Settings → Pages**, set the build and deployment source to **GitHub
Actions**. The [Bug checker workflow](.github/workflows/build.yml) builds both
kernel targets and checks website styles, local links, and JavaScript syntax.
For local website validation, run `npm ci`, `npm run build`, then `npm run check`
from `web/`. The forum uses the cookie-backed
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

## Beta release 1.1.109403

Versioned beta notes and the artifact manifest are in
[`releases/v1.1.109403/`](releases/v1.1.109403/). The repository can build an
x86-64 Limine ISO locally, but no ISO is attached to or published with this
source release. Test it only in a disposable VM; do not replace a Linux
installation or write the ISO to a physical disk. See the
[step-by-step build guides for each host OS](web/docs/steps/).
The website's [beta downloads page](web/downloads/) explains the current ISO
availability, and the [browser loader demo](web/loader/) provides a simulated
preview that does not execute the kernel.

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

Use the repository's issue forms to report reproducible bugs or propose
features. The bug form links to private vulnerability reporting for security
issues; do not post exploitable details publicly.

To publish a source release, push a version tag and create a GitHub Release for
that tag:

```sh
git tag v(whatever you are work)
git push origin v1.1
```

Attach the built ELF files or a separately tested ISO if you want to distribute
the compiled scaffold. The ISO is for VM testing and is not an installable
operating system.

## Boot status

Kernel code is organized under `kernel/`: architecture-specific entry and
linker files are in `kernel/arch/`, public kernel headers are in
`kernel/include/akernel/`, and implementation modules are in `kernel/src/`.
The opt-in Limine kernel target uses the x86-64 Limine protocol, requests an
RGB framebuffer, and renders the package system-information display there. The
ISO target packages Limine for BIOS and UEFI boot. The original x86 and
x86-64 ELF targets still use their legacy entry points and are not bootable
images. This project does not include an installer, a disk-writing target, or
a complete operating system.

## Package and target compatibility

`kernel/include/akernel/akPackage.h` exposes the kernel package version and
build-target metadata. Package code can check
`AKPACKAGE_OS_BUILD_COMPATIBLE` to ensure it is being compiled for a supported
x86 or x86-64 freestanding ELF target. The package descriptors under
`pkg/akernel/` can additionally declare a minimum kernel version and supported
architecture mask.

These checks describe compiler/build compatibility only. The Limine boot path
is currently x86-64; it does not make the legacy ELF targets bootable or add
support for other architectures or firmware.

## License

GNU General Public License v3.0 or any later version. See [LICENSE](LICENSE)
for the SPDX identifier and link to the canonical license text.
