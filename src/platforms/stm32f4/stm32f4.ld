/* Memory layout for STM32F427, dual bank 1 MB package. */

MEMORY
{
  code(rx) : ORIGIN = 0x00000000, LENGTH = 112K,
	rom(rx) : ORIGIN = 0x08000000, LENGTH = 512K

  /* Store configrom in the first 64K of the second bank.  
   * To support 2MB in dual bank, change to 0x8100000 */
	configrom(rx) : ORIGIN = 0x08080000, LENGTH = 64K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
	ccm (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS {
	.isrs : {
		KEEP(*(.isr_vector))
		. = ALIGN(4);
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
  _size_data_test_loops = SIZEOF(.data.test_loops);

  .data.engine_mgmt  : ALIGN(8192) {
		_sdata_engine_mgmt = .;
    KEEP(*/engine_mgmt.process.o(.data*))
		KEEP(*(.dmadata*))	/* DMA regions, no different from normal data on stm32f4 */
    _edata_engine_mgmt = .;
    KEEP(*/engine_mgmt.process.o(.bss*))
    _ebss_engine_mgmt = .;
    . = ALIGN(8192);
  } >ram AT >rom
	_data_engine_mgmt_romaddr = LOADADDR(.data.engine_mgmt);
  _size_data_engine_mgmt = SIZEOF(.data.engine_mgmt);

  .data.console  : ALIGN(65536) {
		_sdata_console = .;
    KEEP(*/console.process.o(.data*))
    _edata_console = .;
    KEEP(*/console.process.o(.bss*))
    _ebss_console = .;
    . = ALIGN(65536);
  } >ram AT >rom
	_data_console_romaddr = LOADADDR(.data.console);
  _size_data_console = SIZEOF(.data.console);

	.configdata : ALIGN(32768) {
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(32768);
		_econfigdata = .;
	} >ram AT >configrom
  _size_configdata = SIZEOF(.configdata);
	_configdata_loadaddr = LOADADDR(.configdata);

  .text : ALIGN(128K) {
    _stext = .;
    *(*.text* *.rodata*)
    . = ALIGN(128K);
    _stext = .;
  } >rom
  _size_text = SIZEOF(.text);

	.data : {
		_sdata = .;
		*(.data*)	/* Read-write initialized data */
    *(.shareddata)
		. = ALIGN(4);
		_edata = .;
	} >ram AT >rom
  _size_data = SIZEOF(.data);
	_data_loadaddr = LOADADDR(.data);

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

