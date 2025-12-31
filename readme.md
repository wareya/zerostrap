# Zerostrap

A boot sector programming language that you can Actually Use.

## About

Zerostrap is a bare-metal-capable programming language meant for bootstrapping (e.g. compiler toolchains) from a trusted root. It comes in **370 or fewer bytes** of machine code, making it genuinely auditable, but it's still powerful enough to actually use despite being so small.

The x86 version runs in 32-bit protected mode and gives you full access to all 32-bit memory, and also the i8042 (PS/2 or USB) keyboard input interrupt. This isn't sleight of hand, either: fitting into 370 bytes *includes the x86 16-bit to 32-bit mode bootstrap and startup-time interpreted program input*; the interpreter itself is somewhere around 270 bytes.

As proof of usefulness, I wrote a **real, working text editor** in zerostrap that should be *mostly* readable to low-level C programmers, in `demo_text_editor.zs`. The syntax is very different, and there's a wall of pseudo-stdlib stuff at the top, but once you get to where functions are defined, the line-by-line structure is surprisingly close to C, with extra boilerplate. It also runs fast enough on realistic hardware emulation that it's viable for nontrivial software, unlike some other bootloader-sized languages where you have to construct basic things like numeric literals out of expensive stack-address-manipulation macros.

https://github.com/user-attachments/assets/94afca9e-b21b-4bb5-90fb-604752ed7557

^-- Text editor written in zerostrap, running in x86 bootloader mode in QEMU

If there isn't enough built-in behavior in the current version, being under 370 bytes gives plenty of room for people to add more functionality without hitting the end of the boot sector. The goal isn't "smallest", it's "most powerful" (without becoming hard to 100% understand at the instruction level).

Zerostrap is written in C with lots of inline assembly. This makes it slightly easier to modify than things this size that are written in *literally just* assembly, because the control flow and most of the logic is normal C code.

```R
    getch y<- # Get a character, write it into variable y. y<- is a macro/function, not an intrinsic feature. so is y, etc.
    
    # If it's not the enter character, jump to label 601 (i.e. jump over this block)
    601 y 10 != ?
        # Add a new slot to the array of strings making up the document
        # memmove(curlineptr + 4, curlineptr, (linecount - row) * 4);
        # NOTE: - and * are wrapper functions that just call @- and @* and immediately return. + is a function that implements addition as a-(0-b).
        curlineptr 4 +   curlineptr   linecount row - 4 *   memmove
        # linecount = linecount + 1
        linecount 1 +   linecount<-
        # *curlineptr = alloc_string();
        alloc_string curlineptr w
        
        # Split the line we're currently editing:
        # Copy left side of old into new
        # memmove(*curlineptr + 1, *(curlineptr + 4) + 1, col);
        curlineptr r 1 +   curlineptr 4 + r 1 +   col   memmove
        # Set new len to left side len
        # w8 is a macro that acts like w but writes a char instead of an int
        # (*(uint8_t *)curlineptr) = col;
        col   curlineptr r   w8
        # Move old right-side contents leftwards
        # memmove(... you get the idea)
        curlineptr 4 + r 1 +   curlineptr 4 + r 1 + col +   curlineptr 4 + r r8 col -   memmove
        # Set old len to right side len
        curlineptr 4 + r r8 col -   curlineptr 4 + r w8
        
        1 neg prevrow<- # prevrow = -1
        row 1 + row<- # row += 1
        
        0 col<- # col = 0
        refresh # refresh()
        620 1? # go to head of an outer loop (not shown in this snippet)
    #601
```

There's also a VGA mode 13h example mandelbrot generator:

https://github.com/user-attachments/assets/b84618d8-b66d-4ad8-9948-d06c3bbdfe2d

## Usage

QEMU/Bochs: Run `disk32_concat.bin` (autoload text editor) or `disk32_input.bin` (boot-time text input, run by hitting esc) as raw disk images. Example: `qemu-system-i386 disk32_concat.bin`

x86 hardware: Same as above, but on a floppy disk.

To compile, follow the rest of the instructions in the main C files.

The current implementation is limited to about 2^15 bytes of program text, but this isn't a fundamental limitation of the interpreter. Rather, it's a limitation from saving space in the 16 to 32 bit boot process. If you need more space, make the pointer that ends up containing 0x10010 contain a larger value, so that the source text doesn't crash into it.

## Design

Zerostrap has:
- Parserless design for source-text interpreters, with concatenative syntax and stack-based evaluation
- Decimal numeric literals like `5341` and basic math like `@-`, `@*`, `@/`, `@&`
- 32-bit memory reads/writes like `<ADDR> @r`, `X <ADDR> @w`
- Labels and conditional jumps like `#5` (label), `40 r 5 ?` (read word at memory address 40, if truthy then go to label 5)
- Macro-like functions (that **support recursion**) like `#x<- 40 w ;` where now if you write `x<-` elsewhere it'll execute `40 w`.
- Getc/putc operands for frontends (in the x86 bare-metal version, just getc, as `,`)
- Comments (any usage of `# ` with a space immediately after the `#`)

Unlike Forth, zerostrap doesn't have a separate compilation phase or any `immediate` behavior. The only pre-passes are for setting up the macro/function and label systems. However, this seems to make implementing a decent interpreter *easier* instead of harder.

Functions aren't stack-hygienic, so they can change the height of the stack, which makes them dramatically more powerful. This is what makes them "macro-like". This means that you can e.g. implement function activation record management in macros, or pseudo-variable accessors for those activation records. In fact, I do this:

```
# ----
# function activation record stack
# ----
3004 300w # w and r are previously-defined but hidden functions that wrap @w and @r with an additional offset. Here we write 3004 to location 300 inside of "standard memory" (which doesn't start at hardware memory 0).
#su 1+ 4* 300r swap + 300w ; activation record stack up
#sd 1+ 4* 300r swap - 300w ; activation record stack down
#sr 1+ 4* 300r swap - r ;
#sw 1+ 4* 300r swap - w ;

#x   0sr ; # zeroth activation record stack element: read
#x<- 0sw ; # zeroth, write.
#y   1sr ; # first, etc.
#y<- 1sw ;
```

## Spiel

The goal, which I think I succeeded at, is to make it possible to jump straight from machine code to something high level enough to write genuinely complex software in, in a single language implementation. Rather than in three or four language implementations.

I think I succeeded: I wrote an **entire text editor** in zerostrap. Complete with line and character insertion and deletion, full-screen display, line numbers, a status bar, etc. It even has a memory allocator. It's janky, but the code is *mostly* readable to programmers that are familiar with low-level C; it's structured weirdly, but a lot of the same idioms apply, unlike with Forths or Lisps. I even have memcpy and memmove functions.

https://github.com/user-attachments/assets/329401e3-1f9c-498d-8f88-6dad322f0e65

The language has enough basic features (e.g. multiplication, division) that it isn't horrendously slow to do basic operations, so you can actually write real software with it if you're compelled to, instead of getting walled by "oops, deciding on the memory values for writing this decimal number's digits to the screen should take like 2 seconds to run, but instead it takes 30 minutes". You don't have to implement basic math in terms of bit fiddling or stack manipulation like you do in most bootloader-level Forths.

As implemented, zerostrap is meant for x86 (i686) CPUs, but it should be straightforward to implement on literally anything. It really doesn't need much. I wrote a high-level emulator (`zs_emu2.c`) that also functions as a reference interpreter that doesn't depend on inline assembly. It emulates the i686 boot-mode environment that the main implementations in `main_x86_concat.c` etc. expect, including VGA and text input. It's where I wrote most of the example text editor.

# Tutorial

```R
# |-----
# | Minitutorial
# |
# | Zerostrap is a concatenative stack-based language. Expressions are written in reverse polish notation:
5 5 @* # push 5, push 5, pop two stack items and feed them to @* and push the result
# | stack is now [25]
9000 @w # push 9000, pop as address, pop as value, *(address) = value (32-bit integer)
# | memory bytes 9000, 9001, 9002, and 9003 just got written to.
# | 9000 contains 25, the others contain 0. Memory accesses are little-endian.
# | 
# | Let's define and use a function, called asdf. ; is the end-of-function character.
# | Functions look like comments. If it's not a function (or label), always add a space after the # character of your comments!
#asdf 9000 @r ;
# | Now the function asdf exists, which reads the 32-bit integer at 9000 and pushes it to the stack.
asdf asdf @* 
# | Er, hmm. We forgot to write a writing macro. Our stack contains [125] now, so...
#asdf<- 9000 @w ;
asdf<-
# | OK, now we wrote 125 back to where asdf is.
# | By the way, take note that functions do not need to leave the stack at the same level. They're like macros in that way.
# | (The source code for zerostrap calls them macros and functions interchangeably.)
# | Functions are space-sensitive. Their uses must be followed by a space/tab/newline/etc.
# | Primitives (@-  @*  @/  @&  @,  @r  ?  ;) and numbers are not space-sensitive.
# |
# | Let's do some control flow now.
# | Control flow uses the ? operator, which pops as test value, pops as label value, and jumps to label value if test value is nonzero.
555555 asdf ?
# SKIPPED
#555555
# | OK, now you know everything you need to know to write zerostrap code. Have fun!
# |-----
```

## Full disassembly of the "input" version, annotated

```as
MODE: 16-bit x86

00007c00 <interpret>:
    ; Raise the A20 line, enabling Odd Megabytes of memory
    7c03: cd 15                         int     0x15
    7c00: b8 01 24                      mov     ax, 0x2401
    ; EBX will hold one of our pointers.
    ; While setting EBXs actual value, we clear the stack and data segment regs too.
    ; Some BIOSes are not fully IBM-compatible and require the data seg to be explicitly zeroed.
    7c05: 66 31 db                      xor     ebx, ebx
    7c08: 8e d3                         mov     ss, bx
    7c0a: 8e db                         mov     ds, bx
    ; Now we set BX = 0x6a00 and set the stack pointer to that.
    7c0c: b7 6a                         mov     bh, 0x6a
    7c0e: 66 89 dc                      mov     esp, ebx
    ; Now we set bx proper to 0x8008. This is where our source text goes.
    ; Take note of this value: when doubled as 2*EBX, it becomes 0x10010.
    ; We will set the data segment to 0x10 later as though doing lowest_8_bits(2*EBX).
    7c11: bb 08 80                      mov     bx, 0x8008
    ; We need a mutable copy of EBX as a write cursor, so we use SI.
    ; We copy using ESI so that the upper half of ESI is zeroed like with EBX.
    7c14: 66 89 de                      mov     esi, ebx

00007c17 <INPUTLOOPHEAD>:
    ; Start collecting input.
    ; Get a character from the BIOS interrupt, write it to *SI and then SI++.
    ; AH=0x0, INT 0x16: read ascii from keyboard input
    7c17: b4 00                         mov     ah, 0x0
    7c19: cd 16                         int     0x16
    7c1b: 88 04                         mov     byte ptr [si], al
    7c1d: 46                            inc     si

00007c1e <INPUT_CHECKBKSP>:
    ; Check for backspace and do si -= 2 if backspace.
    7c1e: 3c 08                         cmp     al, 0x8
    7c20: 75 02                         jne     0x7c24 <INPUT_CHECKCRLF>
    7c22: 4e                            dec     si
    7c23: 4e                            dec     si

00007c24 <INPUT_CHECKCRLF>:
    ; AH=0xE, INT 0x10: Echo whatever we got (still in AL).
    7c24: b4 0e                         mov     ah, 0xe
    7c26: cd 10                         int     0x10
    ; If it was a CR character, replace it with LF and then echo it again.
    7c28: 3c 0d                         cmp     al, 0xd
    7c2a: 75 04                         jne     0x7c30 <INPUT_CHECKESC>
    7c2c: b0 0a                         mov     al, 0xa
    7c2e: cd 10                         int     0x10

00007c30 <INPUT_CHECKESC>:
    ; Check for ESC, break out of the input loop if we find ESC.
    7c30: 3c 1b                         cmp     al, 0x1b
    7c32: 75 e3                         jne     0x7c17 <INPUTLOOPHEAD>

00007c34 <handoff>:
    ; Start getting ready to hand off to 32-bit mode.
    7c34: fa                            cli
    ; Load our custom descriptor table.
    7c35: 0f 01 16 45 7c                lgdtw   [0x7c45]
    ; Set the parts of CR0 that we need to set to switch to 32-bit mode.
    7c3a: b8 11 00                      mov     ax, 0x11
    7c3d: 0f 01 f0                      lmsw    ax
    ; Long jump to our 32-bit code.
    7c40: ea 61 7c 08 00                ljmp    0x8, 0x7c61

00007c45 <.gdtd>:
    ; Data, not code.
    ; Note that this gdtd is deliberately shorter than you would think it needs to be.
    ; Its null bytes are allowed to overlap with the other data near it, so we do that.
    7c45: 17 00 49 7c

00007c49 <.gdt>:
    ; Data, not code.
    7c49: 00 00 00 00 00 00 00 00
          ff ff 00 00 00 9b cf 00 ; code (0x08)
          ff ff 00 00 00 93 cf 00 ; data (0x10)


MODE: 32-bit x86

00007c61 <procode>:
    ; We are now in 32-bit protected mode. We have one more thing to do: set the data segment.
    ; Set EDI to 2*EBX, i.e. 0x10010. Note that this is not actually a memory operation despite using the [] syntax.
    7c61: 8d 3c 1b                      lea     edi, [ebx + ebx]
    ; Now, we set the data segment, which is only 16-bit, to EDI. This sets it to 0x0010, pointing it at our data (0x10) segment.
    7c64: 8e df                         mov     ds, edi
    ; Also copy EDI to EDX.
    7c66: 89 fa                         mov     edx, edi
    ; Now, EDI and EDX point at 0x10010, which is where some interpreter internal data is, and EBX points at 0x8008, where our source code is.
    ; EDI is the macro table. EDX is a base copy for comparison (temporarily. EDX will be general-purpose later).

00007c68 <setup_0>:
    ; Set ESI to 0x400000, where we put even MORE interpreter internal data -- it is the label table.
    7c68: be 00 00 40 00                mov     esi, 0x400000

00007c6d <process_labels_1>:
    ; This is the start of the interpreter code proper. We have a couple preprocessing passes to run.
    ; Spill a copy of EBX (source pointer), we are about to loop over the source text.
    7c6d: 53                            push    ebx

00007c6e <check_individual_2>:
    ; Start of a loop.
    ; In this loop we find labels and log them. We also log macro locations.
    ; Find the next character value.
    7c6e: 8a 03                         mov     al, byte ptr [ebx]
    7c70: 43                            inc     ebx
    ; If we don not run into a #, skip to the next iteration of the loop.
    7c71: 3c 23                         cmp     al, 0x23
    7c73: 75 14                         jne     0x7c89 <check_next_or_end_4>

00007c75 <process_label_3>:
    ; Log macro location. We log even invalid macro names; they will not be searched for.
    7c75: 89 1f                         mov     dword ptr [edi], ebx
    7c77: 83 c7 04                      add     edi, 0x4
    ; What is the next character? Is it a number?
    7c7a: 8a 03                         mov     al, byte ptr [ebx]
    7c7c: 04 d0                         add     al, -0x30
    7c7e: 3c 09                         cmp     al, 0x9
    ; If not a number, go to next iteration of the loop.
    7c80: 77 07                         ja      0x7c89 <check_next_or_end_4>
    ; If yes a number, read its value.
    ; _readint returns into ECX. Set ((uint32_t *)ESI)[ECX*2] to the current source location, after the label.
    ; We now have a LUT of label values to source locations in ESI.
    7c82: 66 e8 d8 00                   call    0x7d5e <_readint>
    7c86: 89 1c ce                      mov     dword ptr [esi + 8*ecx], ebx

00007c89 <check_next_or_end_4>:
    ; If we found an ESC, exit the loop.
    7c89: 80 3b 1b                      cmp     byte ptr [ebx], 0x1b
    7c8c: 75 e0                         jne     0x7c6e <check_individual_2>

    ; EDI now contains a pointer to the "top" of the macro/label table, while EDX contains the "bottom".
    
00007c8e <done_processing_labels_5>:
    ; Unspill and respill source code address. We have another loading pass to run.
    7c8e: 5b                            pop     ebx
    7c8f: 53                            push    ebx

00007c90 <check_macro_loop_6>:
    ; Start of a loop.
    ; In this loop we log whether any given location in the source file corresponds to a macro call or not.
    ; Doing this as a pre-pass is a LOT faster than doing it in the middle of the interpreter loop.
    ; Stop the loop if we run into ESC.
    ; EBX contains a "scan pointer".
    7c90: 80 3b 1b                      cmp     byte ptr [ebx], 0x1b
    7c93: 74 2a                         je      0x7cbf <done_processing_labels_11>
    ; Spill a copy of the macro table if we are still in the loop.
    7c95: 57                            push    edi

00007c96 <handle_apply_macro_loop_7>:
    ; This is the start of an inner loop.
    ; Did we run out of candidate macros to test? If yes, exit the inner loop.
    7c96: 39 d7                         cmp     edi, edx
    7c98: 76 21                         jbe     0x7cbb <handle_apply_macro_continue_10>
    ; Prepare to check the next macro down the macro table.
    7c9a: 83 c7 fc                      add     edi, -0x4
    ; ECX will be our copy of the source location the macro points to, we call it the macro pointer.
    7c9d: 8b 0f                         mov     ecx, dword ptr [edi]
    ; EBP is a temp value (not a base pointer) now containing a copy of our scan pointer.
    7c9f: 89 dd                         mov     ebp, ebx

00007ca1 <handle_apply_macro_loop_inner_8>:
    ; This is the start of an inner-inner loop.
    ; Compare the text behind the scan pointer and macro pointer, character by character.
    7ca1: 8a 01                         mov     al, byte ptr [ecx]
    7ca3: 8a 65 00                      mov     ah, byte ptr [ebp]
    ; Did we hit a control character? If yes, jump to a whitespace test. That is our check for a valid macro usage.
    7ca6: 3c 21                         cmp     al, 0x21
    7ca8: 7c 08                         jl      0x7cb2 <handle_apply_macro_loop_inner_8+0x11> ; aka <WHITESPACE_TEST>
    ; Any other difference? If yes, exit.
    7caa: 38 e0                         cmp     al, ah
    7cac: 75 e8                         jne     0x7c96 <handle_apply_macro_loop_7>
    ; Still the same? Check the next character.
    7cae: 41                            inc     ecx
    7caf: 45                            inc     ebp
    7cb0: eb ef                         jmp     0x7ca1 <handle_apply_macro_loop_inner_8>
; <WHITESPACE_TEST>:
    ; Is the other pointer also a control character? If not, break out of the inner-inner loop and do the next iteration of the inner loop.
    7cb2: 80 fc 20                      cmp     ah, 0x20
    7cb5: 7f df                         jg      0x7c96 <handle_apply_macro_loop_7>

00007cb7 <handle_apply_macro_found_9>:
    ; OK, we can log this as a valid macro lookup. Nice.
    ; Log it at ((uint32_t *)ESI)[scan_pointer*2 + 1]
    ; Yes, this is a horrible waste of memory. But it is VERY simple.
    7cb7: 89 4c de 04                   mov     dword ptr [esi + 8*ebx + 0x4], ecx

00007cbb <handle_apply_macro_continue_10>:
    ; Unspill the macro table pointer.
    ; From now on, EDI is no longer the macro table.
    ; Instead, it is the top of a "breadcrumb pile" for macro evaluation -- basically a function return address table.
    ; Yep, our macros are secretly functions. But they are not stack-hygienic, so calling them macros makes sense too.
    7cbb: 5f                            pop     edi
    ; Check the next source location as our next scan pointer.
    7cbc: 43                            inc     ebx
    7cbd: eb d1                         jmp     0x7c90 <check_macro_loop_6>

00007cbf <done_processing_labels_11>:
    ; Finally out of the loop. Unspill the original source location.
    7cbf: 5b                            pop     ebx

00007cc0 <interpreter_loop_13>:
    ; Our prep work is done. Interpreter time!!!
    ; This is the satrt of the core interpreter loop.
    7cc0: 8a 03                         mov     al, byte ptr [ebx]
    7cc2: 43                            inc     ebx

00007cc3 <check_ifgoto_14>:
    ; Check for ? which is our if-goto operator.
    7cc3: 3c 3f                         cmp     al, 0x3f
    7cc5: 75 0b                         jne     0x7cd2 <check_macroend_17>

00007cc7 <handle_ifgoto_test_15>:
    ; Handle ? -- pop the top of the stack, check if truthy...
    7cc7: 59                            pop     ecx
    7cc8: 58                            pop     eax
    7cc9: 85 c9                         test    ecx, ecx
    7ccb: 74 f3                         je      0x7cc0 <interpreter_loop_13> ; to interpreter loop head

00007ccd <handle_ifgoto_followed_16>:
    ; ... and then jump to the label described by the next value on the stack if it is.
    7ccd: 8b 1c c6                      mov     ebx, dword ptr [esi + 8*eax]
    7cd0: eb ee                         jmp     0x7cc0 <interpreter_loop_13> ; to interpreter loop head

00007cd2 <check_macroend_17>:
    ; Check for ; which is our end-of-macro/function operator
    7cd2: 3c 3b                         cmp     al, 0x3b
    7cd4: 75 07                         jne     0x7cdd <check_space_19>

00007cd6 <handle_macroend_18>:
    ; Handle it: return to the location described by the last entry in the macro breadcumb pile.
    7cd6: 83 c7 fc                      add     edi, -0x4
    7cd9: 8b 1f                         mov     ebx, dword ptr [edi]
    7cdb: eb 3e                         jmp     0x7d1b <handle_primitives_done_36> ; to interpreter loop head

00007cdd <check_space_19>:
    ; Skip any whitespace or control characters.
    7cdd: 3c 20                         cmp     al, 0x20
    7cdf: 76 3a                         jbe     0x7d1b <handle_primitives_done_36> ; to interpreter loop head

00007ce1 <check_primitives_20>:
    ; Check for operational primitives, which are guarded by @ for simpler machine code.
    7ce1: 3c 40                         cmp     al, 0x40
    7ce3: 75 38                         jne     0x7d1d <check_getc_37>

00007ce5 <handle_primitives_21>:
    ; See, these all read from the stack. So we can combine their first pop.
    ; We also read the next character in the source text.
    7ce5: 8a 03                         mov     al, byte ptr [ebx]
    7ce7: 43                            inc     ebx
    7ce8: 59                            pop     ecx

00007ce9 <check_set_22>:
    ; If we found @- which is SET (4-byte memory write)
    7ce9: 3c 77                         cmp     al, 0x77
    7ceb: 75 03                         jne     0x7cf0 <check_sub_24>

00007ced <handle_set_23>:
    7ced: 5a                            pop     edx
    7cee: 89 11                         mov     dword ptr [ecx], edx

00007cf0 <check_sub_24>:
    ; If we found @- which is SUB
    7cf0: 3c 2d                         cmp     al, 0x2d
    7cf2: 75 03                         jne     0x7cf7 <check_and_26>

00007cf4 <handle_sub_25>:
    7cf4: 29 0c 24                      sub     dword ptr [esp], ecx

00007cf7 <check_and_26>:
    ; If we found @& which is AND
    7cf7: 3c 26                         cmp     al, 0x26
    7cf9: 75 03                         jne     0x7cfe <check_mul_28>

00007cfb <handle_and_27>:
    7cfb: 21 0c 24                      and     dword ptr [esp], ecx

00007cfe <check_mul_28>:
    ; If we found @* which is MUL
    7cfe: 3c 2a                         cmp     al, 0x2a
    7d00: 75 05                         jne     0x7d07 <check_read_30>

00007d02 <handle_mul_29>:
    7d02: 5a                            pop     edx
    7d03: 0f af d1                      imul    edx, ecx
    7d06: 52                            push    edx

00007d07 <check_read_30>:
    ; If we found @r which is READ
    7d07: 3c 72                         cmp     al, 0x72
    7d09: 75 02                         jne     0x7d0d <check_callstackpos_32>

00007d0b <handle_read_31>:
    7d0b: ff 31                         push    dword ptr [ecx]

00007d0d <check_callstackpos_32>:
    ; If we found @C which is an evil metaprogramming instruction
    7d0d: 3c 43                         cmp     al, 0x43
    7d0f: 75 01                         jne     0x7d12 <check_div_34>

00007d11 <handle_callstackpos_33>:
    7d11: 57                            push    edi

00007d12 <check_div_34>:
    ; If we found @/ which is DIV
    7d12: 3c 2f                         cmp     al, 0x2f
    7d14: 75 05                         jne     0x7d1b <handle_primitives_done_36>

00007d16 <handle_div_35>:
    7d16: 58                            pop     eax
    7d17: 99                            cdq
    7d18: f7 f9                         idiv    ecx
    7d1a: 50                            push    eax

00007d1b <handle_primitives_done_36>:
    ; OK, that was all of our primitives. Go to the head of the interpreter.
    7d1b: eb a3                         jmp     0x7cc0 <interpreter_loop_13> ; to interpreter loop head

00007d1d <check_getc_37>:
    ; Check for , which is the "getc" operation
    7d1d: 3c 2c                         cmp     al, 0x2c
    7d1f: 75 0d                         jne     0x7d2e <check_comment_40>

00007d21 <handle_getc_38>:
    ; Check PS/2 keyboard controller (handles USB keyboards too)
    ; See: https://wiki.osdev.org/I8042_PS/2_Controller
    7d21: e4 64                         in      al, 0x64
    7d23: 88 c4                         mov     ah, al
    7d25: a8 01                         test    al, 0x1
    ; Do not read from 0x60 if not ready. This is a bugfix.
    7d27: 74 02                         je      0x7d2b <__SKIPNOTREADY39>
    7d29: e4 60                         in      al, 0x60

00007d2b <__SKIPNOTREADY39>:
    7d2b: 50                            push    eax
    7d2c: eb 2e                         jmp     0x7d5c <handle_apply_macro_search_45+0x11> ; to interpreter loop head

00007d2e <check_comment_40>:
    ; Skip any comments indicated by # until the end of that line.
    ; This is also responsible for skipping function/macro and label declarations.
    7d2e: 3c 23                         cmp     al, 0x23
    7d30: 75 08                         jne     0x7d3a <check_consuming_loop_terms_42>

00007d32 <handle_comment_skip_41>:
    ; Skip until a \n specifically.
    7d32: 80 3b 20                      cmp     byte ptr [ebx], 0x20
    7d35: 7c 25                         jl      0x7d5c <handle_apply_macro_search_45+0x11> ; to interpreter loop head
    7d37: 43                            inc     ebx
    7d38: eb f8                         jmp     0x7d32 <handle_comment_skip_41>

00007d3a <check_consuming_loop_terms_42>:
    ; Okay, time for the last two checks. Is this a number? Or a macro?
    7d3a: 4b                            dec     ebx

00007d3b <check_num_43>:
    ; If we found a number...
    7d3b: 04 d0                         add     al, -0x30
    7d3d: 3c 09                         cmp     al, 0x9
    7d3f: 77 08                         ja      0x7d49 <handle_num_44+0x8>

00007d41 <handle_num_44>:
    ; ... read it, then push it to the stack.
    7d41: 66 e8 19 00                   call    0x7d5e <_readint>
    7d45: 4b                            dec     ebx
    7d46: 51                            push    ecx
    7d47: eb 13                         jmp     0x7d5c <handle_apply_macro_search_45+0x11> ; to interpreter loop head
    
    ; Spill macro invokation start source location into EAX.
    7d49: 89 d8                         mov     eax, ebx

00007d4b <handle_apply_macro_search_45>:
    ; Loop: find the end of the name of the macro we are invoking, at the invokation location. They end at whitespace or control chars.
    7d4b: 80 3b 21                      cmp     byte ptr [ebx], 0x21
    7d4e: 7c 03                         jl      0x7d53 <handle_apply_macro_search_45+0x8>
    7d50: 43                            inc     ebx
    7d51: eb f8                         jmp     0x7d4b <handle_apply_macro_search_45>
    ; Drop the location of the end of the macro invokation name onto the macro breadcrumb pile.
    7d53: 89 1f                         mov     dword ptr [edi], ebx
    7d55: 83 c7 04                      add     edi, 0x4
    ; Look up and go to the location in the macro usage table we built in the second preprocessing pass.
    7d58: 8b 5c c6 04                   mov     ebx, dword ptr [esi + 8*eax + 0x4]
    7d5c: eb bd                         jmp     0x7d1b <handle_primitives_done_36> ; to interpreter loop head

; This is our int-reading function.
00007d5e <_readint>:
    7d5e: 31 c9                         xor     ecx, ecx
    7d60: 31 c0                         xor     eax, eax

00007d62 <LOOPHEAD46>:
    ; As long as we are looking at a digit, accumulate it into ECX.
    ; This function also moves EBX to the end of the number.
    7d62: 6b c9 0a                      imul    ecx, ecx, 0xa
    7d65: 01 c1                         add     ecx, eax
    7d67: 8a 03                         mov     al, byte ptr [ebx]
    7d69: 43                            inc     ebx
    7d6a: 2c 30                         sub     al, 0x30
    7d6c: 3c 0a                         cmp     al, 0xa
    7d6e: 72 f2                         jb      0x7d62 <LOOPHEAD46>
    7d70: 66 c3                         ret
```
