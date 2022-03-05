VPATH = ../../src:.
ASM_SRCS = low_level_68000.s cstartup.s
C_SRCS = agent.c ts68hc901.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as68k -DHB68K08 --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc68k -DHB68K08 --data-model=far-only --code-model=small -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-hb68k08

agent-hb68k08: $(OBJS)
	ln68k --list-file=agent-hb68k08.lst --cross-reference -o $@ $^ agent-hb68k08.scm clib-68000-sc-fod.a --rtattr exit=simplified --rtattr cstartup=monitor --output-format s28 --heap-size=0 --stack-size=0x0400

clean:
	-rm $(OBJS) obj/*.lst agent-hb68k08.hex
