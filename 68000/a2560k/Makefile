VPATH = ../../src:.
ASM_SRCS = low_level_68000.s cstartup.s
C_SRCS = agent.c a2560-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as68k -DA2560K --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc68k -DA2560K --data-model=far-only --code-model=small --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent-a2560k

agent-a2560k: $(OBJS)
	ln68k --list-file=agent-a2560k.lst --cross-reference -o $@ $^ agent-a2560k-test.scm clib-68000-sc-fod.a --rtattr exit=simplified --rtattr cstartup=foenix_monitor --output-format s28 --heap-size=0 --sstack-size=0x0200

agent-a2560k.pgz: $(OBJS)
	ln68k --list-file=agent-a2560k.lst --cross-reference -o $@ $^ agent-a2560k-test.scm clib-68000-sc-fod.a --rtattr exit=simplified --rtattr cstartup=foenix_monitor --output-format pgz --heap-size=0 --sstack-size=0x0200 --stack-size=0x0200

clean:
	-rm $(OBJS) obj/*.lst agent-a2560k.s28 agent-a2560k.lst agent-a2560k.pgz agent-a2560k
