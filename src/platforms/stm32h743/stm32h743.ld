MEMORY
{
  vectors (rx)      : ORIGIN = 0x8000000, LENGTH = 128K
  configrom (rx)    : ORIGIN = 0x8020000, LENGTH = 128K
  rom (rx)          : ORIGIN = 0x8040000, LENGTH = 768K
  dtcmram (rwx)     : ORIGIN = 0x20000000, LENGTH = 128K
  axiram (rwx)         : ORIGIN = 0x24000000, LENGTH = 512K
  ram_d1 (rwx)      : ORIGIN = 0x30000000, LENGTH = 128K
  ram_d2 (rwx)      : ORIGIN = 0x30020000, LENGTH = 128K
  ram_d3 (rwx)      : ORIGIN = 0x30040000, LENGTH = 128K
  itcmram (rwx)     : ORIGIN = 0x00000000, LENGTH = 64K
}

EXTERN (vector_table)
ENTRY(reset_handler)

SECTIONS {
	.vectors : {
		*(.vectors)
	} > vectors

	.text : {
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >rom

	/* C++ Static constructors/destructors, also used for __attribute__
	 * ((constructor)) and the likes */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >rom
	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >rom
	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >rom

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >rom
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >rom

	. = ALIGN(4);
	_etext = .;

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >dtcmram AT >rom
	_data_loadaddr = LOADADDR(.data);

	.configdata : {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >dtcmram AT >configrom
	_configdata_loadaddr = LOADADDR(.configdata);

	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >dtcmram

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack = ORIGIN(dtcmram) + LENGTH(dtcmram));

