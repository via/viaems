/* Define memory regions for GD32F4XXXX. */
MEMORY
{
	code(rx) : ORIGIN = 0x00000000, LENGTH = 112K
	rom(rx) : ORIGIN = 0x08000000, LENGTH = 384K
	configrom(rx) : ORIGIN = 0x08060000, LENGTH = 128K
	sram0 (rwx) : ORIGIN = 0x20000000, LENGTH = 112K
	sram1 (rwx) : ORIGIN = 0x2001C000, LENGTH = 16K
	sram2 (rwx) : ORIGIN = 0x20020000, LENGTH = 64K
	addsram (rwx) : ORIGIN = 0x20030000, LENGTH = 64K
	ccm (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

EXTERN (vector_table)
ENTRY(reset_handler)

SECTIONS {

	.text : {
    _stext = .;
		*(.vectors)
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
    _etext = .;
	} >code AT >rom
	_text_loadaddr = LOADADDR(.text);

	.data : {
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
		*(.dmadata*)
		. = ALIGN(4);
		_edata = .;
	} >sram2 AT >rom
	_data_loadaddr = LOADADDR(.data);

	.configdata : {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >sram2 AT >configrom
	_configdata_loadaddr = LOADADDR(.configdata);

	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >sram2

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack = ORIGIN(ccm) + LENGTH(ccm));

