#ifdef HB68K08
#define USART_BASE  0x0E0000
#define UDR         USART_BASE + 0x2f
#endif

#define BREAK_OPCODE 0x4848

              .section nearcode
              .extern registers, handleException, computeSignal
              .public illegalHandler, continueExecution, traceHandler
              .public _catchException, _debug_level7

;;; BKPT and malformed intructions come here
illegalHandler:
              movem.l d0/a0,-(sp)
              move.l  10(sp),a0
              move.w  (a0),d0
              cmp.w   #BREAK_OPCODE,d0
              beq     10$
              movem.l (sp)+,d0/a0
              move.l  #exceptionTable + (4 + 1) * 6, -(sp)
              bra.s   _catchException
10$:          movem.l (sp)+,d0/a0
traceHandler:
              bsr.s   saveRegisters
              moveq.l #19,d0
toMonitor:    jmp     handleException

uartInterrupt:
              move.w  d0,-(sp)
#ifdef HB68K08
              move.b  UDR.l,d0
#endif
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
saveRegisters10:
              move.l  usp,a0
              move.l  a0,registers+64.l
              move.l  6(sp),registers+68.l
              move.w  4(sp),registers+72.l
              rts

continueExecution:
              move.l  registers+64.l,a0
              move.l  a0,usp
              movem.l registers.l,d0-d7/a0-a7
              move.l  registers+68.l,2(sp)
              move.w  registers+72.l,(sp)
              rte

_catchException:
              movem.l d0-d7/a0-a7,registers.l
              move.l  (sp)+,d0
              sub.l   #exceptionTable,d0
              bsr.s   saveRegisters10
              jsr     computeSignal
              bra.s   toMonitor

;;; * This function is called immediately when a level 7 interrupt occurs
;;; * if the previous interrupt level was 7 then we're already servicing
;;; * this interrupt and an rte is in order to return to the debugger.
_debug_level7:
              move.w  d0,-(sp)
              move.w  2(sp),d0
              and.w   #0x700,d0
              cmp.w   #0x700,d0
              beq     10$           ; already servicing level 7
              move.w  (sp)+,d0
              bra.s   _catchException
10$:          move.w  (sp)+,d0
              addq.l  #4,sp
              rte

;;; **** Vector relay table.
;;; The real vector table points to entries in this table. Each entry
;;; is 6 bytes and the default is 'jsr _catchException' which allows
;;; the _catchException routine to calculate the vector based on the
;;; return address pushed on the stack.
;;; There is no entry for the first two vectors, as we do not take over
;;; the reset handling in this way.
              .section vectors6, noinit
              .public exceptionTable
exceptionTable:
              .space  (256 - 2) * 6
