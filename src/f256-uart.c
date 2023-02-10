// F256 Jr. UART
#include "api.h"
#include <intrinsics6502.h>
#include "foenix/interrupt.h"
#include "foenix/uart.h"

extern void breakHandler();
extern void interruptHandler();

#define LCR_PARITY_NONE 0
#define LCR_STOPBIT_1   0
#define LCR_DATABITS_8  3

#define MCR_DTR     1
#define MCR_RTS     2
#define MCR_OUT1    4
#define MCR_OUT2    8
#define MCR_TEST   16

#define UINT_DATA_AVAIL 1       // Enable Recieve Data Available interupt

#define IRQ_VECTOR    *(void**) 0x00fffe

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
  IRQ_VECTOR = interruptHandler;

  // Set speed
  UART.divisor_latch_enable = 1;     // enable divisor latch
  UART.baud = BAUD_115200;
  UART.divisor_latch_enable = 0;    // disable divisor latch

  // Set mode
  UART.lcr = LCR_PARITY_NONE | LCR_STOPBIT_1 | LCR_DATABITS_8;

  UART.fcr = 0b10000111; // enable FIFO

  UART.ier = 0;   // polled mode used when in control

  UART.mcr = MCR_DTR | MCR_RTS;
}

void enableSerialInterrupt (void)
{
  UART.ier = UINT_DATA_AVAIL;
  UART.op2 = 1;
  InterruptMask.uart = 0;
}

void disableSerialInterrupt (void)
{
  UART.ier = 0;
  UART.op2 = 0;
  InterruptMask.uart = 1;
}

char getDebugChar(void)
{
  while (!UART.receive_data_ready)
    ;
  return UART.data;
}

void putDebugChar(char c)
{
  while (!UART.transmit_hold_empty)
    ;
  UART.data = c;
}
