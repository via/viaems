/* Memory layout for STM32F427, dual bank 1 MB package. */

MEMORY
{
	rom(rx) : ORIGIN = 0x08000000, LENGTH = 512K

  /* Store configrom in the first 64K of the second bank.  
   * To support 2MB in dual bank, change to 0x8100000 */
	configrom(rx) : ORIGIN = 0x08080000, LENGTH = 64K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
	ccm (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

_Privileged_Functions_Region_Size = 32K;
_Privileged_Data_Region_Size = 4K;

__privileged_functions_start__ = ORIGIN( rom );
__privileged_functions_end__ = __privileged_functions_start__ + _Privileged_Functions_Region_Size;

__privileged_data_start__ = ORIGIN( ram );
__privileged_data_end__ = ORIGIN( ram ) + _Privileged_Data_Region_Size;

__FLASH_segment_start__ = ORIGIN( rom );
__FLASH_segment_end__ = __FLASH_segment_start__ + LENGTH( rom );


__SRAM_segment_start__ = ORIGIN( ram );
__SRAM_segment_end__ = __SRAM_segment_start__ + LENGTH( ram );

SECTIONS {

	.text : {
		KEEP(*(.isr_vector))
		. = ALIGN(4);

    *(privileged_functions)
    __privileged_functions_actual_end__ = .;
    . = _Privileged_Functions_Region_Size;

    __syscalls_flash_start__ = .;
    *(freertos_system_calls)
    __syscalls_flash_end__ = .;

		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
    _etext = .;
	} >rom


  .privileged_data : {
   *(privileged_data)
    /* Non kernel data is kept out of the first _Privileged_Data_Region_Size
        bytes of SRAM. */
    __privileged_data_actual_end__ = .;
    . = _Privileged_Data_Region_Size;
  } >ram AT >rom
	_privileged_data_loadaddr = LOADADDR(.privileged_data);

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

__Main_Stack_Size = 2048 ;
PROVIDE ( _Main_Stack_Size = __Main_Stack_Size ) ;
__Main_Stack_Limit = _estack  - __Main_Stack_Size ;
