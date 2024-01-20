cpu 8086
org 0

%include "macros.asm"

SUB_write:
    pusha
    mov ax, cs
    mov ds, ax
    mov es, ax
_write_start:
    lea si, [STRING_write_welcome]
    call SUB_print
    mov ax, 0
    int 16h
    ; Wait for keyboard input
    cmp ah, 1
    je _write_exit
    ; exit on escape
    
    mov ah, 0
    mov dl, 0
    int 13h
    ; Reset disk system
    mov ch, 0
    mov cl, 0
    ; ch contains track number
    ; cl bits [6:7] are bits [8:9] of the track number
    _write_write_track_loop:
        ; load the data off cassette, only needs done once as long as it works.
        mov ax, 0
        mov [DATA_errcount], al
        ; reset error count
        pusha
        mov ax, 0
        int 13h
        ; reset disk system
        _write_load_cassette:
            push cx ; keep the current sector intact
            mov al, ch
            call SUB_print8bithex
            
            lea si, [STRING_write_play]
            call SUB_print

            mov ax, 0
            ;int 16h
            ; wait for keyboard input
            cmp ah, 1
            je _write_load_cassette_exit

            mov ah, 0
            int 15h
            ; Turn on cassette motor
            
            mov ah, 2
            lea bx, [DATA_disk]
            mov cx, 8192
            ; two tracks, one track on each side
            int 15h
            ; read
            pushf ; preserve state of carry flag
            push ax ; preserve error code

            mov ah, 1
            int 15h
            ; Turn off cassette motor

            pop ax ; revert error code
            popf ; revert state of carry flag
            pop cx ; keep the current sector intact
            jc _write_load_cassette_error
            ; error occured
            ; no error occured
            popa
        mov ax, 0
        mov [DATA_errcount], al
        ; reset error count
    _write_write_track:
        mov ah, 3
        mov dl, 0
        ; drive 0
        mov dh, 0
        ; head 0
        mov cl, 1
        ; sector 1
        ; ch is track number, that's managed externally
        mov al, 8
        ; sectors to write
        lea bx, [DATA_disk]
        printTrackInfo
        int 13h
        jc _write_track_error
        ;;;;;;; Head 0

        ;;;;;;; Head 1
        mov ah, 3
        mov dh, 1
        ; head 1
        mov cl, 1
        ; sector 1
        ; ch is track number, that's managed externally
        mov al, 8
        ; sectors to write
        lea bx, [DATA_disk+4096]
        printTrackInfo
        int 13h
        jc _write_track_error
        
        inc ch
        ; go to next track
        cmp ch, 40
        je _write_exit
        ; unless we're at the end of the disk
        
        jmp _write_write_track_loop
    
    _write_track_error:
        pusha
        mov al, ah
        call SUB_print8bithex
        mov al, [DATA_errcount]
        inc al
        mov [DATA_errcount], al
        cmp al, 4
        jge _write_track_fail
        lea si, [STRING_write_error]
        call SUB_print
        mov ah, 0
        mov dl, 0
        int 13h
        popa
        jmp _write_write_track

    _write_track_fail:
        popa
        lea si, [STRING_write_fail]
        call SUB_print
        mov ax, 0
        int 16h
        ; Wait for keyboard input
        cmp ah, 1
        je _write_exit

        jmp _write_start


_write_load_cassette_error_quit:
    popa
    popa
    popa
    retf
_write_load_cassette_exit:
    pop cx
    popa
_write_exit:
    popa
    retf



_write_load_cassette_error:
    pusha
    mov al, ah
    call SUB_print8bithex
    lea si, [STRING_write_caserr]
    call SUB_print
    mov ax, 0
    int 16h
    cmp ah, 1
    je _write_load_cassette_error_quit
    popa
    jmp _write_load_cassette


STRING_write_welcome      db 'Push any key to write image to disk. Escape to cancel.',13,10,0
STRING_write_play         db ' < expecting track.',13,10,0
STRING_write_caserr       db ' | An error occurred reading from cassette. Push any key to retry. Escape to cancel.',7,13,10,0
STRING_write_error        db ' | A track failed to write, retrying',13,10,0
STRING_write_fail         db ' | The disk cannot be used, insert a different disk and push any key. Escape to cancel.',7,13,10,0

%include "global.asm"