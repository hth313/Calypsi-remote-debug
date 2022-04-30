VPATH = ../../src
ASM_SRCS = low_level_WDC65816.s
C_SRCS = agent.c c256-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 --data-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 --data-model=small --code-model=small -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-U.hex

agent-U.hex: $(OBJS)
	ln65816 --list-file=agent-U.lst --cross-reference -o $@ $^ agent-U.scm clib-sc-sd.a --rtattr exit=simplified --output-format intel-hex --heap-size=0 --stack-size=0x0400

clean:
	-rm $(OBJS) obj/*.lst agent-U.hex
