/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Linker script for ST STM32F4DISCOVERY (STM32F407VG, 1024K flash, 128K RAM). */

/* Define memory regions. */
MEMORY
{
  remap(rx) : ORIGIN = 0x00000000, LENGTH = 112K
	rom(rx) : ORIGIN = 0x08000000, LENGTH = 1024K
	ram1 (rwx) : ORIGIN = 0x20000000, LENGTH = 112K
	ram2 (rwx) : ORIGIN = 0x2001C000, LENGTH = 16K
	ram3 (rwx) : ORIGIN = 0x20020000, LENGTH = 64K
	ccm (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

EXTERN (vector_table)
ENTRY(reset_handler)

SECTIONS {
	.text : {
		*(.vectors)
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
    __preinit_array_start = .;
    __preinit_array_end = .;
    __init_array_start = .;
    __init_array_end = .;
    __fini_array_start = .;
    __fini_array_end = .;
	} >remap AT >rom

	. = ALIGN(4);
	_etext = .;

	.dmadata : {
	. = ALIGN(4);
		*(.dmadata*)	/* DMA regions, no different from normal data on stm32f4 */
	. = ALIGN(4);
	} >ram2

	.configdata : {
    . = ALIGN(4);
		_sconfigdata = .;
		*(.configdata*)
		. = ALIGN(4);
		_econfigdata = .;
	} >ram3 AT >rom
	_configdata_loadaddr = LOADADDR(.configdata);


	.data : {
	. = ALIGN(4);
		_data = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >ram3 AT >rom
	_data_loadaddr = LOADADDR(.data);

	.bss : {
    . = ALIGN(4);
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >ram3

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack = ORIGIN(ccm) + LENGTH(ccm));

