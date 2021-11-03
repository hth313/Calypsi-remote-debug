VPATH = ../src
ASM_SRCS = low_level_WDC65816.s
C_SRCS = agent.c foenix-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 -D__CALYPSI_DATA_MODEL_SMALL__ --target=foenix --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 --data-model=small --code-model=small --target=foenix -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent.hex

agent.hex: $(OBJS)
	ln65816 --target=foenix --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-sc-sd-Foenix.a --rtattr exit=simplified --output-format intel-hex --heap-size=0 --stack-size=0x0400

clean:
	-rm $(OBJS) obj/*.lst agent.hex
