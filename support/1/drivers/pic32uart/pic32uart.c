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
*   Description:	PIC32 UART driver
*
*************************************************************************/

#include "MEOS.h"

#define	UxMODE(U)	U->regs[0x10/4]
#define	ON	0x8000
#define	STSEL	1
#define	PDSEL_9N	6
#define	PDSEL_8O	4
#define	PDSEL_8E	2
#define	PDSEL_8N	0
#define	UEN_CTSRTS	0x200
#define	UxSTA(U)	U->regs[0x10/4]
#define	UxSTACLR(U)	U->regs[0x14/4]
#define	UxSTASET(U)	U->regs[0x18/4]
#define	URXDA 0x1
#define	TRMT 0x100
#define	UTXBF 0x200
#define	UTXEN 0x400
#define	URXEN 0x1000
#define	UxTXREG(U)	U->regs[0x20/4]
#define	UxRXREG(U)	U->regs[0x30/4]
#define	UxBRG(U)	U->regs[0x40/4]

void PIC32_UART_rxInt(int32_t sig)
{
	IRQ_DESC_T *desc = IRQ_ack(IRQ_cause(sig));
	PIC32_UART_T *uart = (PIC32_UART_T *) desc->priv;

	if (UxSTA(uart) & URXDA) {
		RING_write(&uart->uart.rx, UxRXREG(uart), 0);
		if (uart->uart.rFunc)
			uart->uart.rFunc(uart->uart.rPar);
	}
}

void PIC32_UART_txInt(int32_t sig)
{
	uint8_t c;
	IRQ_DESC_T *desc = IRQ_cause(sig);
	PIC32_UART_T *uart = (PIC32_UART_T *) desc->priv;

	while ((!(UxSTA(uart) & UTXBF)) && (RING_read(&uart->uart.tx, &c, 0)))
		UxTXREG(uart) = c;
	if (UxSTA(uart) & TRMT)
		UxSTACLR(uart) = UTXEN;
	IRQ_ack(desc);
}

int32_t PIC32_UART_config(UART_T * uuart, const char *config)
{
	PIC32_UART_T *uart = (PIC32_UART_T *) uuart;
	const char *p = config;
	uint32_t baud = 0, data = 8, parity = 0;
	uint8_t mode = 0;

	while ((*p >= '0') && (*p <= '9')) {
		baud = baud * 10;
		baud += *p - '0';
		p++;
	}
	switch (*p) {
	case 0:
		goto done;
	case 'n':
	case 'N':
		break;
	case 'o':
	case 'O':
		parity = 1;
		break;
	case 'e':
	case 'E':
		parity = 2;
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
	case '8':
		break;
	case '9':
		data = 9;
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
		mode |= STSEL;
		break;

	default:
		DBG_assert(0,
			   "'%s' is not a valid UART configuration string!\n",
			   config);
		return -1;
	}
	p++;
	if ((parity != 0) && (data == 9)) {
		DBG_assert(0,
			   "'%s' is not a valid UART configuration string!\n",
			   config);
		return -1;
	}
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

	UxBRG(uart) = (uart->clock / (16 * baud)) - 1;
	if (data == 9)
		mode |= PDSEL_9N;
	else {
		switch (parity) {
		default:
		case 0:
			mode |= PDSEL_8N;
			break;
		case 1:
			mode |= PDSEL_8O;
			break;
		case 2:
			mode |= PDSEL_8E;
			break;
		}
	}
	UxMODE(uart) = mode;
	return 0;
}

void PIC32_UART_enableTXEmptyInt(UART_T * uuart)
{
	PIC32_UART_T *uart = (PIC32_UART_T *) uuart;
	UxSTASET(uart) = UTXEN;
}

void PIC32_UART_init(PIC32_UART_T * uart, uint8_t * txBuf, size_t txLen,
		     uint8_t * rxBuf, size_t rxLen, IRQ_DESC_T * txi,
		     IRQ_DESC_T * rxi)
{
	uart->regs = MEM_p2v(uart->pAddr, MEM_P2V_UNCACHED);
	uart->uart.config = PIC32_UART_config;
	uart->uart.enableTXEmptyInt = PIC32_UART_enableTXEmptyInt;
	RING_init(&uart->uart.tx, txBuf, txLen);
	RING_init(&uart->uart.rx, rxBuf, rxLen);
	txi->isrFunc = PIC32_UART_txInt;
	txi->priv = (void *)uart;
	rxi->isrFunc = PIC32_UART_rxInt;
	rxi->priv = (void *)uart;

	UxBRG(uart) = (uart->clock / (16 * 9600)) - 1;
	UxSTA(uart) = 0;
	UxMODE(uart) = ON;
	IRQ_route(txi);
	IRQ_route(rxi);
	UxSTASET(uart) = URXEN;
}
