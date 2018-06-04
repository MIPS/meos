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
*   Description:	Time basic operations test
*
*************************************************************************/

#include "MEOS.h"
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)
#define HIGHEST 4
#define HIGH 3
#define LOW 2
#define LOWEST 1
#define STACKSIZE 2000
#define TRACESIZE 256
KRN_TRACE_T traceBuf[TRACESIZE];
#define TRACEBUF traceBuf

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

uint32_t istack[STACKSIZE];

/* Sample data types */
uint32_t result[CONFIG_TEST_TIMING_N + 1], startup[CONFIG_TEST_TIMING_N + 1];

/* Import asm functions */

uint32_t testOverhead(void);
uint32_t testLatency(void);
uint32_t testReturnOverhead(void);
uint32_t testReturnLatency(void);
uint32_t testTotalOverhead(void);
uint32_t testTotalLatency(void);
uint32_t testIsrOverhead(void);
uint32_t testIsrLatency(void);
uint32_t testIsrReturnOverhead(void);
uint32_t testIsrReturnLatency(void);
uint32_t testIsrTotalOverhead(void);
uint32_t testIsrTotalLatency(void);
void testIsr(int32_t sigNum);
void testIsrRet(int32_t sigNum);
void testIsrTot(int32_t sigNum);
void TIMING_platformAdditional(int64_t so);

/* Test scratch space */
static KRN_TASK_T thread;
static uint32_t workspace[STACKSIZE];
static KRN_TASKQ_T queue;
static KRN_SEMAPHORE_T semaphore;
static KRN_LOCK_T lock;
static KRN_MAILBOX_T mbox;
static LST_LINKAGE_T listable, listable2;
static KRN_FLAGCLUSTER_T flags;
static KRN_POOL_T pool;
static KRN_POOLLINK_T poolbuffer;
static KRN_TIMER_T timer;
static KRN_LOCK_T taskLock;

/*
 * Given sample data representing testing overheads, extract the average
 * so that it may be subtracted from subsequent tests to compensate for said
 * overheads.
 */
int64_t computeOverhead(uint32_t * res, int32_t first, int32_t last)
{
	int32_t i, n = 0;
	int64_t overhead = 0;
	for (i = first; i < last + 1; i++) {
		/* Reject negative results */
		if (res[i] < 0xffff0000) {
			overhead += res[i];
			n++;
		}
	}
	if (n)
		return (overhead * 1000) / n;
	else
		return 0;
}

/* Take sample data, compensate for overheads, compute and display statistics.*/
void
printResult(uint32_t * res, int32_t first, int32_t last, const char *title,
	    int64_t overhead)
{
	int64_t min = 0xffffffff, max = 0, cycles, average = 0, avgdev = 0;
	int32_t confavg = 0, confmin = 0, used = 0, i;

	for (i = first; i < last + 1; i++) {
		cycles = res[i] * 1000;
		if (cycles < overhead)
			res[i] = 0;
		else {
			res[i] = (uint32_t) cycles - overhead;
			used++;
		}
	}

	for (i = first; i < last + 1; i++) {
		cycles = res[i];
		if (cycles) {
			average += cycles;
			if (cycles < min)
				min = cycles;
			if (cycles > max)
				max = cycles;
		}
	}

	if (used)
		average /= used;

	for (i = first; i < last + 1; i++) {
		cycles = res[i] - average;
		if (cycles < 0)
			cycles = -cycles;
		avgdev += cycles;
	}
	avgdev /= (1 + last - first);

	if (avgdev == 0) {
		confavg = 100;
		confmin = 100;
	} else {
		for (i = first; i < last + 1; i++) {
			cycles = res[i];
			if ((cycles < (average + avgdev))
			    && (cycles >= (average - avgdev)))
				confavg++;
			if ((cycles < (min + avgdev))
			    && (cycles >= (min - avgdev)))
				confmin++;
		}
		confavg = (confavg * 100) / (1 + last - first);
		confmin = (confmin * 100) / (1 + last - first);
	}

	DBG_logF
	    ("%7" PRIu32 ".%02" PRIu32 " %7" PRIu32 ".%02" PRIu32 " %7" PRIu32
	     ".%02" PRIu32 " %7" PRIu32 ".%02" PRIu32 " %3" PRIu32 " %3" PRIu32
	     " %s\n", (uint32_t) (average / 1000),
	     (uint32_t) ((average % 1000) / 10), (uint32_t) (min / 1000),
	     (uint32_t) ((min % 1000) / 10), (uint32_t) (max / 1000),
	     (uint32_t) ((max % 1000) / 10), (uint32_t) (avgdev / 1000),
	     (uint32_t) ((avgdev % 1000) / 10), confavg, confmin, title);
}

/* Task testing utility functions */

void taskOther()
{
	for (;;) {
		KRN_hibernate(&queue, -1);
	}
}

void taskXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void taskXTask(int32_t i)
{
	KRN_startTask(taskXTaskOther, &thread, workspace, STACKSIZE,
		      LOW, (void *)((uintptr_t) i), "taskXTask");
	TMR_startCycleCount();
	KRN_hibernate(&queue, -1);
}

/* Semaphore testing utility functions */
void semaphoreXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	KRN_testSemaphore(&semaphore, 1, -1);
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void semaphoreXTask(int32_t i)
{
	KRN_initSemaphore(&semaphore, 0);
	KRN_startTask(semaphoreXTaskOther, &thread, workspace,
		      STACKSIZE, LOW, (void *)((uintptr_t) i),
		      "semaphoreXTask");
	KRN_release();
	TMR_startCycleCount();
	KRN_setSemaphore(&semaphore, 1);
	KRN_hibernate(&queue, -1);
}

/* Lock testing utility functions */
void lockOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	TMR_startCycleCount();
	KRN_lock(&lock, 0);
	result[i] = TMR_stopCycleCount();
	KRN_setSemaphore(&semaphore, 1);
	KRN_removeTask(NULL);
}

void lockXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	KRN_lock(&lock, -1);
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void lockXTask(int32_t i)
{
	KRN_initLock(&lock);
	KRN_lock(&lock, -1);
	KRN_startTask(lockXTaskOther, &thread, workspace, STACKSIZE,
		      LOW, (void *)((uintptr_t) i), "lockXTask");
	KRN_release();
	TMR_startCycleCount();
	KRN_unlock(&lock);
	KRN_hibernate(&queue, -1);
}

/* Mailbox testing utility functions */
void mboxXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	KRN_getMbox(&mbox, -1);
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void mboxXTask(int32_t i)
{
	KRN_initMbox(&mbox);
	KRN_startTask(mboxXTaskOther, &thread, workspace, STACKSIZE,
		      LOW, (void *)((uintptr_t) i), "mboxXTask");
	KRN_release();
	TMR_startCycleCount();
	KRN_putMbox(&mbox, &listable);
	KRN_hibernate(&queue, -1);
}

/* Flag testing utility functions */
void flagsXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	KRN_testFlags(&flags, 1, KRN_ANY, 1, -1);
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void flagsXTask(int32_t i)
{
	KRN_initFlags(&flags);
	KRN_startTask(flagsXTaskOther, &thread, workspace, STACKSIZE,
		      LOW, (void *)((uintptr_t) i), "flagsXTask");
	KRN_release();
	TMR_startCycleCount();
	KRN_setFlags(&flags, 1);
	KRN_hibernate(&queue, -1);
}

/* Pool testing utility functions */
void poolXTaskOther()
{
	int32_t i = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
	KRN_takePool(&pool, -1);
	result[i] = TMR_stopCycleCount();
	KRN_wake(&queue);
	KRN_removeTask(NULL);
}

void poolXTask(int32_t i)
{
	KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
	KRN_takePool(&pool, -1);
	KRN_startTask(poolXTaskOther, &thread, workspace, STACKSIZE,
		      LOW, (void *)((uintptr_t) i), "poolXTask");
	KRN_release();
	TMR_startCycleCount();
	KRN_returnPool(&poolbuffer);
	KRN_hibernate(&queue, -1);
}

/* Timer testing utility functions */
void timerOther(KRN_TIMER_T * timer, void *timerPar)
{
	(void)timer;
	(void)timerPar;
}

void TIMING_capturePlatformSpecific(void) __attribute__ ((weak));
void TIMING_capturePlatformSpecific(void)
{
}

void TIMING_reportPlatformSpecific(void) __attribute__ ((weak));
void TIMING_reportPlatformSpecific(void)
{
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
	int32_t i = 0;
	int64_t so, xo;

	TMR_resetCycleCount();

#ifdef CONFIG_CTX_RESTARTABLE
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
#endif
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack,
			  STACKSIZE, TRACEBUF, TRACESIZE);
		KRN_startOS("Background Task");
		startup[i] = TMR_stopCycleCount();
#ifdef CONFIG_CTX_RESTARTABLE
	}
#endif

	BSP_init();

	DBG_logF("Timing Test\n");

	TIMING_capturePlatformSpecific();

	KRN_initLock(&taskLock);
	DQ_init(&queue);

	KRN_priority(NULL, HIGHEST);

	TMR_resetCycleCount();
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_startCycleCount();
		result[i] = TMR_stopCycleCount();
	}

	so = computeOverhead(result, 1, CONFIG_TEST_TIMING_N);

	TMR_resetCycleCount();
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_startCycleCount();
		KRN_hibernate(&queue, 0);
		int32_t j = (int32_t) ((uintptr_t) KRN_taskParameter(NULL));
		(void)j;
		result[i] = TMR_stopCycleCount();
	}

	xo = computeOverhead(result, 1, CONFIG_TEST_TIMING_N);

	DBG_logF("Active samples:                            %10u\n",
		 CONFIG_TEST_TIMING_N);
	DBG_logF("Interrupt stack @:                           %08" PRIuPTR
		 "\n", (uintptr_t) istack);
	TIMING_reportPlatformSpecific();
	/* Non-perfect OK, may be differences in compiler code for different stores */
	DBG_logF("Straight test overhead (cycles):           %7" PRIu32
		 ".%02" PRIu32 " %s\n", (uint32_t) (so / 1000),
		 (uint32_t) ((so % 1000) / 10),
		 so == 0 ? "(perfect TMR compensation)" : "");
	DBG_logF("X-task test overhead (cycles):             %7" PRIu32
		 ".%02" PRIu32 "\n\n", (uint32_t) (xo / 1000),
		 (uint32_t) ((xo % 1000) / 10));
	DBG_logF("                                            Cnfdnc\n");
	DBG_logF
	    ("AverageCyc     MinCyc     MaxCyc   Avg.Dev. Avg Min Functionality\n");
	DBG_logF
	    ("---------- ---------- ---------- ---------- --- --- -------------\n");
	DQ_init(&queue);
	/* Deferred initialisation */
#ifdef CONFIG_CTX_RESTARTABLE
	printResult(startup, 1, CONFIG_TEST_TIMING_N, "Initialisation", so);
#else
	printResult(startup, 0, 0, "Initialisation", so);
#endif
	/* Create */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_startCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE,
			      LOW, (void *)((uintptr_t) i), "Task test task");
		result[i] = TMR_stopCycleCount();
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Create task", so);
	/* Remove */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE,
			      LOW, (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		KRN_wake(&queue);
		TMR_startCycleCount();
		KRN_removeTask(&thread);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Remove (1 other, low priority, runnable)", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE,
			      LOW, (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		TMR_startCycleCount();
		KRN_removeTask(&thread);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Remove (1 other, hibernating on waitq)", so);
	/* Release */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_release();
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Release (no other)", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE, LOW,
			      (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		TMR_startCycleCount();
		KRN_release();
		result[i] = TMR_stopCycleCount();
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Release (1 other, lower priority, hibernating on waitq)",
		    so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE,
			      LOW, (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		KRN_wake(&queue);
		TMR_startCycleCount();
		KRN_release();
		result[i] = TMR_stopCycleCount();
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Release (1 other, lower priority)", so);
	/* Wake */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE,
			      LOW, (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		KRN_wake(&queue);
		TMR_startCycleCount();
		KRN_wake(&queue);
		result[i] = TMR_stopCycleCount();
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Wake (empty waitq)", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE, LOW,
			      (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		KRN_priority(NULL, HIGHEST);
		TMR_startCycleCount();
		KRN_wake(&queue);
		result[i] = TMR_stopCycleCount();
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Wake (1 other, lower priority, hibernating on waitq)", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_startTask(taskOther, &thread, workspace, STACKSIZE, LOW,
			      (void *)((uintptr_t) i), "Task test task");
		KRN_priority(NULL, LOWEST);
		KRN_release();
		TMR_startCycleCount();
		KRN_wake(&queue);
		result[i] = TMR_stopCycleCount();
		KRN_priority(NULL, HIGHEST);
		KRN_removeTask(&thread);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Wake (1 other, higher priority, hibernating on waitq)",
		    so);
	/* Semaphores */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_initSemaphore(&semaphore, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Semaphore init", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initSemaphore(&semaphore, 0);
		TMR_startCycleCount();
		KRN_setSemaphore(&semaphore, 1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Semaphore set", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initSemaphore(&semaphore, 0);
		TMR_startCycleCount();
		KRN_testSemaphore(&semaphore, 1, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Semaphore(0) test non-block", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initSemaphore(&semaphore, 0);
		KRN_setSemaphore(&semaphore, 1);
		TMR_startCycleCount();
		KRN_testSemaphore(&semaphore, 1, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Semaphore(1) test", so);
	/* Locks */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_initLock(&lock);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Lock init", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initLock(&lock);
		TMR_startCycleCount();
		KRN_lock(&lock, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Lock(unlocked) lock", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initLock(&lock);
		KRN_lock(&lock, -1);
		TMR_startCycleCount();
		KRN_unlock(&lock);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Lock(locked) unlock", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initLock(&lock);
		KRN_lock(&lock, -1);
		KRN_startTask(lockOther, &thread, workspace, STACKSIZE,
			      HIGHEST, (void *)((uintptr_t) i),
			      "Lock other task");
		KRN_testSemaphore(&semaphore, 1, -1);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Lock(locked) lock non-blocking", so);
	/* Mailboxes */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_initMbox(&mbox);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Mailbox init", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		TMR_startCycleCount();
		KRN_putMbox(&mbox, &listable);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Mailbox put 1st", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		KRN_putMbox(&mbox, &listable);
		TMR_startCycleCount();
		KRN_putMbox(&mbox, &listable2);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Mailbox put 2nd", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		KRN_putMbox(&mbox, &listable);
		KRN_putMbox(&mbox, &listable2);
		TMR_startCycleCount();
		KRN_getMbox(&mbox, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Mailbox get 1st", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		KRN_putMbox(&mbox, &listable);
		KRN_putMbox(&mbox, &listable2);
		KRN_getMbox(&mbox, -1);
		TMR_startCycleCount();
		KRN_getMbox(&mbox, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Mailbox get 2nd", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		TMR_startCycleCount();
		KRN_getMbox(&mbox, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Mailbox(empty) get non-block", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initMbox(&mbox);
		KRN_putMbox(&mbox, &listable);
		TMR_startCycleCount();
		KRN_getMbox(&mbox, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Mailbox(!empty) get non-block", so);
	/* Flags */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_initFlags(&flags);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Flags init", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_setFlags(&flags, 1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Flag set", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initFlags(&flags);
		KRN_setFlags(&flags, 1);
		TMR_startCycleCount();
		KRN_clearFlags(&flags, 1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Flag clear", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_toggleFlags(&flags, 1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Flag toggle", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initFlags(&flags);
		KRN_setFlags(&flags, 1);
		TMR_startCycleCount();
		KRN_testFlags(&flags, 1, KRN_ANY, 1, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Flag(available) test",
		    so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initFlags(&flags);
		KRN_clearFlags(&flags, 1);
		TMR_startCycleCount();
		KRN_testFlags(&flags, 1, KRN_ANY, 1, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Flag(taken) test non-block", so);
	/* Pools */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Pool init", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		KRN_takePool(&pool, -1);
		TMR_startCycleCount();
		KRN_emptyPool(&pool);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Pool(empty) empty", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		TMR_startCycleCount();
		KRN_emptyPool(&pool);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Pool(!empty) empty", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		KRN_takePool(&pool, -1);
		TMR_startCycleCount();
		KRN_takePool(&pool, 0);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Pool(empty) take non-block", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		TMR_startCycleCount();
		KRN_takePool(&pool, -1);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Pool(!empty) take", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));
		KRN_takePool(&pool, -1);
		TMR_startCycleCount();
		KRN_returnPool(&poolbuffer);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Pool return", so);
	/* Timers */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		KRN_setTimer(&timer, timerOther, NULL, 1000);
		result[i] = TMR_stopCycleCount();
		KRN_cancelTimer(&timer);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Timer set", so);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		KRN_setTimer(&timer, timerOther, NULL, 1000);
		TMR_startCycleCount();
		KRN_cancelTimer(&timer);
		result[i] = TMR_stopCycleCount();
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Timer cancel", so);
	/* X-task */
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		semaphoreXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Semaphore set test-blocked x-task", xo);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		lockXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Lock unlock lock-blocked x-task", xo);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		mboxXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Mailbox put get-blocked x-task", xo);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		flagsXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Flags set test-blocked x-task", xo);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		poolXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N,
		    "Pool return take-blocked x-task", xo);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		taskXTask(i);
	}

	printResult(result, 1, CONFIG_TEST_TIMING_N, "Thread switch (none)",
		    xo);

	TIMING_platformAdditional(so);

	DBG_logF("Test done\n");
	return 0;
}
