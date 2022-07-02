VPATH = ../../src
ASM_SRCS = low_level_C64_ACIA.s
C_SRCS = agent.c c256-uart.c

BUILD_STYLE = -D FOENIX_JR

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as6502 --core=65c02 $(BUILD_STYLE) --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc6502 --core=65c02 $(BUILD_STYLE) -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent.elf

agent.elf: $(OBJS)
	ln6502 --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-65c02.a --rtattr exit=simplified --output-format intel-hex

clean:
	-rm $(OBJS) obj/*.lst agent
