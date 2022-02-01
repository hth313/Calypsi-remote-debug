#include <stdint.h>
#include "api.h"
#include <intrinsics68000.h>

#define USART_BASE  0x0E0000
#define IERA *(uint8_t*)(USART_BASE + 0x07)
#define UDR  *(uint8_t*)(USART_BASE + 0x2f) // data register
#define UCR  *(uint8_t*)(USART_BASE + 0x29) // control register
#define RSR  *(uint8_t*)(USART_BASE + 0x2b) // receiver status register
#define TSR  *(uint8_t*)(USART_BASE + 0x2d)

void initialize(void)
{
  UCR = 0;  // 8 bits, not parity
  RSR = 1;  // enable receiver
  TSR = 1;  // enable transmitter
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
