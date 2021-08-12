// comm.c - debugger agent using gdbserver protocol

// This code is somewhat based on remcom.c or m68k-stub.c which
// HP has placed in the public domain. It has been reworked here but
// some stuff is defintely recognizable.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "api.h"

#define DEBUG 0

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
#define BUFMAX 400

/************************************************************************/
/* BREAKPOINTS defines the maximum number of software (code modifying breakpoints */
#define BREAKPOINTS 25

#ifdef __CALYPSI_TARGET_6502__
#include <intrinsics6502.h>

typedef struct {
  uint32_t pc;
  uint16_t a;
  uint16_t x;
  uint16_t y;
  uint16_t sp;
  uint8_t sr;
} register_t;

// Breakpoint address type */
typedef void * address_t;
typedef uint8_t backing_t;

#define BREAK_OPCODE 0

#endif // __CALYPSI_TARGET_6502__

#ifdef __CALYPSI_TARGET_65816__
#include <intrinsics65816.h>

typedef struct {
  uint32_t pc;
  uint16_t a;
  uint16_t x;
  uint16_t y;
  uint16_t sp;
  uint16_t dp;
  uint8_t bank;
  uint8_t sr;
} register_t;

// Breakpoint address type */
typedef __far void * address_t;
typedef uint8_t backing_t;

#endif // __CALYPSI_TARGET_65816__

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
      while ((ch = getDebugChar ()) != '$')
        ;

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
        {
          ch = getDebugChar ();
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
          ch = getDebugChar ();
          xmitcsum = hex (ch) << 4;
          ch = getDebugChar ();
          xmitcsum += hex (ch);

          if (checksum != xmitcsum)
            {
#if DEBUG
              if (remote_debug)
                {
                  fprintf (stderr,
                           "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
                           checksum, xmitcsum, buffer);
                }
#endif
              putDebugChar ('-');       /* failed checksum */
            }
          else
            {
              putDebugChar ('+');       /* successful transfer */

              /* if a sequence char is present, reply the sequence ID */
              if (buffer[2] == ':')
                {
                  putDebugChar (buffer[0]);
                  putDebugChar (buffer[1]);

                  return &buffer[3];
                }

              return &buffer[0];
            }
        }
    }
}

/* send the packet in buffer. */

void
putpacket (buffer)
     char *buffer;
{
  unsigned char checksum;
  int count;
  char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      putDebugChar ('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
        {
          putDebugChar (ch);
          checksum += ch;
          count += 1;
        }

      putDebugChar ('#');
      putDebugChar (hexchars[checksum >> 4]);
      putDebugChar (hexchars[checksum % 16]);

    }
  while (getDebugChar () != '+');

}

#if DEBUG
void debug_error (char *format, char *parm)
{
  if (remote_debug)
    fprintf (stderr, format, parm);
}
#endif

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
char *mem2hex (char *mem, char *buf, unsigned count)
{
  int i;
  unsigned char ch;
  for (i = 0; i < count; i++)
    {
      ch = *mem++;
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch % 16];
    }
  *buf = 0;
  return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char *hex2mem (char *buf, char *mem, int count)
{
  int i;
  unsigned char ch;
  for (i = 0; i < count; i++)
    {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      *mem++ = ch;
    }
  return (mem);
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
      hexValue = hex (**ptr);
      if (hexValue >= 0)
        {
          *value = (*value << 4) | hexValue;
          numChars++;
        }
      else
        break;

      (*ptr)++;
    }

  return (numChars);
}

/*
 * This function does all command processing.
 */
void handle_exception (unsigned sigval)
{
  int stepping;
  unsigned length;
  long lvalue;
  char *addr;
  char *ptr;
  int newPC;

  /* reply to host that an exception has occurred */
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] = hexchars[sigval >> 4];
  remcomOutBuffer[2] = hexchars[sigval % 16];
  remcomOutBuffer[3] = 0;

  putpacket (remcomOutBuffer);

  stepping = 0;

  while (1 == 1)
    {
      remcomOutBuffer[0] = 0;
      ptr = getpacket ();
      switch (*ptr++)
        {
        case '?':
          remcomOutBuffer[0] = 'S';
          remcomOutBuffer[1] = hexchars[sigval >> 4];
          remcomOutBuffer[2] = hexchars[sigval % 16];
          remcomOutBuffer[3] = 0;
          break;
        case 'd':
#if DEBUG
          remote_debug = !(remote_debug);       /* toggle debug flag */
#endif
          break;
        case 'g':               /* return the value of the CPU registers */
          mem2hex ((char *) &registers, remcomOutBuffer, sizeof(registers));
          break;
        case 'G':               /* set the value of the CPU registers - return OK */
          hex2mem (ptr, (char *) &registers, sizeof(registers));
          strcpy (remcomOutBuffer, "OK");
          break;

          /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
          if (setjmp (remcomEnv) == 0)
            {
#if defined (__CALYPSI_TARGET_68000__)
              exceptionHandler (2, handle_buserror);
#endif
              /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
              if (hexToLongInt (&ptr, &lvalue))
                {
                  addr = (char*) lvalue;
                  if (   *(ptr++) == ','
                      && hexToLongInt (&ptr, &lvalue))
                    {
                      length = lvalue;
                      ptr = 0;
                      mem2hex ((char *) addr, remcomOutBuffer, length);
                    }
                }
              if (ptr)
                {
                  strcpy (remcomOutBuffer, "E01");
                }
            }
#if defined (__CALYPSI_TARGET_68000__)
          else
            {
              exceptionHandler (2, _catchException);
              strcpy (remcomOutBuffer, "E03");
              debug_error ("bus error");
            }

          /* restore handler for bus error */
          exceptionHandler (2, _catchException);
#endif
          break;

          /* ‘X addr,length:XX…’ where XX.. is binary data */
        case 'X':
          /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
          if (   hexToLongInt (&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              addr = (char*) lvalue;
              if (   hexToLongInt (&ptr, &lvalue)
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
          if (setjmp (remcomEnv) == 0)
            {
#if defined (__CALYPSI_TARGET_68000__)
              exceptionHandler (2, handle_buserror);
#endif
              /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
              if (    hexToLongInt (&ptr, &lvalue)
                  && *(ptr++) == ',')
                {
                  addr = (char*) lvalue;
                  if (   hexToLongInt (&ptr, &lvalue)
                      &&  *(ptr++) == ':')
                    {
                      length = lvalue;
                      hex2mem (ptr, (char *) addr, length);
                      ptr = 0;
                      strcpy (remcomOutBuffer, "OK");
                    }
                }
              if (ptr)
                {
                  strcpy (remcomOutBuffer, "E02");
                }
            }
#if defined (__CALYPSI_TARGET_68000__)
          else
            {
              exceptionHandler (2, _catchException);
              strcpy (remcomOutBuffer, "E03");
#if DEBUG
              debug_error ("bus error");
#endif
            }

          /* restore handler for bus error */
          exceptionHandler (2, _catchException);
#endif
          break;

          /* cAA..AA    Continue at address AA..AA(optional) */
          /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
          stepping = 1;
        case 'c':
          /* try to read optional parameter, pc unchanged if no parm */
          if (hexToLongInt (&ptr, &lvalue))
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
                printf ("frame at 0x%x has pc=0x%x, except#=%d\n",
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
                  if (oldExceptionHook)
                    (*oldExceptionHook) (frame->exceptionVector);
                  newPC = registers[PC];        /* pc may have changed  */
                  if (newPC != frame->exceptionPC)
                    {
#if DEBUG
                      if (remote_debug)
                        printf ("frame at 0x%x has pc=0x%x, except#=%d\n",
                                frame, frame->exceptionPC,
                                frame->exceptionVector);
#endif
                      /* re-use the last frame, we're skipping it (longjump?) */
                      frame = (Frame *) 0;
                      _returnFromException (frame);     /* this is a jump */
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

          _returnFromException (frame);   /* this is a jump */
#else
          // For targets without exception frames, restore the
          // register vector and start execution.
          continueExecution(&registers);  /* this is a jump */
#endif

          break;

          /* kill the program */
        case 'k':               /* do nothing */
          break;

          /* ‘Z0,addr,kind insert a breakpoint */
        case 'Z':
          if (   hexToLongInt (&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              address_t bpAddress = (address_t) lvalue;
              if (hexToLongInt (&ptr, &lvalue))
                {
//                int kind = lvalue;
                  for (unsigned i = 0; i < BREAKPOINTS; i++)
                    {
                      if (!breakpoint[i].active)
                        {
                          breakpoint[i].active = true;
                          breakpoint[i].address = bpAddress;
                          break;
                        }
                    }
                }
            }
          break;

          /* ‘z0,addr,kind remove a breakpoint */
        case 'z':
          if (   hexToLongInt (&ptr, &lvalue)
              && *(ptr++) == ',')
            {
              address_t bpAddress = (address_t) lvalue;
              if (hexToLongInt (&ptr, &lvalue))
                {
//                int kind = lvalue;
                  for (unsigned i = 0; i < BREAKPOINTS; i++)
                    {
                      if (   breakpoint[i].active
                          && breakpoint[i].address == bpAddress)
                        {
                          breakpoint[i].active = false;
                          break;
                        }
                    }
                }
            }
          break;

        }                       /* switch */

      /* reply to the request */
      putpacket (remcomOutBuffer);
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
