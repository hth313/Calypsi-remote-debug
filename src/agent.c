// agent.c - debugger agent using gdbserver protocol

// This code is somewhat based on remcom.c or m68k-stub.c which
// HP has placed in the public domain. It has been reworked here but
// some stuff is defintely recognizable.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "api.h"

#ifndef DEBUG
#define DEBUG 0
#endif

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
#define BUFMAX 400

/************************************************************************/
/* BREAKPOINTS defines the maximum number of software (code modifying breakpoints */
#define BREAKPOINTS 25

/************************************************************************/

#ifdef __CALYPSI_TARGET_6502__
#include <intrinsics6502.h>

typedef struct {
  uint16_t pc;
  uint8_t  a;
  uint8_t  x;
  uint8_t  y;
  uint8_t  sp;
  uint8_t  sr;
} register_t;

// Breakpoint address type */
typedef uint8_t * address_t;
typedef uint8_t backing_t;
typedef char * memory_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_6502__

/************************************************************************/

#ifdef __CALYPSI_TARGET_65816__
#include <intrinsics65816.h>

typedef struct {
  uint16_t a;                       //  0
  uint16_t x;                       //  2
  uint16_t y;                       //  4
  uint16_t sp;                      //  6
  uint16_t dp;                      //  8
  uint8_t  bank;                    // 10
  uint8_t  sr;                      // 11
  uint32_t pc;                      // 12
} register_t;

// Breakpoint address type */
typedef __far uint8_t * address_t;
typedef uint8_t backing_t;
typedef __far char * memory_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_65816__

/************************************************************************/

#ifdef __CALYPSI_TARGET_M68K__
#include <intrinsics65816.h>

typedef struct {
  uint32_t d[8];                    //  0
  uint32_t a[7];                    // 32
  uint32_t ssp;                     // 60
  uint32_t usp;                     // 64
  uint32_t pc;                      // 68
  uint16_t sr;                      // 72
} register_t;

// Breakpoint address type */
typedef uint16_t * address_t;
typedef uint16_t backing_t;
typedef char * memory_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_M68K__

/************************************************************************/

static const char hexchars[]="0123456789abcdef";

// This is the register buffer
register_t registers;

#if DEBUG
int     remote_debug;
#endif

/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

/************* jump buffer used for setjmp/longjmp **************************/
jmp_buf remcomEnv;

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

typedef struct breakpoint {
  address_t address; // address of breakpoint
  bool active;       // set when breakpoint active
  backing_t store;   // store modified bytes in code
} breakpoint_t;

static breakpoint_t breakpoint[BREAKPOINTS];

static unsigned breakpointCount = 0;

static void insertBreakpoints ()
{
  unsigned sofar = 0;
  for (unsigned i = 0; sofar < breakpointCount; i++)
    {
      if (breakpoint[i].active)
        {
          backing_t *p = breakpoint[i].address;
          breakpoint[i].store = *p;
          *p = BREAK_OPCODE;
          sofar++;
        }
    }
}

static void removeBreakpoints ()
{
  unsigned sofar = 0;
  for (unsigned i = 0; sofar < breakpointCount; i++)
    {
      if (breakpoint[i].active)
        {
          backing_t *p = breakpoint[i].address;
          *p = breakpoint[i].store;
          sofar++;
        }
    }
}

int hex (char ch)
{
  if ((ch >= 'a') && (ch <= 'f'))
    return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9'))
    return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F'))
    return (ch - 'A' + 10);
  return (-1);
}

/* scan for the sequence $<data>#<checksum>     */
char *getpacket (void)
{
  char *buffer = remcomInBuffer;
  uint8_t checksum;
  uint8_t xmitcsum;
  unsigned count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar()) != '$')
        ;

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
        {
          ch = getDebugChar();
          if (ch == '$')
            goto retry;
          if (ch == '#')
            break;
          checksum = checksum + ch;
          buffer[count] = ch;
          count = count + 1;
        }
      buffer[count] = 0;

      if (ch == '#')
        {
          ch = getDebugChar();
          xmitcsum = hex(ch) << 4;
          ch = getDebugChar();
          xmitcsum += hex(ch);

          if (checksum != xmitcsum)
            {
#if DEBUG
              if (remote_debug)
                {
                  fprintf(stderr,
                          "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
                          checksum, xmitcsum, buffer);
                }
#endif
              putDebugChar('-');       /* failed checksum */
            }
          else
            {
              putDebugChar('+');       /* successful transfer */

              /* if a sequence char is present, reply the sequence ID */
              if (buffer[2] == ':')
                {
                  putDebugChar(buffer[0]);
                  putDebugChar(buffer[1]);

                  return &buffer[3];
                }

              return &buffer[0];
            }
        }
    }
}

/* send the packet in buffer. */
void putpacket (char *buffer)
{
  unsigned char checksum;
  int count;
  char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      putDebugChar('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
        {
          putDebugChar(ch);
          checksum += ch;
          count += 1;
        }

      putDebugChar('#');
      putDebugChar(hexchars[checksum >> 4]);
      putDebugChar(hexchars[checksum & 15]);

    }
  while (getDebugChar() != '+');
}

#if DEBUG
void debug_error (char *format, char *parm)
{
  if (remote_debug)
    fprintf(stderr, format, parm);
}
#endif

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
memory_t mem2hex (memory_t mem, char *buf, unsigned count)
{
  int i;
  char ch;
  for (i = 0; i < count; i++)
    {
      ch = *mem++;
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch & 15];
    }
  *buf = 0;
  return buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
memory_t hex2mem (char *buf, memory_t mem, int count)
{
  int i;
  char ch;
  for (i = 0; i < count; i++)
    {
      ch = hex(*buf++) << 4;
      ch = ch + hex(*buf++);
      *mem++ = ch;
    }
  return mem;
}

/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToLongInt (char **ptr, long *value)
{
  int numChars = 0;
  int hexValue;

  *value = 0;

  while (**ptr)
    {
      hexValue = hex(**ptr);
      if (hexValue >= 0)
        {
          *value = (*value << 4) | hexValue;
          numChars++;
        }
      else
        break;

      (*ptr)++;
    }

  return numChars;
}

static void signalPacket (unsigned sigval)
{
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] = hexchars[sigval >> 4];
  remcomOutBuffer[2] = hexchars[sigval & 15];
  remcomOutBuffer[3] = 0;
}

/*
 * This function does all command processing.
 */
void handleException (unsigned sigval)
{
  int stepping;
  unsigned length;
  long lvalue;
  memory_t addr;
  char *ptr;
  int newPC;

  /* reply to host that an exception has occurred */
  signalPacket(sigval);
  putpacket(remcomOutBuffer);

  stepping = 0;

  removeBreakpoints();

  while (1 == 1)
    {
      remcomOutBuffer[0] = 0;
      ptr = getpacket();
      switch (*ptr++)
        {
        case '?':
          signalPacket(sigval);
          break;
        case 'd':
#if DEBUG
          remote_debug = !(remote_debug);       /* toggle debug flag */
#endif
          break;
        case 'g':               /* return the value of the CPU registers */
          mem2hex((memory_t) &registers, remcomOutBuffer, sizeof(registers));
          break;
        case 'G':               /* set the value of the CPU registers - return OK */
          hex2mem(ptr, (memory_t) &registers, sizeof(registers));
          strcpy(remcomOutBuffer, "OK");
          break;

          /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
          if (setjmp(remcomEnv) == 0)
            {
#if defined (__CALYPSI_TARGET_68000__)
              exceptionHandler(2, handle_buserror);
#endif
              /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
              if (hexToLongInt(&ptr, &lvalue))
                {
                  addr = (memory_t) lvalue;
                  if (   *(ptr++) == ','
                      && hexToLongInt(&ptr, &lvalue))
                    {
                      length = lvalue;
                      ptr = 0;
                      mem2hex((memory_t)addr, remcomOutBuffer, length);
                    }
                }
              if (ptr)
                {
                  strcpy(remcomOutBuffer, "E01");
                }
            }
#if defined (__CALYPSI_TARGET_68000__)
          else
            {
              exceptionHandler(2, _catchException);
              strcpy(remcomOutBuffer, "E03");
              debug_error("bus error");
            }

          /* restore handler for bus error */
          exceptionHandler(2, _catchException);
#endif
          break;

          /* ‘X addr,length:XX…’ where XX.. is binary data */
        case 'X':
          /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              addr = (memory_t) lvalue;
              if (   hexToLongInt(&ptr, &lvalue)
                  && *(ptr++) == ':')
                {
                  length = lvalue;
                  char c;
                  while (length >= 0)
                    {
                      c = *ptr++;
                      length--;
                      switch (c)
                        {
                        case 0x23:
                        case 0x24:
                          goto illegal_binary_char;
                        case 0x7d: // escape
                          c = *ptr++ ^ 0x20;
                          length--;
                          break;
                        default:
                          break;
                        }
                      *addr = c;
                      addr++;
                    }
                }
illegal_binary_char:
              ;
            }
            break;

          /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':
          if (setjmp(remcomEnv) == 0)
            {
#if defined (__CALYPSI_TARGET_68000__)
              exceptionHandler(2, handle_buserror);
#endif
              /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
              if (    hexToLongInt(&ptr, &lvalue)
                  && *(ptr++) == ',')
                {
                  addr = (memory_t) lvalue;
                  if (   hexToLongInt(&ptr, &lvalue)
                      &&  *(ptr++) == ':')
                    {
                      length = lvalue;
                      hex2mem(ptr, addr, length);
                      ptr = 0;
                      strcpy(remcomOutBuffer, "OK");
                    }
                }
              if (ptr)
                {
                  strcpy(remcomOutBuffer, "E02");
                }
            }
#if defined (__CALYPSI_TARGET_68000__)
          else
            {
              exceptionHandler(2, _catchException);
              strcpy(remcomOutBuffer, "E03");
#if DEBUG
              debug_error("bus error");
#endif
            }

          /* restore handler for bus error */
          exceptionHandler(2, _catchException);
#endif
          break;

          /* cAA..AA    Continue at address AA..AA(optional) */
          /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
          stepping = 1;
        case 'c':
          /* try to read optional parameter, pc unchanged if no parm */
          if (hexToLongInt(&ptr, &lvalue))
            {
              registers.pc = lvalue;
            }

          newPC = registers.pc;

#if defined (__CALYPSI_TARGET_68000__)
          /* clear the trace bit */
          registers[PS] &= 0x7fff;

          /* set the trace bit if we're stepping */
          if (stepping)
            registers[PS] |= 0x8000;
#endif

          /*
           * look for newPC in the linked list of exception frames.
           * if it is found, use the old frame it.  otherwise,
           * fake up a dummy frame in returnFromException().
           */
#if defined(__CALYPSI_TARGET_68000__)
#if DEBUG
          if (remote_debug)
            printf ("new pc = 0x%x\n", newPC);
#endif
          frame = lastFrame;
          while (frame)
            {
#if DEBUG
              if (remote_debug)
                printf("frame at 0x%x has pc=0x%x, except#=%d\n",
                       frame, frame->exceptionPC, frame->exceptionVector);
#endif
              if (frame->exceptionPC == newPC)
                break;          /* bingo! a match */
              /*
               * for a breakpoint instruction, the saved pc may
               * be off by two due to re-executing the instruction
               * replaced by the trap instruction.  Check for this.
               */
              if ((frame->exceptionVector == 33) &&
                  (frame->exceptionPC == (newPC + 2)))
                break;
              if (frame == frame->previous)
                {
                  frame = 0;    /* no match found */
                  break;
                }
              frame = frame->previous;
            }

          /*
           * If we found a match for the PC AND we are not returning
           * as a result of a breakpoint (33),
           * trace exception (9), nmi (31), jmp to
           * the old exception handler as if this code never ran.
           */
          if (frame)
            {
              if ((frame->exceptionVector != 9) &&
                  (frame->exceptionVector != 31) &&
                  (frame->exceptionVector != 33))
                {
                  /*
                   * invoke the previous handler.
                   */
                  if (oldExceptionHook) {
                    (*oldExceptionHook)(frame->exceptionVector);
                  }
                  newPC = registers[PC];        /* pc may have changed  */
                  if (newPC != frame->exceptionPC)
                    {
#if DEBUG
                      if (remote_debug)
                        printf("frame at 0x%x has pc=0x%x, except#=%d\n",
                               frame, frame->exceptionPC,
                               frame->exceptionVector);
#endif
                      /* re-use the last frame, we're skipping it (longjump?) */
                      frame = (Frame *) 0;
                      _returnFromException(frame);     /* this is a jump */
                    }
                }
            }

          /* if we couldn't find a frame, create one */
          if (frame == 0)
            {
              frame = lastFrame - 1;

              /* by using a bunch of print commands with breakpoints,
                 it's possible for the frame stack to creep down.  If it creeps
                 too far, give up and reset it to the top.  Normal use should
                 not see this happen.
               */
              if ((unsigned int) (frame - 2) < (unsigned int) &gdbFrameStack)
                {
                  initializeRemcomErrorFrame ();
                  frame = lastFrame;
                }
              frame->previous = lastFrame;
              lastFrame = frame;
              frame = 0;        /* null so _return... will properly initialize it */
            }

          insertBreakpoints();
          _returnFromException(frame);   /* this is a jump */
#else
          // For targets without exception frames, insert user
          // breakpoints, restore registers and start execution.
          insertBreakpoints();
          continueExecution(&registers);  /* this is a jump */
#endif

          break;

          /* kill the program */
        case 'k':               /* do nothing */
          break;

          /* ‘Z0,addr,kind insert a breakpoint */
        case 'Z':
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              address_t bpAddress = (address_t) lvalue;
              if (hexToLongInt(&ptr, &lvalue))
                {
//                int kind = lvalue;
                  for (unsigned i = 0; i < BREAKPOINTS; i++)
                    {
                      if (!breakpoint[i].active)
                        {
                          breakpoint[i].active = true;
                          breakpoint[i].address = bpAddress;
                          breakpointCount++;
                          strcpy(remcomOutBuffer, "OK");
                          break;
                        }
                    }
                }
            }
          break;

          /* ‘z0,addr,kind remove a breakpoint */
        case 'z':
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              address_t bpAddress = (address_t) lvalue;
              if (hexToLongInt(&ptr, &lvalue))
                {
//                int kind = lvalue;
                  for (unsigned i = 0; i < BREAKPOINTS; i++)
                    {
                      if (   breakpoint[i].active
                          && breakpoint[i].address == bpAddress)
                        {
                          breakpoint[i].active = false;
                          breakpointCount--;
                          strcpy(remcomOutBuffer, "OK");
                          break;
                        }
                    }
                }
            }
          break;

        }                       /* switch */

      /* reply to the request */
      putpacket(remcomOutBuffer);
    }
}

int main ()
{
  initialize();
#if defined(__CALYPSI_TARGET_6502__) || defined(__CALYPSI_TARGET_65816__)
  __break_instruction();
#endif
  return 0;
}
