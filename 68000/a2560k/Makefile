VPATH = ../../src:.
ASM_SRCS = low_level_68000.s
C_SRCS = agent.c a2560-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as68k --target=a2560k --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc68k --target=a2560k --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-a2560k

agent-a2560k: $(OBJS)
	ln68k --list-file=agent-a2560k.lst --cross-reference -o $@ $^ agent-a2560k-test.scm --rtattr exit=simplified --output-format s28 --heap-size=0 --sstack-size=0x0200 --stack-size=4

agent-a2560k.pgz: $(OBJS)
	ln68k --list-file=agent-a2560k.lst --cross-reference -o $@ $^ agent-a2560k-test.scm --rtattr exit=simplified --output-format pgz --heap-size=0 --sstack-size=0x0200 --stack-size=4

# A variant for complete system take over. This is intended to be flashed, instead of the MCP.
agent-a2560k-flash.raw: $(OBJS)
	ln68k --list-file=agent-flash-a2560k.lst --cross-reference -o $@ $^ agent-a2560k-flash.scm --rtattr exit=simplified --output-format raw --heap-size=0 --sstack-size=0x0200 --stack-size=4

clean:
	-rm $(OBJS) obj/*.lst agent-a2560k.s28 agent-a2560k.lst agent-a2560k.pgz agent-a2560k
	-rm agent-a2560k-flash.raw agent-a2560k-flash.lst
