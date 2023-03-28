// agent.c - debugger agent using gdbserver protocol

// This code is somewhat based on remcom.c or m68k-stub.c which
// HP has placed in the public domain. It has been reworked here but
// some stuff is defintely recognizable.

#include <stdbool.h>
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

uint8_t  sr_save;

#define I_BIT   4

// Address type pointing to memory bytes.
typedef uint8_t * address_t;

// Breakpoint address type */
typedef uint8_t * bpaddress_t;
typedef uint8_t backing_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_6502__

/************************************************************************/

#ifdef __CALYPSI_TARGET_65816__
#include <intrinsics65816.h>

#ifdef __CALYPSI_TARGET_SYSTEM_FOENIX__
// Cannot have the math unit enabled as we may debug code that
// uses it and such code must be atomic.
#pragma GCC error "do not compile with Foenix math unit enabled!"
#endif

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

uint8_t  sr_save;

#define I_BIT   4

// Address type pointing to memory bytes.
typedef __far uint8_t * address_t;

// Breakpoint address type */
typedef __far uint8_t * bpaddress_t;
typedef uint8_t backing_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_65816__

/************************************************************************/

// Ensure __CALYPSI_TARGET_68000__ is defined when deprecated
// __CALYPSI_TARGET_M68K__ is given.
#if defined(__CALYPSI_TARGET_M68K__) && !defined(__CALYPSI_TARGET_68000__)
#define __CALYPSI_TARGET_68000__
#endif

#ifdef __CALYPSI_TARGET_68000__

#include <intrinsics68000.h>

#ifdef __CALYPSI_TARGET_SYSTEM_FOENIX__
// Cannot have the math unit enabled as we may debug code that
// uses it and such code must be atomic.
#pragma GCC error "do not compile with Foenix math unit enabled!"
#endif

typedef struct {
  uint32_t d[8];                    //  0
  uint32_t a[7];                    // 32
  uint32_t ssp;                     // 60
  uint32_t usp;                     // 64
  uint32_t pc;                      // 68
  uint16_t sr;                      // 72
} register_t;

// Address type pointing to memory bytes.
typedef uint8_t * address_t;

// Breakpoint address type */
typedef uint16_t * bpaddress_t;
typedef uint16_t backing_t;

#define BREAK_OPCODE 0x4848

#ifdef __CALYPSI_CORE_68020__
/* the size of the exception stack on the 68020 varies with the type of
 * exception.  The following table is the number of WORDS used
 * for each exception format.
 */
static const short exceptionSize[] = { 4,4,6,4,4,4,4,4,29,10,16,46,12,4,4,4 };
#endif

#ifdef __CALYPSI_CORE_68332__
static const short exceptionSize[] = { 4,4,6,4,4,4,4,4,4,4,4,4,16,4,4,4 };
#endif

typedef void (*handler_t)(void);

typedef struct vector_entry {
  uint16_t   opcode;
  handler_t  dest;
} vector_entry_t;

extern vector_entry_t exceptionTable[];

extern void _catchException(void);
extern void _debug_level7(void);
extern void traceHandler(void);
extern void illegalHandler(void);
extern void uartInterrupt(void);

#define jsr 0x4eb9
#define jmp 0x4ef9

static void exceptionHandler (unsigned n, uint16_t call, handler_t handler)
{
  // Point to entry in relay table
  vector_entry_t *p = &exceptionTable[n - 2];

  // Set the real vector to point to the relay table
  *(handler_t**)(n * 4) = (handler_t*)p;

  // Set instruction in relay table
  p->opcode = call;
  p->dest   = handler;
}

#endif // __CALYPSI_TARGET_68000__

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
  bpaddress_t address; // address of breakpoint
  bool active;         // set when breakpoint active
  backing_t store  ;   // store modified bytes in code
} breakpoint_t;

static breakpoint_t breakpoint[BREAKPOINTS];

static unsigned breakpointCount = 0;

static breakpoint_t stepBreakpoint = { .active = true };

bool singleStepping = false;

static void insertBreakpoints (breakpoint_t *breakpoint, unsigned count)
{
  unsigned sofar = 0;
  for (unsigned i = 0; sofar < count; i++)
    {
      if (breakpoint[i].active)
        {
          bpaddress_t p = breakpoint[i].address;
          breakpoint[i].store = *p;
          *p = BREAK_OPCODE;
          sofar++;
        }
    }
}

static void removeBreakpoints (breakpoint_t *breakpoint, unsigned count)
{
  unsigned sofar = 0;
  for (unsigned i = 0; sofar < count; i++)
    {
      if (breakpoint[i].active)
        {
          *breakpoint[i].address = breakpoint[i].store;
          sofar++;
        }
    }
}

static void clearAllBreakpoints ()
{
  for (unsigned i = 0; i < BREAKPOINTS; i++)
    {
      breakpoint[i].active = false;
    }
  breakpointCount = 0;
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
          checksum += ch;
          putDebugChar(ch);
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
address_t mem2hex (address_t mem, char *buf, unsigned count)
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
  return (address_t) buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
address_t hex2mem (char *buf, address_t mem, size_t count)
{
  int i;
  char ch;

#if defined (__CALYPSI_TARGET_68000__)
  long vector4 = *(long*) (4 * 4);
  long vector9 = *(long*) (9 * 4);
#if defined(A2560U)
  long vector0x43 = *(long*) (0x43 * 4);
#endif
#if defined(HB68K08)
  long vector76 = *(long*) (76 * 4);
#endif
#endif

  for (i = 0; i < count; i++)
    {
      ch = hex(*buf++) << 4;
      ch = ch + hex(*buf++);
      *mem++ = ch;
    }
#if defined (__CALYPSI_TARGET_68000__)
  *(long*) (4 * 4) = vector4;
  *(long*) (9 * 4) = vector9;
#if defined(A2560U)
  *(long*) (0x43 * 4) = vector0x43;
#endif
#if defined(HB68K08)
  *(long*) (76 * 4) = vector76;
#endif
#endif

  return mem;
}

/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToLongInt (char **ptr, long *pvalue)
{
  int numChars = 0;
  int hexValue;
  long value = 0;

  while (**ptr)
    {
      hexValue = hex(**ptr);
      if (hexValue >= 0)
        {
          value = (value << 4) | hexValue;
          numChars++;
        }
      else
        break;

      (*ptr)++;
    }

  *pvalue = value;
  return numChars;
}


#if defined (__CALYPSI_TARGET_68000__)
/* a bus error has occurred, perform a longjmp
   to return execution and allow handling of the error */
static void handle_buserror ()
{
  longjmp (remcomEnv, 1);
}

/* this function takes the 68000 exception number expressed as a table
   offset and attempts to translate this number into a unix compatible
   signal value */
int computeSignal (int exceptionVector)
{
#define EXC(n)  ((n - 1) * 6)
  int sigval;
  switch (exceptionVector)
    {
    case EXC(2):
      sigval = 10;
      break;                    /* bus error           */
    case EXC(3):
      sigval = 10;
      break;                    /* address error       */
    case EXC(4):
      sigval = 4;
      break;                    /* illegal instruction */
    case EXC(5):
      sigval = 8;
      break;                    /* zero divide         */
    case EXC(6):
      sigval = 8;
      break;                    /* chk instruction     */
    case EXC(7):
      sigval = 8;
      break;                    /* trapv instruction   */
    case EXC(8):
      sigval = 4;
      break;                    /* privilege violation */
    case EXC(9):
      sigval = 5;
      break;                    /* trace trap          */
    case EXC(10):
      sigval = 4;
      break;                    /* line 1010 emulator  */
    case EXC(11):
      sigval = 4;
      break;                    /* line 1111 emulator  */

      /* Coprocessor protocol violation.  Using a standard MMU or FPU
         this cannot be triggered by software.  Call it a SIGBUS.  */
    case EXC(13):
      sigval = 10;
      break;

    case EXC(31):
      sigval = 2;
      break;                    /* interrupt           */
    case EXC(33):
      sigval = 5;
      break;                    /* breakpoint          */

      /* This is a trap #8 instruction.  Apparently it is someone's software
         convention for some sort of SIGFPE condition.  Whose?  How many
         people are being screwed by having this code the way it is?
         Is there a clean solution?  */
    case EXC(40):
      sigval = 8;
      break;                    /* floating point err  */

    case EXC(48):
      sigval = 8;
      break;                    /* floating point err  */
    case EXC(49):
      sigval = 8;
      break;                    /* floating point err  */
    case EXC(50):
      sigval = 8;
      break;                    /* zero divide         */
    case EXC(51):
      sigval = 8;
      break;                    /* underflow           */
    case EXC(52):
      sigval = 8;
      break;                    /* operand error       */
    case EXC(53):
      sigval = 8;
      break;                    /* overflow            */
    case EXC(54):
      sigval = 8;
      break;                    /* NAN                 */
    default:
      sigval = 7;               /* "software generated" */
    }
  return sigval;
}
#endif

static void signalPacket (unsigned sigval)
{
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] = hexchars[sigval >> 4];
  remcomOutBuffer[2] = hexchars[sigval & 15];
  remcomOutBuffer[3] = 0;
}

#if defined (__CALYPSI_TARGET_68000__)
#define __SINGLE_STEP_CAPABLE_TARGET__ 1
#endif

/*
 * This function does all command processing.
 */
void handleException (unsigned sigval)
{
  size_t length;
  long lvalue;
  address_t addr;
  char *ptr;
  int newPC;

  // Clear any breakpoints placed in the code, adjust registers
  // as needed when single stepping.
  if (singleStepping)
    {
#if !defined __SINGLE_STEP_CAPABLE_TARGET__
      removeBreakpoints(&stepBreakpoint, 1);
#endif
#if defined(__CALYPSI_TARGET_65816__) || defined(__CALYPSI_TARGET_6502__)
      registers.sr = (sr_save & I_BIT) | (registers.sr & ~I_BIT);
#endif
#if defined (__CALYPSI_TARGET_68000__)
          /* clear the trace bit */
          registers.sr &= 0x7fff;
#endif
      singleStepping = false;
      if (sigval == 19)
        {
          sigval = 5;   // say we did a trace trap
        }
    }
  else
    {
      disableSerialInterrupt();
      removeBreakpoints(breakpoint, breakpointCount);
    }

  /* reply to host that an exception has occurred */
  signalPacket(sigval);
  putpacket(remcomOutBuffer);

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
          mem2hex((address_t) &registers, remcomOutBuffer, sizeof(registers));
          break;
        case 'G':               /* set the value of the CPU registers - return OK */
          hex2mem(ptr, (address_t) &registers, sizeof(registers));
          strcpy(remcomOutBuffer, "OK");
          break;

          /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
#if defined (__CALYPSI_TARGET_68000__)
          if (setjmp(remcomEnv) == 0)
            {
              exceptionHandler(2, jmp, handle_buserror);
#endif
              /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
              if (hexToLongInt(&ptr, &lvalue))
                {
                  addr = (address_t) lvalue;
                  if (   *(ptr++) == ','
                      && hexToLongInt(&ptr, &lvalue))
                    {
                      length = lvalue;
                      ptr = 0;
                      mem2hex((address_t)addr, remcomOutBuffer, length);
                    }
                }
              if (ptr)
                {
                  strcpy(remcomOutBuffer, "E01");
                }
#if defined (__CALYPSI_TARGET_68000__)
            }
          else
            {
              exceptionHandler(2, jsr, _catchException);
              strcpy(remcomOutBuffer, "E03");
#if DEBUG
             debug_error("bus error");
#endif
            }

          /* restore handler for bus error */
          exceptionHandler(2, jsr, _catchException);
#endif
          break;

          /* ‘X addr,length:XX…’ where XX.. is binary data */
        case 'X':
          /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              addr = (address_t) lvalue;
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
              exceptionHandler(2, jmp, handle_buserror);
#endif
              /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
              if (    hexToLongInt(&ptr, &lvalue)
                  && *(ptr++) == ',')
                {
                  addr = (address_t) lvalue;
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
              exceptionHandler(2, jsr, _catchException);
              strcpy(remcomOutBuffer, "E03");
#if DEBUG
              debug_error("bus error");
#endif
            }

          /* restore handler for bus error */
          exceptionHandler(2, jsr, _catchException);
#endif
          break;

#ifndef __SINGLE_STEP_CAPABLE_TARGET__
          /*
           * The step command is deprecated in the gdbserver protocol.
           * Here it is used with an extra address argument that specifies
           * the address of the instruction after the next one, where we
           * can insert a breakpoint on targets that does not have a proper
           * step/trace mode.
           */
        case 's':
          if (hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              registers.pc = lvalue;

              if (hexToLongInt(&ptr, &lvalue))
                {
                  stepBreakpoint.address = (address_t) lvalue;

                  // active the temporary breakpoints
                  insertBreakpoints(&stepBreakpoint, 1);

              // disable interrupts
#if defined(__CALYPSI_TARGET_65816__) || defined(__CALYPSI_TARGET_6502__)
                  sr_save = registers.sr;
                  registers.sr |= I_BIT;
#endif
                  // say that we are stepping
                  singleStepping = true;

                  continueExecution(&registers);  /* this is a jump */
                }
            }
          break;
#endif

          /* cAA..AA    Continue at address AA..AA(optional) */
          /* sAA..AA   Step one instruction from AA..AA(optional) */
#ifdef __SINGLE_STEP_CAPABLE_TARGET__
        case 's':
          singleStepping = true;
#endif
        case 'c':
          /* try to read optional parameter, pc unchanged if no parm */
          if (hexToLongInt(&ptr, &lvalue))
            {
              registers.pc = lvalue;
            }

          newPC = registers.pc;

#if defined (__CALYPSI_TARGET_68000__)
          /* clear the trace bit */
          registers.sr &= 0x7fff;

          /* set the trace bit if we're stepping */
          if (singleStepping)
            registers.sr |= 0x8000;
#endif

#ifdef __68000_FRAME__
          /*
           * look for newPC in the linked list of exception frames.
           * if it is found, use the old frame it.  otherwise,
           * fake up a dummy frame in returnFromException().
           */
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
#endif // __68000_FRAME__

          // For targets without exception frames, insert user
          // breakpoints, restore registers and start execution.
#if defined (__CALYPSI_TARGET_68000__)
          if (!singleStepping)
#endif
            {
              insertBreakpoints(breakpoint, breakpointCount);
              enableSerialInterrupt();        /* allow ctrl-c */
            }
          continueExecution(&registers);  /* this is a jump */

          break;

          /* kill the program */
        case 'k':               /* do nothing */
          continue;             /* give no reply */

          /* enable extended mode, this is used to allow debugger to */
          /* use the 'R' restart packet. */
        case '!':
          strcpy(remcomOutBuffer, "OK");
          break;

          /* Restart target.  */
        case 'R':
          clearAllBreakpoints();
          continue;    /* no reply */

          /* ‘Z0,addr,kind insert a breakpoint */
        case 'Z':
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              if (lvalue != 0)
                {
                  // Only type 0 = software breakpoints currently allowed
                  break;
                }
              if (   hexToLongInt(&ptr, &lvalue)
                  && *(ptr++) == ',')
                {
                  bpaddress_t bpAddress = (bpaddress_t) lvalue;
                  if (hexToLongInt(&ptr, &lvalue))
                    {
//                      int kind = lvalue;
                      for (unsigned i = 0; i < BREAKPOINTS; i++)
                        {
                          if ( ! breakpoint[i].active )
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
            }
          break;

          /* ‘z0,addr,kind remove a breakpoint */
        case 'z':
          if (   hexToLongInt(&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              if (lvalue != 0)
                {
                  // Only type 0 = software breakpoints currently allowed
                  break;
                }
              if (   hexToLongInt(&ptr, &lvalue)
                  && *(ptr++) == ',')
                {
                  bpaddress_t bpAddress = (bpaddress_t) lvalue;
                  if (hexToLongInt(&ptr, &lvalue))
                    {
//                      int kind = lvalue;
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
            }
          break;

        }                       /* switch */

      /* reply to the request */
      putpacket(remcomOutBuffer);
    }
}

#ifdef _CALYPSI_MCP_BUILD
int CalypsiDebugger ()
#else
int main ()
#endif
{
#if defined (__CALYPSI_TARGET_68000__)
  for (unsigned exception = 2; exception <= 23; exception++)
    exceptionHandler(exception, jsr, _catchException);

  /* level 7 interrupt              */
  exceptionHandler(31, jsr, _debug_level7);

  /* breakpoint exception (BKPT issues an illegal instruction */
  exceptionHandler(4, jmp, illegalHandler);

  /* single step  */
  exceptionHandler(9, jmp, traceHandler);

  /* 48 to 54 are floating point coprocessor errors */
  for (unsigned exception = 48; exception <= 54; exception++)
    exceptionHandler(exception, jsr, _catchException);

#if defined(HB68K08)
  exceptionHandler(76, jmp, uartInterrupt);
#endif // HB68K08

#if defined(A2560U)
  exceptionHandler(0x43, jmp, uartInterrupt);
#endif

#endif  // __CALYPSI_TARGET_68000__

  initialize();

#if defined(__CALYPSI_TARGET_6502__) || defined(__CALYPSI_TARGET_65816__)
  __break_instruction();
#elif  defined(__CALYPSI_TARGET_68000__)
  __break_instruction(0);
#endif
  return 0;
}
