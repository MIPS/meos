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
*   Description:	FDC driver
*
*************************************************************************/

#include "MEOS.h"

/* DA.WriteDASetting("Max FDC Channels", 16) */

static volatile uint32_t *CDMMBASE;
#define FDACSR CDMMBASE[0x0/sizeof(uint32_t)]
#define FDCFG CDMMBASE[0x8/sizeof(uint32_t)]
#define FDSTAT CDMMBASE[0x10/sizeof(uint32_t)]
#define FDRX CDMMBASE[0x18/sizeof(uint32_t)]
#define FDTX(N) CDMMBASE[(0x20 + (N * 8))/sizeof(uint32_t)]
#define TX_INTTHRESH	0xc0000
#define TX_INTTHRESH_SHIFT 18
#define RX_INTTHRESH	0x30000
#define RX_INTTHRESH_SHIFT 16
#define TXF	1
#define RXE	8
#define RXCHAN	0xf0
#define RXCHAN_SHIFT	4

RING_T *FDC_RXRings[16];
RING_T *FDC_TXRings[16];
FDC_T *FDC_Ts[16];
IRQ_DESC_T FDC_desc;

inline static void *FDC_cdmmBase()
{
	reg_t r = _m32c0_mfc0(15, 2);
	reg_t p = (r & 0x0ffffc00) << 4;
	if (r & 0x200)
		p += 64;
	return MEM_p2v(p, MEM_P2V_UNCACHED);
}

inline static void FDC_cdmmOn()
{
	_m32c0_mtc0(15, 2, _m32c0_mfc0(15, 2) | 0x400);
}

inline static void FDC_cdmmOff()
{
	_m32c0_mtc0(15, 2, _m32c0_mfc0(15, 2) & ~0x400);
}

inline static int32_t FDC_TXInt()
{
	int32_t r;
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	FDC_cdmmOn();
	r = (FDCFG & TX_INTTHRESH) >> TX_INTTHRESH_SHIFT;
	FDC_cdmmOff();
	IRQ_restoreIPL(ipl);
	return r;
}

inline static void FDC_SetTXInt(int32_t on)
{
	FDCFG = (FDCFG & ~TX_INTTHRESH) | (on << TX_INTTHRESH_SHIFT);
}

inline static void FDC_setRXInt(int32_t on)
{
	FDCFG = (FDCFG & ~RX_INTTHRESH) | (on << RX_INTTHRESH_SHIFT);
}

static inline void FDC_sendBytes(uint32_t ch, uint8_t * c, uint32_t i)
{
	switch (i) {
	case 1:
		FDTX(ch) = 0x80808000 | c[0];
		break;
	case 2:
		FDTX(ch) = 0x81810000 | c[0] | (c[1] << 8);
		break;
	case 3:
		FDTX(ch) = 0x82000000 | c[0] | (c[1] << 8) | (c[2] << 16);
		break;
	case 4:
		if (((c[3] == 0x80) && (c[2] == 0x80) && (c[3] == 0x80)) ||
		    ((c[3] == 0x81) && (c[2] == 0x81)) || (c[3] == 0x82)) {
			FDTX(ch) =
			    0x82000000 | c[0] | (c[1] << 8) | (c[2] << 16);
			FDTX(ch) = 0x80808000 | c[3];
		} else {
			FDTX(ch) =
			    c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
		}
		break;
	default:
		break;
	}
}

static inline void FDC_fetchBytes(uint32_t i, uint8_t * c)
{
	c[0] = (uint8_t) i;
	i >>= 8;
	if (i == 0x808080) {
		c[1] = 0;
		c[2] = 0;
		c[3] = 0;
		return;
	}
	c[1] = (uint8_t) i;
	i >>= 8;
	if (i == 0x8181) {
		c[2] = 0;
		c[3] = 0;
		return;
	}
	c[2] = (uint8_t) i;
	i >>= 8;
	if (i == 0x82) {
		c[3] = 0;
		return;
	}
	c[3] = (uint8_t) i;
}

void FDC_pumpFDC(int32_t sig)
{
	uint8_t c[4];
	uint32_t i, ch;
	uint32_t any = 1, result = 0, callback = 0, remaining = 0, full;
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	FDC_cdmmOn();
	if (sig != 0) {
		/* If we're being called as an ISR, acknowledge */
		IRQ_ack(IRQ_cause(sig));
	}
	while (any && !(full = (FDSTAT & TXF))) {
		any = 0;
		for (ch = 0; ch < 16; ch++)
			if (FDC_TXRings[ch]) {
				/* Ring isn't empty, might be more data */
				remaining++;
				for (i = 0; (i < 4) && !(FDSTAT & TXF); i++) {
					result = RING_read(FDC_TXRings[ch],
							   &c[i], 0);
					if (!result) {
						/* Ring is empty, no more data here */
						remaining--;
						break;
					}
				}
				FDC_sendBytes(ch, c, i);
				if (result)
					any = 1;
			}
	}
	if (full) {
		/* FDC is full, ensure FDC interrupt is enabled */
		FDC_SetTXInt(1);
	} else if (!remaining) {
		/* Rings all empty, disable FDC */
		FDC_SetTXInt(0);
	}
	result = 1;
	while (result && !(FDSTAT & RXE)) {
		i = (FDSTAT & RXCHAN) >> RXCHAN_SHIFT;
		callback |= 1 << i;
		FDC_fetchBytes(FDRX, c);
		if (FDC_RXRings[i]) {
			if (c[0])
				result = RING_write(FDC_RXRings[i], c[0], 0);
			if (result && c[1])
				RING_write(FDC_RXRings[i], c[1], 0);
			if (result && c[2])
				RING_write(FDC_RXRings[i], c[2], 0);
			if (result && c[3])
				RING_write(FDC_RXRings[i], c[3], 0);
		}
	}
	FDC_cdmmOff();
	IRQ_restoreIPL(ipl);
	i = 0;
	while (callback) {
		if ((callback & 1) && FDC_Ts[i]->uart.rFunc)
			FDC_Ts[i]->uart.rFunc(FDC_Ts[i]->uart.rPar);
		i++;
		callback >>= 1;
	}
}

int32_t FDC_config(UART_T * uart, const char *config)
{
	return 0;
}

void FDC_enableTXEmptyInt(UART_T * uuart)
{
	FDC_pumpFDC(0);
}

void FDC_init(FDC_T * uart, uint8_t * txBuf, size_t txLen,
	      uint8_t * rxBuf, size_t rxLen)
{
	DBG_assert(FDC_Ts[uart->channel] == NULL,
		   "Channel device already exists!\n");
	FDC_Ts[uart->channel] = uart;
	CDMMBASE = (uint32_t *) FDC_cdmmBase();
	uart->uart.config = FDC_config;
	uart->uart.enableTXEmptyInt = FDC_enableTXEmptyInt;
	RING_init(&uart->uart.tx, txBuf, txLen);
	RING_init(&uart->uart.rx, rxBuf, rxLen);
#if defined(CONFIG_ARCH_MIPS_BASELINE) || defined(CONFIG_ARCH_MIPS_BASELINE_XPSINTC) || defined(CONFIG_ARCH_MIPS_PIC)
	FDC_desc.intNum = IRQ_FDC;
#else
	FDC_desc.intNum = 0;
	FDC_desc.impSpec.extNum = IRQ_FDC;
#endif
	FDC_desc.isrFunc = FDC_pumpFDC;
	FDC_RXRings[uart->channel] = &uart->uart.rx;
	FDC_TXRings[uart->channel] = &uart->uart.tx;
	IRQ_route(&FDC_desc);
	FDC_cdmmOn();
	FDC_setRXInt(2);
	FDC_cdmmOff();
}
