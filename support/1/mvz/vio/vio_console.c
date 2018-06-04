/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Virtio console
*
*************************************************************************/

#include <stdio.h>
#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/mem/mem.h"
#include "meos/uart/uart.h"
#include "meos/inttypes.h"

int VIO_readConsole(void *address, void *buffer, int size, int n, void *priv)
{
	const uint32_t one = 1;
	uintptr_t offset = (uintptr_t) address;
	uint32_t *value = (uint32_t *) buffer;
	switch (offset) {
	case VIRTIO_REG_CONSOLE_MAX_NR_PORTS:
		if ((size * n) != 4) {
			DBG_assert(0,
				   "Guest '%s' attempted invalid read size for Virtio console register %03"
				   PRIx32 "\n", KRN_taskName(NULL), offset);
			MVZ_restart((MVZ_GUEST_T *) _KRN_current);
		}
		*value = MEM_lerw(&one);
		break;
	default:
		DBG_assert(0,
			   "Guest '%s' attempted read from unsupported Virtio console register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
	return size * n;
}

int VIO_writeConsole(void *address, void *buffer, int size, int n, void *priv)
{
	uintptr_t offset = (uintptr_t) address;
	uint32_t sv, *value = (uint32_t *) buffer;
	switch (offset) {
	case VIRTIO_REG_CONSOLE_EMERG_WR:
		if ((size * n) != 4) {
			DBG_assert(0,
				   "Guest '%s' attempted invalid read size for Virtio console register %03"
				   PRIx32 "\n", KRN_taskName(NULL), offset);
			MVZ_restart((MVZ_GUEST_T *) _KRN_current);
		}
		MEM_leww(&sv, *value);
		DBG_logF("%c", (int)sv);
		break;
	default:
		DBG_assert(0,
			   "Guest '%s' attempted read from unsupported Virtio console register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
	return size * n;
}

void VIO_txConsole(MQDRIVER_T * mq, void *cbPar)
{
	VIO_CONSOLE_T *console = (VIO_CONSOLE_T *) cbPar;
	/*(VIO_CONSOLE_T *) ((uintptr_t) queue -
	   (offsetof(VIO_CONSOLE_T, queues) +
	   sizeof(VIO_Q_T))); */
	VIO_readGQ(&console->vio.queues[1], console->w, console->wp, 0);
}

void VIO_rxConsole(MQDRIVER_T * mq, void *cbPar)
{
	VIO_CONSOLE_T *console = (VIO_CONSOLE_T *) cbPar;
	/* Transfer from a reader to a writer */
	static uint8_t buf[4096];
	size_t rlen, wlen;
	uint32_t tries = 16;

	if (!console->vio.queues[0].guest)
		return;

	if (VIO_writableGQ(&console->vio.queues[0]))
		for (;;) {
			/* Read the data */
			rlen =
			    console->r(NULL, buf, 1, sizeof(buf), console->rp);
			/* Bail if we read nothing */
			if (!rlen)
				return;
			/* While there's still data */
			while (rlen && tries) {
				/* Try writing it out */
				wlen =
				    VIO_writeGQ(NULL, buf, rlen, 1,
						&console->vio.queues[0]);
				if (wlen == 0)
					tries--;
				/* Update length to follow write */
				rlen -= wlen;
			}
		}
}

void VIO_budConsole(VIO_T * vio, reg_t down)
{
	VIO_CONSOLE_T *console = (VIO_CONSOLE_T *) vio;
	MVZ_GUEST_T *gt = vio->regs.guest;
	if (down) {
		if (console->r == UART_read)
			UART_readyFunc(console->rp, NULL, NULL);
	} else {
		/* 0: rxq */
		vio->queues[0].guest = gt;
		KRN_initPool(&vio->queues[0].hPool,
			     vio->queues[0].headers,
			     VIO_QUEUE_NUM_MAX,
			     sizeof(vio->queues[0].headers[0]));
		MQDRIVER_init(&vio->queues[0].mq, &vio->queues[0].hPool,
			      vio->queues[0].desc, vio->queues[0].avail,
			      vio->queues[0].used, 4096, gt, vio);
		MQDRIVER_setCallback(&vio->queues[0].mq,
				     (MQ_CALLBACK_T *) VIO_rxConsole, vio);
		/* 1: txq */
		vio->queues[1].guest = gt;
		KRN_initPool(&vio->queues[1].hPool,
			     vio->queues[1].headers,
			     VIO_QUEUE_NUM_MAX,
			     sizeof(vio->queues[1].headers[0]));
		MQDRIVER_init(&vio->queues[1].mq, &vio->queues[1].hPool,
			      vio->queues[1].desc, vio->queues[1].avail,
			      vio->queues[1].used, 4096, gt, vio);
		MQDRIVER_setCallback(&vio->queues[1].mq,
				     (MQ_CALLBACK_T *) VIO_txConsole, vio);
	}
}

static int VIO_writeF(void *address, void *buffer, int size, int n, void *priv)
{
	return fwrite(buffer, size, n, (FILE *) priv) * size;
}

static int VIO_readF(void *address, void *buffer, int size, int n, void *priv)
{
	fseek((FILE *) priv, 0, SEEK_SET);
	return fread(buffer, size, n, (FILE *) priv) * size;
}

void VIO_initFileConsole(VIO_CONSOLE_T * console,
			 uintptr_t base, uint32_t gInt, FILE * in, FILE * out)
{
	VIO_init(&console->vio, VIRTIO_DEV_CONSOLE, VIRTIO_DEV_MIPS);
	memset(console->queues, 0, sizeof(console->queues));
	console->vio.queues = console->queues;
	console->vio.nQueues = 2;
	console->vio.queues[0].vio = (VIO_T *) console;
	console->vio.queues[0].numMax = VIO_QUEUE_NUM_MAX;
	console->vio.queues[1].vio = (VIO_T *) console;
	console->vio.queues[1].numMax = VIO_QUEUE_NUM_MAX;
	console->vio.deviceFeatures[0] = VIRTIO_FLAG_CONSOLE_EMERG_WRITE;
	console->vio.read = VIO_readConsole;
	console->vio.write = VIO_writeConsole;
	console->vio.bringUpDown = VIO_budConsole;
	console->vio.gInt = gInt;
	console->vio.regs.start = base;
	console->vio.regs.stop = base + 0xff;
	console->r = VIO_readF;
	console->rp = in;
	console->w = VIO_writeF;
	console->wp = out;
}

static void VIO_readyFunc(void *rPar)
{
	VIO_CONSOLE_T *console = (VIO_CONSOLE_T *) rPar;
	VIO_rxConsole(&console->vio.queues[0].mq, console);
}

void VIO_initConsole(VIO_CONSOLE_T * console,
		     uintptr_t base, uint32_t gInt, UART_T * uart)
{
	VIO_init(&console->vio, VIRTIO_DEV_CONSOLE, VIRTIO_DEV_MIPS);
	memset(console->queues, 0, sizeof(console->queues));
	console->vio.queues = console->queues;
	console->vio.nQueues = 2;
	console->vio.queues[0].vio = (VIO_T *) console;
	console->vio.queues[0].numMax = VIO_QUEUE_NUM_MAX;
	console->vio.queues[1].vio = (VIO_T *) console;
	console->vio.queues[1].numMax = VIO_QUEUE_NUM_MAX;
	console->vio.deviceFeatures[0] = VIRTIO_FLAG_CONSOLE_EMERG_WRITE;
	console->vio.read = VIO_readConsole;
	console->vio.write = VIO_writeConsole;
	console->vio.bringUpDown = VIO_budConsole;
	console->vio.gInt = gInt;
	console->vio.regs.start = base;
	console->vio.regs.stop = base + 0xff;
	console->r = UART_read;
	console->rp = uart;
	console->w = UART_write;
	console->wp = uart;
	UART_readyFunc(uart, VIO_readyFunc, console);
}
