/***(C)2009***************************************************************
*
* Copyright (C) 2009 MIPS Tech, LLC
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
****(C)2009**************************************************************/

/*************************************************************************
*
*   Description:	ISR scheduling test
*
*************************************************************************/

#include <stdlib.h>		/* for exit! */
#include <string.h>		/* for exit! */

/* we user assert to report failures.... */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#include "MEOS.h"

#define TSTACKSIZE 4000		/* MEOS timer task stack size */
#define STACKSIZE 4000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define debug	DBG_logF

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_SYNC_T sync;
static uint32_t timestack[STACKSIZE];
static uint32_t Task0Stack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SEMAPHORE_T sem;
static KRN_TIMER_T timer;
static IRQ_DESC_T irq;

typedef void VOIDFUNC_T(void);

static void isr(int32_t sigNum)
{
	IRQ_ack(IRQ_cause(sigNum));
	debug("Set the semaphore from an ISR\n");
	KRN_setSemaphore(&sem, 1);
}

void tfunc(KRN_TIMER_T * timer)
{
	IRQ_synthesize(&irq);
}

static void waiter_func(void)
{
	debug("set up waiter_func\n");

	debug("waiting for sem...\n");
	KRN_testSemaphore(&sem, 1, KRN_INFWAIT);

	debug("done waiter_func\n");
	KRN_sync(&sync, KRN_INFWAIT);
	KRN_removeTask(NULL);
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
**
*/
int main(void)
{
	KRN_TASK_T waiter_task;

	DBG_logF("isr scheduler Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	KRN_initSync(&sync, 2);

	KRN_initSemaphore(&sem, 0);

	KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	BSP_init();

	IRQ_soft(0, &irq);
	irq.isrFunc = isr;
	IRQ_route(&irq);

	/* Start tasks, get waiter to run first */
	KRN_startTask(waiter_func, &waiter_task, Task0Stack, STACKSIZE,
		      MAX_PRIORITY - 1, NULL, "waiter");
	KRN_setTimer(&timer, (KRN_TIMERFUNC_T *) tfunc, NULL, 10000);

	KRN_sync(&sync, KRN_INFWAIT);
	debug("Done\n");

	return 0;
}
