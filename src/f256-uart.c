// F256 Jr. UART
#include "api.h"
#include <calypsi/intrinsics65816.h>
#include <f256.h>

extern void breakHandler();
extern void interruptHandler();

#define UINT_DATA_AVAIL 1       // Enable Recieve Data Available interupt

#define IRQ_VECTOR    *(void**) 0x00fffe

void *origIRQVector;
void *origBRKVector;

void initializeTarget(void)
{
  // Debugger agent runs with interrupts disabled.
  __disable_interrupts();

  // Preserve original UART1 interrupt vector
  origIRQVector = IRQ_VECTOR;

  // Set our own interrupt handler.
  // This is used to handle Ctrl-C after allowing target program to
  // continue execution, otherwise communication is polled.
  IRQ_VECTOR = interruptHandler;

  // Set speed
  UART.lcr |= DIVISOR_LATCH_ENABLE;     // enable divisor latch
  UART.baud = BAUD_115200;
  UART.lcr &= ~DIVISOR_LATCH_ENABLE;    // disable divisor latch

  // Set mode
  UART.lcr = NO_PARITY | STOP_BITS_1 | WORD_LENGTH_8;

  UART.fcr = 0b10000111; // enable FIFO

  UART.ier = 0;   // polled mode used when in control

  UART.mcr = MCR_DTR | MCR_RTS;
}

void enableSerialInterrupt (void)
{
  UART.ier = INTERRUPT_ENABLE_RECEIVE;
  UART.mcr |= MCR_OUT2;
  InterruptController.interrupt_mask.uart = 0;
}

void disableSerialInterrupt (void)
{
  UART.ier = 0;
  UART.mcr &= ~MCR_OUT2;
  InterruptController.interrupt_mask.uart = 1;
}

char getDebugChar(void)
{
  while ((UART.lsr & receive_data_ready) == 0)
    ;
  return UART.data;
}

void putDebugChar(char c)
{
  while ((UART.lsr & transmit_hold_empty) == 0)
    ;
  UART.data = c;
}
