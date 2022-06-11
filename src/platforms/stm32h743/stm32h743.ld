MEMORY
{
  rom (rx)        : ORIGIN = 0x8000000, LENGTH = 1024K
  configrom (r)   : ORIGIN = 0x8100000, LENGTH = 128K
  rom2(r)         : ORIGIN = 0x8120000, LENGTH = 896K
  dtcmram (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K
  axiram (rwx)    : ORIGIN = 0x24000000, LENGTH = 512K
  ram_d1 (rwx)    : ORIGIN = 0x30000000, LENGTH = 128K
  ram_d2 (rwx)    : ORIGIN = 0x30020000, LENGTH = 128K
  ram_d3 (rwx)    : ORIGIN = 0x30040000, LENGTH = 128K
  itcmram (rwx)   : ORIGIN = 0x00000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS {
	.text : {
		KEEP(*(.isr_vector))
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
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >dtcmram AT >rom
	_sidata = LOADADDR(.data);

	.configdata : {
		. = ALIGN(4);
		_sconfigdata = .;
		*(.configdata*)
		_econfigdata = .;
		. = ALIGN(32);
	} >dtcmram AT >configrom
	_configdata_loadaddr = LOADADDR(.configdata);

  .dmadata : {
    *(.dmadata)
    . = ALIGN(4);
  } >axiram AT >rom

	.bss : {
    _sbss = .;
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >dtcmram

	. = ALIGN(4);
	end = .;
}

PROVIDE(_estack = ORIGIN(dtcmram) + LENGTH(dtcmram));

