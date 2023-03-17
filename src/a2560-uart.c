// C256 Foenix U/U+ UART
#include <stdint.h>
#include "api.h"

extern void breakHandler();
extern void interruptHandler();

#define GAVIN 0x00B00000
#define UART_BASE (GAVIN + 0x28f8)

#define UART_300     4167
#define UART_1200    1042
#define UART_2400     521
#define UART_4800     260
#define UART_9600     130
#define UART_19200     65
#define UART_38400     33
#define UART_57600     21
#define UART_115200    10

#define UART_TRHB *(volatile uint8_t*)(UART_BASE + 0)    // Transmit/Receive Hold Buffer
#define UART_IER  *(volatile uint8_t*)(UART_BASE + 1)    // Interupt Enable Register
#define UART_FCR  *(volatile uint8_t*)(UART_BASE + 2)    // FIFO Control Register
#define UART_ISR  *(volatile uint8_t*)(UART_BASE + 2)    // FIFO Control Register
#define UART_LCR  *(volatile uint8_t*)(UART_BASE + 3)    // Line Control Register
#define UART_MCR  *(volatile uint8_t*)(UART_BASE + 4)    // Modem Control Register
#define UART_LSR  *(volatile uint8_t*)(UART_BASE + 5)    // Line Status Register

#define UART_DLL  UART_TRHB
#define UART_DLM  UART_IER

#define UINT_DATA_AVAIL 1       // Enable Recieve Data Available interupt

#define LSR_XMIT_EMPTY 0x20     // Empty transmit holding register

#define LCR_PARITY_NONE 0
#define LCR_STOPBIT_1   0
#define LCR_DATABITS_8  3
#define LCR_DLB         0x80

#define MCR_DTR     1
#define MCR_RTS     2
#define MCR_OUT1    4
#define MCR_OUT2    8
#define MCR_TEST   16

#define INT_MASK_REG1 *(volatile uint16_t*)(GAVIN + 0x011a)

#define FNX1_INT04_COM1 0x08

void initialize(void)
{
  // Set speed
  UART_LCR |= LCR_DLB;     // enable divisor latch
  UART_DLL = UART_115200 & 255;
  UART_DLM = UART_115200 >> 8;

  // Set mode, also disables divisor latch
  UART_LCR = LCR_PARITY_NONE | LCR_STOPBIT_1 | LCR_DATABITS_8;

  // Enable and reset FIFO
  UART_FCR = 0b100111;

  UART_IER = 0;   // polled mode used when in control

  UART_MCR = MCR_DTR | MCR_RTS;

  putDebugChar('H');
  putDebugChar('I');
}

void enableSerialInterrupt (void)
{
  UART_IER = UINT_DATA_AVAIL;
  UART_MCR |= MCR_OUT2;
  INT_MASK_REG1 &= ~FNX1_INT04_COM1;
}

void disableSerialInterrupt (void)
{
  UART_IER = 0;
  UART_MCR &= ~MCR_OUT2;
  INT_MASK_REG1 |= FNX1_INT04_COM1;
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
