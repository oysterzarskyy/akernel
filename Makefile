CC = gcc
LD ?= ld
NASM ?= nasm
ZIP ?= zip
QEMU ?= qemu-system-x86_64
CC_X86 ?= $(CC)
LD_X86 ?= $(LD)
CC_X86_64 ?= $(CC)
LD_X86_64 ?= $(LD)
LIMINE_VERSION := 12.9.4
LIMINE_SHA256 := 847334bc4bc142de996fbd132806b89ed73ac9ab34dbc1e90cfebf1fcc68c2b1
LIMINE_ARCHIVE := build/limine-binary-$(LIMINE_VERSION).tar.gz
LIMINE_DIR := build/limine-binary
LIMINE_URL := https://github.com/Limine-Bootloader/limine/releases/download/v$(LIMINE_VERSION)/limine-binary.tar.gz
ISO_IMAGE := build/akernel-v1.2.1.067-x86_64.iso
ISO_ZIP := build/akernel-v1.2.1.067-x86_64.zip

CFLAGS := -std=c11 -ffreestanding -fno-pie -fno-stack-protector \
          -Wall -Wextra -Werror -O2 -Ikernel/include

# Limine enters before the kernel initializes SSE, so keep early code on GPRs.
LIMINE_CFLAGS := $(CFLAGS) -m64 -mcmodel=kernel -mno-red-zone \
                 -mgeneral-regs-only \
                 -DAKPACKAGE_BOOT_PROTOCOL='"Limine"'

.DELETE_ON_ERROR:
.PHONY: all check-x86 check-x86_64 test-aash iso iso-zip run-qemu clean

all: check-x86 check-x86_64

check-x86: build/x86/kernel.elf

check-x86_64: build/x86_64/kernel.elf

iso: $(ISO_IMAGE)

iso-zip: $(ISO_ZIP).sha256

test-aash: build/test-aash
	./build/test-aash

run-qemu: iso
	$(QEMU) -m 2G -cdrom $(ISO_IMAGE) -no-reboot -no-shutdown

build/test-aash: tests/aash_test.c pkg/akernel/aash/aash.c \
                 pkg/akernel/aash/aash.h \
                 pkg/akernel/setup/include/setup.h \
                 kernel/include/akernel/console/framebuffer.h \
                 kernel/include/akernel/version.h
	mkdir -p build
	$(CC) -std=c11 -Wall -Wextra -Werror -Ikernel/include \
		-Ipkg/akernel/aash -Ipkg/akernel/setup \
		tests/aash_test.c pkg/akernel/aash/aash.c -o $@

build/x86/main.o: kernel/src/legacy/main.c \
                  kernel/include/akernel/kernel.h \
                  kernel/include/akernel/akPackage.h \
                  kernel/include/akernel/errCheck.h \
                  kernel/include/akernel/version.h \
                  kernel/include/akernel/core/halt.h
	mkdir -p build/x86
	$(CC_X86) $(CFLAGS) -m32 -c $< -o $@

build/x86/halt.o: kernel/src/core/halt.c kernel/include/akernel/core/halt.h
	mkdir -p build/x86
	$(CC_X86) $(CFLAGS) -m32 -c $< -o $@

build/x86/entry.o: kernel/arch/x86/entry.asm
	mkdir -p build/x86
	$(NASM) -f elf32 $< -o $@

build/x86/kernel.elf: build/x86/main.o build/x86/halt.o build/x86/entry.o kernel/arch/x86/linker.ld
	$(LD_X86) -m elf_i386 -T kernel/arch/x86/linker.ld -o $@ \
		build/x86/entry.o build/x86/main.o build/x86/halt.o

build/x86_64/main.o: kernel/src/legacy/main.c \
                     kernel/include/akernel/kernel.h \
                     kernel/include/akernel/akPackage.h \
                     kernel/include/akernel/errCheck.h \
                     kernel/include/akernel/version.h \
                     kernel/include/akernel/core/halt.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(CFLAGS) -m64 -c $< -o $@

build/x86_64/halt.o: kernel/src/core/halt.c kernel/include/akernel/core/halt.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(CFLAGS) -m64 -c $< -o $@

build/x86_64/entry.o: kernel/arch/x86_64/entry.asm
	mkdir -p build/x86_64
	$(NASM) -f elf64 $< -o $@

build/x86_64/kernel.elf: build/x86_64/main.o build/x86_64/halt.o \
                         build/x86_64/entry.o kernel/arch/x86_64/linker.ld
	$(LD_X86_64) -m elf_x86_64 -T kernel/arch/x86_64/linker.ld -o $@ \
		build/x86_64/entry.o build/x86_64/main.o build/x86_64/halt.o

build/x86_64/limine.o: kernel/src/boot/limine.c \
                       kernel/include/akernel/boot/limine.h \
                       kernel/include/akernel/console/framebuffer.h \
                       third_party/limine-protocol/limine.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ithird_party/limine-protocol -c $< -o $@

build/x86_64/kernel.o: kernel/src/kernel.c \
                       kernel/include/akernel/boot/limine.h \
                       kernel/include/akernel/console/framebuffer.h \
                       kernel/include/akernel/core/halt.h \
                       pkg/akernel/aash/aash.h \
                       pkg/akernel/setup/include/setup.h \
                       pkg/akernel/fetch/fetch.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ipkg/akernel/aash \
		-Ipkg/akernel/fetch -Ipkg/akernel/setup -c $< -o $@

build/x86_64/framebuffer.o: kernel/src/console/framebuffer.c \
                            kernel/include/akernel/console/framebuffer.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -c $< -o $@

build/x86_64/limine-halt.o: kernel/src/core/halt.c kernel/include/akernel/core/halt.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -c $< -o $@

build/x86_64/limine-fetch.o: pkg/akernel/fetch/fetch.c \
                             pkg/akernel/fetch/fetch.h \
                             pkg/akernel/fetch/include/internals.h \
                             pkg/akernel/package.h \
                             kernel/include/akernel/akPackage.h \
                             kernel/include/akernel/version.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ipkg/akernel/fetch -c $< -o $@

build/x86_64/limine-aash.o: pkg/akernel/aash/aash.c \
                            pkg/akernel/aash/aash.h \
                            pkg/akernel/setup/include/setup.h \
                            kernel/include/akernel/version.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ipkg/akernel/aash \
		-Ipkg/akernel/setup -c $< -o $@

build/x86_64/limine-setup.o: pkg/akernel/setup/setup.c \
                             pkg/akernel/setup/include/setup.h \
                             kernel/include/akernel/console/framebuffer.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ipkg/akernel/setup -c $< -o $@

build/x86_64/serial.o: kernel/src/drivers/serial.c \
                       kernel/include/akernel/drivers/serial.h \
                       kernel/include/akernel/arch/io.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -c $< -o $@

build/x86_64/keyboard.o: kernel/src/drivers/keyboard.c \
                         kernel/include/akernel/drivers/keyboard.h \
kernel/include/akernel/arch/io.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -c $< -o $@

build/x86_64/shell.o: kernel/src/shell.c \
                      kernel/include/akernel/shell.h \
                      kernel/include/akernel/console/framebuffer.h \
                      kernel/include/akernel/drivers/keyboard.h \
                      kernel/include/akernel/drivers/serial.h \
                      kernel/include/akernel/arch/io.h \
                      pkg/akernel/fetch/fetch.h
	mkdir -p build/x86_64
	$(CC_X86_64) $(LIMINE_CFLAGS) -Ipkg/akernel/fetch -c $< -o $@

build/x86_64/akernel-limine.elf: build/x86_64/kernel.o \
                                build/x86_64/limine.o \
                                build/x86_64/framebuffer.o \
                                build/x86_64/limine-halt.o \
                                build/x86_64/limine-fetch.o \
                                build/x86_64/limine-aash.o \
                                build/x86_64/limine-setup.o \
                                kernel/arch/x86_64/limine.ld \
                                build/x86_64/serial.o \
                                build/x86_64/keyboard.o \
                                build/x86_64/shell.o
	$(LD_X86_64) -m elf_x86_64 -z max-page-size=0x1000 \
		-T kernel/arch/x86_64/limine.ld -o $@ \
		build/x86_64/kernel.o build/x86_64/limine.o \
		build/x86_64/framebuffer.o build/x86_64/limine-halt.o \
		build/x86_64/limine-fetch.o build/x86_64/limine-aash.o \
		build/x86_64/limine-setup.o \
		build/x86_64/serial.o build/x86_64/keyboard.o build/x86_64/shell.o

$(LIMINE_ARCHIVE):
	mkdir -p build
	curl -fL --retry 3 -o $@.part $(LIMINE_URL)
	printf '%s  %s\n' '$(LIMINE_SHA256)' '$@.part' | sha256sum -c -
	mv $@.part $@

$(LIMINE_DIR)/.ready: $(LIMINE_ARCHIVE)
	printf '%s  %s\n' '$(LIMINE_SHA256)' '$<' | sha256sum -c -
	rm -rf $(LIMINE_DIR)
	tar -xzf $< -C build
	touch $@

$(LIMINE_DIR)/limine: $(LIMINE_DIR)/.ready
	$(MAKE) -C $(LIMINE_DIR)

$(ISO_IMAGE): build/x86_64/akernel-limine.elf $(LIMINE_DIR)/limine limine.conf
	rm -rf build/iso-root
	mkdir -p build/iso-root/boot build/iso-root/EFI/BOOT
	mkdir -p build/iso-root/boot/limine
	cp limine.conf build/iso-root/boot/limine/limine.conf
	cp $< build/iso-root/boot/akernel.elf
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-bios.sys \
		$(LIMINE_DIR)/limine-uefi-cd.bin build/iso-root/boot/limine/
	cp $(LIMINE_DIR)/LICENSE $(LIMINE_DIR)/3RDPARTY.md \
		build/iso-root/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI build/iso-root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label \
		build/iso-root -o $@
	$(LIMINE_DIR)/limine bios-install $@

$(ISO_ZIP): $(ISO_IMAGE)
	rm -f $@
	$(ZIP) -j -9 $@ $<

$(ISO_ZIP).sha256: $(ISO_ZIP)
	sha256sum $< > $@

clean:
	rm -rf build
