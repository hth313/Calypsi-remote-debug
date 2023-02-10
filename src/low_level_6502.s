              .public interruptHandler
              .public interruptHandler
              .public continueExecution
              .extern _Zp, registers, handleException, origIRQVector

#ifdef ACIA_6551
ACIA:         .equ    0xde00
#endif

#ifdef __CALYPSI_TARGET_FOENIX__
INT_PENDING_1: .equlab 0xd661
UART_BASE:    .equlab 0xd630

UART_TRHB:    .equlab UART_BASE
UART_LSR:     .equlab (UART_BASE + 5)

//FNX1_INT04_COM1: .equ 0x10

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

#ifdef __CALYPSI_TARGET_FOENIX__
              lda     #1            ; UART interrupt pending?
              bit     INT_PENDING_1
              beq     10$           ; no
              sta     INT_PENDING_1 ; acknowledge interrupt
              bit     UART_LSR      ; is there a character?
              beq     10$           ; no
              lda     UART_TRHB     ; read character
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
              plx                   ; no, pass down the chain
#else
              pla
              tax
#endif
              pla
              jmp     (origIRQVector)

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
