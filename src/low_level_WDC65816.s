;;; Low level exection handling WDC65816

              .public continueExecution, uart_interrupt

              .section code
continueExecution:
              rts


uart_interrupt:
