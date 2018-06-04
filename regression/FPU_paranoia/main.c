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
*   Description:	FPU paranoia test
*
*************************************************************************/

#include <stdint.h>
#include <stdlib.h>		/* for exit! */

/* we user assert to report failures.... */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000		/* task stack size */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define NUM_LOOPS 3

#define debug	DBG_logF

extern int main_paranoia1(void);
extern int main_paranoia2(void);
static int32_t errors = 0;

static KRN_TASK_T *bgtask;
static KRN_TASK_T *timetask;
static KRN_TASK_T Task0;
static KRN_TASK_T Task1;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T sleepQueue;
static uint32_t timestack[STACKSIZE];
static uint32_t Task0Stack[STACKSIZE];
static uint32_t Task1Stack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SEMAPHORE_T sem;

void error()
{
	errors++;
	DBG_logF("Test failed\n");
}

static void Task0_func(void)
{
	int32_t loop;
	debug("task0\n");

	for (loop = 0; loop < NUM_LOOPS; loop++) {
		if (main_paranoia1())
			error();
		/* Force a switch */
		KRN_release();
	}

	debug("done task0\n");
	KRN_setSemaphore(&sem, 1);
	KRN_removeTask(NULL);
	return;
}

static void Task1_func(void)
{
	int32_t loop;

	debug("task1\n");

	for (loop = 0; loop < NUM_LOOPS; loop++) {
		if (main_paranoia2())
			error();
		/* Force a switch */
		KRN_release();
	}

	debug("done task1\n");
	KRN_setSemaphore(&sem, 1);
	KRN_removeTask(NULL);
	return;
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
**
*/
int main()
{
	DBG_logF("FPU paranoia test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	DQ_init(&sleepQueue);

	/* Acquire the lock */
	KRN_initSemaphore(&sem, 0);
	bgtask = KRN_startOS("Background Task");
	timetask = KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	BSP_init();

	/* Enable timeslicing to try and get maximum entropy in the test */
	KRN_setTimeSlice(100000);

	/* Make sure we complete this first */
	KRN_priority(NULL, PRIORITIES - 1);
	KRN_startTask(Task0_func, &Task0, Task0Stack, STACKSIZE,
		      PRIORITIES - 1, NULL, "Task0");
	KRN_startTask(Task1_func, &Task1, Task1Stack, STACKSIZE,
		      PRIORITIES - 1, NULL, "Task1");

	/* Drop us down, and wait for both threads to complete */
	KRN_priority(NULL, 0);
	KRN_testSemaphore(&sem, 2, KRN_INFWAIT);

	debug("Done\n");

	return errors;
}
