VPATH = ../src
# ASM_SRCS = foo.s
C_SRCS = agent.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 --target=foenix --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 --data-model=small --code-model=small --target=foenix -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

all: agent

agent: $(OBJS)
	ln65816 --target=foenix --list-file=agent.lst --cross-reference -o $@ $^ agent.scm clib-sc-sd-Foenix.a --rtattr exit=simplified --output-format pgz

clean:
	-rm $(OBJS) obj/*.lst agent
