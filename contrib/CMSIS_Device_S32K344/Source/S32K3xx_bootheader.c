/*---------------------------------------------------------------------------
 * Name:    S32K3xx_bootheader.c
 * Purpose: S32K3xx Boot Header
 * Rev.:    1.0.0
 *---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2021 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include "S32K3xx_bootheader.h"
#include "S32K3xx_lc_xrdc_config.h"

#include "S32K3xx_memmap.h"

extern const xrdc_config_t xrdc_config;
extern const lc_config_t   lc_config;

extern const S32K3xx_bootheader_t boot_header;
       const S32K3xx_bootheader_t boot_header __attribute__((section (".bootheader"),used)) = {
  .Header             =               0x5AA55AA5,  /* valid  boot header marker */
  .BootConfig         =                        1,  /* bit 0: CM7_0_ENABLE; Cortex-M7_0 application core clock ungated after boot */
                                                   /* bit 1: CM7_1_ENABLE; Cortex-M7_1 application core clock ungated after boot */
                                                   /* bit 3: BOOT_SEQ;     Secure Boot application image is executed by HSE after authentication */
                                                   /* bit 5: APP_SWT_INIT; SBAF initializes SWT0 before enabling the application cores */
  .CM7_0_StartAddress = (const void *)CM7_0_FLASH_START,
  .CM7_1_StartAddress = (const void *)CM7_1_FLASH_START,
  .XRDC_ConfigAddress = (const void *)&xrdc_config,
  .LC_ConfigAddress   = (const void *)&lc_config
};
