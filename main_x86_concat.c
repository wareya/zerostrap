
// $ clang -fomit-frame-pointer -target i386-unknown-none -m32 -g -Wall -Wextra -Oz -c main_x86_concat.c && objdump -r -d -M intel main_x86_concat.o|sed $'s~$~\e[0m~g'
// Then open main_x86_concat.o in a hex editor and manually locate the produced machine code (starts with B8 01 24 CD 15, ends with 72 F2 66 C3) and copy it over the start of disk32_concat.bin
// Then:
// dd if=demo_text_editor.zs oflag=seek_bytes seek=520 of=disk32_concat.bin & qemu-system-i386 disk32_concat.bin

#define HELPER_USERABI inline __attribute__((always_inline))
#define ROOT_USERABI __attribute__((noinline))

#define mem_int int
#define ZS_SUPPORT_PUTC 0
#define ZS_SUPPORT_GETC 1
#define ZS_NORETURN 1
#define ZS_NODIV 0
#define ZS_NOMUL 0
#define ZS_SOURCE_TERMINATOR '\xe7'

#define PRESALL

PRESALL HELPER_USERABI
__attribute__((force_align_arg_pointer))
unsigned int zs_getc(void) {
    register unsigned int c asm("eax");
    asm volatile (
        ".code32\n"
        "in   $0x64, %%al\n"
        "mov  %%al, %%ah\n"
        "test   $0x1, %%al\n"
        "jz __SKIPNOTREADY%=\n"
        "in   $0x60, %%al\n"
        "__SKIPNOTREADY%=:\n"
        : "=r" (c)
    );
    return c;
}

#define INTERPRETER_PRELUDE \
void interpret() \
{ \
    register char * source asm("ebx"); \
    register char ** macros asm("edi"); \
    asm volatile( \
            ".code16\n"\
            /* Note: the IBM PC programming manual specifies, NORMATIVELY, that the BIOS initializes: */ \
            /* - CS as 0, EIP as 0x7c00 */ \
            /* - DS and ES as ABS0 (absolute 0) */ \
            /* So despite common internet research, bootloaders for IBM PC-compatible BIOSes */\
            /*   do not TECHNICALLY need to set DS/ES, or worry that CS might be 0x7c0. */ \
            /* We need to set (just) DS anyway in practice because bochs's default bios doesn't comply with this, though. */ \
            "mov $0x2401, %%ax\nint $0x15\n" /* Raise the A20 line, enabling "odd megabytes" of memory */ \
            "mov $0x023f, %%ax\n" /* Load 64K of disk to RAM. */ \
            "mov $0x0800, %%bx\n" \
            "mov %%bx, %%es\n" \
            "xorl %0, %0\n" \
            "mov $0x0002, %%cx\n" \
            "mov $0x0, %%dh\n" \
            "int $0x13\n" /* Finish loading disk to RAM. */ \
            "mov %w0, %%ss\n" /* Gotta clear the stack segment. */ \
            "mov %w0, %%ds\n" /* Bochs's default BIOS is not IBM compatible and doesn't set DS to zero. */ \
            "mov $0x6a, %h0\n" \
            "mov %0, %%esp\n" /* And set the stack. */ \
            "mov $0x8008, %w0\n" /* This strange address is so we can double and use lower-16 to set the data segment later. */ \
            :"=r"(source)::"eax");\
    /* 32-bit handoff */ \
    /* !!!! NEEDS TO BE MANUALLY UPDATED IF THE ABOVE PART OF THE BOOTSTRAP CHANGES LENGTH !!!! */ \
    asm volatile( \
        "handoff:\n.code16\n" \
        "cli\n" \
        "lgdtw 0x7c34\n" \
        "mov $0x0011, %%ax\n lmsw %%ax\n" /* enable protected mode -- we don't care about any of the other flags in cr0:16 */ \
        "ljmp $0x8, $0x7c50\n"\
          ".gdtd:\n .word 0x17\n .word 0x7c38\n \n" \
          ".gdt:\n .quad 0x0000000000000000\n" \
                 " .quad 0x00cf9b000000ffff\n" \
                 " .quad 0x00cf93000000ffff\n" \
        ".code32\n" \
        :::"cr0","eax","memory" \
    ); \
    asm volatile(".code32\n"\
                 "procode:\n" \
                 "lea (%0, %0), %1\n" \
                 "mov %w1, %%ds\n" \
                 /* For the stack we can rely on the cached segment descriptor */ \
                 /*   because our stack is in the lowest 16 bits of memory. */ \
                :"+r"(source),"+r"(macros));

#include "zerostrap11.c"
