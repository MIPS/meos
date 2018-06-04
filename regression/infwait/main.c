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
*   Description:	Infinite wait test
*
*************************************************************************/

/*
 * This test checks that infinite waits work without a timer task.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static KRN_TASK_T waittask;
static KRN_TASK_T waketask;

static uint32_t waitstack[STACKSIZE];
static uint32_t wakestack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_SEMAPHORE_T waitsem;
static KRN_SEMAPHORE_T wakesem;
static KRN_SEMAPHORE_T mainsem;

int32_t passed = -1;

static void wait_task(void)
{
	DBG_logF("wait_task(): taking wait\n");
	KRN_testSemaphore(&waitsem, 1, KRN_INFWAIT);
	DBG_logF("wait_task(): got wait, giving main\n");
	passed = 0;
	KRN_setSemaphore(&mainsem, 1);
	KRN_removeTask(NULL);
}

/* When we get scheduled, grab the counters, then notify that the test is main
 */
static void wake_task(void)
{
	DBG_logF("wake_task(): taking wake\n");
	KRN_testSemaphore(&wakesem, 1, KRN_INFWAIT);
	DBG_logF("wake_task(): giving wait\n");
	KRN_setSemaphore(&waitsem, 1);
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
int main()
{
	DBG_logF("Infinite wait with no timer task Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	bgtask = KRN_startOS("Background Task");
	BSP_init();
	KRN_priority(NULL, KRN_LOWEST_PRIORITY);

	KRN_initSemaphore(&waitsem, 0);
	KRN_initSemaphore(&wakesem, 0);
	KRN_initSemaphore(&mainsem, 0);

	DBG_logF("main(): starting wait task\n");
	KRN_startTask(wait_task, &waittask, waitstack, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Wait task");

	DBG_logF("main(): starting wake task\n");
	KRN_startTask(wake_task, &waketask, wakestack, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Wake task");

	DBG_logF("main(): giving wake\n");
	KRN_setSemaphore(&wakesem, 1);

	DBG_logF("main(): taking main\n");
	KRN_testSemaphore(&mainsem, 1, KRN_INFWAIT);

	return passed;
}
