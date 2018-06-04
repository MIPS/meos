/***(C)2012***************************************************************
*
* Copyright (C) 2012 MIPS Tech, LLC
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
****(C)2012**************************************************************/

/*************************************************************************
*
*   Description:	Stack usage test
*
*************************************************************************/

/*
 * This test checks stack usage and sizes for itself
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static uint32_t timestack[STACKSIZE];
static uint32_t fgstack[STACKSIZE];
static uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T queue;

int32_t passed = 0;		/* Zero success, non-zero fail */

static void test_func(void)
{
	for (;;)
		KRN_release();
}

static void fill(void)
{
	uint32_t i;
	uint32_t buffer[256];
	for (i = 0; i < 256; i++)
		buffer[i] = 0x73117a1e;
	bgtask->stackStart = buffer;
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main(int argc, char *argv[])
{
	KRN_TASK_T *timetask;
	KRN_TASK_T fgtask;
	int32_t total, free, used;

	DBG_logF("Resource usage Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0x73117a1e, istack,
		  STACKSIZE, NULL, 0);
	DQ_init(&queue);

	bgtask = KRN_startOS("Background Task");
	if (!bgtask->stackEnd) {
		bgtask->stackEnd = (uint32_t *) & argc;
		fill();
	}
	timetask = KRN_startTimerTask("Timer Task", (uint32_t *) timestack,
				      TSTACKSIZE);
	BSP_init();

	KRN_startTask(test_func, &fgtask, (uint32_t *) fgstack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "fg");

	KRN_hibernate(&queue, 10);

	DBG_logF("Task:\tTotal:\tFree:\tUsed:\n");
	KRN_stackInfo(timetask, &total, &free, &used);
	DBG_logF("Timer\t%" PRIu32 "\t%" PRIu32 "\t%" PRIu32 "\n", total, free,
		 used);
	KRN_stackInfo(&fgtask, &total, &free, &used);
	DBG_logF("FG\t%" PRIu32 "\t%" PRIu32 "\t%" PRIu32 "\n", total, free,
		 used);
	KRN_stackInfo(bgtask, &total, &free, &used);
	DBG_logF("BG\t%" PRIu32 "\t%" PRIu32 "\t%" PRIu32 "\n", total, free,
		 used);

	KRN_hibernate(&queue, 10);	/* Allow time for FDC to drain */

	return passed;
}
