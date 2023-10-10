MEMORY
{
  boot    (r)   : ORIGIN = 0x10000000, LENGTH = 0x00000100

  pflash    (rx)  : ORIGIN = 0x00400000, LENGTH = 0x003D0000  /* 0x00400000 */
  sram      (rwx) : ORIGIN = 0x20400000, LENGTH = 0x00050000  /* (2 x 160kB) */
  dtcm     (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00020000

  dflash         (r)   : ORIGIN = 0x10000100, LENGTH = 0x0001FF00
}

ENTRY(Reset_Handler)

SECTIONS {
  .bootheader :
  {
    KEEP(*(.bootheader))
  } > boot

	.text : {
		KEEP(*(.vectors))
		. = ALIGN(4);
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
    KEEP(*(.lc_xrdc_config))
    _etext = .;
	} >pflash

	.data : {
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
		*(.dmadata*)	/* DMA regions, no different from normal data on stm32f4 */
		. = ALIGN(4);
		_edata = .;
	} >dtcm AT >pflash
	_data_loadaddr = LOADADDR(.data);

	.configdata : {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >dtcm AT >dflash
	_configdata_loadaddr = LOADADDR(.configdata);

	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >dtcm

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_estack = ORIGIN(sram) + LENGTH(sram));
