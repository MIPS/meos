/***(C)2011***************************************************************
*
* Copyright (C) 2011 MIPS Tech, LLC
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
****(C)2011**************************************************************/

/*************************************************************************
*
*   Description:	MQ test, thread 1
*
*************************************************************************/

/*
 * This test models the master/slave mode of communication via MQ, as used by
 * Linux when communicating with embedded systems. In this model, the master
 * owns the buffers, and the slave requests/fills/kicks as appropriate.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define LOW_LEVEL

#define TSTACKSIZE 4000		/* MEOS timer task stack size */
#define STACKSIZE 4000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define NUM_BUFFERS 4
#define MASTER 0
#define SLAVE 1

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

uint8_t *sendBuffer = &_IPM_BUF[0];
uint8_t *recvBuffer = &_IPM_BUF[8192];

MQ_T sendQ, recvQ;
MQ_MSG_T sendHBuffers[NUM_BUFFERS];
KRN_POOL_T sendHPool;
MQ_MSG_T recvHBuffers[NUM_BUFFERS];
KRN_POOL_T recvHPool;

IRQ_DESC_T in[2], out[2];

void verify(MQ_MSG_T * msg, int32_t i)
{
	int32_t *pl;
	(void)pl;
	DBG_assert(msg->length == sizeof(int32_t),
		   "Unexpected message size: %" PRId16 "\n", msg->length);
	pl = (int32_t *) msg->buffer;
	DBG_assert(*pl == i, "Count incorrect: %" PRId32 "\n", *pl);
}

volatile int32_t count = 1;

KRN_WQ_T wq;
KRN_TASK_T wqTasks[1];
uint32_t wqStacks[1][STACKSIZE];
KRN_JOB_T wqJobs[8];

void callback(MQ_T * mq, void *cbPar)
{
	(void)mq;
	(void)cbPar;
	void doCallback() {
		MQ_MSG_T *inM;
		while ((inM = MQ_recv(&sendQ, 0))) {
			int32_t *iPl, *oPl;
			DBG_logF("Recv: %p/%" PRId16 "\n", inM->buffer,
				 inM->length);
			verify(inM, count++);
			MQ_MSG_T *outM = MQ_take(&recvQ, KRN_INFWAIT);
			iPl = (int32_t *) inM->buffer;
			outM->length = sizeof(int32_t);
			oPl = (int32_t *) outM->buffer;
			*oPl = *iPl;
			DBG_logF("Send: %p/%" PRId16 "\n", outM->buffer,
				 outM->length);
			DBG_logF("Received message reading %" PRIu32
				 ", replying reading %" PRIu32 "\n", *iPl,
				 *oPl);
			MQ_return(&sendQ, inM);
			MQ_send(&recvQ, outM);
		}
	}
	KRN_queueWQ(&wq, doCallback, NULL, "ISR callback", 0);
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main()
{
	DBG_logF("MQ slave Test\n");
	DBG_logF("sendBuffer: %p recvBuffer: %p\n", sendBuffer, recvBuffer);

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	KRN_TASKQ_T waitq;
	DQ_init(&waitq);
	KRN_hibernate(&waitq, 10);

	KRN_initWQ(&wq, wqTasks, (uint32_t *) wqStacks, 1, STACKSIZE,
		   KRN_maxPriority() - 1, wqJobs, 8);

	/* Create two queues, one for each direction */
	IRQ_ipi(IRQ_SELF, &in[0]);
	IRQ_ipi(IRQ_SELF, &in[1]);
	IRQ_ipi(0, &out[0]);
	IRQ_ipi(0, &out[1]);
	KRN_initPool(&sendHPool, sendHBuffers, NUM_BUFFERS, sizeof(MQ_MSG_T));
	KRN_initPool(&recvHPool, recvHBuffers, NUM_BUFFERS, sizeof(MQ_MSG_T));
	MQ_init(&sendQ, &sendHPool, sendBuffer, 4096, &in[0], &out[0]);
	MQ_init(&recvQ, &recvHPool, recvBuffer, 4096, &in[1], &out[1]);
	MQ_setCallback(&sendQ, callback, NULL);

	while (count < 9) ;

	return 0;
}
