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

#include "mrf_raw.h"

#include "mrf_reg.h"
#include "mrf_com.h"
#include "mrf_timer.h"
#include "mrf_const.h"
#include "meos/mrf24g/mrf.h"

static const uint8_t s_RawIndexReg[MRF_NUM_RAW_WINDOWS] =
    { MRF_MRF_RAW_0_INDEX_REG, MRF_RAW_1_INDEX_REG, MRF_RAW_2_INDEX_REG,
	MRF_RAW_3_INDEX_REG,
	MRF_RAW_4_INDEX_REG, MRF_RAW_5_INDEX_REG
};

static const uint8_t s_RawStatusReg[MRF_NUM_RAW_WINDOWS] =
    { MRF_MRF_RAW_0_STATUS_REG, MRF_RAW_1_STATUS_REG, MRF_RAW_2_STATUS_REG,
	MRF_RAW_3_STATUS_REG,
	MRF_RAW_4_STATUS_REG, MRF_RAW_5_STATUS_REG
};

static const uint16_t s_RawCtrl0Reg[MRF_NUM_RAW_WINDOWS] =
    { MRF_MRF_RAW_0_CTRL_0_REG, MRF_RAW_1_CTRL_0_REG, MRF_RAW_2_CTRL_0_REG,
	MRF_RAW_3_CTRL_0_REG,
	MRF_RAW_4_CTRL_0_REG, MRF_RAW_5_CTRL_0_REG
};

static const uint16_t s_RawCtrl1Reg[MRF_NUM_RAW_WINDOWS] =
    { MRF_MRF_RAW_0_CTRL_1_REG, MRF_RAW_1_CTRL_1_REG, MRF_RAW_2_CTRL_1_REG,
	MRF_RAW_3_CTRL_1_REG,
	MRF_RAW_4_CTRL_1_REG, MRF_RAW_5_CTRL_1_REG
};

static const uint16_t s_RawDataReg[MRF_NUM_RAW_WINDOWS] =
    { MRF_RAW_0_DATA_REG, MRF_RAW_1_DATA_REG, MRF_RAW_2_DATA_REG,
	MRF_RAW_3_DATA_REG,
	MRF_RAW_4_DATA_REG, MRF_RAW_5_DATA_REG
};

static const uint32_t MRF_RAW_TIMEOUT = 500000;

static int MRF_raw_wait_while_busy(const uint8_t id, uint32_t timeout)
{
	const uint32_t start_count = MRF_count_get();
	uint16_t busy;
	do {
		if (MRF_timed_out(start_count, timeout)) {
			return -1;
		}
		busy = MRF_reg_read2(s_RawStatusReg[id]);
	} while ((busy & MRF_RAW_STATUS_REG_BUSY_MASK) != 0);
	return 0;
}

uint16_t MRF_raw_move(const uint8_t id, const uint16_t src_dst,
		      const uint16_t size, uint16_t ctrl)
{
	ctrl |= (src_dst << 12) | (size & 0x0fff);
	MRF_reg_write2(s_RawCtrl0Reg[id], ctrl);

	switch (id) {
	case MRF_MRF_RAW_ID_0:
		{
			const uint32_t start_count = MRF_count_get();	// start timer
			while (MRF_timed_out(start_count, MRF_RAW_TIMEOUT) == 0) {
				if (MRF_get_interrupt(MRF_INT_RAW0) != 0) {
					return MRF_reg_read2(s_RawCtrl1Reg[id])
					    & 0x0fff;
				}
			}
			break;
		}
	case MRF_RAW_ID_1:
		{
			const uint32_t start_count = MRF_count_get();	// start timer
			while (MRF_timed_out(start_count, MRF_RAW_TIMEOUT) == 0) {
				if (MRF_get_interrupt(MRF_INT_RAW1) != 0) {
					return MRF_reg_read2(s_RawCtrl1Reg[id])
					    & 0x0fff;
				}
			}
			break;
		}
	default:
		{
			const uint32_t start_count = MRF_count_get();	// start timer
			while (MRF_timed_out(start_count, MRF_RAW_TIMEOUT) == 0) {
				if (MRF_get_interrupt(MRF_INT_RAW4) != 0) {
					// this should only be executed when mounting
					// scratch
					return MRF_reg_read2(s_RawCtrl1Reg[id])
					    & 0x0fff;
				}
			}
			break;
		}
	}
	return 0;		// timeout
	// TODO: return something better on a timeout as I think at least the scratch moves are size 0
}

uint16_t MRF_raw_move_src(const uint8_t id, const uint16_t src,
			  const uint16_t size)
{
	return MRF_raw_move(id, src, size, 0);
}

uint16_t MRF_raw_move_dst(const uint8_t id, const uint16_t dst,
			  const uint16_t size)
{
	return MRF_raw_move(id, dst, size, 0x8000);
}

int MRF_raw_index_set(const uint8_t id, const uint16_t index)
{
	MRF_reg_write2(s_RawIndexReg[id], index);
	return MRF_raw_wait_while_busy(id, MRF_RAW_TIMEOUT);
}

uint16_t MRF_raw_index_get(const uint8_t id)
{
	return MRF_reg_read2(s_RawIndexReg[id]);
}

void MRF_raw_rel_read(const uint8_t id, uint8_t * const data,
		      const size_t length)
{
	MRF_array_read(s_RawDataReg[id], data, length);
}

void MRF_raw_rel_write(const uint8_t id, const uint8_t * const data,
		       const size_t length)
{
	MRF_array_write(s_RawDataReg[id], data, length);
}

int MRF_raw_read(const uint8_t id, const uint16_t index,
		 uint8_t * const data, const size_t length)
{
	if (MRF_raw_index_set(id, index) != 0) {
		return -1;
	}
	MRF_raw_rel_read(id, data, length);
	return 0;
}

int MRF_raw_write(const uint8_t id, const uint16_t index,
		  const uint8_t * const data, const size_t length)
{
	if (MRF_raw_index_set(id, index) != 0) {
		return -1;
	}
	MRF_raw_rel_write(id, data, length);
	return 0;
}

void MRF_scratch_mount(uint8_t id)
{
	(void)MRF_raw_move_dst(id, MRF_RAW_SCRATCH_POOL, 0);
}

void MRF_scratch_umount(uint8_t id)
{
	(void)MRF_raw_move_src(id, MRF_RAW_SCRATCH_POOL, 0);
}

int MRF_raw_alloc_tx_buf(const uint16_t size)
{
	// least significat 12 bits contain bytes available for data tx memory pool
	const uint16_t bytes_available =
	    MRF_reg_read2(MRF_HOST_WFIFO_BCNT0_REG) & 0x0fff;

	if (bytes_available < size) {
		return -1;
	}
	MRF_raw_move_dst(MRF_RAW_DATA_TX_ID, MRF_RAW_DATA_POOL, size);
	return 0;
}

void MRF_raw_send_data_frame(const uint16_t size)
{
	(void)MRF_raw_move_src(MRF_RAW_DATA_TX_ID, MRF_RAW_MAC, size);
}
