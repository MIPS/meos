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
*   Description:	Floating point lazy context test
*
*************************************************************************/

/*
 * This test checks MIPS FPU lazy context switch
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int32_t passed = -1;		/* Zero success, non-zero fail */

static KRN_TASK_T oneTask, twoTask;
static uint32_t oneStack[STACKSIZE], twoStack[STACKSIZE];
static KRN_SYNC_T sync;

void test_func(void)
{
	/* This works more by luck than judgement - CodeSourcery toolchain has
	   issues ensuring the sanctity of pinned floating point register vars */
	register float f0 asm("$f10");
	register float f1 asm("$f11");
	register float f2 asm("$f12");
	register float f3 asm("$f13");
	register float f4 asm("$f14");
	register float f5 asm("$f15");
	register float f6 asm("$f16");
	register float f7 asm("$f17");
	register float f8 asm("$f18");
	register float f9 asm("$f19");
	register float f10 asm("$f21");
	register float f11 asm("$f22");
	register float f12 asm("$f23");
	register float f13 asm("$f24");
	int i = 0xabcd0123, j;

	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		f0 = *(float *)&i;
		i += 0x11;
		f1 = *(float *)&i;
		i += 0x11;
		f2 = *(float *)&i;
		i += 0x11;
		f3 = *(float *)&i;
		i += 0x11;
		f4 = *(float *)&i;
		i += 0x11;
		f5 = *(float *)&i;
		i += 0x11;
		f6 = *(float *)&i;
		i += 0x11;
		f7 = *(float *)&i;
		i += 0x11;
		f8 = *(float *)&i;
		i += 0x11;
		f9 = *(float *)&i;
		i += 0x11;
		f10 = *(float *)&i;
		i += 0x11;
		f11 = *(float *)&i;
		i += 0x11;
		f12 = *(float *)&i;
		i += 0x11;
		f13 = *(float *)&i;
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		i = 0x12345678;
		f0 = *(float *)&i;
		i += 0x23;
		f1 = *(float *)&i;
		i += 0x23;
		f2 = *(float *)&i;
		i += 0x23;
		f3 = *(float *)&i;
		i += 0x23;
		f4 = *(float *)&i;
		i += 0x23;
		f5 = *(float *)&i;
		i += 0x23;
		f6 = *(float *)&i;
		i += 0x23;
		f7 = *(float *)&i;
		i += 0x23;
		f8 = *(float *)&i;
		i += 0x23;
		f9 = *(float *)&i;
		i += 0x23;
		f10 = *(float *)&i;
		i += 0x23;
		f11 = *(float *)&i;
		i += 0x23;
		f12 = *(float *)&i;
		i += 0x23;
		f13 = *(float *)&i;
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		DBG_logF("FSR: %08" PRIx32 "\n", fp_csr());
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		memset(&oneTask.savedContext.fpa, 0, sizeof(CTX_FPC));
		oneTask.savedContext.fpa.flags = CTX_FP_TYPE_FP;
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		i = 0xabcd0123;
		DBG_assert(f0 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f1 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f2 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f3 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f4 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f5 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f6 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f7 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f8 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f9 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f10 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f11 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f12 == *(float *)&i, "Float corrupted!\n");
		i += 0x11;
		DBG_assert(f13 == *(float *)&i, "Float corrupted!\n");
		memset(&twoTask.savedContext.fpa, 0, sizeof(CTX_FPC));
		twoTask.savedContext.fpa.flags = CTX_FP_TYPE_FP;
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		i = 0x12345678;
		j = 0xabcd0123;
		DBG_assert((f0 != *(float *)&i)
			   && (f0 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f1 != *(float *)&i)
			   && (f1 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f2 != *(float *)&i)
			   && (f2 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f3 != *(float *)&i)
			   && (f3 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f4 != *(float *)&i)
			   && (f4 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f5 != *(float *)&i)
			   && (f5 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f6 != *(float *)&i)
			   && (f6 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f7 != *(float *)&i)
			   && (f7 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f8 != *(float *)&i)
			   && (f8 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f9 != *(float *)&i)
			   && (f9 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f10 != *(float *)&i)
			   && (f10 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f11 != *(float *)&i)
			   && (f11 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f12 != *(float *)&i)
			   && (f12 != *(float *)&j), "Float corrupted!\n");
		i += 0x23;
		j += 0x11;
		DBG_assert((f13 != *(float *)&i)
			   && (f13 != *(float *)&j), "Float corrupted!\n");
	}

	KRN_sync(&sync, KRN_INFWAIT);

	passed = 0;
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
	DBG_logF("MIPS FPU lazy context switch test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	KRN_initSync(&sync, 3);
	KRN_startTask(test_func, &oneTask, oneStack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "1");
	KRN_startTask(test_func, &twoTask, twoStack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "2");

	test_func();

	return passed;
}
