/***(C)2001***************************************************************
*
* Copyright (C) 2001 MIPS Tech, LLC
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
****(C)2001**************************************************************/

/*************************************************************************
*
*   Description:	NS16550A driver
*
*************************************************************************/

#include "MEOS.h"

#define IIR 4
#define IID 0x0e
#define IID_SHIFT 1
#define RDA 0x02
#define CTI 0x06
#define LSR 10
#define DR 0x01
#define RBR 0
#define THRE 0x20
#define THR 0
#define TEMT 0x40
#define IER 2
#define ETBEI 0x02
#define LCR 6
#define DLAB 0x80
#define DLL 0
#define DLM 2
#define WLS0 1
#define WLS1 2
#define STB 4
#define PEN 8
#define EPS 16
#define MCR 8
#define FCR 4
#define RXCLR 0x02
#define TXCLR 0x04
#define RTL 0x40
#define RTM 0x80
#define EN 0x01
#define ERBI 0x01

void NS16550A_regWrite(NS16550A_T * uart, uintptr_t offset, uint8_t value)
{
	uint8_t *reg = (uint8_t *) uart->vAddr + (offset << uart->pitch);
	switch (uart->width) {
	case 1:
		*reg = value;
		break;
	case 2:
		{
			uint16_t *s = (uint16_t *) reg;
			*s = value;
			break;
		}
	case 4:
		{
			uint32_t *l = (uint32_t *) reg;
			*l = value;
			break;
		}
	}
}

uint8_t NS16550A_regRead(NS16550A_T * uart, uintptr_t offset)
{
	uint8_t *reg = (uint8_t *) uart->vAddr + (offset << uart->pitch);
	switch (uart->width) {
	case 1:
		return *reg;
		break;
	case 2:
		{
			uint16_t *s = (uint16_t *) reg;
			return (uint8_t) * s;
		}
	case 4:
		{
			uint32_t *l = (uint32_t *) reg;
			return (uint8_t) * l;
		}
	}
	return 0;
}

#define RR NS16550A_regRead
#define RW NS16550A_regWrite

void NS16550A_isr(int32_t sig)
{
	IRQ_DESC_T *desc = IRQ_cause(sig);
	NS16550A_T *uart = (NS16550A_T *) desc->priv;
	uint32_t iid = ((RR(uart, IIR) & IID) >> IID_SHIFT);
	if ((iid == RDA) || (iid == CTI)) {
		if (RR(uart, LSR) & DR) {
			RING_write(&uart->uart.rx, RR(uart, RBR), 0);
			if (uart->uart.rFunc)
				uart->uart.rFunc(uart->uart.rPar);
		}
	} else {
		uint8_t c;

		while ((RR(uart, LSR) & THRE)
		       && (RING_read(&uart->uart.tx, &c, 0)))
			RW(uart, THR, c);
		if (RR(uart, LSR) & TEMT)
			RW(uart, IER, RR(uart, IER) & ~ETBEI);
	}
	IRQ_ack(desc);
}

int32_t NS16550A_config(UART_T * uuart, const char *config)
{
	NS16550A_T *uart = (NS16550A_T *) uuart;
	const char *p = config;
	uint32_t baud = 0;
	uint16_t d;
	uint8_t lcr = 0;

	while ((*p >= '0') && (*p <= '9')) {
		baud = baud * 10;
		baud += *p - '0';
		p++;
	}
	d = uart->clock / baud;
	switch (*p) {
	case 0:
		goto done;
	case 'n':
	case 'N':
		break;
	case 'o':
	case 'O':
		lcr |= PEN;
		break;
	case 'e':
	case 'E':
		lcr |= PEN | EPS;
		break;
	default:
		DBG_assert(0,
			   "'%s' is not a valid UART configuration string!\n",
			   config);
		return -1;
	}
	p++;
	switch (*p) {
	case 0:
		goto done;
	case '5':
		break;
	case '6':
		lcr |= WLS0;
		break;

	case '7':
		lcr |= WLS1;
		break;
	case '8':
		lcr |= WLS0 | WLS1;
		break;
	default:
		DBG_assert(0,
			   "'%s' is not a valid UART configuration string!\n",
			   config);
		return -1;
	}
	p++;
	switch (*p) {
	case 0:
		goto done;
		/*case 'r':
		   case 'R':
		   flow = 1;
		   goto done; */
	case '1':
		break;
	case '2':
		lcr |= STB;
		break;

	default:
		DBG_assert(0,
			   "'%s' is not a valid UART configuration string!\n",
			   config);
		return -1;
	}
	p++;
	/*switch (*p) {
	   case 0:
	   goto done;
	   case 'r':
	   case 'R':
	   flow = 1;
	   break;
	   default:
	   DBG_assert(0,
	   "'%s' is not a valid UART configuration string!\n",
	   config);
	   return -1;
	   } */
      done:

	RW(uart, LCR, RR(uart, LCR) | DLAB);	/* Enable access to divisors */
	RW(uart, DLL, d & 0xff);
	RW(uart, DLM, d >> 8);
	RW(uart, LCR, lcr);	/* Disable access to divisors, select data/parity/stop */
	return 0;
}

void NS16550A_enableTXEmptyInt(UART_T * uuart)
{
	NS16550A_T *uart = (NS16550A_T *) uuart;
	RW(uart, IER, RR(uart, IER) | ETBEI);
}

void NS16550A_init(NS16550A_T * uart, uint8_t * txBuf, size_t txLen,
		   uint8_t * rxBuf, size_t rxLen, IRQ_DESC_T * irq)
{
	uart->vAddr = MEM_p2v(uart->pAddr, MEM_P2V_UNCACHED);
	uart->uart.config = NS16550A_config;
	uart->uart.enableTXEmptyInt = NS16550A_enableTXEmptyInt;
	RING_init(&uart->uart.tx, txBuf, txLen);
	RING_init(&uart->uart.rx, rxBuf, rxLen);
	irq->isrFunc = NS16550A_isr;
	irq->priv = (void *)uart;
	RW(uart, LCR, RR(uart, LCR) | DLAB);	/* Enable access to divisors */
	RW(uart, DLL, 24);	/* 9600 on malta */
	RW(uart, DLM, 0);
	RW(uart, LCR, WLS0 | WLS1);	/* Disable access to divisors, select 8N1 */
	RW(uart, MCR, 0);	/* No flow control */
	RW(uart, FCR, RXCLR | TXCLR | RTL | RTM);	/* Buffer up 14 bytes before rx int */
	KRN_delay(100);
	RW(uart, FCR, EN | RTL | RTM);	/* Enable */
	RW(uart, IER, ERBI);
	IRQ_route(irq);
}
