#include "stm32h743xx.h"

/* Flash is composed of 2 banks, each with 8 128K sectors.  We keep all text in
 * bank 1, and use sector 0 of bank 2 for the configrom section.  This allows us
 * to write to it without affecting cpu operations.
 */

extern uint32_t _configdata_loadaddr, _sconfigdata, _econfigdata;

void platform_save_config() 
{
  /* Unlock FLASH_CR2 */
  FLASH->KEYR2 = 0x45670123;
  FLASH->KEYR2 = 0xCDEF89AB;

  /* Enable write with PG2 in FLASH_CR2*/
  FLASH->CR2 |= FLASH_CR_PG;

  /* Erase sector 0 of bank 2 */
  FLASH->CR2 &= ~(FLASH_CR_SER | FLASH_CR_SNB | FLASH_CR_START);
  FLASH->CR2 |= FLASH_CR_SER | 
         _VAL2FLD(FLASH_CR_SNB, 0) | 
         FLASH_CR_START;

  while (FLASH->SR2 & FLASH_SR_QW); /* Wait until erase is complete */

  FLASH->CR2 |= FLASH_CR_PSIZE; /* 32 bit writes */

  uint32_t *src;
  uint32_t *dest;
  for (src = &_sconfigdata, dest = &_configdata_loadaddr; src < &_econfigdata;
       src++, dest++) {
    *dest = *src;
    while (FLASH->SR2 & FLASH_SR_QW); /* Wait until erase is complete */
  }

  FLASH->CR2 |= FLASH_CR_LOCK;
}

void platform_load_config() {
  uint32_t *src;
  uint32_t *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
}

