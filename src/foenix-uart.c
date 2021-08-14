// C256 Foenix U/U+ UART
#include "api.h"
#include <intrinsics65816.h>

extern void *breakHandler;
extern void *uartInterrupt;

#ifdef SYS_C256_FMX
/* Base address for UART 1 (COM1) */
#define UART_BASE 0xaf13f8
/* Base address for UART 2 (COM2) */
//#define UART2_BASE 0xaf12f8
#else
/* Base address for UART 1 (COM1) in the C256 Foenix U (only 1 Serial port) */
#define UART_BASE 0xaf18f8
//#define UART2_BASE 0xaf18f8
#endif

#define UART_TRHB *(volatile __far char*)(UART_BASE + 0)    // Transmit/Receive Hold Buffer
#define UART_IER  *(volatile __far char*)(UART_BASE + 1)    // Interupt Enable Register
#define UART_FCR  *(volatile __far char*)(UART_BASE + 2)    // FIFO Control Register
#define UART_LSR  *(volatile __far char*)(UART_BASE + 5)    // Line Status Register

#define UINT_DATA_AVAIL 1       // Enable Recieve Data Available interupt

#define LSR_XMIT_EMPTY 0x20     // Empty transmit holding register

#define BRK_VECTOR    *(__far void**) 0x00ffe6
#define IRQ_VECTOR    *(__far void**) 0x00ffee

void *origIRQVector;
void *origBRKVector;

void initialize(void)
{
  // Debugger agent runs with interrupts disabled.
  __disable_interrupts();

  // Preserve original UART1 interrupt vector
  origIRQVector = IRQ_VECTOR;

  // Set our own interrupt handler. This is used to handle Ctrl-C
  IRQ_VECTOR = uartInterrupt;

  // Preserve original BRK vector
  origBRKVector = BRK_VECTOR;

  // Insert our own BRK vector
  BRK_VECTOR = breakHandler;

  // no need to set speed and 8n1 for U/U+ UART
  UART_FCR = 0b11100001; // enable FIFO

  // Enable interrupt when receiving data
  UART_IER = UINT_DATA_AVAIL;
}

char getDebugChar(void)
{
  while ((UART_LSR & UINT_DATA_AVAIL) == 0)
    ;
  return UART_TRHB;
}

void putDebugChar(char c)
{
  while ((UART_LSR & LSR_XMIT_EMPTY) == 0)
    ;
  UART_TRHB = c;
}
