/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
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
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	MEOS-VZ hello demo
*
*************************************************************************/

/*
 * This provides a guest for the MVZ_static demo
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

static KRN_TASK_T oneTask, twoTask;
static uint32_t oneStack[STACKSIZE], twoStack[STACKSIZE];

void hello()
{
	const char *name = KRN_taskName(NULL);
	KRN_TASKQ_T q;
	DQ_init(&q);

	for (;;) {
		DBG_logF("Hello from MEOS thread %s!\n", name);
		KRN_hibernate(&q, 1000000);
	}
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
	KRN_TASKQ_T q;
	DBG_logF("Hello Demo\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	/* Initialise board - this configures middleware and device drivers */
	BSP_init();

	KRN_startTask((KRN_TASKFUNC_T *) hello,
		      &oneTask,
		      oneStack, STACKSIZE, KRN_LOWEST_PRIORITY, NULL, "1");
	KRN_startTask((KRN_TASKFUNC_T *) hello,
		      &twoTask,
		      twoStack, STACKSIZE, KRN_LOWEST_PRIORITY, NULL, "2");

	DQ_init(&q);
	KRN_hibernate(&q, -1);

	return 0;
}
