// C256 Foenix U/U+ UART
#include "api.h"
#include <intrinsics65816.h>

extern void breakHandler();
extern void uartInterrupt();

#ifdef __TARGET_C256_FMX__
/* Base address for UART 1 (COM1) */
#define UART_BASE 0xaf13f8
/* Base address for UART 2 (COM2) */
#define UART2_BASE 0xaf12f8
#define UART_300      384
#define UART_1200      96
#define UART_2400      48
#define UART_4800      24
#define UART_9600      12
#define UART_38400      3
#define UART_115200     1
#else
/* Base address for UART 1 (COM1) in the C256 Foenix U (only 1 Serial port) */
#define UART_BASE 0xaf18f8
#define UART2_BASE UART_BASE

// The UART in the C256/U uses the CPU clock (14.318MHz).
// While not an ideal clock for standard RS232 speeds,
//it is serviceable
#define UART_300   2983
#define UART_600   1491
#define UART_1200   746
#define UART_2400   373
#define UART_4800   186
#define UART_9600    93
#define UART_19200   47
#define UART_38400   23
#define UART_57600   16
#define UART_115200   8
#endif

#define UART_TRHB *(volatile __far char*)(UART_BASE + 0)    // Transmit/Receive Hold Buffer
#define UART_DL   *(volatile __far unsigned short*)(UART_BASE + 0)
#define UART_IER  *(volatile __far char*)(UART_BASE + 1)    // Interupt Enable Register
#define UART_FCR  *(volatile __far char*)(UART_BASE + 2)    // FIFO Control Register
#define UART_LCR  *(volatile __far char*)(UART_BASE + 3)    // Line Control Register
#define UART_LSR  *(volatile __far char*)(UART_BASE + 5)    // Line Status Register

#define UINT_DATA_AVAIL 1       // Enable Recieve Data Available interupt

#define LSR_XMIT_EMPTY 0x20     // Empty transmit holding register

#define LCR_PARITY_NONE 0
#define LCR_STOPBIT_1   0
#define LCR_DATABITS_8  3
#define LCR_DLB         0x80

#define BRK_VECTOR    *(void**) 0x00ffe6
#define IRQ_VECTOR    *(void**) 0x00ffee

void *origIRQVector;
void *origBRKVector;

void initialize(void)
{
  // Debugger agent runs with interrupts disabled.
  __disable_interrupts();

  // Preserve original UART1 interrupt vector
  origIRQVector = IRQ_VECTOR;

  // Set our own interrupt handler.
  // This is used to handle Ctrl-C after allowing target program to
  // continue execution, otherwise communication is polled.
  IRQ_VECTOR = uartInterrupt;

  // Preserve original BRK vector
  origBRKVector = BRK_VECTOR;

  // Insert our own BRK vector
//  BRK_VECTOR = breakHandler;

  // Set speed
  UART_LCR |= LCR_DLB;     // enable divisor latch
  UART_DL = UART_115200;
  UART_LCR &= ~LCR_DLB;    // disable divisor latch

  // Set mode
  UART_LCR = LCR_PARITY_NONE | LCR_STOPBIT_1 | LCR_DATABITS_8;

//  UART_FCR = 0b11100001; // enable FIFO
  UART_FCR = 0b10000111; // enable FIFO

  // Enable interrupt when receiving data
//  UART_IER = UINT_DATA_AVAIL;
  UART_IER = 0;   // polled mode used when in control
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
