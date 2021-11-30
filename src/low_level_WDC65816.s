;;; Low level exection handling WDC65816

#define UART_BASE 0xaf18f8
#define UART_TRHB UART_BASE
#define UART_LSR  (UART_BASE + 5)

#define INT_PENDING_REG1 0x000141
#define FNX1_INT04_COM1 0x10

              .public breakHandler, continueExecution, uartInterrupt
              .extern handleException, origIRQVector, registers
              .extern _DirectPageStart
              .section code
breakHandler:
              jsr     saveRegisters
              dec     abs:registers + 12 ; adjust stop address with BRK
              dec     abs:registers + 12
              lda     ##19
toMonitor:    jmp     handleException

uartInterrupt:
              sep     #0x20         ; 8 bits data
              pha
              lda     long:UART_LSR
              lsr     a             ; data available?
              bcc     100$          ; no
              lda     long:UART_TRHB
              xba
              lda     #FNX1_INT04_COM1
              sta     long:INT_PENDING_REG1
              xba
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

              stx     abs:registers + 2 ; save X
              sty     abs:registers + 4 ; save Y

              lda     3,s           ; save status register and low pc
              sta     abs:registers + 11

              tsc
              clc
              adc     ##6           ; correct SP
              sta     abs:registers + 6 ; save SP

              lda     5,s           ; save rest of PC
              sta     abs:registers + 13
              rts

;;; Prepare target for executing the code we are debugging.
continueExecution:
              lda     abs:registers + 6
              sec
              sbc     ##5           ; reserve space for RTI frame and bank
              tcs                   ; set stack pointer

              lda     abs:registers + 9 ; data bank
              sta     0,s

              lda     abs:registers + 11 ; status register (and PC)
              sta     2,s

              lda     abs:registers + 13 ; more PC
              sta     4,s

              ldy     abs:registers + 4
              ldx     abs:registers + 2

              lda     abs:registers + 8 ; load target direct page
              tcd
              lda     abs:registers + 0
              plb                   ; load target data bank
              rti
