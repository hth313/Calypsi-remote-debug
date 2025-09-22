VPATH = ../../src
ASM_SRCS = low_level_WDC65816.s
C_SRCS = agent.c f256-uart.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

obj/%.o: %.s
	as65816 --target=f256 --data-model=small --code-model=compact --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 --target=f256 --data-model=small --code-model=compact -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

agent-F256.elf: $(OBJS)
	ln65816 --list-file=agent-F256.lst -o $@ $^ agent-flash.scm --rtattr exit=simplified --output-format raw --heap-size=0 --stack-size=0x0400 --rom-code

agent-F256.hex: $(OBJS)
	ln65816 --list-file=agent-F256.lst -o $@ $^ agent-F256.scm --rtattr exit=simplified --output-format pgz --heap-size=0 --stack-size=0x0400 --rom-code

clean:
	-rm $(OBJS) obj/*.lst agent-F256.hex agent-F256.lst
