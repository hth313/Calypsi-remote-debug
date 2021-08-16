VPATH = ../src
ASM_SRCS = low_level_C64_ACIA.s c64startup.s
C_SRCS = agent.c 6551-acia.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as6502 --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc6502 --target=c64 -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent

agent: $(OBJS)
	ln6502 --target=c64 --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-6502.a --rtattr exit=simplified --rtattr cstartup=\"c64-remote-debug\" --output-format intel-hex

clean:
	-rm $(OBJS) obj/*.lst agent
