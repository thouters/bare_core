
#include <stdint.h>
#include <string.h>

extern int main(void);
extern void __init_registers();
extern void __copy_rom_sections_to_ram(void);
extern char __S_romp[];
extern uint32_t _end_heap_magic[];
extern uint32_t _end_stack_magic[];
extern uint32_t _guard_magic[];


static void zero_fill_bss(void)
{
	extern char __START_BSS[];
	extern char __END_BSS[];
    extern char __START_BSS2[];
    extern char __END_BSS2[];

	memset(__START_BSS, 0, (__END_BSS - __START_BSS));
	memset(__START_BSS2, 0, (__END_BSS2 - __START_BSS2));
}

void __thumb_startup(void)
{
    // Setup registers
    __init_registers();

    // setup hardware
    //wdt_disable();

    //	zero-fill the .bss section
    zero_fill_bss();

    // Initialize initialized data
    if (__S_romp != 0L)
        __copy_rom_sections_to_ram();

    _end_heap_magic[0] = (uint32_t)_guard_magic;
    _end_stack_magic[0] = (uint32_t)_guard_magic;

    //	call main
    main();
    //	should never get here
}

extern uint32_t _end_stack[];
// __init_registers, __init_hardware, __init_user suggested by Kobler
void __init_registers(void);
void __init_registers(void)
{
    int addr = (int)_end_stack;
        // setup the stack before we attempt anything else
        // skip stack setup if __SP_INIT is 0
        // assume sp is already setup.
    __asm (
    "mov    r0,%0\n\t"
    "cmp    r0,#0\n\t"
    "beq    skip_sp\n\t"
    "mov    sp,r0\n\t"
    "sub    sp,#4\n\t"
    "mov    r0,#0\n\t"
    "mvn    r0,r0\n\t"
    "str    r0,[sp,#0]\n\t"
    "add    sp,#4\n\t"
    "skip_sp:\n\t"
    ::"r"(addr));

    __asm volatile ("mov pc,lr\n");
}

