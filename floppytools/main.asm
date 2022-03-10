; This is the main file that will be assembled.
; formattool.asm and writetool.asm can function as individual programs, but this is going to be a simple menu wrapper.

cpu 8086
org 0

%define GLOBAL

%ifndef MACROS
    %macro pusha 0
    push ax
    push bx
    push cx
    push dx
    push ds
    push es
    pushf
    %endmacro

    %macro popa 0
    popf
    pop es
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    %endmacro

    %macro printTrackInfo 0
        ; ch = track
        ; cl = sector
        ; dh = head
        ; dl = drive
        pusha
        push cx
        push dx
        ; print out track info
        lea si, [STRING_disk_info]
        call SUB_print

        pop dx
        mov al, dh
        call SUB_print8bithex
        ; print out head number

        pop cx
        mov al, ch
        call SUB_print8bithex
        ; print out track number

        lea si, [STRING_return]
        call SUB_print
        ; print a new line

        popa
    %endmacro

    %define MACROS
%endif

SUB_main:
    pusha
    mov ax, cs
    mov ds, ax
    mov es, ax

_main_setup:
    mov ax, 0x0003
    int 10h
    ; clear screen

    lea si, [STRING_main_menu]
    call SUB_print
_main_loop:
    mov ax, 0
    int 16h

    cmp ah, 1
    je _main_exit
    ; escape

    cmp al, '1'
    je _main_format

    cmp al, '2'
    je _main_write

    jmp _main_loop

_main_format:
    push cs
    mov ax, _main_setup
    push ax 
    jmp SUB_format

_main_write:
    push cs
    mov ax, _main_setup
    push ax
    jmp SUB_write

_main_exit:
    popa
    retf

STRING_main_menu    db 'Disk tools, written by Mason Gulu.',13,10
                    db '[1] Format',13,10
                    db '[2] Write Image',13,10
                    db '[ESC] Quit',13,10,0

%include "writetool.asm"
%include "formattool.asm"

%include "global.asm"