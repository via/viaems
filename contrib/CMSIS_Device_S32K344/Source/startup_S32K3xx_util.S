/*-----------------------------------------------------------------------------
 * Name:    startup_S32K3xx_util.S
 * Purpose: S32K3xx startup utilities
 * Rev.:    1.0.0
 *----------------------------------------------------------------------------*/
/*
 * Copyright (C) 2021 Arm Limited or its affiliates. All rights reserved.
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

        .syntax  unified

        #include "S32K3xx_memmap.h"


        .thumb
        .section ".text"
        .align   2

        .thumb_func
        .type    DisableSWT0, %function
        .global  DisableSWT0
        .fnstart
        .cantunwind

DisableSWT0:
        // Disable SWT 0
// code not yet tested
//        LDR      R2,=SWT_BASE+0x018      // SWT Service Key Register (SK)
//        LDR      R3,=0x0000C520          // 'Clear Soft Lock Key1'
//        STR      R3,[R2]                 // Write 'Clear Soft Lock Key1' to SWT_SK
//        LDR      R3,=0x0000D928          // 'Clear Soft Lock Key2'
//        STR      R3,[R2]                 // Write 'Clear Soft Lock Key2' to SWT_SK
//        LDR      R2,=SWT_BASE+0x000      // SWT Control Register (CR)
//        LDR      R3,=0xFF000000          //
//        STR      R3,[R2]                 // Disable SWT
        BX       LR                      // Return

        .fnend
        .size    DisableSWT0, .-DisableSWT0


        .thumb_func
        .type    InitECC, %function
        .global  InitECC
        .fnstart
        .cantunwind

InitECC:
        // Initialize SRAM
        LDR      R2,=0
        LDR      R3,=0
        LDR      R4,=RAM_START__
        LDR      R5,=RAM_SIZE__
        
        LSRS     R5,R5,#3
SRAM_Loop:
        STRD     r2,r3,[R4,#0]
        ADDS     R4,R4,#0x08
        SUBS     R5,R5,#1
        CMP      R5,#0x00
        BNE      SRAM_Loop

        // Initialize DTCM
        LDR      R2,=0
        LDR      R3,=0
        LDR      R4,=DTCM_START__
        LDR      R5,=DTCM_SIZE__
        
        LSRS     R5,R5,#3
DTCM_Loop:
        STRD     r2,r3,[R4,#0]
        ADDS     R4,R4,#0x08
        SUBS     R5,R5,#1
        CMP      R5,#0x00
        BNE      DTCM_Loop

        BX       LR                     // Return

        .fnend
        .size    InitECC, .-InitECC

        .end
