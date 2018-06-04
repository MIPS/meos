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

#ifndef MRF_MGMT_H
#define MRF_MGMT_H

#include <stdint.h>


#define MRF_MGMT_TX_BASE mgmt_tx_base              // start of mgmt tx buffer in scratch (host -> MRF24WG)
#define MRF_MGMT_RX_BASE (MRF_MGMT_TX_BASE + MRF_MGMT_BUF_SIZE)       // start of mgmt response buffer in scratch (MRF24WG -> host)
#define MRF_MGMT_INDICATE_BASE (MRF_MGMT_RX_BASE + MRF_MGMT_BUF_SIZE)       // start of mgmt indicate buffer in scratch (MRF24WG -> host)
#define MRF_MGMT_RX_ACK_BASE (MRF_MGMT_INDICATE_BASE + MRF_MGMT_BUF_SIZE) // signals MRF24WG that mgmt response read by host
#define MRF_MGMT_INDICATE_ACK_BASE (MRF_MGMT_RX_ACK_BASE + MRF_MGMT_ACK_SIZE)   // signals MRF24WG that mgmt indicate read by host

static const uint16_t MRF_MGMT_BUF_SIZE = 128;                  // all mgmt msgs fit within 128 bytes


typedef struct MRF_mgmt_rx_hdr
{
    uint8_t  type; /* always 0x02 */
    uint8_t  subtype; /* mgmt msg subtype */
    uint8_t  result; /* 1 if success, else failure */
    uint8_t  macState; /* not used */
} MRF_mgmt_rx_hdr_t;

extern uint16_t mgmt_tx_base;
extern uint16_t mrf_scan_results_count;

int MRF_mgmt_rx(void);
void MRF_mgmt_base_init(void);
int MRF_mgmt_send_nofree(const uint8_t* header, const uint8_t header_length,
    const uint8_t* data, const uint8_t data_length);
void MRF_mgmt_send_nowait(const uint8_t * header, const uint8_t header_length,
				 const uint8_t * data, const uint8_t data_length);
int MRF_mgmt_send(const uint8_t* header, const uint8_t header_length,
    const uint8_t* data, const uint8_t data_length);
int MRF_mgmt_send_read(const uint8_t* header, const uint8_t header_length,
    const uint8_t* data, const uint8_t data_length, const uint8_t read_index,
    uint8_t* read_data, uint8_t read_length);

#endif
