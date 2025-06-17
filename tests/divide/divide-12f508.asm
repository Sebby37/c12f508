    processor 12f508
    radix dec
    include "p12f508.inc"

    org 0x00
    goto START

    
quotient    equ 0x07
remainder   equ 0x08
denominator equ 0x09
numerator   equ 0x0A
i           equ 0x0B
i_mask      equ 0x0C
temp        equ 0x0D
rotate_temp equ 0x0E

; Better algorithm I found on Wikipedia
; Repeated subtraction, truly ez as the name suggests
divide_ez:
    movf    numerator,w ; Remainder = Numerator
    movwf   remainder
    clrf    quotient    ; Quotient = 0
    
    divide_ez_loop:
        ; Compute w=r-d
        movf    denominator,w
        subwf   remainder,w
        
        ; If carry clear, r<d, meaning end loop
        btfss   STATUS,C
        goto divide_ez_end
        
        ; Store result
        movwf   remainder
        
        ; Inc quotient
        incf    quotient,f
        
        goto    divide_ez_loop
    divide_ez_end:
    retlw   0

; Weird crap algorithm chatgpt gave me
; Couldn't get it to work, always got 11/3 = 2r2
; Maybe I implemented it wrong though, idk
; I'll prolly remove it once I commit it, so it exists in history and all
divide:
    clrf quotient  ; Quotient = 0
    clrf remainder ; Remainder = 0

    movlw 7
    movwf i ; i = 7
    movlw 128
    movwf i_mask ; stores 1 << i (starts at 128 since i starts at 7)
    divide_loop:
        bcf STATUS,C
        rlf remainder,f  ; remainder << 1

        ; numerator >> i
        movf numerator,w
        movwf rotate_temp ; rotate_temp = numerator
        movf i,w
        movwf temp ; [temp = i]
        rotate_loop:
            bcf STATUS,C
            rrf rotate_temp,f ; [rotate_temp <<= 1]
            decfsz temp,f ; [if temp == 0: break]
            goto rotate_loop
        
        ; remainder |= (numerator >> i) & 1
        movf rotate_temp,w
        andlw 1
        iorwf remainder,f

        ; remainder >= denominator     movf denominator       subwf remainder,w
        btfss STATUS,C        
        goto divide_loop_end ; Goto end of loop if remainder < denominator      ; Remainder = remainder - denominatornveniently already there from >= check :)
        movwf remainder
        ; quotient |= (1 << i)
        movf i_mask,w
        iorwf quotient,f

        divide_loop_end:
            bcf STATUS,C
            rrf i_mask,f ; rotate backwards since i decreases
            decfsz i,f ; i -= 1
            goto divide_loop
    retlw 0 ; quotient/remainder are in their specific registers

START:
    movlw   11 ; Numerator
    movwf   numerator
    movlw   3  ; Denominator
    movwf   denominator
    call    divide_ez
    sleep
    
    end