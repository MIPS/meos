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
*   Description:	Kernel interthread messaging test, thread 0
*
*************************************************************************/

#include <stdlib.h>

/* we require assert to make this program work */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#include "MEOS.h"

#define MAXIMP 32
#define MAXEXP 32
#define STACKSIZE 4000
#define INFWAIT (-1)
#define CLEAR 1
#define NOCLEAR 0

#define PRIORITIES 2
#define MAX_PRIORITY (PRIORITIES-1)

#define POOLSIZE 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#ifdef CONFIG_DEBUG_PARANOIA
#define ECHOES 1
#else
#define ECHOES 1
#endif

typedef struct {
	KRN_POOLLINK;
	volatile uintptr_t content;
} POOL_ITEM_T;

static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_SCHEDULE_T sched;
static KRN_TASK_T *bgtask;
static KRN_TASK_T *timtask;
static KRN_SYNC_T sync;
static KRN_SYNC_T wsync;
static KRN_SEMAPHORE_T s1;
static KRN_SEMAPHORE_T s2;
static KRN_SEMAPHORE_T s3;
static KRN_SEMAPHORE_T sx;
static KRN_LOCK_T l0;
static KRN_LOCK_T l1;
static KRN_LOCK_T l2;
static KRN_LOCK_T l3;
static KRN_LOCK_T lx;
static KRN_MAILBOX_T mb0;
static KRN_MAILBOX_T mb1;
static KRN_MAILBOX_T mb2;
static KRN_MAILBOX_T mb3;
static KRN_POOL_T p0;
static KRN_POOL_T mbp;
static KRN_WQ_T wq0;
static KRN_FLAGCLUSTER_T f0;
static uint32_t task2Stack[STACKSIZE];
static uint32_t task3Stack[STACKSIZE];
static KRN_TASK_T task2Task, task3Task;
static KRN_TASK_T *task0, *task1, *task2, *task3;

POOL_ITEM_T poolItems[POOLSIZE], mbItems[4];
static KRN_TASK_T wqTasks[1];
static uint32_t wqStacks[1][STACKSIZE];
static KRN_JOB_T wqJobs[8];
static uint32_t wqCount = 0;

static uint32_t timstack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_EXPORT_T expTable[MAXEXP];
static KRN_IMPORT_T impTable[MAXIMP];
static uint8_t maxImps[] = { 0, MAXIMP, 0, 0 };	/* T1 exports */

KRN_TRACE_T traceBuf[4096];

void wq_test(void)
{
	wqCount++;
}

void wq_sync(void)
{
	KRN_sync(&wsync, KRN_INFWAIT);
}

void test(void)
{
	int32_t n;
	void *packet, *p, *p1 = NULL, *p2 = NULL;
	POOL_ITEM_T *pi;
	uint32_t flags;
	int32_t trtstart, trtime, trtmin, trtmax, trtsum;	/* turnround time statistics for mailbox echo timing test */
	KRN_TASK_T *t;
	KRN_TASKQ_T waitq;

	DQ_init(&waitq);

	DBG_logF("%" PRIu32 "/%s: Starting test\n", KRN_proc(),
		 KRN_taskName(NULL));

	DBG_logF("%" PRIu32 "/%s: *** Simple two thread semaphore test ***\n",
		 KRN_proc(), KRN_taskName(NULL));

	for (n = 0; n < 5; n++) {
		t = KRN_sync(&sync, KRN_INFWAIT);
		if (t == task1) {
			DBG_logF("%" PRIu32
				 "/%s: Setting exported semaphore s1...\n",
				 KRN_proc(), KRN_taskName(NULL));
			KRN_setSemaphore(&s1, 1);
			DBG_logF("%" PRIu32
				 "/%s: Waiting on exported sempahore s2...\n",
				 KRN_proc(), KRN_taskName(NULL));
			if (!KRN_testSemaphore(&s2, 1, n & 1 ? INFWAIT : 1000000))	/* alternate tests use infinite or finite timeout */
				DBG_logF("%" PRIu32
					 "/%s: *** ERROR *** test returned failure, should have succeeded\n",
					 KRN_proc(), KRN_taskName(NULL));
			DBG_logF("%" PRIu32
				 "/%s: ...Reply received via imported semaphore\n",
				 KRN_proc(), KRN_taskName(NULL));
		} else if (t == task0) {
			DBG_logF("%" PRIu32
				 "/%s: Waiting on imported semaphore s1...\n",
				 KRN_proc(), KRN_taskName(NULL));
			if (!KRN_testSemaphore(&s1, 1, n & 1 ? 1000000 : INFWAIT))	/* alternate tests use infinite or finite timeout */
				DBG_logF("%" PRIu32
					 "/%s: *** ERROR *** test returned failure, should have succeeded\n",
					 KRN_proc(), KRN_taskName(NULL));
			DBG_logF("%" PRIu32 "/%s: ... s1 seized\n", KRN_proc(),
				 KRN_taskName(NULL));
			DBG_logF("%" PRIu32
				 "/%s: Setting exported semaphore s2...\n",
				 KRN_proc(), KRN_taskName(NULL));
			KRN_setSemaphore(&s2, 1);
		}
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == task0) {
		DBG_logF("%" PRIu32 "/%s: Testing Timeout expiry on s2...\n",
			 KRN_proc(), KRN_taskName(NULL));
		if (KRN_testSemaphore(&s2, 1, 10))
			DBG_logF("%" PRIu32
				 "/%s: *** ERROR *** test returned success, should have failed\n",
				 KRN_proc(), KRN_taskName(NULL));
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Contending threads semaphore test ***\n",
		 KRN_proc(), KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		for (n = 0; n < 3; n++) {
			DBG_logF("%" PRIu32
				 "/%s: Setting exported semaphore s3...\n",
				 KRN_proc(), KRN_taskName(NULL));
			KRN_setSemaphore(&s3, 1);
		}
	} else {
		/* issue one (of 3) responses in the multi=thread test */
		DBG_logF("%" PRIu32
			 "/%s: Waiting on imported semaphore s3...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_testSemaphore(&s3, 1, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... s3 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		for (n = 0; n < 3; n++) {
			DBG_logF("%" PRIu32
				 "/%s: Waiting on exported sempahore s2...\n",
				 KRN_proc(), KRN_taskName(NULL));
			KRN_testSemaphore(&s2, 1, INFWAIT);
			DBG_logF("%" PRIu32
				 "/%s: ...Reply received via exported semaphore s2\n",
				 KRN_proc(), KRN_taskName(NULL));
		}
	} else {
		DBG_logF("%" PRIu32 "/%s: Setting imported semaphore s2...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_setSemaphore(&s2, 1);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Lock test ***\n", KRN_proc(),
		 KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task0) {
		DBG_logF("%" PRIu32 "/%s: Seizing Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 seized\n", KRN_proc(),
			 KRN_taskName(NULL));

	} else if (t == task1) {
		DBG_logF("%" PRIu32 "/%s: Seizing Lock 1...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l1, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 1 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
	} else if (t == task2) {
		DBG_logF("%" PRIu32 "/%s: Seizing Lock 2...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l2, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 2 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
	} else if (t == task3) {
		DBG_logF("%" PRIu32 "/%s: Seizing Lock 3...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l3, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 3 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
	}
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task0) {
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 0\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l0);
		DBG_logF("%" PRIu32 "/%s: Seizing Locks 1,2,3 and 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_lock(&l1, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Lock 1 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l2, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Lock 2 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l3, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Lock 3 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Lock 0 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
	} else if (t == task1) {
		DBG_logF("%" PRIu32 "/%s: Waiting for Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 1...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l1);
		DBG_logF("%" PRIu32 "/%s: ...Lock 1 released\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l0);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 released\n", KRN_proc(),
			 KRN_taskName(NULL));
	} else if (t == task2) {
		DBG_logF("%" PRIu32 "/%s: Waiting for Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 2...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l2);
		DBG_logF("%" PRIu32 "/%s: ...Lock 2 released\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l0);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 released\n", KRN_proc(),
			 KRN_taskName(NULL));
	} else if (t == task3) {
		DBG_logF("%" PRIu32 "/%s: Waiting for Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_lock(&l0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 seized\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 3...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l3);
		DBG_logF("%" PRIu32 "/%s: ...Lock 3 released\n", KRN_proc(),
			 KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Releasing Lock 0...\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_unlock(&l0);
		DBG_logF("%" PRIu32 "/%s: ...Lock 0 released\n", KRN_proc(),
			 KRN_taskName(NULL));
	}

	KRN_hibernate(&waitq, 1);
	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Mailbox Test ***\n", KRN_proc(),
		 KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	packet = KRN_takePool(&mbp, INFWAIT);
	((POOL_ITEM_T *) packet)->content = 0xdeadbeef;
	DBG_logF("%" PRIu32 "/%s: Taken packet %p\n",
		 KRN_proc(), KRN_taskName(NULL), packet);
	/*KRN_flushCache(packet, sizeof(POOL_ITEM_T),
	   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
	if (t == task0) {
		DBG_logF("%" PRIu32 "/%s: Sending packet to Mailbox 0\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_putMbox(&mb0, packet);
		DBG_logF("%" PRIu32 "/%s: Taking packet from Mailbox 3...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb3, INFWAIT);
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T), KRN_FLUSH_FLAG_D); */
		DBG_logF("%" PRIu32 "/%s: ...Packet received from Mailbox 3\n",
			 KRN_proc(), KRN_taskName(NULL));
		assert(p == packet);
	} else if (t == task1) {
		DBG_logF("%" PRIu32 "/%s: Taking packet from Mailbox 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb0, INFWAIT);
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T), KRN_FLUSH_FLAG_D); */
		DBG_logF("%" PRIu32 "/%s: ...Packet received from Mailbox 0\n",
			 KRN_proc(), KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Sending packet to Mailbox 1\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb1, p);
	} else if (t == task2) {
		DBG_logF("%" PRIu32 "/%s: Taking packet from Mailbox 1...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb1, INFWAIT);
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T), KRN_FLUSH_FLAG_D); */
		DBG_logF("%" PRIu32 "/%s: ...Packet received from Mailbox 1\n",
			 KRN_proc(), KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Sending packet to Mailbox 2\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb2, p);
	} else if (t == task3) {
		DBG_logF("%" PRIu32 "/%s: Taking packet from Mailbox 2...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb2, INFWAIT);
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T), KRN_FLUSH_FLAG_D); */
		DBG_logF("%" PRIu32
			 "/%s: ...Packet received from Mailbox 2...\n",
			 KRN_proc(), KRN_taskName(NULL));
		DBG_logF("%" PRIu32 "/%s: Sending packet to Mailbox 3\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb3, p);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Mailbox echo timing test ***\n",
		 KRN_proc(), KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task0) {
		trtsum = 0;
		trtmin = 0x7fffffff;
		trtmax = 0;
		for (n = 0; n < ECHOES; n++) {
			trtstart = TMR_getMonotonic();
			/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
			   KRN_FLUSH_FLAG_D |
			   KRN_FLUSH_FLAG_WRITEBACK_D); */
			KRN_putMbox(&mb0, packet);
			p = KRN_getMbox(&mb1, INFWAIT);
			/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
			   KRN_FLUSH_FLAG_D); */
			trtime = TMR_getMonotonic() - trtstart;
			trtsum += trtime;
			if (trtime < trtmin)
				trtmin = trtime;
			if (trtime > trtmax)
				trtmax = trtime;
		}
		DBG_logF("%" PRIu32 "/%s: Mailbox Round Trip Times (us): Mean=%"
			 PRId32 " Min=%" PRId32 " Max=%" PRId32 "\n",
			 KRN_proc(), KRN_taskName(NULL), trtsum / 1000, trtmin,
			 trtmax);
	} else if (t == task1) {
		for (n = 0; n < ECHOES; n++) {
			p = KRN_getMbox(&mb0, INFWAIT);
			/*KRN_flushCache(p, sizeof(POOL_ITEM_T),
			   KRN_FLUSH_FLAG_D); */
			KRN_putMbox(&mb1, p);
		}
	}
	KRN_returnPool(packet);
	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		p = KRN_getMbox(&mb0, 0);
		if (p)
			DBG_logF("%" PRIu32
				 "/%s: Unexpectedly received %p from mb0!\n",
				 KRN_proc(), KRN_taskName(NULL), p);
		assert(!p);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Contending local pool take test *** \n",
		 KRN_proc(), KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task1) {
		DBG_logF("%" PRIu32 "/%s: Allocating item from Pool 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p1 = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: Allocating item from Pool 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p2 = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Delivering item to mailbox 1\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p2, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb1, p2);
		DBG_logF("%" PRIu32 "/%s: Allocating item from Pool 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p2 = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Delivering item to mailbox 2\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p2, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb2, p2);
		DBG_logF("%" PRIu32 "/%s: Allocating item from Pool 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p2 = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Delivering item to mailbox 3\n",
			 KRN_proc(), KRN_taskName(NULL));
		/*KRN_flushCache(p2, sizeof(POOL_ITEM_T),
		   KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); */
		KRN_putMbox(&mb3, p2);
	} else if (t == task0) {
		DBG_logF("%" PRIu32 "/%s: Taking item from mailbox 1...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb1, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Returning item (%p) to pool...\n",
			 KRN_proc(), KRN_taskName(NULL), p);
		KRN_returnPool(p);
	} else if (t == task2) {
		DBG_logF("%" PRIu32 "/%s: Taking item from mailbox 2...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb2, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Returning item to pool...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_returnPool(p);
	} else if (t == task3) {
		DBG_logF("%" PRIu32 "/%s: Taking item from mailbox 3...\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_getMbox(&mb3, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ... Returning item (%p) to pool...\n",
			 KRN_proc(), KRN_taskName(NULL), p);
		KRN_returnPool(p);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Contending remote pool take test ***\n",
		 KRN_proc(), KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task0) {
		p = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: Pause after taking %p from pool\n",
			 KRN_proc(), KRN_taskName(NULL), p);
		KRN_hibernate(&waitq, 10);
		DBG_logF("%" PRIu32 "/%s: Returning %p to pool\n", KRN_proc(),
			 KRN_taskName(NULL), p);
		KRN_returnPool(p);
	} else if (t == task2) {
		DBG_logF("%" PRIu32 "/%s: Starting remote pool-take test\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: Pause after taking %p from pool\n",
			 KRN_proc(), KRN_taskName(NULL), p);
		KRN_hibernate(&waitq, 10);
		DBG_logF("%" PRIu32 "/%s: Returning %p to pool\n", KRN_proc(),
			 KRN_taskName(NULL), p);
		KRN_returnPool(p);
	} else if (t == task3) {
		DBG_logF("%" PRIu32 "/%s: Starting remote pool-take test\n",
			 KRN_proc(), KRN_taskName(NULL));
		p = KRN_takePool(&p0, INFWAIT);
		DBG_logF("%" PRIu32
			 "/%s: 1 Pause after taking %p from pool\n",
			 KRN_proc(), KRN_taskName(NULL), p);
		KRN_hibernate(&waitq, 10);
		DBG_logF("%" PRIu32 "/%s: Returning %p to pool\n", KRN_proc(),
			 KRN_taskName(NULL), p);
		KRN_returnPool(p);
	}
	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		DBG_logF("%" PRIu32
			 "/%s: Returning 1st item (%p) to local pool\n",
			 KRN_proc(), KRN_taskName(NULL), p1);
		KRN_returnPool(p1);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Event Flag Tests ***\n", KRN_proc(),
		 KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task0) {
		/* set flags 0...3 in turn */
		for (n = 0; n < 4; n++) {
			KRN_hibernate(&waitq, 10);
			DBG_logF("%" PRIu32 "/%s: Setting flag %" PRId32 "\n",
				 KRN_proc(), KRN_taskName(NULL), n);
			KRN_setFlags(&f0, 1 << n);
		}

		/* wait for our own flag, clear it and set flag 4 */
		DBG_logF("%" PRIu32 "/%s: Wait for flag 0\n", KRN_proc(),
			 KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 1 << 0, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		DBG_logF("%" PRIu32 "/%s: Setting flag 4\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 4);
	} else if (t == task1) {
		/* wait for flag 1 and clear it */
		DBG_logF("%" PRIu32 "/%s: Waiting for flag 1\n", KRN_proc(),
			 KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 1 << 1, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		/* set flag 5 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 5\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 5);
	} else if (t == task2) {
		/* wait for flag 2 and clear it */
		DBG_logF("%" PRIu32 "/%s: Waiting for flag 2\n", KRN_proc(),
			 KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 1 << 2, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		/* set flag 6 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 6\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 6);

	} else if (t == task3) {
		/* wait for flag 3 and clear it */
		DBG_logF("%" PRIu32 "/%s: Waiting for flag 3\n", KRN_proc(),
			 KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 1 << 3, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		/* set flag 7 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 7\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 7);
	}

	if (t == task0) {
		/* wait for all of flags 4...7 */
		DBG_logF("%" PRIu32 "/%s: Waiting for ALL of flags 4...7\n",
			 KRN_proc(), KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 0x0f0, KRN_ALL, 1, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);

		/* set flags 8...11 */
		for (n = 8; n < 12; n++) {
			KRN_hibernate(&waitq, 10);
			DBG_logF("%" PRIu32 "/%s: Setting flag %" PRId32 "\n",
				 KRN_proc(), KRN_taskName(NULL), n);
			KRN_setFlags(&f0, 1 << n);
		}
	} else if (t == task1) {
		/* Wait for any of flags 8...11 (no clear) */
		DBG_logF("%" PRIu32 "/%s: Waiting for ANY of flags 8...11\n",
			 KRN_proc(), KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 0xf00, KRN_ANY, NOCLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);

		/* set flag 5 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 5\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 5);
	} else if (t == task2) {
		/* Wait for ALL of flags 8...11 and clear */
		DBG_logF("%" PRIu32 "/%s: Waiting for ALL of flags 8...9\n",
			 KRN_proc(), KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 0x300, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		/* set flag 6 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 6\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 6);
	} else if (t == task3) {
		/* Wait for ANY of flags 10...11 (no clear) */
		DBG_logF("%" PRIu32
			 "/%s: Waiting for ANY of flags 10...11\n",
			 KRN_proc(), KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 0xc00, KRN_ANY, NOCLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
		/* set flag 7 */
		DBG_logF("%" PRIu32 "/%s: Setting flag 7\n", KRN_proc(),
			 KRN_taskName(NULL));
		KRN_setFlags(&f0, 1 << 7);
	}

	if (t == task0) {
		/* wait for all of 5...7 */
		DBG_logF("%" PRIu32 "/%s: Waiting for ALL of flags 5...7\n",
			 KRN_proc(), KRN_taskName(NULL));
		flags = KRN_testFlags(&f0, 0x0e0, KRN_ALL, CLEAR, INFWAIT);
		DBG_logF("%" PRIu32 "/%s: ...Flag result = %08" PRIx32 "\n",
			 KRN_proc(), KRN_taskName(NULL), flags);
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: *** Work Queue Tests ***\n", KRN_proc(),
		 KRN_taskName(NULL));
	t = KRN_sync(&sync, KRN_INFWAIT);
	if (t == task1) {

		DBG_logF("%" PRIu32
			 "/%s: Delivering test function to mailbox 0 3 times...\n",
			 KRN_proc(), KRN_taskName(NULL));
		for (n = 0; n < 3; n++) {
			pi = (POOL_ITEM_T *) KRN_takePool(&mbp, INFWAIT);
			DBG_logF("%" PRIu32 "/%s: Before %p in %p@%p\n",
				 KRN_proc(), KRN_taskName(NULL),
				 (void *)pi->content, (void *)pi,
				 (void *)&pi->content);
			pi->content = (uintptr_t) wq_test;
			/*KRN_flushCache(pi, sizeof(pi),
			   KRN_FLUSH_FLAG_D |
			   KRN_FLUSH_FLAG_WRITEBACK_D); */
			DBG_logF("%" PRIu32 "/%s: Sending %p in %p@%p\n",
				 KRN_proc(), KRN_taskName(NULL),
				 (void *)pi->content, (void *)pi,
				 (void *)&pi->content);
			KRN_putMbox(&mb0, pi);
		}
		KRN_queueWQ(&wq0, (KRN_TASKFUNC_T *) wq_test, NULL,
			    "WQ callback", 0);
	} else {
		DBG_logF("%" PRIu32
			 "/%s: Taking test function from mailbox 0...\n",
			 KRN_proc(), KRN_taskName(NULL));
		pi = (POOL_ITEM_T *) KRN_getMbox(&mb0, INFWAIT);
		DBG_logF("pi=%p\n", pi);
		uint32_t count;
		for (count = 0; pi->content == 0; count++) ;
		DBG_logF("Stayed NULL %" PRIu32 " times\n", count);
		/*KRN_flushCache(pi, sizeof(pi), KRN_FLUSH_FLAG_D); */

		DBG_logF("%" PRIu32
			 "/%s: queueing call to %p from %p@%p...\n",
			 KRN_proc(), KRN_taskName(NULL), (void *)pi->content,
			 (void *)pi, (void *)&pi->content);
		KRN_queueWQ(&wq0, (KRN_TASKFUNC_T *) pi->content, NULL,
			    "WQ callback", 0);
		KRN_returnPool(pi);
	}
	DBG_logF("%" PRIu32 "/%s: Primary wq test initiated\n",
		 KRN_proc(), KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		DBG_logF("%" PRIu32 "/%s: Waiting for wq to complete...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_queueWQ(&wq0, wq_sync, NULL, "WQ sync", 0);
	} else {
		DBG_logF("%" PRIu32 "/%s: Waiting for wq to complete...\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_sync(&wsync, KRN_INFWAIT);
	}
	DBG_logF("%" PRIu32 "/%s: wq completed...\n",
		 KRN_proc(), KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == task1) {
		assert(wqCount == 4);
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == task0) {
#ifdef TEST_ASSERTS
#ifndef NDEBUG
		/* pause for other tests to finish */
		DBG_logF("%" PRIu32
			 "/%s: Trying to set mismatched semaphore -  will cause a failure\n",
			 KRN_proc(), KRN_taskName(NULL));
		KRN_setSemaphore(&sx, 1);
#endif
#endif
	}

	KRN_sync(&sync, KRN_INFWAIT);
	DBG_logF("%" PRIu32 "/%s: Goodbye!\n", KRN_proc(), KRN_taskName(NULL));
	KRN_sync(&sync, KRN_INFWAIT);
	exit(0);

}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE,
		  traceBuf, 4096);
	KRN_installImpExp(maxImps, MAXEXP, impTable, expTable);

	bgtask = KRN_startOS("Task 0");
	if (KRN_proc() == 1)
		bgtask->name = "Task 1";

	DBG_logF("%" PRIu32 "/%s: Inter-Thread Kernel Test\n", KRN_proc(),
		 KRN_taskName(NULL));

	timtask =
	    KRN_startTimerTask("Timer Processing Task", timstack, STACKSIZE);

	BSP_init();

	KRN_initSync(&sync, 4);
	KRN_initSync(&wsync, 4);

	KRN_initSemaphore(&s1, 0);
	KRN_initSemaphore(&s2, 0);
	KRN_initSemaphore(&s3, 0);

	KRN_initLock(&l0);
	KRN_initLock(&l1);
	KRN_initLock(&l2);
	KRN_initLock(&l3);
	KRN_initLock(&lx);
	KRN_initMbox(&mb0);
	KRN_initMbox(&mb1);
	KRN_initMbox(&mb2);
	KRN_initMbox(&mb3);
	KRN_initPool(&p0, poolItems, POOLSIZE, sizeof(POOL_ITEM_T));
	KRN_initPool(&mbp, mbItems, 4, sizeof(POOL_ITEM_T));
	KRN_initFlags(&f0);
	if (KRN_proc() == 1) {
		KRN_initWQ(&wq0, wqTasks, (uint32_t *) wqStacks, 1, STACKSIZE,
			   KRN_maxPriority() - 1, wqJobs, 8);
		/* Processor 1 exports, allowing 0 to be Linux */
		KRN_export(1, &s1);
		KRN_export(2, &s2);
		KRN_export(3, &s3);
		KRN_export(4, &l0);
		KRN_export(5, &l1);
		KRN_export(6, &l2);
		KRN_export(7, &l3);
		KRN_export(8, &mb0);
		KRN_export(9, &mb1);
		KRN_export(10, &mb2);
		KRN_export(11, &mb3);
		KRN_export(12, &p0);
		KRN_export(13, &f0);
		KRN_export(14, &lx);
		KRN_export(15, &sync);
		KRN_export(16, &mbp);
		KRN_export(17, &wq0);
		KRN_export(18, &wsync);
	} else {
		KRN_initWQ(&wq0, NULL, NULL, 0, 0, 0, NULL, 0);
		KRN_import(1, 1, &s1);
		KRN_import(1, 2, &s2);
		KRN_import(1, 3, &s3);
		KRN_import(1, 4, &l0);
		KRN_import(1, 5, &l1);
		KRN_import(1, 6, &l2);
		KRN_import(1, 7, &l3);
		KRN_import(1, 8, &mb0);
		KRN_import(1, 9, &mb1);
		KRN_import(1, 10, &mb2);
		KRN_import(1, 11, &mb3);
		KRN_import(1, 12, &p0);
		KRN_import(1, 13, &f0);
		KRN_import(1, 14, &sx);	/* deliberate mistake - import an exported lock as a semaphore */
		KRN_import(1, 15, &sync);
		KRN_import(1, 16, &mbp);
		KRN_import(1, 17, &wq0);
		KRN_import(1, 18, &wsync);
	}
	if (KRN_proc() == 0) {
		task0 = KRN_me();
		test();
	} else {
		task1 = KRN_me();
		task2 = &task2Task;
		task3 = &task3Task;
		switch (MIN(KRN_procs(), CONFIG_FEATURE_MAX_PROCESSORS)) {
		default:
			KRN_startTask(test, &task2Task, task2Stack, STACKSIZE,
				      MAX_PRIORITY, NULL, "Task 2");
			KRN_startTask(test, &task3Task, task3Stack, STACKSIZE,
				      MAX_PRIORITY, NULL, "Task 3");

			break;
		case 3:
			if (KRN_proc() != 1) {
				KRN_startTask(test, &task2Task, task2Stack,
					      STACKSIZE, MAX_PRIORITY, NULL,
					      "Task 2");
				KRN_startTask(test, &task3Task, task3Stack,
					      STACKSIZE, MAX_PRIORITY, NULL,
					      "Task 3");
				KRN_removeTask(NULL);
			}
		case 4:
			switch (KRN_proc()) {
			case 2:
				KRN_startTask(test, &task2Task, task2Stack,
					      STACKSIZE, MAX_PRIORITY, NULL,
					      "Task 2");
				KRN_removeTask(NULL);
				break;
			case 3:
				KRN_startTask(test, &task3Task, task3Stack,
					      STACKSIZE, MAX_PRIORITY, NULL,
					      "Task 3");
				KRN_removeTask(NULL);
				break;
			default:
				break;
			}
		}
		test();
	}

	return 0;
}
