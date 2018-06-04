/***(C)2014***************************************************************
*
* Copyright (C) 2014 MIPS Tech, LLC
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
****(C)2014**************************************************************/

/*************************************************************************
*
*   Description:	Kernel module coverage test
*
*************************************************************************/

#undef NDEBUG

#include <stdlib.h>

#include "MEOS.h"

#define STACKSIZE 2000

#define PRIORITIES 256
#define MAX_PRIORITY (PRIORITIES - 1)	/* timer task priority */
#define TRACE_DEPTH 10

#define POOLITEMS 3
#define CLEARFLAGS 1
#define BIT31 0x80000000ul

typedef void VOIDFUNC_T(void);

extern uint32_t stack1[STACKSIZE];
extern uint32_t stack2[STACKSIZE];
extern uint32_t stack3[STACKSIZE];

KRN_TASK_T *bgtask;
KRN_TASK_T *timtask;
KRN_TASK_T fgtask;
KRN_TASK_T hptask;
KRN_TASK_T pttask;
KRN_TASK_T evctask;
KRN_TIMER_T evctimer;
KRN_TASK_T hibtask;
KRN_MAILBOX_T mbox;
KRN_LOCK_T lock;
KRN_LOCK_T evclock, evclock2;
KRN_FLAGCLUSTER_T efcluster;
KRN_TASKQ_T hibQ;

typedef struct {
	LST_LINK;
	char *text;
} MESSAGE_T;
MESSAGE_T message;

typedef struct {
	KRN_POOLLINK;
	int32_t itemNumber;
} POOLITEM_T;
KRN_POOL_T pool;
POOLITEM_T poolbuf[POOLITEMS];
KRN_TASKQ_T hibernateQ;

static int32_t do_nothing(void)
{
	return 0;
}

void printStackInfo(KRN_TASK_T * task)
{
	int32_t t, f, u;
	DBG_logF("%s: \n",
		 (task ==
		  ((void *)(-1))) ? "Interrupt Stack" : KRN_taskName(task));
	if (KRN_stackInfo(task, &t, &f, &u))
		DBG_logF("size=%" PRId32 ", used=%" PRId32 ", free=%" PRId32
			 "\n", t, u, f);
	else
		DBG_logF("No stack information available\n");

}

void reportStacks()
{
	DBG_logF("\nStack use report follows:\n");
	/* this only works because TCBs and stacks are not re-used after the tasks die */
	printStackInfo(bgtask);
	printStackInfo(timtask);
	printStackInfo(&fgtask);
	printStackInfo(&hptask);
	printStackInfo(&pttask);
	printStackInfo(&evctask);
	DBG_logF("\n");
}

void ptTaskFunc(void)
{
	POOLITEM_T *p;
	int32_t n;

	for (n = 0; n < POOLITEMS; n++) {
		/* wait for pool item */
		DBG_logF("%s waiting for  pool item.\n", KRN_taskName(NULL));
		p = (POOLITEM_T *) KRN_takePool(&pool, KRN_INFWAIT);
		KRN_emptyPool(&pool);
		DBG_assert(p != NULL, "NULL pool take");
		DBG_logF("%s allocated pool item\n", KRN_taskName(NULL));
	}
	DBG_assert(KRN_takePool(&pool, KRN_NOWAIT) == NULL, "NULL pool take");

	DBG_logF("%s killing itself\n", KRN_taskName(NULL));
	KRN_removeTask(NULL);
	DBG_assert(0, "Can't happen");	/* should never get here */
}

void evcTaskFunc(void)
{
	int32_t n;

	DBG_logF("%s waiting for handshake flag.\n", KRN_taskName(NULL));
	DBG_assert((KRN_testFlags
		    (&efcluster, BIT31, KRN_ALL, CLEARFLAGS,
		     KRN_INFWAIT) & BIT31) == BIT31,
		   "Wrong handshake flag set");
	for (; KRN_lock(&evclock, 0);) {
		for (n = 0; n < 8; n++) {
			KRN_setFlags(&efcluster, 1 << n);
			KRN_release();
		}
		KRN_unlock(&evclock);
		KRN_release();
	}
	KRN_lock(&evclock2, INT_MAX);
}

void hptaskFunc(void)
{
	int32_t n = 0;
	int32_t m;
	MESSAGE_T *mp;

	/* wait for a message via the mailbox */
	mp = (MESSAGE_T *) KRN_getMbox(&mbox, KRN_INFWAIT);
	DBG_assert(mp != NULL, "NULL message received");
	DBG_logF("%s received message containing %s\n", KRN_taskName(NULL),
		 mp->text);

	for (m = 0; m < 10; m++) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL), n++);
		KRN_release();
	}

	/* drop priority */
	DBG_logF("%s dropping priority\n", KRN_taskName(NULL));

	KRN_priority(NULL, KRN_LOWEST_PRIORITY);

	for (;;) {
		if ((n % 10) == 0)
			DBG_logF("%s iteration %" PRId32 "\n",
				 KRN_taskName(NULL), n++);
		KRN_release();
	}
}

static int32_t set31(void)
{
	KRN_setFlags(&efcluster, BIT31);
	return 0;
}

void fgtaskFunc(void)
{
	int32_t n = 0;
	int32_t m;
	POOLITEM_T *pi[POOLITEMS];
	IRQ_IPL_T ipl;

	/* initialise the mailbox */
	KRN_initMbox(&mbox);

	/* start the high priority task */
	KRN_startTask(hptaskFunc, &hptask, stack2, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Higher Priority task");

	for (m = 0; m < 10; m++) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL), n++);
		KRN_release();
	}

	DBG_logF("%s sending mailbox message to release high priority task\n",
		 KRN_taskName(NULL));
	/* send message to unblock the high priority task */
	KRN_putMbox(&mbox, &message);
	for (m = 0; m < 5; m++) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL), n++);
		KRN_release();
	}
	DBG_logF("%s waiting for background task to release lock\n",
		 KRN_taskName(NULL));
	KRN_lock(&lock, KRN_INFWAIT);
	DBG_logF("%s seized resource lock\n", KRN_taskName(NULL));
	for (m = 0; m < 5; m++) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL), n++);
		KRN_release();
	}
	DBG_logF("%s killing HP task\n", KRN_taskName(NULL));
	KRN_removeTask(&hptask);
	for (m = 0; m < 5; m++) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL), n++);
		KRN_release();
	}

	/* create a pool and allocate all the items */
	KRN_initPool(&pool, poolbuf, POOLITEMS, sizeof(POOLITEM_T));
	KRN_emptyPool(&pool);
	ipl = IRQ_raiseIPL();
	/* allocate all the items */
	for (m = 0; m < POOLITEMS; m++)
		DBG_assert((pi[m] = (POOLITEM_T *)
			    KRN_takePool(&pool, KRN_NOWAIT)) != NULL,
			   "NULL message received");
	DBG_assert(KRN_takePool(&pool, KRN_NOWAIT) == NULL,
		   "Non-NULL message received");
	IRQ_restoreIPL(ipl);

	/* start a (higher priority)consumer task */
	KRN_startTask(ptTaskFunc, &pttask, stack2, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Pool Consumer Task");

	/* release all the items back to the pool */

	for (m = 0; m < POOLITEMS; m++) {
		DBG_logF("%s returning pool item\n", KRN_taskName(NULL));
		KRN_returnPool(pi[m]);
	}

	/* initialise an event flag cluster */

	KRN_initFlags(&efcluster);
	KRN_setTimer(&evctimer, (KRN_TIMERFUNC_T *) set31, NULL, 10000);
	KRN_testFlags(&efcluster, BIT31, KRN_ALL, CLEARFLAGS, KRN_INFWAIT);

	/* start an event flag test task */
	KRN_startTask(evcTaskFunc, &evctask, stack2, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "Event Flag Test Task");

	/* handshake by setting flag 31 */

	DBG_logF("%s setting handshake flag 31 \n", KRN_taskName(NULL));
	KRN_setFlags(&efcluster, BIT31);

	for (m = 0; m < 10; m++) {
		uint32_t flags;

		/* wait for any of  flags 0..7 */
		DBG_logF("%s waiting for ANY of flags 0..7 \n",
			 KRN_taskName(NULL));
		flags =
		    KRN_testFlags(&efcluster, 0x0ff, KRN_ANY, CLEARFLAGS,
				  KRN_INFWAIT);
		DBG_logF("%s received flags value %" PRIx32 " \n",
			 KRN_taskName(NULL), flags);

		/* wait for all of flags 0..7 */
		DBG_logF("%s waiting for ALL of flags 0..7 \n",
			 KRN_taskName(NULL));
		flags =
		    KRN_testFlags(&efcluster, 0x0ff, KRN_ALL, CLEARFLAGS,
				  KRN_INFWAIT);
		DBG_logF("%s received flags value %" PRIx32 " \n",
			 KRN_taskName(NULL), flags);
	}
	KRN_setTimer(&evctimer, (KRN_TIMERFUNC_T *) do_nothing, NULL,
		     INT_MAX / 2);
	KRN_lock(&evclock2, KRN_INFWAIT);
	KRN_lock(&evclock, KRN_INFWAIT);
	KRN_release();
	DBG_logF("%s killing %s\n", KRN_taskName(NULL), KRN_taskName(&evctask));
	KRN_removeTask(&evctask);
	KRN_cancelTimer(&evctimer);

	DBG_logF("%s waiting for pool timeout to expire\n", KRN_taskName(NULL));
	DBG_assert(KRN_takePool(&pool, 10) == NULL,
		   "Non-NULL message received");
	DBG_logF("%s timeout expired\n", KRN_taskName(NULL));

	DQ_init(&hibQ);
	for (m = 0; m < 20; m += 5) {
		DBG_logF("%s hibernating for %" PRId32 " ticks\n",
			 KRN_taskName(NULL), m);
		KRN_hibernate(&hibQ, m);
		DBG_logF("%s woken after timeout\n", KRN_taskName(NULL));
	}

	for (m = 0; m < 20; m += 5) {
		DBG_logF
		    ("%s hibernating for %" PRId32
		     " ticks leaving system idle apart from timer\n",
		     KRN_taskName(NULL), m);
		KRN_hibernate(&hibQ, m);
		DBG_logF("%s woken after timeout\n", KRN_taskName(NULL));
	}

	reportStacks();
	DBG_logF("%s returning\n", KRN_taskName(NULL));
	KRN_unlock(&lock);
	KRN_removeTask(NULL);
}

void timFunc(void)
{
	uint32_t total = TMR_stopCycleCount();
	DBG_logF("Task startup total: %" PRIu32 ".\n", total);
	for (;;) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_release();
		total = TMR_stopCycleCount();
		DBG_logF("KRN_release() (2 re-schedules) total: %" PRIu32 ".\n",
			 total);
	}
}

void hibFunc(void)
{
	/* this task gets killed shortly after being started - purpose of the test
	 * is to ensure that the hibernate timeout doesn't accidentally wake it up again later
	 */
	DBG_logF("%s hibernating, should be killed before it wakes\n",
		 KRN_taskName(NULL));
	KRN_hibernate(&hibernateQ, 10000000);
	DBG_assert(0, "Hibernating task erroneously re-activated\n");
}

volatile int32_t isrCount = 0;
IRQ_DESC_T soft;

void myHandler(int32_t sigNum)
{
	(void)sigNum;
	IRQ_ack(&soft);
	isrCount++;
}

KRN_FLAGCLUSTER_T flags;

void flags_test()
{
	IRQ_IPL_T ipl[2];
	KRN_initFlags(&flags);
	KRN_testFlags(&flags, 1, KRN_ANY, 1, 0);
	KRN_testFlags(&flags, 1, KRN_ANY, 1, 1);
	ipl[0] = IRQ_raiseIPL();
	ipl[1] = IRQ_raiseIPL();
	KRN_setFlags(&flags, 1);
	KRN_testFlags(&flags, 1, KRN_ANY, 1, 0);
	KRN_testFlagsProtected(&flags, 1, KRN_ALL, 1, &ipl[1]);
	KRN_testFlagsProtected(&flags, 1, KRN_ALL, 1, &ipl[0]);
	KRN_setFlags(&flags, 1);
	KRN_toggleFlags(&flags, 1);
	KRN_clearFlags(&flags, 1);
	IRQ_restoreIPL(ipl[1]);
	IRQ_restoreIPL(ipl[0]);
}

KRN_TIMER_T ttimer1, ttimer2, ttimer3, ttimer4;

void timer_test()
{
	uint32_t ten;
	uint32_t start;
	KRN_setTimer(&ttimer1, (KRN_TIMERFUNC_T *) do_nothing, NULL, 0);
	KRN_setTimer(&ttimer2, (KRN_TIMERFUNC_T *) do_nothing, NULL,
		     INT_MAX / 3);
	KRN_setTimer(&ttimer3, (KRN_TIMERFUNC_T *) do_nothing, NULL,
		     INT_MAX / 2);
	KRN_setSoftTimer(&ttimer4, (KRN_TIMERFUNC_T *) do_nothing, NULL,
			 (INT_MAX / 2), 1000);
	KRN_cancelTimer(&ttimer2);
	KRN_cancelTimer(&ttimer3);
	KRN_cancelTimer(&ttimer4);
	KRN_setTimeSlice(1000000);
	/* Ensure a timeslice occurs */
	ten = TMR_clockSpeed() * 10 * 1000;	/* 10ms */
	start = TMR_getMonotonic();
	while ((TMR_getMonotonic() - start) < ten) ;
	KRN_setTimeSlice(-1);
}

void mbox_test()
{
	IRQ_IPL_T ipl;
	KRN_MAILBOX_T mb;
	KRN_initMbox(&mb);
	ipl = IRQ_raiseIPL();
	KRN_getMbox(&mb, KRN_NOWAIT);
	IRQ_restoreIPL(ipl);
	KRN_getMbox(&mb, KRN_NOWAIT);
	KRN_getMbox(&mb, 1);
}

static KRN_TASK_T waketask;
static KRN_TASKQ_T sleep, sleep1, sleep2;
static uint32_t wakestack[STACKSIZE], hibstack[STACKSIZE];

void wake_hib()
{
	KRN_hibernate(&sleep1, 10);
	KRN_hibernate(&sleep, KRN_INFWAIT);
	KRN_hibernate(&sleep1, 10);
	KRN_hibernate(&sleep, KRN_INFWAIT);
	KRN_priority(NULL, KRN_maxPriority() - 1);
	KRN_hibernate(&sleep1, 10);
	KRN_hibernate(&sleep, KRN_INFWAIT);
	KRN_hibernate(&sleep1, 10);
	KRN_hibernate(&sleep, KRN_INFWAIT);
}

void wake_wake()
{
	KRN_hibernate(&sleep2, 20);
	KRN_wakeAll(&sleep);
	KRN_hibernate(&sleep2, 20);
	KRN_wake(&sleep);
	KRN_hibernate(&sleep2, 20);
	KRN_wakeAll(&sleep);
	KRN_hibernate(&sleep2, 20);
	KRN_wake(&sleep);
}

extern LST_T _LST_all;

void wake_test()
{
	DQ_init((DQ_T *) & sleep);
	DQ_init((DQ_T *) & sleep1);
	DQ_init((DQ_T *) & sleep2);
	KRN_startTask(wake_hib, &hibtask, hibstack, STACKSIZE,
		      KRN_maxPriority() - 2, NULL, "Hibernate");
	KRN_startTask(wake_wake, &waketask, wakestack, STACKSIZE,
		      KRN_maxPriority() - 2, NULL, "Wake");
}

static KRN_TASK_T nontask;
static uint32_t nonstack[STACKSIZE];

void krn_task()
{
	(void)KRN_taskName(KRN_me());
	(void)KRN_taskParameter(NULL);
}

void krn_test()
{
	KRN_startTask(krn_task, &nontask, nonstack, STACKSIZE,
		      KRN_maxPriority(), NULL, NULL);
	KRN_hibernate(&hibernateQ, 10);
}

void krn(KRN_TASK_T * bg, KRN_TASK_T * tim)
{
	int32_t iteration = 0;
	uint32_t total;
	IRQ_DESC_T *old;

	bgtask = bg;
	timtask = tim;

	DBG_logF("Kernel Test\n");

	KRN_priority(timtask, 0);

	DQ_init(&hibernateQ);
	KRN_hibernate(&hibernateQ, 3000);

	KRN_initLock(&lock);
	KRN_initLock(&evclock);
	KRN_initLock(&evclock2);
	message.text = (char *)"Mailbox message text";

	/* Basic scheduler timings - KRN_release() with no scheduling consequences */
	DBG_logF("*** Cycle Counting Tests ***\n");
	TMR_resetCycleCount();
	TMR_startCycleCount();
	KRN_release();
	total = TMR_stopCycleCount();
	DBG_logF("KRN_release() (no schedule) total: %" PRIu32 ".\n", total);

	TMR_resetCycleCount();
	TMR_startCycleCount();
	KRN_startTask(timFunc, &fgtask, stack1, STACKSIZE, KRN_LOWEST_PRIORITY,
		      NULL, "Counter task");
	KRN_release();
	KRN_release();
	KRN_release();
	KRN_removeTask(&fgtask);
	DBG_logF("*** Cycle Counting Tests Done ***\n");

	DBG_logF("*** Testing user ISR handlers ***\n");
	IRQ_soft(0, &soft);
	old = IRQ_find(&soft);
	DBG_assert(old == NULL, "Non-NULL desc");
	soft.isrFunc = myHandler;
	IRQ_route(&soft);
	old = IRQ_find(&soft);
	DBG_assert(old != NULL, "NULL desc");
	IRQ_synthesize(&soft);

	KRN_release();		/* just cause a few cycles delay */
	DBG_assert(isrCount == 1, "isrCount == %" PRIu32 ", expected 1\n",
		   isrCount);
	soft.isrFunc = NULL;
	IRQ_route(&soft);
	old = IRQ_find(&soft);
	DBG_assert(old == NULL, "Non-NULL desc");
	/* generate a second interrupt which should be masked */
	IRQ_synthesize(&soft);

	DBG_assert(isrCount =
		   1, "isrCount == %" PRIu32 ", expected 1\n", isrCount);
	/* reinstall the handler - should pick up the pending but masked interrupt */
	soft.isrFunc = myHandler;
	IRQ_route(&soft);
	old = IRQ_find(&soft);
	DBG_assert(old != NULL, "NULL desc");
	KRN_release();		/* delay.... */
	DBG_assert(isrCount == 2, "isrCount == %" PRIu32 ", expected 2\n",
		   isrCount);
	soft.isrFunc = NULL;
	IRQ_route(&soft);
	old = IRQ_find(&soft);
	DBG_assert(old == NULL, "Non-NULL desc");
	DBG_logF("*** Handler testing done ***\n");
	KRN_lock(&lock, KRN_INFWAIT);	/* seize the lock */
	KRN_startTask(hibFunc, &hibtask, stack3, STACKSIZE,
		      KRN_maxPriority(), NULL, "Hibernating task");
	KRN_priority(&hibtask, MAX_PRIORITY + 1);
	KRN_startTask(fgtaskFunc, &fgtask, stack1, STACKSIZE, -1,
		      NULL, "Foreground task");
	KRN_priority(&fgtask, -1);
	KRN_priority(&fgtask, 1);
	KRN_priority(&fgtask, 0);

	KRN_hibernate(&hibernateQ, 5000);
	DBG_logF("%s Woken from hibernation and killing %s \n",
		 KRN_taskName(NULL), KRN_taskName(&hibtask));

	KRN_removeTask(&hibtask);	/* remove the hibernating task before it wakes */
	DBG_logF("%s releasing lock \n", KRN_taskName(NULL));

	KRN_unlock(&lock);

	while (iteration < 10) {
		DBG_logF("%s iteration %" PRId32 "\n", KRN_taskName(NULL),
			 iteration++);
		KRN_release();
	}

	KRN_lock(&lock, KRN_INFWAIT);

	flags_test();
	timer_test();
	mbox_test();
	wake_test();
	krn_test();
#ifdef CONFIG_DEBUG_PROFILING_DUMP
	extern void _KRN_dumpPerf(void);
	_KRN_dumpPerf();
#endif
}
