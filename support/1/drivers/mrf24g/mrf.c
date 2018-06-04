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

#include "meos/mrf24g/mrf.h"

#include "mrf_com.h"
#include "mrf_reg.h"
#include "mrf_raw.h"
#include "mrf_timer.h"
#include "mrf_mgmt.h"

/* Level 1 interrupts */
static const uint8_t MRF_L1INT_LVL2 = 0x01;
static const uint8_t MRF_L1INT_RAW0 = 0x02;	/* Data Rx */
static const uint8_t MRF_L1INT_RAW1 = 0x04;	/* Data Tx */
static const uint8_t MRF_L1INT_FIFO0 = 0x40;
static const uint8_t MRF_L1INT_FIFO1 = 0x80;

/* Level 2 interrupts */
static const uint16_t MRF_L2INT_RAW2 = 0x0010;	/* Mgmt Rx  */
static const uint16_t MRF_L2INT_RAW3 = 0x0020;	/* Mgmt Tx  */
static const uint16_t MRF_L2INT_RAW4 = 0x0004;	/* Scratch  */
static const uint16_t MRF_L2INT_RAW5 = 0x0008;	/* Not used */
static const uint16_t MRF_L2INT_MAILBOX0 = 0x0001;
static const uint16_t MRF_L2INT_MAILBOX1 = 0x0002;

static const unsigned long int MRF_RESET_TIMEOUT = 3000000;

MRF_IPCB_T input_cb;
void *input_cb_arg;

static uint8_t MRF_bitbang_port_get(uint8_t type)
{
	switch (type) {
	case MRF_ANALOG_PORT_3_REG_TYPE:
		return 2;
		break;
	case MRF_ANALOG_PORT_2_REG_TYPE:
		return 3;
		break;
	case MRF_ANALOG_PORT_1_REG_TYPE:
		return 1;
		break;
	case MRF_ANALOG_PORT_0_REG_TYPE:
		return 0;
		break;
	}
	return 0xff;
}

static int MRF_bitbang_write(uint8_t type, uint16_t address, uint16_t value)
{
	uint8_t port = MRF_bitbang_port_get(type);
	if (port == 0xff) {
		return -1;
	}
	uint16_t hr_base = MRF_HR_HOST_ANA_SPI_EN_MASK | (port << 6);
	MRF_reg_write2(MRF_HOST_RESET_REG, hr_base);
	uint8_t spi_address =
	    (address << 2) | MRF_SPI_AUTO_INCREMENT_ENABLED_MASK |
	    MRF_SPI_WRITE_MASK;
	uint8_t i = 0;
	while (i < 8) {
		uint16_t hr = hr_base | ((spi_address & 0x80) ?
					 MRF_HR_HOST_ANA_SPI_DOUT_MASK : 0);
		spi_address <<= 1;
		/* data and clock falling */
		MRF_reg_write2(MRF_HOST_RESET_REG, hr);
		/* clock rising */
		MRF_reg_write2(MRF_HOST_RESET_REG,
			       hr | MRF_HR_HOST_ANA_SPI_CLK_MASK);
		++i;
	}
	i = 0;
	while (i < 16) {
		uint16_t hr = hr_base | ((value & 0x8000) ?
					 MRF_HR_HOST_ANA_SPI_DOUT_MASK : 0);
		value <<= 1;
		/* data and clock falling */
		MRF_reg_write2(MRF_HOST_RESET_REG, hr);
		/* clock rising */
		MRF_reg_write2(MRF_HOST_RESET_REG,
			       hr | MRF_HR_HOST_ANA_SPI_CLK_MASK);
		++i;
	}
	MRF_reg_write2(MRF_HOST_RESET_REG,
		       (hr_base & ~MRF_HR_HOST_ANA_SPI_EN_MASK) |
		       MRF_HR_HOST_ANA_SPI_CLK_MASK);
	return 0;
}

static void MRF_pll_reset(void)
{
	/* workaround bitbang stuff */
	MRF_bitbang_write(MRF_ANALOG_PORT_3_REG_TYPE, MRF_PLL0_REG, 0x8021);
	MRF_bitbang_write(MRF_ANALOG_PORT_3_REG_TYPE, MRF_PLL0_REG, 0x6021);
	MRF_bitbang_write(MRF_ANALOG_PORT_1_REG_TYPE, MRF_OSC0_REG, 0x6b80);
	MRF_bitbang_write(MRF_ANALOG_PORT_1_REG_TYPE, MRF_BIAS_REG, 0xc000);
}

static void MRF_interrupt_enable(uint8_t mask)
{
	uint8_t val = MRF_reg_read1(MRF_HOST_MASK_REG) | mask;
	MRF_reg_write1(MRF_HOST_INTR_REG, mask);	/* clear interrupt flags */
	MRF_reg_write1(MRF_HOST_MASK_REG, val);
}

static void MRF_interrupt_disable(uint8_t mask)
{
	uint8_t val = MRF_reg_read1(MRF_HOST_MASK_REG) & ~mask;
	MRF_reg_write1(MRF_HOST_MASK_REG, val);
}

static void MRF_interrupt2_enable(uint16_t mask)
{
	uint16_t val = MRF_reg_read2(MRF_HOST_INTR2_MASK_REG) | mask;
	MRF_reg_write2(MRF_HOST_INTR2_REG, mask);	/* clear interrupt flags */
	MRF_reg_write2(MRF_HOST_INTR2_MASK_REG, val);
}

static void MRF_interrupt2_disable(const uint16_t mask)
{
	uint16_t val = MRF_reg_read2(MRF_HOST_INTR2_MASK_REG) & ~mask;
	MRF_reg_write2(MRF_HOST_INTR2_MASK_REG, val);
}

int MRF_rx_read(uint8_t * const buf, const size_t offset, const size_t size)
{
	return MRF_raw_read(MRF_RAW_DATA_RX_ID, offset, buf, size);
}

int MRF_tx_write(const uint8_t * const buf, const size_t offset,
		 const size_t size)
{
	return MRF_raw_write(MRF_RAW_DATA_TX_ID, offset, buf, size);
}

int MRF_get_interrupt(enum MRF_interrupt_types type)
{
	static uint8_t host1;
	static uint16_t host2;
	if (MRF_int_pending() != 0) {
		uint8_t val1 = MRF_reg_read1(MRF_HOST_INTR_REG);
		val1 &= MRF_reg_read1(MRF_HOST_MASK_REG);
		host1 |= val1 & 0xc7;
		if ((host1 & MRF_L1INT_LVL2) != 0) {
			uint16_t val2 = MRF_reg_read2(MRF_HOST_INTR2_REG);
			host2 |= val2 & 0x003f;
			MRF_reg_write2(MRF_HOST_INTR2_REG, val2);	/* clear host */
		}
		MRF_reg_write1(MRF_HOST_INTR_REG, val1);	/* clear host */
	}
	/* Level 1 interrupts */
	switch (type) {
	case MRF_INT_RAW0:
		if ((host1 & MRF_L1INT_RAW0) != 0) {
			host1 &= ~MRF_L1INT_RAW0;
			return 1;
		}
		break;
	case MRF_INT_RAW1:
		if ((host1 & MRF_L1INT_RAW1) != 0) {
			host1 &= ~MRF_L1INT_RAW1;
			return 1;
		}
		break;
	case MRF_INT_FIFO0:
		if ((host1 & MRF_L1INT_FIFO0) != 0) {
			host1 &= ~MRF_L1INT_FIFO0;
			return 1;
		}
		break;
	case MRF_INT_FIFO1:
		if ((host1 & MRF_L1INT_FIFO1) != 0) {
			host1 &= ~MRF_L1INT_FIFO1;
			return 1;
		}
		break;
	default:		/* lvl 2 */
		if ((host1 & MRF_L1INT_LVL2) != 0) {
			switch (type) {
			case MRF_INT_RAW2:
				if ((host2 & MRF_L2INT_RAW2) != 0) {
					host2 &= ~MRF_L2INT_RAW2;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			case MRF_INT_RAW3:
				if ((host2 & MRF_L2INT_RAW3) != 0) {
					host2 &= ~MRF_L2INT_RAW3;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			case MRF_INT_RAW4:
				if ((host2 & MRF_L2INT_RAW4) != 0) {
					host2 &= ~MRF_L2INT_RAW4;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			case MRF_INT_RAW5:
				if ((host2 & MRF_L2INT_RAW5) != 0) {
					host2 &= ~MRF_L2INT_RAW5;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			case MRF_INT_MAILBOX0:
				if ((host2 & MRF_L2INT_MAILBOX0) != 0) {
					host2 &= ~MRF_L2INT_MAILBOX0;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			case MRF_INT_MAILBOX1:
				if ((host2 & MRF_L2INT_MAILBOX1) != 0) {
					host2 &= ~MRF_L2INT_MAILBOX1;
					if (host2 == 0) {
						host1 &= ~MRF_L1INT_LVL2;
					}
					return 1;
				}
				break;
			default:
				break;
			}
		}
		break;
	}
	return 0;
}

int MRF_idle(void)
{
	if (MRF_get_interrupt(MRF_INT_FIFO0) != 0) {
		input_cb(input_cb_arg);
	}
	if (MRF_get_interrupt(MRF_INT_MAILBOX1) != 0) {
		if (((MRF_reg_read2(MRF_HOST_MAIL_BOX_1_MSW_REG)) >> 8)
		    == 0x60) {
			return MRF_mgmt_rx();
		}
	}
	return 0;
}

int MRF_init(MRF_IPCB_T _input_cb, void *_input_cb_arg)
{
	input_cb = _input_cb;
	input_cb_arg = _input_cb_arg;

	MRF_gpio_init();
	MRF_spi_init();
	MRF_wake();
	MRF_start();
	MRF_pll_reset();
	/* disable low power mode */
	MRF_reg_write2(MRF_PSPOLL_H_REG, 0x0000);
	/* set host reset bit to reset the device */
	MRF_reg_write2(MRF_HOST_RESET_REG,
		       MRF_reg_read2(MRF_HOST_RESET_REG) | MRF_HOST_RESET_MASK);
	/* clear host reset bit */
	MRF_reg_write2(MRF_HOST_RESET_REG,
		       MRF_reg_read2(MRF_HOST_RESET_REG) &
		       ~MRF_HOST_RESET_MASK);
	/* poll for reset completed */
	uint16_t val = 0;
	uint32_t start_count = MRF_count_get();
	while ((val & MRF_HW_STATUS_NOT_IN_RESET_MASK) == 0) {
		if ((MRF_timed_out(start_count, MRF_RESET_TIMEOUT)) != 0) {	/* timeout */
			return -1;
		}
		MRF_reg_write2(MRF_INDEX_ADDR_REG, MRF_HW_STATUS_REG);
		val = MRF_reg_read2(MRF_INDEX_DATA_REG);
	}

	if (val == 0xffff) {	/* spi likely not connected */
		return -2;
	}
	/* wait for byte count */
	val = 0;
	start_count = MRF_count_get();
	while (val == 0) {
		if ((MRF_timed_out(start_count, MRF_RESET_TIMEOUT)) != 0) {	/* timeout */
			return -1;
		}
		val = MRF_reg_read2(MRF_HOST_WFIFO_BCNT0_REG);
	}

	/* init interrupts */
	MRF_interrupt2_disable(MRF_HOST_2_INT_MASK_ALL_INT);
	MRF_interrupt_disable(MRF_HOST_INT_MASK_ALL_INT);
	MRF_interrupt_enable(MRF_HOST_INT_MASK_FIFO_0_THRESHOLD |	/* Data Rx Msg interrupt */
			     MRF_HOST_INT_MASK_RAW_0_INT_0 |	/* RAW0 Move Complete (Data Rx) interrupt */
			     MRF_HOST_INT_MASK_RAW_1_INT_0 |	/* RAW1 Move Complete (Data Tx) interrupt */
			     MRF_HOST_INT_MASK_INT2);
	MRF_interrupt2_enable(MRF_HOST_2_INT_MASK_RAW_4_INT_0 |	/* RAW4 Move Complete (Scratch) interrupt */
			      MRF_HOST_2_INT_MASK_MAIL_BOX_1_WRT);

	/* raw init */
	MRF_scratch_umount(MRF_RAW_ID_1);
	MRF_scratch_mount(MRF_RAW_SCRATCH_ID);

	MRF_mgmt_base_init();
	MRF_VINFO_T info;
	if (MRF_versionGet(&info) != 0) {
		return -1;
	}
	if ((info.version != 0x30) && (info.version != 0x31)) {
		return -1;
	}
	if (MRF_pctxCreate() != 0) {
		return -1;
	}
	uint8_t mac[6];
	if (MRF_macGet(mac) != 0) {
		return -1;
	}
	uint8_t domain;
	return MRF_domainGet(&domain);
}
