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

#include "mrf_mgmt.h"

#include "mrf_raw.h"
#include "mrf_timer.h"
#include "mrf_com.h"
#include "mrf_reg.h"
#include "mrf_const.h"
#include "meos/mrf24g/mrf.h"

static const uint16_t MRF_MGMT_ACK_SIZE = 1;	// all mgmt ack's are one byte

static const uint8_t MRF_MGMT_RESPONSE_SET_FLAG = 0xa5;	// written by MRF24WG to signal new mgmt response
static const uint8_t MRF_MGMT_RESPONSE_CLEAR_FLAG = 0x5a;	// written by host to signal mgmt response has been read

static const uint8_t MRF_MGMT_INDICATE_CLEAR_FLAG = 0x5a;	// written by host to signal mgmt indicate has been read
static const uint8_t MRF_MGMT_INDICATE_SET_FLAG = 0xa5;	// written by MRF24WG to signal new mgmt indicate

uint16_t mgmt_tx_base;
uint16_t mrf_scan_results_count;

static const uint32_t MRF_MGMT_TIMEOUT = 5000000;

static int MRF_mgmt_handle_indicate(void)
{
	uint8_t buf[6];
	uint16_t event_info;
	// read mgmt indicate header
	uint8_t hdr[2];
	if (MRF_raw_read
	    (MRF_RAW_SCRATCH_ID, MRF_MGMT_INDICATE_BASE, hdr, sizeof(hdr))
	    != 0) {
		return -1;
	}
	if (hdr[0] != MRF_MGMT_INDICATE_TYPE) {
		return -1;
	}
	switch (hdr[1]) {
	case MRF_EVENT_CONNECTION_ATTEMPT_STATUS_SUBTYPE:
		MRF_raw_rel_read(MRF_RAW_SCRATCH_ID, buf, 2);
		if (buf[0] == CONNECTION_ATTEMPT_SUCCESSFUL) {
			event_info = MRF_NO_ADDITIONAL_INFO;
		} else {
			event_info = (uint16_t) (buf[0] << 8 | buf[1]);	/* contains connection failure code */
		}
		break;
	case MRF_EVENT_CONNECTION_LOST_SUBTYPE:
		MRF_raw_rel_read(MRF_RAW_SCRATCH_ID, buf, 2);
		if (buf[0] == CONNECTION_TEMPORARILY_LOST) {
			event_info = (uint16_t) buf[1];	/* lost due to beacon timeout or deauth */
		} else if (buf[0] == CONNECTION_PERMANENTLY_LOST) {
			event_info = (uint16_t) buf[1];	/* lost due to beacon timeout or deauth */
		} else if (buf[0] == CONNECTION_REESTABLISHED) {
			event_info = (uint16_t) buf[1];	/* originally lost due to beacon timeout or deauth */
		} else {
			return -1;
		}
		break;
	case MRF_EVENT_KEY_CALCULATION_REQUEST_SUBTYPE:
		break;
	case MRF_EVENT_SCAN_RESULTS_READY_SUBTYPE:
		MRF_raw_rel_read(MRF_RAW_SCRATCH_ID, buf, 1);

		mrf_scan_results_count = (uint16_t) buf[0];	/* number of scan results */
		break;
	case MRF_EVENT_SCAN_IE_RESULTS_READY_SUBTYPE:
		MRF_raw_rel_read(MRF_RAW_SCRATCH_ID, (uint8_t *) & event_info,
				 2);
		break;
	case MRF_EVENT_SOFT_AP_EVENT_SUBTYPE:
		break;
	case MRF_EVENT_DISCONNECT_DONE_SUBTYPE:
		break;
	default:
		return -1;
	}
	return 0;
}

int MRF_mgmt_rx(void)
{
	uint8_t mb;
	if (MRF_raw_read(MRF_RAW_SCRATCH_ID, MRF_MGMT_RX_ACK_BASE, &mb, 1) != 0) {
		return -1;
	}
	if (mb == MRF_MGMT_RESPONSE_SET_FLAG) {
		// is mgmt response clear the mailbox
		const uint8_t rcf = MRF_MGMT_RESPONSE_CLEAR_FLAG;
		if (MRF_raw_write(MRF_RAW_SCRATCH_ID, MRF_MGMT_RX_ACK_BASE,
				  &rcf, 1) != 0) {
			return -1;
		}
	} else {
		// test for indicate
		if (MRF_raw_read
		    (MRF_RAW_SCRATCH_ID, MRF_MGMT_INDICATE_ACK_BASE, &mb, 1)
		    != 0) {
			return -1;
		}
		if (mb == MRF_MGMT_INDICATE_SET_FLAG) {
			// is mgmt indicate clear the mailbox
			const uint8_t rcf = MRF_MGMT_INDICATE_CLEAR_FLAG;
			if (MRF_raw_write
			    (MRF_RAW_SCRATCH_ID, MRF_MGMT_INDICATE_ACK_BASE,
			     &rcf, 1) != 0) {
				return -1;
			}
			if (MRF_mgmt_handle_indicate() != 0) {
				return -1;
			}
		}
	}
	return 0;
}

static int MRF_mgmt_wait_nofree(void)
{
	const uint32_t start_count = MRF_count_get();
	while (MRF_timed_out(start_count, MRF_MGMT_TIMEOUT) == 0) {
		if (MRF_get_interrupt(MRF_INT_MAILBOX1) != 0) {
			if (((MRF_reg_read2(MRF_HOST_MAIL_BOX_1_MSW_REG)) >> 8)
			    == 0x60) {
				return MRF_mgmt_rx();
			}
		}
	}
	return -1;		// timeout
}

void MRF_mgmt_base_init(void)
{
	MRF_reg_write2(MRF_INDEX_ADDR_REG, MRF_SCRATCHPAD_0_REG);
	mgmt_tx_base = MRF_reg_read2(MRF_INDEX_DATA_REG);
}

static int MRF_mgmt_wait(void)
{
	int ret = MRF_mgmt_wait_nofree();
	if (ret != 0) {
		return ret;
	}
	// free
	MRF_mgmt_rx_hdr_t hdr;
	ret =
	    MRF_raw_read(MRF_RAW_SCRATCH_ID, MRF_MGMT_RX_BASE,
			 (uint8_t *) & hdr,
			 (uint16_t) (sizeof(MRF_mgmt_rx_hdr_t)));
	if (ret != 0) {
		return ret;
	}
	if (hdr.result != 1) {
		return -1;
	}
	return 0;
}

static int MRF_mgmt_wait_read(const uint8_t index, uint8_t * data,
			      const uint8_t length)
{
	int ret = MRF_mgmt_wait();
	if (ret != 0) {
		return ret;
	}

	return MRF_raw_read(MRF_RAW_SCRATCH_ID, MRF_MGMT_RX_BASE + index, data,
			    length);
}

void MRF_mgmt_send_nowait(const uint8_t * header, const uint8_t header_length,
			  const uint8_t * data, const uint8_t data_length)
{
	// write header
	MRF_raw_write(MRF_RAW_SCRATCH_ID, MRF_MGMT_TX_BASE, header,
		      header_length);
	// write data
	MRF_raw_rel_write(MRF_RAW_SCRATCH_ID, data, data_length);
	// signal that mgmt message has been writen
	MRF_reg_write2(MRF_HOST_MAIL_BOX_0_MSW_REG, 0x0400);
	MRF_reg_write2(MRF_HOST_MAIL_BOX_0_LSW_REG, 0x0000);
}

int MRF_mgmt_send_nofree(const uint8_t * header, const uint8_t header_length,
			 const uint8_t * data, const uint8_t data_length)
{
	MRF_mgmt_send_nowait(header, header_length, data, data_length);
	return MRF_mgmt_wait_nofree();
}

int MRF_mgmt_send(const uint8_t * header, const uint8_t header_length,
		  const uint8_t * data, const uint8_t data_length)
{
	MRF_mgmt_send_nowait(header, header_length, data, data_length);
	return MRF_mgmt_wait();
}

int MRF_mgmt_send_read(const uint8_t * header, const uint8_t header_length,
		       const uint8_t * data, const uint8_t data_length,
		       const uint8_t read_index, uint8_t * read_data,
		       uint8_t read_length)
{
	MRF_mgmt_send_nowait(header, header_length, data, data_length);
	return MRF_mgmt_wait_read(read_index, read_data, read_length);
}
