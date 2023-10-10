/*---------------------------------------------------------------------------
 * Name:    S32K3xx_lc_xrdc_config.h
 * Purpose: S32K3xx LC XRDC Configuration definition
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

#ifndef S32K3xx_LC_XRDC_CONFIG_H
#define S32K3xx_LC_XRDC_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t lc_config_t;

typedef struct xrdc_config
{
  const uint32_t Header;                         /* Offset: 0x000  XRDC Configuration Header (0xCC5577CC)*/
  const uint32_t Data[66];                       /* Offset: 0x...  data */
  const uint32_t CheckSum;                       /* Offset: 0x...  Checksum */
} xrdc_config_t;

#ifdef __cplusplus
}
#endif

#endif /* S32K3xx_LC_XRDC_CONFIG_H */
