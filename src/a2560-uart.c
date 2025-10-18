#include <stdint.h>
#include <foenix.h>

extern void breakHandler();
extern void interruptHandler();

void initializeTarget(void)
{
  // Set speed
  UART.lcr |= DIVISOR_LATCH_ENABLE;
  UART.dll = BAUD_115200 & 255;
  UART.dlm = BAUD_115200 >> 8;

  // Set mode, also disables divisor latch
  UART.lcr = NO_PARITY | STOP_BITS_1 | WORD_LENGTH_8;

  // Enable and reset FIFO
  UART.fcr = 0b100111;

  UART.ier = 0;   // polled mode used when in control

  UART.mcr = MCR_DTR | MCR_RTS;
}

void enableSerialInterrupt (void)
{
  UART.ier = INTERRUPT_ENABLE_RECEIVE;
  UART.mcr |= MCR_OUT2;
  InterruptController.mask.gavin &= ~INT_COM1;
}

void disableSerialInterrupt (void)
{
  UART.ier = 0;
  UART.mcr &= ~MCR_OUT2;
  InterruptController.mask.gavin |= INT_COM1;
}

char getDebugChar(void)
{
  while ((UART.lsr & RECEIVE_DATA_READY) == 0)
    ;
  return UART.data;
}

void putDebugChar(char c)
{
  while ((UART.lsr & TRANSMIT_HOLD_EMPTY) == 0)
    ;
  UART.data = c;
}
