/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	MRF24G driver
*
*************************************************************************/

#ifndef MRF_REG_H
#define MRF_REG_H

#include <stdlib.h>
#include <stdint.h>

/*-----------------------------*/
/* MRF24W 8-bit Host Registers */
/*-----------------------------*/
#define MRF_HOST_INTR_REG            ((uint8_t)(0x01)) /* 8-bit register containing 1st level interrupt bits. */
#define MRF_HOST_MASK_REG            ((uint8_t)(0x02)) /* 8-bit register containing 1st level interrupt mask. */

/*------------------------------*/
/* MRF24W 16-bit Host Registers */
/*------------------------------*/
#define MRF_HOST_MAIL_BOX_0_MSW_REG  ((uint16_t)(0x10))
#define MRF_HOST_MAIL_BOX_0_LSW_REG  ((uint16_t)(0x12))
#define MRF_HOST_MAIL_BOX_1_MSW_REG  ((uint16_t)(0x14))
#define MRF_HOST_MAIL_BOX_1_LSW_REG  ((uint16_t)(0x16))
#define MRF_HOST_RAW0_CTRL1_REG      ((uint16_t)(0x26))
#define MRF_HOST_RAW1_CTRL1_REG      ((uint16_t)(0x2a))
#define MRF_HOST_RAW2_CTRL1_REG      ((uint16_t)(0x19))
#define MRF_HOST_RAW3_CTRL1_REG      ((uint16_t)(0x1d))
#define MRF_HOST_RAW4_CTRL1_REG      ((uint16_t)(0x0b))
#define MRF_HOST_RAW5_CTRL1_REG      ((uint16_t)(0x0f))

#define MRF_HOST_RESET_REG           ((uint16_t)(0x3c))
#define MRF_HOST_RESET_MASK          ((uint16_t)(0x0001))

/* Scratchpad registers */
#define MRF_SCRATCHPAD_0_REG         ((uint16_t)(0x3d))
#define MRF_SCRATCHPAD_1_REG         ((uint16_t)(0x3e))

#define MRF_HOST_RAW0_STATUS_REG     ((uint16_t)(0x28))

#define MRF_HOST_INTR2_REG           ((uint16_t)(0x2d)) /* 16-bit register containing 2nd level interrupt bits */
#define MRF_HOST_INTR2_MASK_REG      ((uint16_t)(0x2e))

#define MRF_HOST_WFIFO_BCNT0_REG     ((uint16_t)(0x2f)) /* 16-bit register containing available write size for fifo 0 (data tx)*/
                                                       /* (LS 12 bits contain the length)                                     */

#define MRF_HOST_WFIFO_BCNT1_REG     ((uint16_t)(0x31)) /* 16-bit register containing available write size for fifo 1 (mgmt tx)*/
                                                       /* (LS 12 bits contain the length)                                     */

#define MRF_HOST_RFIFO_BCNT0_REG     ((uint16_t)(0x33)) /* 16-bit register containing number of bytes in read fifo 0 (data rx) */
                                                       /* (LS 12 bits contain the length)                                     */

#define MRF_HOST_RFIFO_BCNT1_REG     ((uint16_t)(0x35)) /* 16-bit register containing number of bytes in read fifo 1 (mgmt rx) */
                                                       /* (LS 12 bits contain the length)                                     */

#define MRF_PSPOLL_H_REG             ((uint16_t)(0x3d)) /* 16-bit register used to control low power mode                      */
#define MRF_INDEX_ADDR_REG           ((uint16_t)(0x3e)) /* 16-bit register to move the data window                             */
#define MRF_INDEX_DATA_REG           ((uint16_t)(0x3f)) /* 16-bit register to read or write address-indexed register           */

/*----------------------------------------------------------------------------------------*/
/* WiFi registers accessed via the MRF_INDEX_ADDR_REG and MRF_INDEX_DATA_REG registers */
/*----------------------------------------------------------------------------------------*/
#define MRF_HW_STATUS_REG            ((uint16_t)(0x2a)) /* 16-bit read only register providing hardware status bits */
#define MRF_CONFIG_CTRL0_REG         ((uint16_t)(0x2e)) /* 16-bit register used to initiate Hard reset              */
#define MRF_WAKE_CONTROL_REG         ((uint16_t)(0x2f)) /* NEW */
#define MRF_SCRATCHPAD_0_REG         ((uint16_t)(0x3d)) /* NEW */
#define MRF_SCRATCHPAD_1_REG         ((uint16_t)(0x3e)) /* 16-bit register read to determine when low power is done */
#define MRF_PSPOLL_CONFIG_REG        ((uint16_t)(0x40)) /* NEW */
#define MRF_XTAL_SETTLE_TIME_REG     ((uint16_t)(0x41)) /* NEW */

uint8_t MRF_reg_read1(const uint8_t id);
void MRF_reg_write1(const uint8_t id, const uint8_t data);
uint16_t MRF_reg_read2(const uint8_t id);
void MRF_reg_write2(const uint8_t id, const uint16_t data);
void MRF_array_read(const uint8_t id, uint8_t* data,
    const size_t length);
void MRF_array_write(const uint8_t id, const uint8_t* data,
    const size_t length);

#endif
