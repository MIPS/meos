/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	Mailbox stress test, thread 0
*
*************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "MEOS.h"

#include "mbxstress.h"

#define SLAVES 3

#define PRIORITIES 2
#define STACKSIZE 1000
#define NUM_MESSAGES 5

KRN_SCHEDULE_T sched;
KRN_TASKQ_T schedQueues[PRIORITIES];
uint32_t timstack[STACKSIZE];
uint32_t slaveDriverStacks[SLAVES][STACKSIZE];
uint32_t istack[STACKSIZE];
KRN_TASK_T slaveDrivers[SLAVES];
KRN_TASK_T *timtask;
KRN_TASKQ_T waitq;
KRN_MAILBOX_T slaveBoxes[6];
KRN_EXPORT_T expTable[7];
KRN_POOL_T msgPool;
MESSAGE_T messages[NUM_MESSAGES];
KRN_SYNC_T complete;

#define check(X) __extension__ ({if (!(X)) {DBG_logF("**** TEST FAILED ****\n");_DBG_stop(__FILE__, __LINE__);}})

static void validateMessage(MESSAGE_T * snd, MESSAGE_T * rcv, int32_t slave,
			    uint32_t count)
{
	uint8_t *p;
	int32_t n;

	check(snd == rcv);
	check(rcv->slave == slave);
	check(rcv->count == count);
	p = rcv->data;
	for (n = 0; n < MSG_DATA_SIZE; n++)
		check(*p++ == (~(count++) & 0xff));
}

static void setupMessage(MESSAGE_T * m, int32_t slave, uint32_t count)
{
	uint32_t n;
	uint8_t *p = m->data;
	m->slave = slave;
	m->count = count;

	for (n = 0; n < MSG_DATA_SIZE; n++)
		*p++ = (count++ & 0xff);
}

void slaveDriver()
{
	MESSAGE_T *snd, *rcv;
	KRN_MAILBOX_T *sndMbox;
	KRN_MAILBOX_T *rcvMbox;
	uint32_t n;
	int32_t slaveThread = (int32_t) KRN_taskParameter(NULL);

	sndMbox = &slaveBoxes[slaveThread - 1];
	rcvMbox = &slaveBoxes[slaveThread + 3 - 1];
	KRN_initMbox(sndMbox);
	KRN_initMbox(rcvMbox);
	KRN_export(slaveThread, sndMbox);
	KRN_export(slaveThread + 3, rcvMbox);

	for (n = 0; n < MESSAGES; n++) {
		if ((n % 100) == 0)
			DBG_logF("Slave thread %" PRId32 ": %" PRId32
				 " messages echoed...\n", slaveThread, n);
		snd = KRN_takePool(&msgPool, KRN_INFWAIT);
		setupMessage(snd, slaveThread, n);
		KRN_putMbox(sndMbox, snd);
		rcv = KRN_getMbox(rcvMbox, KRN_INFWAIT);
		KRN_flushCache(rcv, sizeof(MESSAGE_T),
			       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_I |
			       KRN_FLUSH_FLAG_WRITEBACK_D);
		validateMessage(snd, rcv, slaveThread, n);
		KRN_returnPool(rcv);
	}
	DBG_logF("Slave thread %" PRId32 ": %" PRId32 " messages echoed...\n",
		 slaveThread, n);
	DBG_logF("Slave driver for thread %" PRId32 " terminating\n",
		 slaveThread);
	KRN_sync(&complete, KRN_INFWAIT);

	KRN_removeTask(NULL);
}

int main(int argc, char **argv)
{
	int32_t n;

	(void)argc;
	(void)argv;

	DBG_logF("InterThread Mailbox stress test\n");

	DQ_init(&waitq);
	KRN_reset(&sched, schedQueues, PRIORITIES - 1, 0xdeadbeef, istack,
		  STACKSIZE, NULL, 0);
	{
		uint8_t maxImpIds[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		KRN_installImpExp(maxImpIds, 7, NULL, expTable);
	}
	KRN_startOS("Background Task");
	timtask = KRN_startTimerTask("Timer Task", timstack, STACKSIZE);
	BSP_init();
	KRN_initPool(&msgPool, messages, NUM_MESSAGES, sizeof(MESSAGE_T));
	KRN_initSync(&complete, 8);
	KRN_export(7, &complete);
	DBG_logF("Starting Slave drivers and receivers\n");
	for (n = 1; n <= SLAVES; n++) {
		KRN_startTask(slaveDriver, &slaveDrivers[n - 1],
			      &slaveDriverStacks[n - 1][0], STACKSIZE,
			      KRN_LOWEST_PRIORITY, (void *)(n), "Slave driver");
	}

	KRN_sync(&complete, KRN_INFWAIT);
	DBG_logF("Test Complete\n");

	return 0;
}
