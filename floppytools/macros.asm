
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