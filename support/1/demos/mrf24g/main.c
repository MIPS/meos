/***(C)2016***************************************************************
*
* Copyright (C) 2016 MIPS Tech, LLC
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
****(C)2016**************************************************************/

/*************************************************************************
*
*   Description:        MRF24G demo
*
*************************************************************************/
#include "MEOS.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define TSTACKSIZE (2000)	/* MEOS timer task stack size */
#define STACKSIZE (2000)
#define PRIORITIES (5)
#define MAX_PRIORITY (PRIORITIES -1)

#define MAX_SCAN_RESULTS (32)

#define PB_CLK (100000000UL)
#define BAUDRATE (115200)

static KRN_TASK_T *bgtask;
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

static void uart_init(void)
{
	/* disable */
	U4MODE = 0;

	/* pps */
	RPF8R = 0x02;

	U4BRG = PB_CLK / (4 * BAUDRATE) - 1;

	U4STASET = _U4STA_UTXEN_MASK;
	U4MODESET = _U4MODE_BRGH_MASK;
	U4MODESET = _U4MODE_ON_MASK;
}

size_t uart_tx(const uint8_t * data, size_t size)
{
	size_t i = 0;
	while (i < size) {
		while (U4STAbits.TRMT == 0) {
		}
		U4TXREG = data[i];
		++i;
	}
	return i;
}

size_t uart_tx_str(const char *str)
{
	size_t i = 0;
	while (str[i] != 0) {
		while (U4STAbits.TRMT == 0) {
		}
		U4TXREG = str[i];
		++i;
	}
	return i;
}

void digital_io_setup(void)
{
	ANSELA = 0;
	ANSELB = 0;
	ANSELC = 0;
	ANSELD = 0;
	ANSELE = 0;
	ANSELF = 0;
	ANSELG = 0;
}

int MRFNETIF_idle(void);

int scan()
{
	if (MRF_scanTypeSet(MRF_ACTIVE_SCAN) != 0) {
		return -1;
	}
	if (MRF_scanCountSet(1) != 0) {
		return -1;
	}
	if (MRF_scanMinChannelTimeSet(200) != 0) {	/* ms */
		return -1;
	}
	if (MRF_scanMaxChannelTimeSet(400) != 0) {	/* ms */
		return -1;
	}
	if (MRF_scanProbeDelaySet(20) != 0) {	/* us */
		return -1;
	}
	const uint8_t channels[] = { };
	if (MRF_channelListSet(channels, sizeof(channels) / sizeof(channels[0]))
	    != 0) {
		return -1;
	}
	if (MRF_scan() != 0) {
		return -1;
	}
	uint16_t res_count = 0;
	while (res_count == 0) {
		MRFNETIF_idle();
		res_count = MRF_scanGetResultsCount();
		/* yeild */
	}
	MRF_SCAN_RESULT_T result;
	uint16_t i = 0;
	while (i < res_count) {
		if (MRF_scanGetResult(&result, i) != 0) {
			return -1;
		}
		uart_tx(result.ssid, result.ssidLen);
		uint8_t buf[36];
		sprintf((char *)buf, "\r\nrssi: %u\r\nchannel: %u\r\n",
			result.rssi, result.channel);
		uart_tx_str((char *)buf);
		++i;
	}
	return 0;
}

int main()
{
	digital_io_setup();
	uart_init();

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	bgtask = KRN_startOS("Background Task");

	MRFNETIF_T mrfnetif;

	LWIP_T lwip;
	LWIP_init(&lwip);
	tcpip_init(NULL, NULL);

	IRQ_DESC_T irq;
	MRFNETIF_dtInit(&mrfnetif, &irq);
	netif_set_default(&mrfnetif.netif);

	uart_tx_str("scanning\r\n");
	if (scan() != 0) {
		uart_tx_str("scan failed\r\n");
	}
	uart_tx_str("scan complete\r\n");

	return 0;
}
