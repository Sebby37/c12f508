    processor 12f508
    radix dec
    include "p12f508.inc"

    __CONFIG _OSC_IntRC & _WDT_OFF & _CP_OFF & _MCLRE_ON

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

; Better algorithm I found on Wikipedia (Repeated Subtraction)
divide:
    movf    numerator,w ; Remainder = Numerator
    movwf   remainder
    clrf    quotient    ; Quotient = 0
    
    divide_loop:
        ; Compute w=r-d
        movf    denominator,w
        subwf   remainder,w
        
        ; If carry clear, r<d, meaning end loop
        btfss   STATUS,C
        goto divide_end
        
        ; Store result
        movwf   remainder
        
        ; Inc quotient
        incf    quotient,f
        
        goto    divide_loop
    divide_end:
    retlw   0

START:
    movlw   11 ; Numerator
    movwf   numerator
    movlw   3  ; Denominator
    movwf   denominator
    call    divide
    sleep
    
    end