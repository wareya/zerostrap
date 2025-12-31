//
// ZEROSTRAP (version 11)
// an ULTRA SMALL (roughly 350 bytes on x86) Forth-like language with Macros, Jump Labels, and Comments
// 

// The below documentation might be out of date. The source code and examples are the sources of truth.
//
// list of primitives:
// #                comment. skip until end of line
// #[^0-9 ][^ ]*    define macro/function
// #[0-9]+          define label with index defined by number as pointing to current location
// [0-9]+           push number (integer) to stack
// -                SUB ; pop into B, pop into A, calculate A - B, push. [4 15 -] acts like [4 - 15].
// /                DIV ; pop into B, pop into A, calculate A / B, push. Rounds towards zero.
// * (DISABLED)     MUL ; pop twice, multiply, push. DISABLED BY DEFAULT, can be enabled.
// &                AND ; pop twice, AND, push
// ?                IFGT; pop into A, pop into L, if A then jump to label with index L
// w                WRTE; pop into A, pop into B, write value B into memory location A
// r                READ; pop, find value at that memory location, push that value
// . (DISABLED)     PUTC; pop, call zs_putc() with popped value converted to char
// , (redefinable)  GETC; call zs_getc(), push resulting char to stack. DISABLED BY DEFAULT, can be enabled.
// C                CSTK; push position/address of macro callstack to stack. allows for slight metaprogramming.

// Comparisons, addition, modulo, etc. can all be built out of these primitives using macros/functions. Note that macros are functionlike and can be called recursively, but they're not stack-coherent and there's no TCO.

// NOTE: Division (`/`) can be replaced with "shift-arithmetic-right" (`\`). However, this is not usually a space savings.
// NOTE: Macros are not stack-coherent. They can produce or consume net stack depth.
// NOTE: Macros cannot start with the character for another primitive. For example, because `r` is a primitive, you cannot have a `return` macro; it needs to be `_return` or similar.
// NOTE: Labels are processed in advance, before execution.
// NOTE: Labels with a value of 0 are forbidden.

// Running on bare metal? From boot? Want even less bytes? ZS_NORETURN makes things even smaller.

// Want function activation records? You can build "call" and "return" macros. Remember, macros aren't stack-coherent.

// debug print spew -- not for bare metal
#ifndef ZS_DEBUG
#define ZS_DEBUG 0
#endif
#ifndef ZS_DEBUG_MACROS
#define ZS_DEBUG_MACROS 0
#endif
#if ZS_DEBUG || ZS_DEBUG_MACROS
#include <stdio.h>
#endif

// configurable
// default prelude
#ifndef macro_or_label_count
#define macro_or_label_count (1<<10)
#endif
// getc in general
#ifndef ZS_SUPPORT_GETC
#define ZS_SUPPORT_GETC 0
#endif
// going on bare metal? you can disable returning from interpret() to get rid of its function prologue/epilogue
#ifndef ZS_NORETURN
#define ZS_NORETURN 0
#endif
// if true, replaces the / operator with a shift-arithmetic-right \ operator
#ifndef ZS_NODIV
#define ZS_NODIV 0
#endif
// if true, deletes the * operator entirely
// see pf.zs for a macro implementation based on the other operators
#ifndef ZS_NOMUL
#define ZS_NOMUL 1
#endif
// if false, deletes the . operator
#ifndef ZS_SUPPORT_PUTC
#define ZS_SUPPORT_PUTC 1
#endif

#ifndef ZS_SOURCE_TERMINATOR
#define ZS_SOURCE_TERMINATOR 0
#endif

#ifndef mem_int
#define mem_int int
#endif

#ifndef ptr_uint
#if __x86_64__ | __aarch64__
    #define ptr_uint unsigned long long int
    #define fast_int int
#elif _X86_ | __arm__
    #define ptr_uint unsigned int
    #define fast_int int
#elif i386
    #define ptr_uint unsigned mem_int
    #define fast_int short
#else
    #define ptr_uint unsigned int
    #define fast_int int
#endif
#endif

#define NOREORDER() __atomic_signal_fence(__ATOMIC_SEQ_CST);

#if __x86_64__
    #if defined(__GNUC__) && !defined(__clang__)
        #define PRESNONE __attribute__((sysv_abi))
        #define PRESALL __attribute__((sysv_abi))
    #else
        #define PRESNONE __attribute__((preserve_none))
        #define PRESALL __attribute__((preserve_all))
    #endif
#else
    #define PRESNONE __attribute__((cdecl))
    #define PRESALL
#endif

#ifndef HELPER_USERABI
#define HELPER_USERABI
#endif
#ifndef ROOT_USERABI
#define ROOT_USERABI
#endif

#if ZS_SUPPORT_PUTC
    PRESALL HELPER_USERABI
    void zs_putc(char c);
#endif

#if ZS_SUPPORT_GETC
    PRESALL HELPER_USERABI
    unsigned int zs_getc(void);
#endif

#if defined(__i386__) || defined(__x86_64__)
    #define push_int(X) {\
        ptr_uint _temp_x = (ptr_uint)(X); \
        asm volatile ( "push %0\n" : : "r" (_temp_x) : "rsp"); \
    }
    #define pop_int(X) { \
        ptr_uint _temp_x; \
        asm volatile ( "pop %0\n"  : "=r" (_temp_x) : : "rsp"); \
        X = (typeof(X)) _temp_x;\
    }
    #define push_reg1_byte_punned_evil(X) { \
        register unsigned short x asm("ax") = X; \
        asm volatile ( "push %%eax\n" : : "r" (x)); \
    }
#else
    ptr_uint stack[256];
    ptr_uint * sp = stack;
    #define push_int(X) { *(sp++) = X; }
    #define pop_int(X) { X = *(--sp); }
    #define push_reg1_byte_punned_evil(X) { *(sp++) = X; }
#endif

PRESNONE ROOT_USERABI
#if ZS_NORETURN
    __attribute__((noreturn))
    _Noreturn
#endif
#ifndef INTERPRETER_PRELUDE
void interpret(char * source, mem_int * memory)
{
    unsigned int _macros[macro_or_label_count];
    unsigned int * macros = &_macros[macro_or_label_count];
#else
INTERPRETER_PRELUDE
#endif
    
    #if ZS_DEBUG
    #define dputs(X) puts(X)
    #else
    #define dputs(X)
    #endif
    
    #define HINT(X) asm volatile(X "_%=:":);// dputs(X);
    #define LABELHINT(X) asm volatile(X "_%=:":);
    
    LABELHINT("setup");
    
    
    asm volatile("":"+r"(source));
    char ** _labels = (char **)(0x400000);
    asm volatile("":"+r"(_labels));
    
    char ** macros_orig = macros;
    
    LABELHINT("process_labels");
    push_int(source);
    {
        REPEAT:
        { }
        unsigned char c;
        asm volatile("":"+r"(c));
        HINT("check_individual");
        c = *source;
        source++;
        asm volatile("":"+r"(source));
        //if (c == '$')
        if (c == '#')
        {
            HINT("process_label");
            *macros = source;
            asm volatile("":"+r"(macros));
            macros = (char **)((char *)macros + 4);
            asm volatile("":"+r"(macros));
            
            c = *source;
            asm volatile("":"+r"(c));
            asm volatile("":"+r"(source));
            if (c >= '0' && c <= '9')
            {
                register unsigned int x asm("ecx");
                asm volatile (
                    "callw _readint\n"
                    :"=r"(x),"+r"(source)::"eax","memory"
                );
                _labels[x*2] = source;
            }
        }
        asm volatile("":"=r"(c));
        HINT("check_next_or_end");
        if (*source != ZS_SOURCE_TERMINATOR) goto REPEAT;
    }
    HINT("done_processing_labels");
    pop_int(source);
    
    push_int(source);
    {
        REPEAT2:
        HINT("check_macro_loop");
        unsigned char c = *source;
        if ((char)c != ZS_SOURCE_TERMINATOR)
        {
            char * j;
            char * k;
            
            push_int(macros);
            
            asm volatile("":::"eax");
            do {
                LABELHINT("handle_apply_macro_loop");
                if (macros <= macros_orig) goto FORCECONTINUE;
                // macros, macros orig: edi, edx
                
                macros = (char **)((char *)macros - 4);
                asm volatile("":"+r"(macros));
                // j, k: ecx, ebp
                j = *macros;
                k = source;
                asm volatile("":"+r"(k),"+r"(j));
                // source: ebx
                // _labels: esi
                
                // *j, *k: al, ah (eax)
                NOREORDER();
                LABELHINT("handle_apply_macro_loop_inner");
                while (*j > 32 && *j == *k)
                {
                    j++;
                    k++;
                }
            } while (*j > 32 || *k > 32);
            asm volatile("":::"eax");
            asm volatile("":"+r"(k));
            asm volatile("":"+r"(source));
            
            NOREORDER();
            LABELHINT("handle_apply_macro_found");
            NOREORDER();
            
            _labels[((int)source)*2+1] = j;
            
            NOREORDER();
            LABELHINT("handle_apply_macro_continue");
            NOREORDER();
            
            FORCECONTINUE:
            pop_int(macros);
            source++;
            goto REPEAT2;
        }
    }
    HINT("done_processing_labels");
    pop_int(source);
    
    LABELHINT("interpreter_loop");
    #if !(ZS_NORETURN)
    TOP:
    while (*source)
    #else
    while (1)
    TOP:
    #endif
    {
        LABELHINT("interpreter_loop");
        unsigned int c = *source;
        source++;
        asm volatile("":"+r"(source));
        
        NOREORDER();
        LABELHINT("check_ifgoto");
        NOREORDER();
        if (c == '?')
        {
            HINT("handle_ifgoto_test");
            unsigned mem_int val;
            register unsigned int target asm("edx");
            pop_int(val);
            pop_int(target);
            if (val)
            {
                HINT("handle_ifgoto_followed");
                source = _labels[target*2];
                goto TOP;
            }
            
            continue;
        }
        
        NOREORDER();
        LABELHINT("check_macroend");
        NOREORDER();
        if (c == ';')
        {
            HINT("handle_macroend");
            
            asm volatile("":"+r"(macros));
            macros = (char **)((char *)macros - 4);
            asm volatile("":"+r"(macros));
            
            source = *macros;
            NOREORDER();
            
            NOREORDER();
            ASDFASDF2:
            asm volatile("":"+r"(source));
            NOREORDER();
            continue;
        }
        
        NOREORDER();
        LABELHINT("check_space");
        NOREORDER();
        if (c <= 32)
        {
            NOREORDER();
            goto ASDFASDF2;
        }
        
        LABELHINT("check_primitives");
        if (c == '@')
        {
            LABELHINT("handle_primitives");
            c = *source;
            source++;
            asm volatile("":"+r"(source));
            
            register mem_int b asm("ecx");
            pop_int(b);
            
            NOREORDER();
            LABELHINT("check_set");
            NOREORDER();
            if (c == 'w')
            {
                HINT("handle_set");
                
                mem_int loc = b;
                mem_int val;
                pop_int(val);
                *(mem_int *)((char *)loc) = val;
            }
            
            NOREORDER();
            LABELHINT("check_sub");
            NOREORDER();
            if (c == '-')
            {
                HINT("handle_sub");
                asm volatile("sub %0, (%%esp)\n"::"r"(b):"memory","rsp");
            }
            
            NOREORDER();
            LABELHINT("check_and");
            NOREORDER();
            if (c == '&')
            {
                HINT("handle_and");
                asm volatile("and %0, (%%esp)\n"::"r"(b):"memory","rsp");
            }
            #if !(ZS_NOMUL)
                NOREORDER();
                LABELHINT("check_mul");
                NOREORDER();
                if (c == '*')
                {
                    HINT("handle_mul");
                    
                    unsigned int a;
                    pop_int(a);
                    push_int(a*b);
                }
            #endif
            #if ZS_NODIV
                NOREORDER();
                LABELHINT("check_sar");
                NOREORDER();
                if (c == '\\')
                {
                    HINT("handle_sar");
                    asm volatile("sarl %b0, (%%esp)\n"::"r"(b):"memory","rsp");
                }
            #endif
            
            NOREORDER();
            LABELHINT("check_read");
            NOREORDER();
            if (c == 'r')
            {
                HINT("handle_read");
                
                mem_int loc = b;
                asm volatile("push (%0)\n"::"r"(loc):"rsp");
                
                NOREORDER();
            }
            
            // NOREORDER();
            // LABELHINT("check_stackpos");
            // NOREORDER();
            // if (c == 'S')
            // {
            //     HINT("handle_stackpos");
            //     asm volatile("push %%esp\n":);
            //     NOREORDER();
            // }
            
            NOREORDER();
            LABELHINT("check_callstackpos");
            NOREORDER();
            if (c == 'C')
            {
                HINT("handle_callstackpos");
                asm volatile("push %0\n"::"r"(macros));
                NOREORDER();
            }
            
            #if !(ZS_NODIV)
                NOREORDER();
                LABELHINT("check_div");
                NOREORDER();
                if (c == '/')
                {
                    HINT("handle_div");
                    
                    int a;
                    pop_int(a);
                    push_int(a/b);
                }
            #endif
            
            LABELHINT("handle_primitives_done");
            
            NOREORDER();
            goto ASDFASDF2;
        }
        #if ZS_SUPPORT_GETC
            NOREORDER();
            LABELHINT("check_getc");
            NOREORDER();
            if (c == ',')
            {
                HINT("handle_getc");
                
                unsigned int val = zs_getc();
                push_int(val);
                
                ASDFASDF5:
                NOREORDER();
                asm volatile("":"+r"(source));
                goto ASDFASDF2;
            }
        #endif
        
        #if ZS_SUPPORT_PUTC
            NOREORDER();
            LABELHINT("check_putc");
            NOREORDER();
            if (c == '.')
            {
                HINT("handle_putc");
                
                unsigned mem_int val;
                pop_int(val);
                zs_putc(val);
                
                NOREORDER();
                goto ASDFASDF5;
            }
        #endif

        NOREORDER();
        LABELHINT("check_comment");
        NOREORDER();
        if (c == '#')
        {
            HINT("handle_comment_skip");
            while (*source >= 32)
            {
                NOREORDER();
                source++;
            }
            NOREORDER();
            goto ASDFASDF5;
        }
        
        NOREORDER();
        LABELHINT("check_consuming_loop_terms");
        NOREORDER();
        
        source--;
        
        NOREORDER();
        LABELHINT("check_num");
        NOREORDER();
        if (c >= '0' && c <= '9')
        {
            LABELHINT("handle_num");
            register unsigned int x asm("ecx");
            asm volatile (
                "callw _readint\n"
                :"=r"(x),"+r"(source)::"eax","memory"
            );
            source--;
            push_int(x);
            
            NOREORDER();
            goto ASDFASDF5;
        }
        
        asm volatile("":"+r"(source));
        #if ZS_DEBUG
        LABELHINT("check_apply_macro");
        if (c > 32)
        #endif
        {
            LABELHINT("handle_apply_macro_search");
            NOREORDER();
            
            int k = (int)source;
            while (*source > 32)
            {
                asm volatile("":"+r"(source));
                source++;
            }
            
            *macros = source;
            
            asm volatile("":"+r"(macros));
            macros = (char **)((char *)macros + 4);
            asm volatile("":"+r"(macros));
            
            source = _labels[k*2+1];
            
            NOREORDER();
            goto ASDFASDF5;
        }
        #if ZS_DEBUG
            __builtin_trap();
        #endif
    }
    #if !(ZS_NORETURN)
    LABELHINT("interpret_done");
    return;
    #endif

#if 1
}
#endif
#if 1 == 0
}
#endif
#undef dputs

__attribute__((naked, noinline, unused))
void __readint(void)
{
    asm volatile (".section .text.interpret_body");
    asm volatile(
        "_readint:\n"
        ".code32\n"
        "xor %%ecx, %%ecx\n" // x = 0
        "xor %%eax, %%eax\n" // c = 0
        "LOOPHEAD%=:\n"
            "imul $0xa, %%ecx, %%ecx\n" // x *= 10
            "add %%eax, %%ecx\n"        // x += c
            "mov (%%ebx), %%al\n"       // c = *source
            "inc %%ebx\n"               // source++
            "sub $0x30, %%al\n"         // c -= '0'
            "cmp $0xa, %%al\n"          // if c < 10
        "jb LOOPHEAD%=\n"               //     repeat
        "retw\n"                        // return
        :::"memory"
    );
}

// clang -v
// clang version 21.1.1

// --- 64-bit output:
// --- output is 0x187 (391) bytes on my system
// clang -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && objdump -r -d -M intel --source-comment="ZZZZZZ;" zerostrap7.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'
// --- 151 instructions of actual assembly
// clang -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && objdump --no-show-raw-insn -d -M intel zerostrap7.o | sed /^$/d | sed /^0.*/d | wc -l
// --- 185 lines of assembly with labels (no empty lines)
// clang -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && objdump --no-show-raw-insn -d -M intel zerostrap7.o | sed /^$/d | wc -l

// --- 32-bit output:
// --- output is 0x14f (335) bytes on my system
// $ clang -m32 -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && objdump -r -d -M intel --source-comment="ZZZZZZ;" zerostrap7.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'
// --- 147 instructions of actual assembly
// $ clang -m32 -Wall -Wextra -fno-align-functions -mno-stack-arg-probe -g -fno-jump-tables -Oz -c zerostrap7.c && objdump --no-show-raw-insn -d -M intel zerostrap7.o | sed /^$/d | sed /^0.*/d | wc -l
// --- 181 lines of assembly with labels (no empty lines)
// $ clang -m32 -Wall -Wextra -fno-align-functions -mno-stack-arg-probe -g -fno-jump-tables -Oz -c zerostrap7.c && objdump --no-show-raw-insn -d -M intel zerostrap7.o | sed /^$/d | wc -l

// --- 16-bit output doesn't fare as well:
// --- output is 0x1b1
// clang -target i386-unknown-none -m16 -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && objdump -r -d -m i8086 -M intel --source-comment="ZZZZZZ;" zerostrap7.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'

// --- riscv 64
// --- 0x236 bytes
// clang --target=riscv64-unknown-elf -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && riscv64-unknown-elf-objdump.exe -r -d --source-comment="ZZZZZZ;" zerostrap7.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'

// --- riscv 64
// --- 0x224 bytes
// clang --target=riscv32-unknown-elf -g -Wall -Wextra -mno-stack-arg-probe -Oz -c zerostrap7.c && riscv64-unknown-elf-objdump.exe -r -d --source-comment="ZZZZZZ;" zerostrap7.o|sed $'s~ZZZZZZ~\e[2m~g'|sed $'s~$~\e[0m~g'
