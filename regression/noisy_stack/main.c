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
*   Description:	Noisy stack test
*
*************************************************************************/

#include <stdlib.h>		/* for exit! */
#include <string.h>		/* for exit! */

#include <alloca.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 1024
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define debug	DBG_logF

int main2(void);

static int32_t errors = 0;

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T sleepQueue;
static uint32_t timestack[TSTACKSIZE];
static uint32_t Task0Stack[STACKSIZE];
static uint32_t Task1Stack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_LOCK_T lock;

void notify()
{
	errors++;
	DBG_logF("Test failed\n");
	assert(0);
}

static void Task0_func(void)
{
	debug("set up task0\n");

	/* Drop us below the other task */
	KRN_priority(NULL, PRIORITIES - 3);
	KRN_release();
	debug("check task0\n");

	debug("done task0\n");
	KRN_removeTask(NULL);
	return;
}

static void Task1_func(void)
{
	debug("set up task1\n");

	/* Drop us below the other task */
	KRN_priority(NULL, PRIORITIES - 4);
	KRN_release();
	debug("check task1\n");

	debug("done task1\n");

	KRN_unlock(&lock);
	KRN_removeTask(NULL);
	return;
}

int main()
{
	uint8_t *cp = (uint8_t *) alloca(1024);
	int32_t i;

	/* Wipe the stack with garbage */
	for (i = 0; i < 1024; i++) {
		cp[i] = 0xa5;
	}

	return main2();
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
**
*/
int main2(void)
{
	KRN_TASK_T Task0;
	KRN_TASK_T Task1;

	DBG_logF("Kernel noisy stack Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	DQ_init(&sleepQueue);

	KRN_startOS("Background Task");

	/* Acquire the lock */
	KRN_initLock(&lock);
	KRN_lock(&lock, 0);

	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	BSP_init();

	/* Make sure we complete this first */
	KRN_priority(NULL, PRIORITIES - 1);

	/* Deliberately corrupt the task structs before we begin */
	memset((void *)&Task0, 0xa7, sizeof(Task0));
	memset((void *)&Task0Stack, 0xa7, STACKSIZE);
	memset((void *)&Task1, 0xa7, sizeof(Task1));
	memset((void *)&Task1Stack, 0xa7, STACKSIZE);

	KRN_startTask(Task0_func, &Task0, Task0Stack, STACKSIZE,
		      PRIORITIES - 1, NULL, "Task0");
	KRN_startTask(Task1_func, &Task1, Task1Stack, STACKSIZE,
		      PRIORITIES - 2, NULL, "Task1");

	/* Drop us down, and wait for the lock to be released (signalling
	 * completion. */
	KRN_priority(NULL, 0);
	KRN_lock(&lock, KRN_INFWAIT);

	debug("Done\n");

	return errors;
}
