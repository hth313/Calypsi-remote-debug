VPATH = ../src
ASM_SRCS = low_level_C64_ACIA.s c64startup.s
C_SRCS = agent.c 6551-acia.c

BUILD_STYLE = -D AS_BASIC_ROM_REPLACEMENT -D COMMODORE_64
# BUILD_STYLE = -D AS_PRG_STYLE

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as6502 $(BUILD_STYLE) --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc6502 $(BUILD_STYLE) --target=c64 -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent.elf

agent.elf: $(OBJS)
	ln6502 --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-6502.a --rtattr exit=simplified --rtattr cstartup=\"c64-remote-debug\" --output-format raw

clean:
	-rm $(OBJS) obj/*.lst agent
