              .public interruptHandler
              .public interruptHandler
              .public continueExecution
              .extern _Zp, registers, handleException, origIRQVector

#ifdef ACIA_6551
ACIA:         .equ    0xde00
#endif

#ifdef FOENIX_JR
UART_BASE:    .equ    0xd630

UART_TRHB:    .equ    UART_BASE
UART_LSR:     .equ    (UART_BASE + 5)

INT_PENDING_REG1: .equ 0x000141
FNX1_INT04_COM1: .equ 0x10

#endif
              .section code
;;; * Break instruction is the software breakpoint
breakHandler:                       ; save to 'registers'
              jsr     saveFrame
              tax                   ; X= actual stack pointer
              sec                   ; adjust stop address for BRK
              lda     registers + 0
              sbc     #2
              sta     registers + 0
              bcs     10$
              dec     registers + 1
10$:          lda     #19           ; stop signal
toMonitor:    sta     zp:_Zp+0
#ifdef __CALYPSI_CORE_65C02__
              stz     zp:_Zp+1
#else
              lda     #0
              sta     zp:_Zp+1
#endif
              txs                   ; set correct stack
              jmp     handleException

;;; * Check for ACIA control C, otherwise call old handler
interruptHandler:
              pha
#ifdef __CALYPSI_CORE_65C02__
              phx
#else
              txa
              phx
#endif
              tsx
              lda     0x103,x
              and     #0x10
              bne     breakHandler

#ifdef ACIA_6551
              lda     ACIA + 1
              and     #8            ; got a character?
              beq     10$           ; no
              lda     ACIA          ; read character
#endif
#ifdef FOENIX_JR
              lda     UART_LSR
              lsr     a             ; data available?
              bcc     10$           ; no
              lda     UART_TRHB     ; read character
              phx
              ldx     #FNX1_INT04_COM1
              stx     INT_PENDING_REG1
              plx
#endif
              cmp     #3            ; Ctrl-C?
              bne     10$           ; no
              tsx
              jsr     saveFrame     ; yes, save registers and stop execution
              tax                   ; X= correct stack pointer
              lda     #2            ; INT (interrupt by Ctrl-C) signal
              bne     toMonitor     ; always jump

10$:
#ifdef __CALYPSI_CORE_65C02__
              plx                   ; restore A and X
#else
              pla
              tax
#endif
              pla
              jmp     (origIRQVector) ; let next in chain handle it

saveFrame:    lda     0x101,x       ; X
              sta     registers + 3
              lda     0x102,x       ; A
              sta     registers + 2
              lda     0x103,x       ; status register
              sta     registers + 6
              lda     0x104,x       ; save PC
              sta     registers + 0
              lda     0x105,x
              sta     registers + 1
              cld
              clc
              txa
              adc     #7
              sta     registers + 5 ; stack pointer
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
