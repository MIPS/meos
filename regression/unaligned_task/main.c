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
*   Description:	Unaligned task test
*
*************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define TSTACKSIZE 4000		/* MEOS timer task stack size */
#define STACKSIZE 4000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T testtask;
static KRN_TASK_T swtask;
static uint32_t timestack[TSTACKSIZE];
static uint32_t teststack[STACKSIZE];
static uint32_t swstack[STACKSIZE];
uint32_t istack[STACKSIZE];
KRN_TASKQ_T hibqm, hibqt;

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

int32_t passed = 16;		/* Zero success, non-zero fail */

static void recurse(uint32_t n) __attribute__ ((noinline));
static void recurse(uint32_t n)
{
	asm("");		/* No optimize away */
	DBG_logF("%" PRIu32 "\n", n);
	if (n)
		recurse(n - 1);
}

static void swfunc(void)
{
	DBG_logF("Switched\n");
	recurse(8);
	return;
}

static void testfunc(void)
{
	DBG_logF("Testing\n");
	KRN_startTask(swfunc, &swtask, swstack, STACKSIZE, PRIORITIES - 1, NULL,
		      "sw");
	recurse(8);
	KRN_hibernate(&hibqt, 10);
	passed--;
	return;
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
	int32_t i;

	DBG_logF("Unaligned task Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);
	BSP_init();

	KRN_priority(NULL, 0);
	DQ_init(&hibqm);
	DQ_init(&hibqt);

	for (i = 0; i < 16; i++) {
		DBG_logF("offset = %" PRIu32 "\n", i);
		KRN_startTask(testfunc, &testtask,
			      (uint32_t *) (((uintptr_t) teststack) +
					    i), STACKSIZE,
			      PRIORITIES - 2, NULL, "test");
		KRN_hibernate(&hibqm, 30);
	}

	return passed;
}
