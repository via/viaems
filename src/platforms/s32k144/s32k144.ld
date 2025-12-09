MEMORY
{
	vectors(rx) : ORIGIN = 0x00000000, LENGTH = 1K
  flashconfig(rx): ORIGIN = 0x00000400, LENGTH = 1k
  rom(rx) : ORIGIN = 0x00000800, LENGTH = 510K

  flexnvm(rx) : ORIGIN = 0x10000000, LENGTH = 64K
	
	sram_l(rwx)  : ORIGIN = 0x1FFF8000, LENGTH = 32K 
	sram_u(rwx)  : ORIGIN = 0x20000000, LENGTH = 28K
}

ENTRY(Reset_Handler)

SECTIONS {

  .vectors : {
		. = ALIGN(4);
		KEEP(*(.isr_vector))
		. = ALIGN(4);
  } >vectors

  .flashconfig : {
    KEEP(*(.flashconfig))
  } >flashconfig

	.text : {
		. = ALIGN(4);
		_stext = .;
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		_etext = .;
	} >rom
	_text_loadaddr = LOADADDR(.text);


	.data : {
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >sram_u AT >rom
	_data_loadaddr = LOADADDR(.data);

	.configdata : {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >sram_u AT >rom
	_configdata_loadaddr = LOADADDR(.configdata);

	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >sram_u

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_sram_l_start = ORIGIN(sram_l));
PROVIDE(_sram_u_end = ORIGIN(sram_u) + LENGTH(sram_u));
PROVIDE(_stack_end = ORIGIN(sram_u) + LENGTH(sram_u));

