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
*   Description:	MEOS coverage test
*
*************************************************************************/

/*
 * This test coverage checks the MEOS API.
 */

#undef NDEBUG

#include "MEOS.h"
#include <string.h>

void lst_inline(void);
void lst(void);
void dq_inline(void);
void dq(void);
void tre(void);
void krn(KRN_TASK_T * bgtask, KRN_TASK_T * timetask);
void dbg();
#ifdef CONFIG_QIO
void qio(void);
#endif

#define NUM_PERFSTATS_SLOTS 48
#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

KRN_STATS_T perfStats[NUM_PERFSTATS_SLOTS];

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];
uint32_t stack1[STACKSIZE];
uint32_t stack2[STACKSIZE];
uint32_t stack3[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T hibernateQ;

#ifdef CONFIG_DEBUG_STACK_CHECKING
void stackInfo_test()
{
	KRN_TASK_T fake;
	int32_t t, f, u;
	DBG_PPL_T ppl;
	(void)t;
	(void)f;
	(void)u;

	memset(&fake, 0, sizeof(KRN_TASK_T));

	ppl = DBG_raisePPL();
	_KRN_current = &fake;
	fake.stackStart = NULL;
	fake.stackEnd = (uint32_t *) NULL + 8;
	DBG_assert(KRN_stackInfo(NULL, &t, &f, &u) == 0,
		   "KRN_stackInfo should fail with NULL stack start");
	fake.stackStart = (uint32_t *) NULL + 8;
	fake.stackEnd = (uint32_t *) NULL + 1;
	DBG_assert(KRN_stackInfo(NULL, &t, &f, &u) == 0,
		   "KRN_stackInfo should fail with NULL stack end");
	fake.stackStart = NULL;
	fake.stackEnd = (uint32_t *) NULL + 1;
	DBG_assert(KRN_stackInfo(NULL, &t, &f, &u) == 0,
		   "KRN_stackInfo should fail with NULL stack extents");
	_KRN_current = NULL;
	DBG_restorePPL(ppl);
}
#endif

typedef void VOIDFUNC_T(void);
KRN_TASK_T *timetask;

KRN_TRACE_T traceBuf[32];

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main()
{
#ifdef CONFIG_DEBUG_STACK_CHECKING
	KRN_TASK_T fake;
	int32_t t, f, u;
#endif

	DBG_logF("MEOS Coverage Test\n");

	/* MEOS initialisation tests */
	DBG_assert(KRN_reset(NULL, NULL, 0, 0, NULL, 0, NULL, 0) == 1,
		   "KRN_reset with NULL scheduler structure should succeed");
#ifdef CONFIG_DEBUG_STACK_CHECKING
	memset(&fake, 0, sizeof(KRN_TASK_T));
	DBG_assert(KRN_stackInfo(&fake, &t, &f, &u) == 0,
		   "KRN_stackInfo() should not succeed with 0 fill word");
#endif
	DBG_assert(KRN_reset
		   (&sched, schedQueues, 0, 0xdeadbeef, istack, STACKSIZE, NULL,
		    0) == 0, "KRN_reset should fail with 0 priority levels");
	DBG_assert(KRN_reset
		   (&sched, schedQueues, CONFIG_FEATURE_MAX_PRIORITIES,
		    0xdeadbeef, istack, STACKSIZE, NULL, 0) == 0,
		   "KRN_reset should fail with CONFIG_FEATURE_MAX_PRIORITIES priority levels");
	DBG_assert(KRN_reset
		   (&sched, schedQueues, MAX_PRIORITY, 0xdeadbeef, istack,
		    STACKSIZE, traceBuf, 32) == 1,
		   "KRN_reset should succeed with valid number of priority levels");

	KRN_installPerfStats(perfStats, NUM_PERFSTATS_SLOTS);

#ifdef CONFIG_DEBUG_STACK_CHECKING
	stackInfo_test();
#endif

	KRN_taskName(NULL);

	bgtask = KRN_startOS("Background Task");

	/* Timer task initialisation tests */
	DBG_assert((timetask =
		    KRN_startTimerTask("Timer Task", timestack,
				       TSTACKSIZE)) != NULL,
		   "Timer task failed to start");

	BSP_init();

	DQ_init(&hibernateQ);

	dbg();
	lst();
	lst_inline();
	dq();
	dq_inline();
	tre();
	krn(bgtask, timetask);
#ifdef CONFIG_QIO
	qio();
#endif
	DBG_assert(TMR_getMonotonic() <= TMR_getMonotonic(),
		   "Monotonic clock went backwards");
	return 0;
}
