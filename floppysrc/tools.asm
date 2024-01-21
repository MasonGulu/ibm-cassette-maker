    .model    small
    .code

    extern    test_:near
    extern    _disk_base:near
    global    entry:near
    global    print_:near
    global    read_:near
    global    format_:near
    global    reset_:near
    global    verify_:near
entry:

    push    ds
    push    es

    mov    ax, cs
    mov    ds, ax
    mov    es, ax

    mov    bx, ss
    mov    old_ss, bx
    mov    old_sp, sp

    mov    ss, ax
    mov    sp, stack_top

    call    test_

    mov    ax, old_ss
    mov    ss, ax
    mov    sp, old_sp

    pop    es
    pop    ds

    retf

print_ proc near
push si
mov si, ax
SUB_print:
    ; Print a 0-terminated ASCII string in DS:SI
	mov ax,0E00h
	lodsb				; byte[DS:SI] => AL
	cmp al,00			; Terminating character
	je _SUB_print_printfinished
	push bp				; I had an ancient machine that destroyed these two!!!
	push si
	int 10h				; Blurp it out
	pop si
	pop bp
	jmp SUB_print
_SUB_print_printfinished:
    pop si
	ret
print_ endp

read_ proc near
    mov ax, 0
    int 16h
    ret
read_ endp

format_ proc near
    ; char sectors, format_loc loc, format_desc *info, char track
    push es

    push ax
    mov ax, cs
    mov es, ax ; set ES to current CS
    pop ax

    mov ah, 5
    mov ch, cl
    mov cl, 0
    int 13h
    mov al, ah ; return char

    pop es
    ret
format_ endp

; AX, DX, BX, CX register parameter order, for some reason????
verify_ proc near
    ; char sectors, format_loc loc, format_desc *info, char track
    mov ah, 4
    mov ch, cl ; track #
    mov cl, 1 ; start at sector 1
    ; es:bx is pointer to memory buffer, not really sure what this is used for
    int 13h
    adc al, 0 ; return success char
    ret
verify_ endp

reset_ proc near
    push dx
    mov dx, ax
    mov ax, 0
    int 13h
    pop dx
    ret
reset_ endp

    .data?

old_ss    dw    ?
old_sp    dw    ?

    stack segment 'stack'
    
    db    200 dup (?)
stack_top:
    stack ends

    end