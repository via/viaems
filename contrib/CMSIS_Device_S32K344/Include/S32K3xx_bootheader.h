/*---------------------------------------------------------------------------
 * Name:    S32K3xx_bootheader.h
 * Purpose: S32K3xx Boot Header definition
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

#ifndef S32K3xx_BOOTHEADER_H
#define S32K3xx_BOOTHEADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct
{
  const uint32_t Header;                         /* Offset: 0x000  Boot Header (0x5AA55AA5) */
  const uint32_t BootConfig;                     /* Offset: 0x004  Boot Configuration Word */
  const uint32_t Reserved0;                      /* Offset: 0x008  Reserved */
  const void*    CM7_0_StartAddress;             /* Offset: 0x00C  CM7_0 Application Start Address */
  const uint32_t Reserved1;                      /* Offset: 0x010  Reserved */
  const void*    CM7_1_StartAddress;             /* Offset: 0x014  CM7_1 Application Start Address */
  const uint32_t Reserved2;                      /* Offset: 0x018  Reserved */
  const uint32_t Reserved3;                      /* Offset: 0x01C  Reserved */
  const void*    XRDC_ConfigAddress;             /* Offset: 0x020  XRDC Configuration Data Start Address */
  const void*    LC_ConfigAddress;               /* Offset: 0x024  LC Configuration Word Address */
  const uint8_t  Reserved[216];                  /* Offset: 0x028  Reserved for future use */
} S32K3xx_bootheader_t;

#ifdef __cplusplus
}
#endif

#endif /* S32K3xx_BOOTHEADER_H */
