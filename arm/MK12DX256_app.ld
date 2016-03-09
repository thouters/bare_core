/*
*****************************************************************************
**
**  File        : K12DX256M5_flash.ld
**
**  Default linker command file for Flash targets
**
*****************************************************************************
*/
/* Entry Point */
ENTRY(__thumb_startup)

/* Heap and stack sizes */
__heap_size = 0x00C;            /* required amount of heap (none) */
__stack_size = 0x800;   /* required amount of stack for app (ARM process stack)*/
__patches_size = 124K;
_guard_magic = 0xDEADBEEF;

/* Specify the memory areas
            addr    len     content
            0       8K      boot
            8K      124K    app
            132K    124K    patch area */
MEMORY
{
  m_boot        (rx) : ORIGIN = 0x00000000, LENGTH = 8K
  m_interrupts  (rx) : ORIGIN = 0x00002000, LENGTH = 0x144
  m_text        (rx) : ORIGIN = 0x00002144, LENGTH = 124K - 0x144
  m_patches     (rw) : ORIGIN = 132K, LENGTH = 124K
  m_ram1       (rwx) : ORIGIN = 0x1FFFC000, LENGTH = 16K            /* Lower SRAM */
  m_ram2       (rwx) : ORIGIN = 0x20000000, LENGTH = 16K - 0x100    /* Upper SRAM */
}

		
/* Define output sections */
SECTIONS
{
  .boot :
  {
    KEEP(*/boot.o(.data))
  } > m_boot
  
  /* The startup code goes first into Flash */
  .interrupts :
  {
	__vector_table = .;
    . = ALIGN(4);
    KEEP(*(.vectortable)) /* Startup code */
    . = ALIGN(4);
  } > m_interrupts

  /* The program code and other data goes into Flash */
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
	___ROM_AT = .;
  } > m_text

  .patches :
  {
  	. = ALIGN(4);
    __patches_start = . ;
  	*(.patches)
  	*(.patches*)
  	. = ALIGN(4);
  } > m_patches

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > m_text
  .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
  } > m_text
  
  /* Removed .ctors and .dtors - no C++ here */

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    /* HEAP */
	__heap_addr = .;
    . = . + __heap_size;
    
    /* If any code overflows a buffer in heap or overruns the user stack,
       this magic # will be overwritten */
    . = ALIGN(4);
    _end_heap_magic = .;
    LONG(_guard_magic);
    
    /* USER STACK */
    . = . + __stack_size;
    _end_stack = .;
    
    /* If any stack buffers overrun enough, this magic # will be overwritten */ 
    . = ALIGN(4);
    _end_stack_magic = .;
    LONG(_guard_magic);
  } > m_ram1

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT(___ROM_AT)
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.ram_funcs*)
    *(.data)           /* .data sections */
    *(.data.*)          /* .data* sections */
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } > m_ram1
  
  ___data_size = _edata - _sdata;
  
  ___m_ram2_ROMStart = ___ROM_AT + SIZEOF(.data);
  .data2 : AT(___m_ram2_ROMStart)
  {
     . = ALIGN(4);
    _sdata2 = .;        /* create a global symbol at data start */
     ___m_ram2_RAMStart = .;
     *(.data2) /* This is an User defined section */
     *(.data2.*) /* This is an User defined section */
     . = ALIGN(4);
     ___m_ram2_RAMEnd = .;
    _edata2 = .;        /* define a global symbol at data end */
     . = ALIGN(4);
  } > m_ram2
  ___m_ram2_ROMSize = ___m_ram2_RAMEnd - ___m_ram2_RAMStart;

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    /* This is used by the startup in order to initialize the .bss section */
    __START_BSS = .;
	PROVIDE ( __bss_start__ = __START_BSS );
    *(.bss)
    *(.bss.*)
    *(COMMON)

    . = ALIGN(4);
    __END_BSS = .;
	PROVIDE ( __bss_end__ = __END_BSS );
  } > m_ram1

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss2 (NOLOAD) :
  {
    /* This is used by the startup in order to initialize the .bss section */
    __START_BSS2 = .;
	PROVIDE ( __bss2_start__ = __START_BSS2 );
    *(.bss2)	/* This is an User defined section */
    *(.bss2.*)	/* This is an User defined section */

    . = ALIGN(4);
    __END_BSS2 = .;
	PROVIDE ( __bss2_end__ = __END_BSS2 );
  } > m_ram2
  
  _romp_at = ___ROM_AT + SIZEOF(.data) + SIZEOF(.data2);
  .romp __END_BSS2 : AT(_romp_at)
  {
	__S_romp = _romp_at;
    LONG(___ROM_AT);
    LONG(_sdata);
    LONG(___data_size);
    LONG(___m_ram2_ROMStart);
    LONG(___m_ram2_RAMStart);
    LONG(___m_ram2_ROMSize);
    LONG(0);
    LONG(0);
    LONG(0);
  } > m_ram2
 
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
