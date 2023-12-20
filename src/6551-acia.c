// 6551 ACIA SwiftLink style for the Commodore 64

#include <stdint.h>
#include "api.h"
#include <calypsi/intrinsics6502.h>

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

void initializeTarget(void)
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

  // baud rate generator, 8n1, 19200 in standard and 38400 in SwiftLink mode
  ACIA_CONTROL = 0x1f;

#ifdef COMMODORE_64
  static const char signon[] = "CALYPSI DEBUG AGENT ($A000-$BFFF ROM)         SWIFTLINK/6551 VER 1.0.0";
  char* p = (char*)(0x400 + 40 + 1);
  for (uint8_t i = 0; signon[i] != 0; ++i)
    {
      char c = signon[i];
      if (c >= 'A')
        *p++ = c - 64;
      else
        *p++ = c;
    }

  // Change border to pink
  *(char*)53280 = 4;
#endif

  ACIA_DATA = 'A';
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
