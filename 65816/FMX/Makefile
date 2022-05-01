VPATH = ../../src
ASM_SRCS = low_level_WDC65816.s
C_SRCS = agent.c c256-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 -D__TARGET_C256_FMX__ --data-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 -D__TARGET_C256_FMX__ --data-model=small --code-model=small -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-FMX.hex

agent-FMX.hex: $(OBJS)
	ln65816 --list-file=agent-FMX.lst --cross-reference -o $@ $^ agent-FMX.scm clib-sc-sd.a --rtattr exit=simplified --output-format intel-hex --heap-size=0 --stack-size=0x0400

clean:
	-rm $(OBJS) obj/*.lst agent-FMX.hex agent-FMX.lst
