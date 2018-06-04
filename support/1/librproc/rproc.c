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
*   Description:        rproc compatibility library
*
*************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

/*
 * !!!WARNING!!!
 *
 * This library makes liberal use of truncated structures and weak functions
 * so that it is small by itself, but can integrate into a proper IPM system.
 */

/*
 * Somewhere for the UHI coprocessor boot protocol to save its goodies.
 * These must be initialised to ensure they aren't zeroed by BSS init.
 */
uint32_t _copro_irq_to = -1, _copro_irq_from = -1;

/*
 * This comes from the device tree
 */
extern const uint32_t _IRQ_linuxOffset;

/* Define our resource table */
struct resources {
	RES_TABLE_T header;
	uint32_t offsets[2];
	RES_ENTRY_T memoryH;
	RES_ENTRY_CARVEOUT_T memory;
	RES_ENTRY_T rprocserH;
	RES_ENTRY_DEVICE_T rprocser;
	RES_ENTRY_RING_T rprocserTQ;
	RES_ENTRY_RING_T rprocserRQ;
	RES_ENTRY_RPS_CONFIG_T rprocserCfg;
} __attribute__ ((packed));

extern int __memory_base[];
extern int __memory_size[];

__attribute__ ((section(".resource_table")))
struct resources resource_table = {
	.header = {1, 2, {0}},
	.offsets = {
		    offsetof(struct resources, memoryH),
		    offsetof(struct resources, rprocserH)
		    },
	.memoryH = {.type = RES_TYPE_CARVEOUT},
	.memory = {
		   .myAddress = (uint32_t) __memory_base,
		   .phyAddress = (uint32_t) __memory_base,
		   .length = (uint32_t) __memory_size,
		   .name = "firmware"},
	.rprocserH = {.type = RES_TYPE_VDEV},
	.rprocser = {
		     .type = RES_DEVICE_RPROC_SERIAL,
		     .notifyId = 4,
		     .configLength = sizeof(RES_ENTRY_RPS_CONFIG_T),
		     .numRings = 2}
	,
	.rprocserRQ = {
		       .myAddress = 0,
		       .pageAlign = 4096,
		       .numBuffers = 24,
		       .notifyId = 0}
	,
	.rprocserTQ = {
		       .myAddress = 0,
		       .pageAlign = 4096,
		       .numBuffers = 24,
		       .notifyId = 1}
	,
	.rprocserCfg = {0, 0, 0, 0}
};

/* Truncated so we don't waste RAM */
typedef struct IPM_tag {
	MQGUEST_T inQ;
	MQHOST_T outQ;		/* Guest is a subset of host */
	KRN_POOL_T inHPool;
	KRN_POOL_T outHPool;
	MQ_MSG_T inHeader[24];
	MQ_MSG_T outHeader[24];
	IRQ_DESC_T inKick;
	IRQ_DESC_T inShin;
	IRQ_DESC_T outKick;
	IRQ_DESC_T outShin;
} IPM_T;

uint32_t IPM_nix = 0;
__attribute__ ((weak))
IPM_T *IPM_config[1];
IPM_T IPM_rproc;

#define STACKSIZE 4000
static KRN_WQ_T rxq;
static KRN_TASK_T rxqTasks[1];
static uint32_t rxqStacks[1][STACKSIZE];
static KRN_JOB_T rxqJobs[1];

/*
** FUNCTION:	IPM_send
**
** DESCRIPTION:	Send a message to another processor.
**
** RETURNS:	void
*/
void IPM_send(KRN_MSG_T * msg) __attribute__ ((weak));
void IPM_send(KRN_MSG_T * msg)
{
	MQGUEST_T *const q = (MQGUEST_T *) & IPM_rproc.outQ;
	MQ_MSG_T *out = NULL;

	do {
		out = MQGUEST_take(q, 0);
		_KRN_ignoreZeroTimeout();
	}
	while (!out);

	MQ_MSG_set(out, KRN_MSG_SIZE, &msg->from);

	_IPM_impexpDebug("TX", msg);
	MQGUEST_send(q, out);
}

/*
** FUNCTION:	_IPM_callback
**
** DESCRIPTION:	IPI callback handler - feed incoming messages upwards.
**
** RETURNS:	void
*/
void _IPM_callback(void) __attribute__ ((weak));
void _IPM_callback(void)
{
	MQGUEST_T *mq;
	MQ_MSG_T *in;
	KRN_MSG_T msg;
	if ((_KRN_schedule->hwThread != 0)
	    && (mq = &IPM_config[0]->inQ))
		while ((in = MQGUEST_recv(mq, 0))) {
			memcpy((void *)&msg.from,
			       (void *)MQ_MSG_data(in), KRN_MSG_SIZE);
			MQGUEST_return(mq, in);
			IPM_recv(&msg);
			KRN_release();
		}
}

/*
** FUNCTION:	_IPM_qCallback
**
** DESCRIPTION:	IPI interrupt handler - feed incoming messages upwards.
**
** RETURNS:	void
*/
void _IPM_qCallback(MQ_T * mq, void *cbPar) __attribute__ ((weak));
void _IPM_qCallback(MQ_T * mq, void *cbPar)
{
	/* If this fails, then there must be one in the pipe anyway */
	KRN_queueWQ(&rxq, (KRN_TASKFUNC_T *) _IPM_callback, mq, "IPM cb", 0);
}

static void RPROC_spamMem(uintptr_t paddr, size_t length,
			  void *vaddr, void *cbPar)
{
	KRN_MSG_T *msg = (KRN_MSG_T *) cbPar;
	msg->sID = (uint32_t) ((uint64_t) paddr >> 32);
	msg->cID = (uint32_t) (paddr & 0xffffffff);
	msg->p64a = length;
	msg->p64b = (uintptr_t) vaddr;
	IPM_send(msg);
}

void RPROC_bind()
{
	uint32_t s = _KRN_schedule->hwThread;
	KRN_MSG_T msg;

	/* Allow us to fall back to Linuxless operation if not started via rproc */
	if ((_copro_irq_to == -1) || (_copro_irq_from == -1))
		return;

	/* NOP for us, but let's full IPM know CPU 0 uses an MQGUEST_T for its outQ */
	IPM_nix = 1;

	/* NOP for us, but important if we're integrating with full IPM */
	IPM_config[0] = &IPM_rproc;

	/* Set up interrupt mapping based on boot protocol */
	IRQ_ipi(_copro_irq_to - _IRQ_linuxOffset, &IPM_rproc.inShin);
	IRQ_ipi(_copro_irq_to - _IRQ_linuxOffset, &IPM_rproc.outShin);
	IRQ_ipi(_copro_irq_from - _IRQ_linuxOffset, &IPM_rproc.inKick);
	IRQ_ipi(_copro_irq_from - _IRQ_linuxOffset, &IPM_rproc.outKick);

	/* Prepare buffers for Linux hosting both sides */
	KRN_initPool(&IPM_rproc.inHPool, &IPM_rproc.inHeader, 24,
		     sizeof(MQ_MSG_T));
	KRN_initPool(&IPM_rproc.outHPool, &IPM_rproc.outHeader, 24,
		     sizeof(MQ_MSG_T));

	/* Create guest queues */
	MQGUEST_init((MQGUEST_T *) & IPM_rproc.outQ,
		     &IPM_rproc.outHPool,
		     (void *)resource_table.rprocserTQ.myAddress,
		     resource_table.rprocserTQ.pageAlign,
		     &IPM_rproc.outShin, &IPM_rproc.outKick);
	MQGUEST_init(&IPM_rproc.inQ, &IPM_rproc.inHPool,
		     (void *)resource_table.rprocserRQ.myAddress,
		     resource_table.rprocserRQ.pageAlign,
		     &IPM_rproc.inShin, &IPM_rproc.inKick);

	/* Wire up incoming messages */
	MQGUEST_setCallback(&IPM_rproc.inQ, _IPM_qCallback, NULL);

	/* Return our memory map to Linux. This also serves to lock vports. */
	msg.from = s;
	msg.to = 0;
	msg.cmd = KRN_COMMAND_MEM_ANNOUNCE;
	MEM_revmap(__memory_base, (size_t) __memory_size, RPROC_spamMem, &msg);
}

/*
** FUNCTION:    IPM_start
**
** DESCRIPTION: Deferred initialisation performed once the scheduler is running.
**
** RETURNS:     void
*/
void IPM_start(void) __attribute__ ((weak));
void IPM_start(void)
{
	/* RX WQ */
	KRN_initWQ(&rxq, rxqTasks, (uint32_t *) rxqStacks, 1, STACKSIZE,
		   KRN_maxPriority() - 1, rxqJobs, 1);
	/* Bind linux */
	RPROC_bind();
}
