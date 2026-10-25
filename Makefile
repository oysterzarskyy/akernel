CC ?= gcc
LD ?= ld
NASM ?= nasm

CFLAGS := -std=c11 -ffreestanding -fno-pie -fno-stack-protector \
          -Wall -Wextra -Werror -O2 -Iinclude

.PHONY: all check-x86 check-x86_64 clean

all: check-x86 check-x86_64

check-x86: build/x86/kernel.elf

check-x86_64: build/x86_64/kernel.elf

build/x86/main.o: main.c include/kernel.h include/errCheck.h
	mkdir -p build/x86
	$(CC) $(CFLAGS) -m32 -c main.c -o $@

build/x86/entry.o: arch/x86/entry.asm
	mkdir -p build/x86
	$(NASM) -f elf32 $< -o $@

build/x86/kernel.elf: build/x86/main.o build/x86/entry.o arch/x86/linker.ld
	$(LD) -m elf_i386 -T arch/x86/linker.ld -o $@ build/x86/entry.o build/x86/main.o

build/x86_64/main.o: main.c include/kernel.h include/errCheck.h
	mkdir -p build/x86_64
	$(CC) $(CFLAGS) -m64 -c main.c -o $@

build/x86_64/entry.o: arch/x86_64/entry.asm
	mkdir -p build/x86_64
	$(NASM) -f elf64 $< -o $@

build/x86_64/kernel.elf: build/x86_64/main.o build/x86_64/entry.o arch/x86_64/linker.ld
	$(LD) -m elf_x86_64 -T arch/x86_64/linker.ld -o $@ build/x86_64/entry.o build/x86_64/main.o

clean:
	rm -rf build
