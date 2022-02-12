#include <stdint.h>
#include "api.h"
#include <intrinsics68000.h>

#define USART_BASE  0x0E0000
#define IERA *(uint8_t*)(USART_BASE + 0x07)
#define UDR  *(uint8_t*)(USART_BASE + 0x2f) // data register
#define UCR  *(uint8_t*)(USART_BASE + 0x29) // control register
#define RSR  *(uint8_t*)(USART_BASE + 0x2b) // receiver status register
#define TSR  *(uint8_t*)(USART_BASE + 0x2d)

#define TCDR *(uint16_t*)(USART_BASE + 0x23)

#define GPIP *(uint32_t*)(USART_BASE + 0x01)
#define IERB *(uint32_t*)(USART_BASE + 0x09)
#define ISRB *(uint32_t*)(USART_BASE + 0x11)

#define B57600    0x0101
#define B28800    0x0202
#define B19200    0x0303
#define B14400    0x0404
#define B9600     0x0606
#define B4800     0x0c0c
#define B2400     0x1818
#define B1200     0x3030

#define DIV16     0x80
#define ASYNC     0x08

void initialize(void)
{
#if 0
  GPIP = 0;
  IERB = 0;            // disable all interrupts
  ISRB = 0;
#endif
  TCDR = B57600;       // 7.3728 MHz / 2 with Timer C & D -> 57600 bit/s
  UCR = DIV16 | ASYNC; // 8 bits, no parity, 1 stop, async, div by 16
  RSR = 1;             // enable receiver
  TSR = 1;             // enable transmitter
}

void enableSerialInterrupt (void)
{
  IERA |= 0x10;
}

void disableSerialInterrupt (void)
{
  IERA &= ~0x10;
}

char getDebugChar(void)
{
  while ((RSR & 0x80) == 0)
    ;
  return UDR;
}

void putDebugChar(char c)
{
  while ((TSR & 0x80) == 0)
    ;
  UDR = c;
}
