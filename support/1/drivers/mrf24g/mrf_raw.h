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

#ifndef MRF_RAW_H
#define MRF_RAW_H

#include <stdint.h>
#include <stdlib.h>

/* 8-bit RAW registers */
#define MRF_RAW_0_DATA_REG (0x20)   /* Data Rx       */
#define MRF_RAW_1_DATA_REG (0x21)   /* Data Tx       */
#define MRF_RAW_2_DATA_REG (0x06)   /* Mgmt Rx       */
#define MRF_RAW_3_DATA_REG (0x07)   /* Mgmt Tx       */
#define MRF_RAW_4_DATA_REG (0x08)   /* Scratch Tx/Rx */
#define MRF_RAW_5_DATA_REG (0x09)   /* not used      */

/* 16 bit RAW registers */
#define MRF_MRF_RAW_0_CTRL_0_REG (0x25)      /* RAW 0 -- Data Rx       */
#define MRF_MRF_RAW_0_CTRL_1_REG (0x26)
#define MRF_MRF_RAW_0_INDEX_REG (0x27)
#define MRF_MRF_RAW_0_STATUS_REG (0x28)

#define MRF_RAW_1_CTRL_0_REG (0x29)      /* RAW 1 -- Data Tx       */
#define MRF_RAW_1_CTRL_1_REG (0x2a)
#define MRF_RAW_1_INDEX_REG (0x2b)
#define MRF_RAW_1_STATUS_REG (0x2c)

#define MRF_RAW_2_CTRL_0_REG (0x18)      /* RAW 2 -- Mgmt Rx       */
#define MRF_RAW_2_CTRL_1_REG (0x19)
#define MRF_RAW_2_INDEX_REG (0x1a)
#define MRF_RAW_2_STATUS_REG (0x1b)

#define MRF_RAW_3_CTRL_0_REG (0x1c)      /* RAW 3 -- Mgmt Tx       */
#define MRF_RAW_3_CTRL_1_REG (0x1d)
#define MRF_RAW_3_INDEX_REG (0x1e)
#define MRF_RAW_3_STATUS_REG (0x1f)

#define MRF_RAW_4_CTRL_0_REG (0x0a)      /* RAW 4 -- Scratch Tx/Rx */
#define MRF_RAW_4_CTRL_1_REG (0x0b)
#define MRF_RAW_4_INDEX_REG (0x0c)
#define MRF_RAW_4_STATUS_REG (0x0d)

#define MRF_RAW_5_CTRL_0_REG (0x0e)      /* RAW 5 -- Not used      */
#define MRF_RAW_5_CTRL_1_REG (0x0f)
#define MRF_RAW_5_INDEX_REG (0x22)
#define MRF_RAW_5_STATUS_REG (0x23)

#define MRF_NUM_RAW_WINDOWS (6)	/* only using raw windows 0 thru 4 */

/* RAW register masks */
static const uint16_t MRF_RAW_STATUS_REG_BUSY_MASK = 0x0001;
static const uint16_t MRF_RAW_STATUS_REG_ERROR_MASK = 0x0002;

/* Supported RAW Windows */
enum raw_ids {
    MRF_MRF_RAW_ID_0 = 0,
    MRF_RAW_ID_1 = 1,
    MRF_RAW_ID_2 = 2,
    MRF_RAW_ID_3 = 3,
    MRF_RAW_ID_4 = 4,
    MRF_RAW_ID_5 = 5
};

/* Usage of RAW Windows */ // used in mrf.c and raw.c, see if i can put these in the raw header
enum raw_usage {
    MRF_RAW_DATA_RX_ID = MRF_MRF_RAW_ID_0,
    MRF_RAW_DATA_TX_ID = MRF_RAW_ID_1,
    MRF_RAW_UNUSED_2_ID = MRF_RAW_ID_2,
    MRF_RAW_UNUSED_3_ID = MRF_RAW_ID_3,
    MRF_RAW_SCRATCH_ID = MRF_RAW_ID_4,
    MRF_RAW_UNUSED_5_ID = MRF_RAW_ID_5
};

// Source/Destination objects on the MRF24W
static const int MRF_RAW_MAC = 0x00;   /* Cmd processor (aka MRF24W MAC) */
static const int MRF_RAW_MGMT_POOL = 0x01;   /* For 802.11 Management packets (no longer used) */
static const int MRF_RAW_DATA_POOL = 0x02;   /* Data Memory pool used for tx and rx operations */
static const int MRF_RAW_SCRATCH_POOL = 0x03;   /* Scratch object (used for mgmt tx/rx) */
static const int MRF_RAW_STACK_MEM = 0x04;   /* single level stack to save state of RAW (no longer needed) */
static const int MRF_RAW_COPY = 0x07;   /* RAW to RAW copy (no longer needed) */

uint16_t MRF_raw_move(const uint8_t id, const uint16_t src_dst,
			     const uint16_t size, uint16_t ctrl);
uint16_t MRF_raw_move_src(const uint8_t id, const uint16_t src,
 					const uint16_t size);
uint16_t MRF_raw_move_dst(const uint8_t id, const uint16_t dst,
 					const uint16_t size);
int MRF_raw_index_set(const uint8_t id, const uint16_t index);
uint16_t MRF_raw_index_get(const uint8_t id);
void MRF_raw_rel_read(const uint8_t id, uint8_t* const data,
    const size_t length);
void MRF_raw_rel_write(const uint8_t id, const uint8_t* const data,
    const size_t length);
int MRF_raw_read(const uint8_t id, const uint16_t index, uint8_t* const data,
    const size_t length);
int MRF_raw_write(const uint8_t id, const uint16_t index,
    const uint8_t* const data, const size_t length);
void MRF_scratch_mount(uint8_t id);
void MRF_scratch_umount(uint8_t id);
int MRF_raw_alloc_tx_buf(const uint16_t size);
void MRF_raw_send_data_frame(const uint16_t size);

#endif
