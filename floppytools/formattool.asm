cpu 8086
org 0


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

SUB_format:
    pusha
    mov ax, cs
    mov ds, ax
    mov es, ax
_format_start:
    lea si, [STRING_welcome]
    call SUB_print
    mov ax, 0
    int 16h
    ; Wait for keyboard input
    cmp ah, 1
    je _format_exit
    ; exit on escape
    mov ax, 0
    mov [DATA_errcount], al
    ; reset error count
    mov ah, 0
    mov dl, 0
    int 13h
    ; Reset disk system
    mov ch, 0
    mov cl, 0
    ; ch contains track number
    ; cl bits [6:7] are bits [8:9] of the track number
    pusha
    mov bx, 0
    ; offset
    jmp _format_format_track_increment_table
    _format_format_track:
        mov ah, 05h
        mov al, 8
        ; ah = 05h
        ; al # of sectors to format, nevermind. IBM bios does not support this.
        mov dh, 0
        mov dl, 0
        ; dh head number
        ; dl drive number, 0=A
        lea bx, [DATA_head0]
        ; es:bx is pointer to track data fields
        printTrackInfo
        cli
        int 13h
        sti
        jc _format_track_error
        ; format

        ; verify
        mov ah, 02
        mov al, 8
        ; sectors to read
        mov cl, 1
        ; cl sector
        lea bx, [DATA_disk]
        ; es:bx is pointer to memory buffer
        cli
        int 13h
        sti
        jc _format_track_error
        ;;;;;;; Head 0

        ;;;;;;; Head 1
        mov dh, 1
        ; head number
        mov ah, 05h
        ; int parameter (format)
        mov al, 8
        ; sectors to format
        lea bx, [DATA_head1]
        printTrackInfo
        cli
        int 13h
        sti
        jc _format_track_error
        ; format

        ; verify
        mov ah, 02
        ; ah = 02h
        mov al, 8
        ; sectors to read
        mov cl, 1
        ; cl sector
        lea bx, [DATA_disk]
        ; es:bx is pointer to memory buffer
        cli
        int 13h
        sti
        jc _format_track_error

        inc ch
        cmp ch, 40
        je _format_exit

        pusha
        mov bx, 0 ; offset
        _format_format_track_increment_table:
            ; This is a loop that will copy the current track to all entries
            ; in the format data table
            mov [DATA_head0+bx], ch
            mov [DATA_head1+bx], ch
            add bx, 4
            cmp bx, 35
            jg _format_format_track_increment_finished
            jmp _format_format_track_increment_table
        _format_format_track_increment_finished:
            mov ax, 0
            mov [DATA_errcount], al
            popa
            jmp _format_format_track
    
    _format_track_error:
        pusha
        mov al, ah
        call SUB_print8bithex
        mov al, [DATA_errcount]
        inc al
        mov [DATA_errcount], al
        cmp al, 4
        jge _format_track_fail
        lea si, [STRING_error]
        call SUB_print
        mov ah, 0
        mov dl, 0
        int 13h
        popa
        jmp _format_format_track

    _format_track_fail:
        popa
        lea si, [STRING_fail]
        call SUB_print
        mov ax, 0
        int 16h
        ; Wait for keyboard input
        cmp ah, 1
        je _format_exit

        mov ch, 0
        mov cl, 0
        pusha
        mov bx, 0
        ; offset
        jmp _format_format_track_increment_table
        ; jump in and use this part of the subroutine to set all the track numbers back to 0

_format_exit:
    popa
    retf




STRING_welcome      db 'Push any key to format disk A. Escape to cancel.',13,10,0
STRING_error        db ' | A track failed to write, retrying',13,10,0
STRING_fail         db ' | The disk cannot be used, insert a different disk and push any key. Escape to cancel.',7,13,10,0

DATA_head0          db 0,0,1,2 ; track, head, sector, sector size code
                    db 0,0,2,2
                    db 0,0,3,2
                    db 0,0,4,2
                    db 0,0,5,2
                    db 0,0,6,2
                    db 0,0,7,2
                    db 0,0,8,2
                    db 0,0,9,2

DATA_head1          db 0,1,1,2 ; track, head, sector, sector size code
                    db 0,1,2,2
                    db 0,1,3,2
                    db 0,1,4,2
                    db 0,1,5,2
                    db 0,1,6,2
                    db 0,1,7,2
                    db 0,1,8,2
                    db 0,1,9,2

%ifndef GLOBAL
    %define GLOBAL
    %include "global.asm"
%endif