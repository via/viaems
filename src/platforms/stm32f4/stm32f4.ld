/* Memory layout for STM32F427, dual bank 1 MB package. */

MEMORY
{
  code(rx) : ORIGIN = 0x00000000, LENGTH = 112K,
	rom(rx) : ORIGIN = 0x08000000, LENGTH = 512K

  /* Store configrom in the first 64K of the second bank.  
   * To support 2MB in dual bank, change to 0x8100000 */
	configrom(rx) : ORIGIN = 0x08080000, LENGTH = 64K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
	ccm (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS {
	.isrs : {
		KEEP(*(.isr_vector))
		. = ALIGN(4);
  } >rom

  .text.test_loops : ALIGN(4096) {
		_stext_test_loops = .;
    */test_loops.process.o(.text* .rodata*)
    _etext_test_loops = .;
    . = ALIGN(4096);
  } >rom

  .data.test_loops : ALIGN(4096) {
		_sdata_test_loops = .;
    KEEP(*/test_loops.process.o(.data*))
    _edata_test_loops = .;
    KEEP(*/test_loops.process.o(.bss*))
    _ebss_test_loops = .;
    . = ALIGN(4096);
  } >ram AT >rom
	_data_test_loops_romaddr = LOADADDR(.data.test_loops);


  .text : {
    *(*.text* *.rodata*)
  } >rom


	.data : {
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
		*(.dmadata*)	/* DMA regions, no different from normal data on stm32f4 */
		. = ALIGN(4);
		_edata = .;
	} >ram AT >rom
	_data_loadaddr = LOADADDR(.data);

	.configdata : {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >ram AT >configrom
	_configdata_loadaddr = LOADADDR(.configdata);

	.bss : {
		_sbss = .;
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >ram

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_estack = ORIGIN(ram) + LENGTH(ram));

