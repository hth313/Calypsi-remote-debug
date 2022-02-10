// 6551 ACIA SwiftLink style for C64

#include "api.h"
#include <intrinsics6502.h>

#define ACIA_BASE      0xde00
#define ACIA_DATA      *(volatile char*)(ACIA_BASE + 0)
#define ACIA_STATUS    *(volatile char*)(ACIA_BASE + 1)
#define ACIA_COMMAND   *(volatile char*)(ACIA_BASE + 2)
#define ACIA_CONTROL   *(volatile char*)(ACIA_BASE + 3)

extern void interruptHandler();
extern void breakHandler();

#define IRQ_VECTOR   (*(void**)0x314)
#define BREAK_VECTOR (*(void**)0x316)

static void *origBreakVector;
void *origIRQVector;

void initialize(void)
{
  // Debugger agent runs with interrupts disabled.
  __disable_interrupts();

  // Preserve original vectors
  origBreakVector = BREAK_VECTOR;
  origIRQVector = IRQ_VECTOR;

  // Set our own interrupt handler. This is used to handle Ctrl-C
  IRQ_VECTOR = interruptHandler;

  // Insert our own BRK vector
  BREAK_VECTOR = breakHandler;

  ACIA_CONTROL = 0x1f;   // baud rate generator, 8n1, 19200
}

void enableSerialInterrupt (void)
{
  ACIA_COMMAND = 0;
}

void disableSerialInterrupt (void)
{
  ACIA_COMMAND = 2;
}

char getDebugChar(void)
{
  while ((ACIA_STATUS & 8) == 0)
    ;
  return ACIA_DATA;
}

void putDebugChar(char c)
{
  while ((ACIA_STATUS & 0x10) == 0)
    ;
  ACIA_DATA = c;
}
