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

#ifdef CONFIG_ARCH_MIPS_PCINT
#define NUM_LOOPS 4
#define DEN_LOOPS 3
#else
#define NUM_LOOPS 1
#define DEN_LOOPS 3
#endif

#define debug	DBG_logF

#ifdef __cplusplus
extern "C" {
#endif
	extern int paranoia_main(void);
#ifdef __cplusplus
}
#endif
static int32_t errors = 0;

static KRN_TASK_T *bgtask;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T sleepQueue;
static uint32_t timestack[STACKSIZE];
static uint64_t results[DEN_LOOPS][TMR_PERFCOUNTERS];
uint32_t istack[STACKSIZE];

void error()
{
	errors++;
	DBG_logF("Test failed\n");
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
	int32_t loop, i;
	uint64_t r;
	DBG_logF("Perf count demo\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	DQ_init(&sleepQueue);

	/* Acquire the lock */
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	for (i = 0; i < TMR_PERFCOUNTERS; i++) {
		TMR_configPerfCount(i, i);
		TMR_resetPerfCount(i);
	}
	for (loop = 0; loop < DEN_LOOPS; loop++) {
		for (i = 0; i < NUM_LOOPS; i++)
			if (paranoia_main())
				error();
		for (i = 0; i < TMR_PERFCOUNTERS; i++)
			results[loop][i] = TMR_resetPerfCount(i) / NUM_LOOPS;
		/* Force a switch */
		KRN_release();
	}
	for (i = 0; i < TMR_PERFCOUNTERS; i++) {
		r = 0;
		for (loop = 0; loop < NUM_LOOPS; loop++) {
			r += results[loop][i];
		}
		DBG_logF("PC%" PRId32 " average: %" PRIu64 "\n", i, r);
	}
	debug("Done\n");

	return errors;
}
