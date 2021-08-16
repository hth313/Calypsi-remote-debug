              .public interruptHandler
              .public breakHandler
              .public continueExecution
              .extern _Zp, registers, handleException, origIRQVector

#define ACIA 0xde00

              .section code
;;; * Break instruction is the software breakpoint
breakHandler:                       ; save to 'registers'
              jsr     saveFrame
              lda     #19           ; stop signal
toMonitor:    sta     zp:_Zp+0
              lda     #0
              sta     zp:_Zp+1
              jmp     handleException

;;; * Check for ACIA control C, otherwise call old handler
interruptHandler:
              lda     ACIA + 1
              and     #8            ; got a character?
              beq     10$           ; no
              lda     ACIA          ; read character
              cmp     #3            ; Ctrl-C?
              bne     10$           ; no
              jsr     saveFrame     ; yes, save registers and stop execution
              lda     #2            ; INT (interrupt by Ctrl-C) signal
              bne     toMonitor
10$:          jmp     (origIRQVector) ; let next in chain handle it

saveFrame:    txa                   ; A=sp
              cld
              clc
              adc     #6            ; A=actual SP
              sta     registers + 5 ; save SP

              lda     0x104,x       ; save status register
              sta     registers + 6

              lda     0x101,x       ; save Y
              sta     registers + 4

              lda     0x102,x       ; save X
              sta     registers + 3

              lda     0x103,x       ; save A
              sta     registers + 2

              lda     0x105,x       ; save PC
              sta     registers + 0
              lda     0x106,x
              sta     registers + 1
              rts

continueExecution:
              ldx     registers + 5 ; get stack pointer
              dex                   ; make room for PC and status register
              dex
              dex
              txs                   ; set stack pointer
              lda     registers + 0 ; prepare PC
              sta     0x102,x
              lda     registers + 1
              sta     0x103,x
              lda     registers + 6 ; prepare status register
              sta     0x101,x
              ldx     registers + 3 ; X
              ldy     registers + 4 ; Y
              lda     registers + 2 ; A
              rti
