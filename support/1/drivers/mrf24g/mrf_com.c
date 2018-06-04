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

#include "mrf_com.h"

#include "MEOS.h"

static const unsigned long int MRF_SPI_BAUDRATE = 10000000;

/*void MRF_eint_enable(void)
{
	if (PORTAbits.RA15 == 0) {
		IFS0SET = 1 << 23;
	}

	IEC0SET = 1 << 23;
}

void MRF_eint_disable(void)
{
	IEC0CLR = 1 << 23;
}

int MRF_eint_is_enabled(void)
{
	return ((IEC0 & (1 << 23)) != 0) ? 1 : 0;
}*/

void MRF_reset(void)
{
	LATGbits.LATG1 = 0;
}

// bring mrf out of reset
void MRF_start(void)
{
	LATFbits.LATF4 = 1;
}

void MRF_hibernate(void)
{
	LATFbits.LATF4 = 1;
}

void MRF_wake(void)
{
	LATGbits.LATG1 = 0;
}

int MRF_int_pending(void)
{
	return ((PORTAbits.RA15 == 0) ? 1 : 0);
}

void MRF_gpio_init(void)
{
	// reset
	LATFbits.LATF4 = 0;
	TRISFbits.TRISF4 = 0;
	// interrupt
	//IEC0CLR = 1 << 23;
	// configure INT1 pin
	LATAbits.LATA15 = 1;	// init
	TRISAbits.TRISA15 = 1;
	//INTCONbits.INT4EP = 0;        // falling edge triggered
	//IFS0CLR = 1 << 23;    // clear INT1IF bit (clears any pending interrupt)
	//IPC5CLR = 63 << 24;
	//IPC5SET = 3 << 26;    // priority 3
	//IPC5SET = 0;          // sub priority 0
	//IEC0SET = 1 << 23;    // enable
	// hibernate
	LATGbits.LATG1 = 1;
	TRISGbits.TRISG1 = 0;
}

void MRF_spi_cs_clear(void)
{
	LATDbits.LATD9 = 0;
}

void MRF_spi_cs_set(void)
{
	LATDbits.LATD9 = 1;
}

// blocking SPI
static uint8_t MRF_spi_byte_txrx(uint8_t c)
{
	SPI4BUF = c;
	while (!SPI4STATbits.SPITBE || !SPI4STATbits.SPIRBF) {
	}
	c = SPI4BUF;
	return c;
}

void MRF_spi_byte_tx(const uint8_t c)
{
	MRF_spi_byte_txrx(c);
}

uint8_t MRF_spi_byte_rx(void)
{
	return MRF_spi_byte_txrx(0xff);
}

void MRF_spi_tx(const uint8_t * data, const size_t length)
{
	const uint8_t *const lim = data + length;
	while (data != lim) {
		MRF_spi_byte_tx(*data);
		++data;
	}
}

void MRF_spi_rx(uint8_t * data, const size_t length)
{
	uint8_t *const lim = data + length;
	while (data != lim) {
		*data = MRF_spi_byte_rx();
		++data;
	}
}

void MRF_spi_nocs(const const uint8_t * tx_data,
		  const size_t tx_length, uint8_t * rx_data,
		  const size_t rx_length)
{
	MRF_spi_tx(tx_data, tx_length);
	MRF_spi_rx(rx_data, rx_length);
}

void MRF_spi(const uint8_t * tx_data, const size_t tx_length,
	     uint8_t * rx_data, const size_t rx_length)
{
	MRF_spi_cs_clear();
	MRF_spi_nocs(tx_data, tx_length, rx_data, rx_length);
	MRF_spi_cs_set();
}

void MRF_spi_init(void)
{
	// PPS
	SDI4R = 0x02;		// SDI4     RF5
	RPG0R = 0x08;		// SDO4     RG0
	// CS
	LATDbits.LATD9 = 1;
	TRISDbits.TRISD9 = 0;
	// disable
	SPI4CONbits.ON = 0;
	// flush
	volatile const uint8_t tmp = SPI4BUF;
	(void)tmp;
	const uint32_t pclk = (SYS_CLK / (PB2DIVbits.PBDIV + 1));
	SPI4BRG = (pclk / (2 * MRF_SPI_BAUDRATE)) - 1;
	SPI4CONbits.CKP = 1;	// idle high
	SPI4CONbits.MSTEN = 1;	// Master
	SPI4CONbits.SMP = 1;	// sample data at the end of output time
	// enable
	SPI4CONbits.ON = 1;
}
