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

#include "mrf_reg.h"

#include "mrf_com.h"

static const uint8_t MRF_READ_REG_MASK = 0x40;

uint8_t MRF_reg_read1(const uint8_t id)
{
	const uint8_t tx_data = id | MRF_READ_REG_MASK;
	uint8_t rx_data;
	MRF_spi(&tx_data, 1, &rx_data, 1);
	return rx_data;
}

void MRF_reg_write1(const uint8_t id, const uint8_t data)
{
	const uint8_t tx_data[] = { id, data };
	MRF_spi(tx_data, 2, 0, 0);
}

uint16_t MRF_reg_read2(const uint8_t id)
{
	const uint8_t tx_data = id | MRF_READ_REG_MASK;
	uint8_t rx_data[2];
	MRF_spi(&tx_data, 1, rx_data, 2);
	return (((uint16_t) rx_data[0]) << 8) | rx_data[1];
}

void MRF_reg_write2(const uint8_t id, const uint16_t data)
{
	const uint8_t tx_data[] = { id, data >> 8, data & 0xff };
	MRF_spi(tx_data, 3, 0, 0);
}

void MRF_array_read(const uint8_t id, uint8_t * data, const size_t length)
{
	if (length > 0) {
		const uint8_t tx_data = id | MRF_READ_REG_MASK;
		MRF_spi(&tx_data, 1, data, length);
	}
}

void MRF_array_write(const uint8_t id, const uint8_t * data,
		     const size_t length)
{
	if (length > 0) {
		MRF_spi_cs_clear();
		MRF_spi_nocs(&id, 1, 0, 0);
		MRF_spi_nocs(data, length, 0, 0);
		MRF_spi_cs_set();
	}
}
