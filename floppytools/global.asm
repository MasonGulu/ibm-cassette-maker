%ifndef GLOBAL

;;;;;;;;;;;;;;;;;;;;;;; Credit for this code goes to J. Bogin ;;;;;;;;;;;;;;;;;;;;;;;
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
	ret
;;;;;;;;;;;;;;;;;;;;;;; End of code from J. Bogin ;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;; print an 8 bit hex number ( 2 digits ) ;;;;;;;;;;;;;;;;;;;;;;;
SUB_print8bithex:
	; This expects the 8 bit number in al
    ; performs NO register preservation
	push ax
	
	and ax, 0x00F0
	mov cl, 4
	shr ax, cl	; shr ax, 4 is only valid on the 80186 and later, so this is a workaround
	add al, '0'	; align 0 with '0'
	cmp al, '9'	
	jle _print8bithex_skipaddMS
	add al, ('A'-'9'-1) ; If digit is larger than '9' then add the offset required to make 10 align with 'A'
    _print8bithex_skipaddMS:
        mov ah, 0x0E
        mov bh, 0
        
        int 0x10

        pop ax
        and ax, 0x000F
        add al, '0'
        cmp al, '9'
        jle _print8bithex_skipaddLS
        add al, ('A'-'9'-1)
    _print8bithex_skipaddLS:
        mov ah, 0x0E
        mov bh, 0
        
        int 0x10
        ret

STRING_return       db 13,10,0
STRING_disk_info    db 'Head:Track ',0
DATA_errcount       db 0
DATA_disk:

%define GLOBAL
%endif