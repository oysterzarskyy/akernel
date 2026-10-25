; Architecture-specific entry stubs live in arch/x86/entry.asm and
; arch/x86_64/entry.asm. These are kernel entry points, not disk boot sectors.
; A separate bootloader must load the matching ELF and enter in the documented
; CPU mode before jumping to _start.
