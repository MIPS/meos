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
*   Description:	Mailbox stress test, thread 1
*
*************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "MEOS.h"

#include "mbxstress.h"

#define STACKSIZE 1000
#define PRIORITIES 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

KRN_SCHEDULE_T sched;
KRN_TASKQ_T schedQueues[PRIORITIES];

uint32_t timstack[STACKSIZE];
uint32_t istack[STACKSIZE];
uint32_t slave1Stack[STACKSIZE], slave2Stack[STACKSIZE], slave3Stack[STACKSIZE];
KRN_TASK_T slave1, slave2, slave3;
KRN_TASK_T *timtask;
KRN_IMPORT_T impTab[7];
KRN_SYNC_T complete;

#define check(X) __extension__ ({if (!(X)) {DBG_logF("**** TEST FAILED ****\n");_DBG_stop(__FILE__, __LINE__);}})

static void validateMessage(MESSAGE_T * m, int32_t slave, uint32_t count)
{
	uint8_t *p;
	int32_t n;

	check(m->slave == slave);
	check(m->count == count);
	p = m->data;
	for (n = 0; n < MSG_DATA_SIZE; n++) {
		check(*p++ == (count++ & 0xff));
	}
}

static void modifyMessage(MESSAGE_T * m)
{
	uint8_t *p;
	int32_t n;

	p = m->data;
	for (n = 0; n < MSG_DATA_SIZE; n++) {
		*p = ~(*p);
		p++;
	}
}

void slave(void)
{
	KRN_TASKQ_T waitq;
	uint32_t m, n;
	uint32_t threadId = (uint32_t) KRN_taskParameter(NULL);
	KRN_MAILBOX_T inBox;
	KRN_MAILBOX_T outBox;
	MESSAGE_T *msg;

	DQ_init(&waitq);
	KRN_initMbox(&inBox);
	KRN_initMbox(&outBox);
	KRN_import(0, threadId, &inBox);	/* 1..3 for threads 1..3 */
	KRN_import(0, threadId + 3, &outBox);	/* 4..6 for threads 1..3 */

	DBG_logF("Starting %" PRIu32 "\n", threadId);

	for (n = 0; n < MESSAGES; n++) {
		msg = KRN_getMbox(&inBox, KRN_INFWAIT);
		KRN_flushCache(msg, sizeof(MESSAGE_T),
			       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_I |
			       KRN_FLUSH_FLAG_WRITEBACK_D);
		for (m = 0; m < (2 * threadId); m++)	/* slightly different load on each thread to de-regularise timings */
			validateMessage(msg, threadId, n);
		modifyMessage(msg);
		KRN_putMbox(&outBox, msg);
	}
	KRN_sync(&complete, KRN_INFWAIT);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	KRN_reset(&sched, schedQueues, PRIORITIES - 1, 0, istack, STACKSIZE,
		  NULL, 0);
	{
		uint8_t maxImpIds[4] = { 7, 0, 0, 0 };
		KRN_installImpExp(maxImpIds, 0, impTab, NULL);
	}
	KRN_startOS("Slave Task");
	timtask = KRN_startTimerTask("Timer Task", timstack, STACKSIZE);
	BSP_init();
	KRN_initSync(&complete, 8);
	KRN_import(0, 7, &complete);

	switch (MIN(KRN_procs(), CONFIG_FEATURE_MAX_PROCESSORS)) {
	default:
		KRN_startTask(slave, &slave1, slave1Stack, STACKSIZE,
			      KRN_LOWEST_PRIORITY, (void *)1, "Slave 1");
		KRN_startTask(slave, &slave2, slave2Stack, STACKSIZE,
			      KRN_LOWEST_PRIORITY, (void *)2, "Slave 2");
		KRN_startTask(slave, &slave3, slave3Stack, STACKSIZE,
			      KRN_LOWEST_PRIORITY, (void *)3, "Slave 3");
		break;
	case 3:
		if (KRN_proc() == 1) {
			KRN_startTask(slave, &slave1, slave1Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)1,
				      "Slave 1");
		} else {
			KRN_startTask(slave, &slave2, slave2Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)2,
				      "Slave 2");
			KRN_startTask(slave, &slave3, slave3Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)3,
				      "Slave 3");
		}
		break;
	case 4:
		switch (KRN_proc()) {
		case 1:
			KRN_startTask(slave, &slave1, slave1Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)1,
				      "Slave 1");
			break;
		case 2:
			KRN_startTask(slave, &slave2, slave2Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)2,
				      "Slave 2");
			break;
		case 3:
			KRN_startTask(slave, &slave3, slave3Stack, STACKSIZE,
				      KRN_LOWEST_PRIORITY, (void *)3,
				      "Slave 3");
			break;
		}

	}
	KRN_sync(&complete, KRN_INFWAIT);
	return 0;
}
