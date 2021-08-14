;;; Low level exection handling WDC65816

#define UART_BASE 0xaf18f8
#define UART_TRHB UART_BASE
#define UART_LSR  (UART_BASE + 5)

              .public breakHandler, continueExecution, uartInterrupt
              .extern handleException, origIRQVector, registers
              .extern _DirectPageStart
              .section code
breakHandler:
              jsr     saveRegisters
              lda     ##19
toMonitor:    jmp     long:handleException

uartInterrupt:
              pha
              sep     #0x20         ; 8 bits data
              lda     long:UART_LSR
              lsr     a             ; data available?
              bcc     100$          ; no
              lda     long:UART_TRHB
              cmp     #3            ; Ctrl-C?
              bne     100$          ; no, ignore it
              pla
              jsr     saveRegisters
              lda     ##2           ; INT (interrupt by Ctrl-C) signal
              bra     toMonitor
100$:         pla
              jmp     (origIRQVector)

;;; Save registers and set up C state.
saveRegisters:
              rep     #0x30         ; 16 bits data and index mode
              sta     long:registers ; save A

              phb                   ; save data bank
              phb
              pla
              sta     long:registers + 10

              tdc                   ; save DP
              sta     long:registers + 8

              lda     ##_DirectPageStart
              tcd                   ; set direct page

#ifdef __CALYPSI_DATA_MODEL_SMALL__
              lda     ##0
#else
              .extern _NearBaseAddress
              lda     ##.word2 _NearBaseAddress
#endif
              xba
              pha
              plb                   ; pop 8 dummy
              plb                   ; set data bank

              stx     abs:registers + 6 ; save X
              sty     abs:registers + 8 ; save Y

              lda     3,s
              sta     abs:registers + 11 ; save status register

              tsc
              inc     a
              inc     a
              sta     abs:registers + 6 ; save SP

              lda     4,s           ; save PC
              sta     abs:registers + 12
              lda     6,s
              sta     abs:registers + 14
              rts

continueExecution:
              lda registers + 10
              inc     a
              tcs                   ; set stack pointer

              lda     registers + 15 ; status register
              sta     2,s

              lda     registers + 2 ; PC upper part
              xba
              sta     4,s
              lda     registers + 0 ; PC lower part
              sta     3,s

              lda     registers + 13 ; data bank
              sta     0,s
              plb

              lda     registers + 12 ; direct page
              tcd
              ldy     registers + 8
              ldx     registers + 6
              lda     registers + 4
              rti
