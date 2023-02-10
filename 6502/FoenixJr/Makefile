VPATH = ../../src
ASM_SRCS = low_level_6502.s
C_SRCS = agent.c f256-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as6502 --core=65c02 --target=foenix --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc6502 --core=65c02 --target=foenix -O2 --list-file=$(@:%.o=%.lst) -o $@ $< -I/home/hth/projects/Calypsi-6502-Foenix/include

all: agent.elf

agent.elf: $(OBJS)
	ln6502 --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-65c02.a --rtattr exit=simplified --output-format intel-hex

clean:
	-rm $(OBJS) obj/*.lst agent
