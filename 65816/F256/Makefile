VPATH = ../../src
ASM_SRCS = low_level_WDC65816.s
C_SRCS = agent.c f256-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 --target=f256 --data-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 --target=f256 --data-model=small --code-model=small -O2 --no-cross-call --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-F256.hex

agent-F256.hex: $(OBJS)
	ln65816 --list-file=agent-F256.lst -o $@ $^ agent-F256.scm --rtattr exit=simplified --output-format intel-hex --heap-size=0 --stack-size=0x0400 --rom-code

clean:
	-rm $(OBJS) obj/*.lst agent-F256.hex agent-F256.lst
