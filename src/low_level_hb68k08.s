#define USART_BASE  0x0E0000
#define UDR         USART_BASE + 0x2f
              .section nearcode
              .extern registers, handleException
              .public illegalHandler, continueExecution, traceHandler

;;; BKPT and malformed intructions come here
illegalHandler:
traceHandler:
              bsr.s   saveRegisters
              moveq.l #19,d0
toMonitor:    jmp     handleException

uartInterrupt:
              move.w  d0,-(sp)
              move.b  UDR.l,d0
              cmp.b   #3,d0         ; Ctrl-C ?
              beq.s   10$
              move.w  (sp)+,d0      ; no, ignore it
              rte
10$:          move.w  (sp)+,d0
              bsr.s   saveRegisters
              moveq.l #2,d0         ; INT (interrupt by Ctrl-C signal)
              bra.s   toMonitor

saveRegisters:
              movem.l d0-d7/a0-a7,registers.l
              move.l  usp,a0
              move.l  a0,registers+64.l
              move.l  4(sp),registers+68.l
              move.w  6(sp),registers+72.l
              rts

continueExecution:
              move.l  registers+64.l,a0
              move.l  a0,usp
              movem.l registers.l,d0-d7/a0-a7
              move.l  registers+68.l,(sp)
              move.w  registers+72.l,2(sp)
              rte
