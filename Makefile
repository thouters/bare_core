ARM_C_FILES = $(shell find arm -name \*.c)
ARM_O_FILES = $(ARM_C_FILES:.c=.o)
ARM_DEPS = $(ARM_O_FILES:.o=.d)

all:	target server test

test:
	./test_main
	file core | grep ARM
	arm-none-eabi-gdb arm/ex1.elf core

target:	arm/ex1.elf

arm/ex1.elf:	$(ARM_O_FILES)
	arm-none-eabi-gcc -nostartfiles -Wl,--gc-sections -Xlinker --script=arm/MK12DX256_app.ld \
		-Xlinker -Map=arm/ex1.map $(ARM_O_FILES) \
		-o arm/ex1.elf

server:	test_main

test_main:	test_main.c elfcore.c elfcore.h
	gcc -I . test_main.c elfcore.c -o test_main

clean:
	rm -f test_main arm/ex1.elf $(ARM_O_FILES) $(ARM_DEPS)

-include $(DEPS)

arm/%.o: arm/%.c
	arm-none-eabi-gcc -g -mcpu=cortex-m4 -mlittle-endian -mthumb -g3 -O0 -fmessage-length=0 \
		-ffunction-sections -fdata-sections -funsigned-char -std=gnu11 -MP -MMD \
		 -c $< -o $@
