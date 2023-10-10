/*---------------------------------------------------------------------------
 * Name:    S32K3xx_LC_XRDC.c
 * Purpose: S32K3xx LC XRDC Configuration
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

#include "S32K3xx_lc_xrdc_config.h"

extern
const lc_config_t    lc_config;  
const lc_config_t    lc_config   __attribute__((section (".lc_xrdc_config"), used)) = {
             0xFFFFFFFF
};
       
extern 
const xrdc_config_t  xrdc_config;
const xrdc_config_t  xrdc_config __attribute__((section (".lc_xrdc_config"), used)) = {
  .Header =  0xFFFFFFFF
};
