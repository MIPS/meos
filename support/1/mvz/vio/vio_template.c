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
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Virtio template
*
*************************************************************************/

#include <stdio.h>
#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/mem/mem.h"
#include "meos/uart/uart.h"
#include "meos/inttypes.h"

#error Rename this function and fix in VIO_initTemplate
int VIO_readTemplate(void *address, void *buffer, int size, int n, void *priv)
{
	uint32_t result;
	uintptr_t offset = (uintptr_t) address;
	uint32_t *value = (uint32_t *) buffer;
	#error Add support for registers 0x100+
	switch (offset) {
	case MY_REGISTER:
		if ((size * n) != 4) {
			DBG_assert(0,
				   "Guest '%s' attempted invalid read size for Virtio template register %03"
				   PRIx32 "\n", KRN_taskName(NULL), offset);
			MVZ_restart((MVZ_GUEST_T *) _KRN_current);
		}
		#error Set result to the value to be read
		*value = MEM_lerw(&result);
		break;
	#error More registers here
	default:
		DBG_assert(0,
			   "Guest '%s' attempted read from unsupported Virtio template register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
	return size * n;
}

#error Rename this function and fix in VIO_initTemplate
int VIO_writeTemplate(void *address, void *buffer, int size, int n, void *priv)
{
	uintptr_t offset = (uintptr_t) address;
	uint32_t sv, *value = (uint32_t *) buffer;
	#error Add support for registers 0x100+
	switch (offset) {
	case MY_REGISTER:
		if ((size * n) != 4) {
			DBG_assert(0,
				   "Guest '%s' attempted invalid read size for Virtio template register %03"
				   PRIx32 "\n", KRN_taskName(NULL), offset);
			MVZ_restart((MVZ_GUEST_T *) _KRN_current);
		}
		MEM_leww(&sv, *value);
		#error Do something with value
		break;
	#error More registers here
	default:
		DBG_assert(0,
			   "Guest '%s' attempted read from unsupported Virtio template register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
	return size * n;
}

#error Rename this function and fix in VIO_budTemplate
void VIO_txTemplate(MQDRIVER_T * mq, void *cbPar)
{
	VIO_TEMPLATE_T *template = (VIO_TEMPLATE_T *) cbPar;
	#error Do something with the pending data, e.g.
	//VIO_readGQ(&template->vio.queues[1], writefunc, writeprivate, 0);
}

#error Rename this function and fix in VIO_budTemplate
void VIO_rxTemplate(MQDRIVER_T * mq, void *cbPar)
{
	VIO_TEMPLATE_T *template = (VIO_TEMPLATE_T *) cbPar;

	if (!template->vio.queues[0].guest)
		return;

	if (VIO_writableGQ(&template->vio.queues[0]))
		for (;;) {
			#error Provide some data, e.g.
#if 0
			/* Transfer from a reader to a writer */
			static uint8_t buf[4096];
			size_t rlen, wlen;
			uint32_t tries = 16;
			/* Read the data */
			rlen =
			    readfunc(NULL, buf, 1, sizeof(buf), readprivate);
			/* Bail if we read nothing */
			if (!rlen)
				return;
			/* While there's still data */
			while (rlen && tries) {
				/* Try writing it out */
				wlen =
				    VIO_writeGQ(NULL, buf, rlen, 1,
						&template->vio.queues[0]);
				if (wlen == 0)
					tries--;
				/* Update length to follow write */
				rlen -= wlen;
			}
		}
#endif
}

#error Rename this function and fix in VIO_initTemplate
void VIO_budTemplate(VIO_T * vio, reg_t down)
{
	VIO_TEMPLATE_T *template = (VIO_TEMPLATE_T *) vio;
	MVZ_GUEST_T *gt = vio->regs.guest;
	if (down) {
		#error Shut down device, e.g. remove callbacks, etc.
	} else {
		#error Initialise queues, e.g.
#if 0
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
				     (MQ_CALLBACK_T *) VIO_rxTemplate, vio);
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
				     (MQ_CALLBACK_T *) VIO_txTemplate, vio);
#endif
	}
}

#error Rename this function, add extra parameters
void VIO_initTemplate(VIO_TEMPLATE_T * template,
		     uintptr_t base, uint32_t gInt, )
{
	uint32_t i;
	const uint32_t m = sizeof(template->queues) / sizeof(VIO_Q_T)
	#error Change identifiers
	VIO_init(&template->vio, MY_DEVICE_ID, MY_MANUFACTURER_ID);
	memset(template->queues, 0, sizeof(template->queues));
	template->vio.queues = template->queues;
	template->vio.nQueues = m;
	for (i = 0; i < m; i++) {
		template->vio.queues[i].vio = (VIO_T *) template;
		template->vio.queues[i].numMax = VIO_QUEUE_NUM_MAX;
	}
	#error Set feature bits
	template->vio.deviceFeatures[0] = 0;
	#error Rename these functions
	template->vio.read = VIO_readTemplate;
	template->vio.write = VIO_writeTemplate;
	template->vio.bringUpDown = VIO_budTemplate;
	template->vio.gInt = gInt;
	template->vio.regs.start = base;
	#error Increase this to cover your registers
	template->vio.regs.stop = base + 0xff;
	#error Initialise private data
}
