// 6551 ACIA SwiftLink style for C64

#include "api.h"
#include <intrinsics6502.h>

#define ACIA_BASE      0xde00
#define ACIA_DATA      *(volatile char*)(ACIA_BASE + 0)
#define ACIA_STATUS    *(volatile char*)(ACIA_BASE + 1)
#define ACIA_COMMAND   *(volatile char*)(ACIA_BASE + 2)
#define ACIA_CONTROL   *(volatile char*)(ACIA_BASE + 3)

extern void* interrupt_handler;
extern void* break_handler;

#define IRQ_VECTOR   (*(void**)0x314)
#define BREAK_VECTOR (*(void**)0x316)

static void *origBreakVector;
void *origIRQVector;

void initialize(void)
{
  __interrupt_state_t st = __get_interrupt_state();
  __disable_interrupts();

  // Preserve original vectors
  origBreakVector = BREAK_VECTOR;
  origIRQVector = IRQ_VECTOR;

  // Set our own interrupt handler. This is used to handle Ctrl-C
  IRQ_VECTOR = interrupt_handler;

  // Insert our own BRK vector
  BREAK_VECTOR = break_handler;

  ACIA_CONTROL = 0x1f;   // baud rate generator, 8n1, 19200
  ACIA_COMMAND = 0;      // no interrupts, no parity

  __restore_interrupt_state(st);
}

char getDebugChar(void)
{
  while ((ACIA_STATUS & 8) == 0)
    ;
  return ACIA_DATA;
}

void putDebugChar(char c)
{
  while ((ACIA_STATUS & 0x10) != 0)
    ;
  ACIA_DATA = c;
}

void enableCtrlC(void)
{
  // Enable interrupt when receiving data
  ACIA_COMMAND = 2;
}
