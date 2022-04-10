VPATH = ../../src:.
ASM_SRCS = low_level_68000.s
LOAD_SRCS = cstartup.s
REPLACE_SRCS = stay_supervisor.c
C_SRCS = agent.c ts68hc901.c

OBJS_BASE = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

OBJS_REPLACE = $(OBJS_BASE) $(REPLACE_SRCS:%.c=obj/%.o)
OBJS_LOAD = $(OBJS_BASE) $(LOAD_SRCS:%.s=obj/%.o)

obj/%.o: %.s
	as68k -DHB68K08 --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc68k -DHB68K08 --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-hb68k08 native-hb68k08

agent-hb68k08: $(OBJS_LOAD)
	ln68k --list-file=agent-hb68k08.lst --cross-reference -o $@ $^ agent-hb68k08.scm clib-68000-sc-fod.a --rtattr exit=simplified --rtattr cstartup=monitor --output-format s28 --heap-size=0 --stack-size=0x0400

# Version that takes over the board
native-hb68k08: $(OBJS_REPLACE)
	ln68k --list-file=native-hb68k08.lst --cross-reference -o $@ $^ low-mem-hb68k08.scm clib-68000-sc-fod.a --rtattr exit=simplified --rtattr cstartup=normal --output-format intel-hex --heap-size=0 --stack-size=4096 --sstack-size=4096

clean:
	-rm $(OBJS_REPLACE) $(OBJS_LOAD) obj/*.lst native-hb68k08 agent-hb68k08 native-hb68k08.hex agent-hb68k08.s28
