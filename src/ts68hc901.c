#include <stdint.h>
#include "api.h"
#include <intrinsics68000.h>

#define USART_BASE  0x0E0000
#define UDR  *(uint8_t*)(USART_BASE + 0x2f) // data register
#define UCR  *(uint8_t*)(USART_BASE + 0x29) // control register
#define RSR  *(uint8_t*)(USART_BASE + 0x2b) // receiver status register
#define TSR  *(uint8_t*)(USART_BASE + 0x2d)

#define TCDCR *(uint8_t*)(USART_BASE + 0x1d)

#define TCDR *(uint8_t*)(USART_BASE + 0x23)
#define TDDR *(uint8_t*)(USART_BASE + 0x25)

#define GPIP *(uint8_t*)(USART_BASE + 0x01)
#define IERA *(uint8_t*)(USART_BASE + 0x07)
#define IERB *(uint8_t*)(USART_BASE + 0x09)
#define IPRA *(uint8_t*)(USART_BASE + 0x0b)
#define IPRB *(uint8_t*)(USART_BASE + 0x0d)
#define ISRA *(uint8_t*)(USART_BASE + 0x0f)
#define ISRB *(uint8_t*)(USART_BASE + 0x11)
#define IMRA *(uint8_t*)(USART_BASE + 0x13)

#define VR *(uint8_t*)(USART_BASE + 0x17)

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

void initializeTarget(void)
{
  // disable all interrupts
  uint8_t *p = &GPIP;
  for (int i = 0; i < 12; i++, p += 2) {
    *p = 0;
  }

  // 7.3728 MHz / 2 with Timer C & D -> 57600 bit/s
  TCDR = (uint8_t) (B57600 >> 8);
  TDDR = (uint8_t) B57600;
  TCDCR = 0x11;   //   div by 4

  UCR = DIV16 | ASYNC; // 8 bits, no parity, 1 stop, async, div by 16
  RSR = 1;             // enable receiver
  TSR = 1;             // enable transmitter

  // TS68HC901 assign interrupts 64-95, UART receive is 64 + 12 = 76
  VR = 0x40;
}

void enableSerialInterrupt (void)
{
  IERA |= 0x10;
  IMRA |= 0x10;
}

void disableSerialInterrupt (void)
{
  IERA &= ~0x10;
  IMRA &= ~0x10;
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
