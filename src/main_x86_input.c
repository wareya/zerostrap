
// $ clang --version
// clang version 21.1.1 # in case something breaks in the future, this is the version I'm using
/*
    clang -fomit-frame-pointer -target i386-unknown-none -m32 -Wall -Wextra -Oz -c main_x86_input.c
    objdump -r -d -M intel main_x86_input.o|sed $'s~$~\e[0m~g' # sanity check
    objcopy -O binary --only-section=.text.interpret_body main_x86_input.o temp.bin
    dd if=temp.bin of=disk32_input.bin
    printf '\x55\xAA' | dd of=disk32_input.bin bs=1 seek=510 conv=notrunc
    rm temp.bin
    qemu-system-i386 disk32_input.bin
*/


#define HELPER_USERABI inline __attribute__((always_inline))
#define ROOT_USERABI __attribute__((noinline))

#define mem_int int
#define ZS_SUPPORT_PUTC 0
#define ZS_SUPPORT_GETC 1
#define ZS_NORETURN 1
#define ZS_NODIV 0
#define ZS_NOMUL 0
#define ZS_SOURCE_TERMINATOR '\x1B'

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
    asm volatile (".section .text.interpret_body"); \
    asm volatile ("_boot_start:"); \
    register char * source asm("ebx"); \
    register mem_int * multipurpose asm("esi"); \
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
            "xorl %0, %0\n" \
            "mov %w0, %%ss\n" /* Gotta clear the stack segment. */ \
            "mov %w0, %%ds\n" /* Bochs's default BIOS is not IBM compatible and doesn't set DS to zero. */ \
            "mov $0x6a, %h0\n" \
            "mov %0, %%esp\n" /* And set the stack. */ \
            "mov $0x8008, %w0\n" /* This strange address is so we can double and use lower-16 to set the data segment later. */ \
            "mov %0, %1\n" \
            :"=r"(source),"=r"(multipurpose)::"eax");\
    /* Read user input, supporting backspacing on the current line, until ESC. */ \
    asm volatile ("INPUTLOOPHEAD:\n");\
        register char c0 asm("al"); \
        asm volatile(".code16\nmov $0x00, %%ah\n   int $0x16\n" : "=r" (c0)); /* getchar */ \
        /* *source = c0; i++; */ \
        asm volatile(".code16\nmov %b0, (%w1)\ninc %w1\nINPUT_CHECKBKSP:\n" : "=r" (c0): "r"(multipurpose) :"memory");\
        asm volatile(".code16\ncmp $0x8, %1\n   jne INPUT_CHECKCRLF\n" /* if (c0 == BACKSPACE) i -= 2; */ \
                     "dec %w0\ndec %w0\n   INPUT_CHECKCRLF:\n" : "+r"(multipurpose) : "r"(c0)); \
        asm volatile(".code16\nmov $0x0E, %%ah\n   int $0x10\n" /* echo */ \
                     "cmp $0x0D, %0\n   jne INPUT_CHECKESC\n   " /* additional \n echo if input is \r */ \
                     "mov $0x0A, %0\n   int $0x10\n   INPUT_CHECKESC:" \
                     : "+r" (c0)); \
        asm volatile(".code16\ncmp $0x1B, %0\n jne INPUTLOOPHEAD\n"::"r"(c0)); /* loop if ESC */ \
    /* 32-bit handoff */ \
    /* !!!! NEEDS TO BE MANUALLY UPDATED IF THE ABOVE PART OF THE BOOTSTRAP CHANGES LENGTH !!!! */ \
    asm volatile( \
        "handoff:\n.code16\n" \
        "cli\n" \
        "lgdtw 0x7c45\n" \
        "mov $0x0011, %%ax\n lmsw %%ax\n" /* enable protected mode -- we don't care about any of the other flags in cr0:16 */ \
        /* Bit 0 of al must currently be 0, so using `inc` will do the same thing as `or 1`. */ \
        /* "mov %%cr0, %%eax\n inc %%ax\n mov %%eax, %%cr0\n" */ \
        "ljmp $0x8, $0x7c61\n"\
          ".gdtd:\n .word 0x17\n .word 0x7c49\n \n" \
          ".gdt:\n .quad 0x0000000000000000\n" \
                 " .quad 0x00cf9b000000ffff\n" \
                 " .quad 0x00cf93000000ffff\n" \
        ".code32\n" \
        :"=r"(multipurpose)::"cr0","eax","memory" \
    ); \
    asm volatile(".code32\n"\
                 "procode:\n" \
                 "lea (%0, %0), %1\n" \
                 "mov %w1, %%ds\n" \
                 /* For the stack we can rely on the cached segment descriptor */ \
                 /*   because our stack is in the lowest 16 bits of memory. */ \
                :"+r"(source),"+r"(macros));

#include "zerostrap11.c"

// scrap: objdump -r -d -M intel --source-comment="ZZZZZZ;" main_x86_input.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'
