bits 32

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    and esp, 0FFFFFFF0h
    call kernel_main

.halt:
    hlt
    jmp .halt

section .bss align=16
stack_bottom:
    resb 16384
stack_top:
