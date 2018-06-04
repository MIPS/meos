/***(C)2014***************************************************************
*
* Copyright (C) 2014 MIPS Tech, LLC
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
****(C)2014**************************************************************/

/*************************************************************************
*
*   Description:	Debug module coverage test
*
*************************************************************************/

#include <string.h>
#include "MEOS.h"

#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
void _DBG_dumpTrace();
#endif

static uint32_t allowDeath = 1;

int32_t _DBG_assert(const char *file, const int line, const char *message, ...)
{
	return allowDeath;
}

static void yield(void) __attribute__ ((noreturn));
static void yield(void)
{
	for (;;)
		KRN_release();
}

static void busy(void) __attribute__ ((noreturn));
static void busy(void)
{
	for (;;) ;
}

#define STACKSIZE 2000
static KRN_TASK_T task;
extern uint32_t stack1[STACKSIZE];

#ifdef __cplusplus
extern "C" {
#endif
	KRN_TRACE_T *DBG_stepTrace(KRN_TRACE_T ** p, uint32_t n);
	void DBG_atrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i,
			uintptr_t * ap);
#ifdef __cplusplus
}
#endif
PARAEXTERN(Dque, DQ_LINKAGE_T);

void dbg()
{
	DBG_PPL_T ppl;
	KRN_TASKQ_T q;

	DBG_info("Information test\n");
	DBG_insist(0, "Warning test\n");
	DBG_insist(1, "Warning test\n");

	DQ_init(&q);
	PARAADD(Dque, &q);
	allowDeath = 0;
	memset(&q, 0, sizeof(DQ_LINKAGE_T));
	PARACHECK();
	DQ_init(&q);
	allowDeath = 1;

	ppl = DBG_raisePPL();
	KRN_startTask(yield, &task, stack1, STACKSIZE, KRN_LOWEST_PRIORITY,
		      NULL, "yield");
	KRN_hibernate(&q, 5000);
#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
	_DBG_dumpTrace();
#endif
	KRN_removeTask(&task);
	KRN_setTimeSlice(1000000);
	KRN_startTask(busy, &task, stack1, STACKSIZE, KRN_LOWEST_PRIORITY, NULL,
		      "busy");
	KRN_hibernate(&q, 3000000);
	KRN_setTimeSlice(-1);
#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
	_DBG_dumpTrace();
#endif
	KRN_removeTask(&task);
	KRN_hibernate(&q, 5000);
#ifdef CONFIG_DEBUG_TRACE_SOFT
	uintptr_t dummy[3];
	KRN_TRACE_T *wp;
	wp = DBG_openTrace(1);
	memset(wp, 0, sizeof(KRN_TRACE_T));
	wp = DBG_openTrace(8);
	DBG_atrace(&wp, 0, 0, dummy);
	DBG_atrace(&wp, 0, 1, dummy);
	wp->event = DBG_TRACE_CONTINUATION1;
	wp->supplemental.p1 = 0x11111111;
	wp->supplemental.p2 = 0x11111112;
	wp->supplemental.p3 = 0x11111113;
	DBG_stepTrace(&wp, 1);
	wp->event = DBG_TRACE_CONTINUATION2;
	wp->supplemental.p1 = 0x22222221;
	wp->supplemental.p2 = 0x22222222;
	wp->supplemental.p3 = 0x22222223;
	DBG_stepTrace(&wp, 1);
	wp->event = DBG_TRACE_CONTINUATION3;
	wp->supplemental.p1 = 0x33333331;
	wp->supplemental.p2 = 0x33333332;
	wp->supplemental.p3 = 0x33333333;
	DBG_stepTrace(&wp, 1);
	wp->event = DBG_TRACE_EXTRA1;
	wp->supplemental.p1 = 0x44444441;
	wp->supplemental.p2 = 0x44444442;
	wp->supplemental.p3 = 0x44444443;
	DBG_stepTrace(&wp, 1);
	wp->event = DBG_TRACE_EXTRA2;
	wp->supplemental.p1 = 0x55555551;
	wp->supplemental.p2 = 0x55555552;
	wp->supplemental.p3 = 0x55555553;
	DBG_stepTrace(&wp, 1);
	wp->event = DBG_TRACE_EXTRA3;
	wp->supplemental.p1 = 0x66666661;
	wp->supplemental.p2 = 0x66666662;
	wp->supplemental.p3 = 0x66666663;
#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
	_DBG_dumpTrace();
#endif
#endif

	DBG_restorePPL(ppl);

	DBG_assert(_DBG_assert(__FILE__, __LINE__, "Test") == 1,
		   "_DBG_assert unexpected aborted termination");
	allowDeath = 0;
	DBG_assert(0, "DBG_assert unexpected termination");
	allowDeath = 1;
}
