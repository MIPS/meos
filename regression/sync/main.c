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
*   Description:	KRN_sync() test
*
*************************************************************************/

/*
 * This test checks that KRN_sync() works
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
uint32_t other1Stack[STACKSIZE], other2Stack[STACKSIZE], other3Stack[STACKSIZE];
KRN_TASK_T other1, other2, other3;
KRN_SYNC_T sync;

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

int32_t passed = 8;		/* Zero success, non-zero fail */

void test(void)
{
	DBG_logF("%s: Start test\n", KRN_taskName(NULL));
	passed--;
	DBG_logF("%s: Wait Other 1\n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &other1) {
		DBG_logF("%s: Other 1 test\n", KRN_taskName(NULL));
		if (passed == 4)
			passed--;
	}

	DBG_logF("%s: Wait Other 2\n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &other2) {
		DBG_logF("%s: Other 2 test\n", KRN_taskName(NULL));
		if (passed == 3)
			passed--;
	}

	DBG_logF("%s: Wait Other 3\n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &other3) {
		DBG_logF("%s: Other 3 test\n", KRN_taskName(NULL));
		if (passed == 2)
			passed--;
	}

	DBG_logF("%s: Wait BG\n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == bgtask) {
		DBG_logF("%s: BG test\n", KRN_taskName(NULL));
		if (passed == 1)
			passed--;
	}
	DBG_logF("%s: Test complete\n", KRN_taskName(NULL));
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
	DBG_logF("Sync Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);
	BSP_init();

	KRN_initSync(&sync, 4);
	KRN_startTask(test, &other1, other1Stack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, (void *)1, "Other 1");
	KRN_startTask(test, &other2, other2Stack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, (void *)1, "Other 2");
	KRN_startTask(test, &other3, other3Stack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, (void *)1, "Other 3");
	test();

	return passed;
}
