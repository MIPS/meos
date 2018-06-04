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
*   Description:	KRN_takePool in interrupt test
*
*************************************************************************/

/*
 * This test checks that KRN_takePool works from an interrupt
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
static KRN_TASKQ_T sleepQueue;
static KRN_POOL_T pool;
static KRN_POOLLINK_T poolbuffer;
static IRQ_DESC_T soft;

typedef void VOIDFUNC_T(void);

static void test(int32_t sigNum)
{
	IRQ_ack(IRQ_cause(sigNum));
	KRN_takePool(&pool, 0);
	KRN_returnPool(&poolbuffer);
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
	DBG_logF("Pool take in an interrupt Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);
	BSP_init();

	KRN_initPool(&pool, &poolbuffer, 1, sizeof(KRN_POOLLINK_T));

	IRQ_soft(0, &soft);
	soft.isrFunc = test;
	IRQ_route(&soft);
	IRQ_synthesize(&soft);

	DQ_init(&sleepQueue);
	KRN_hibernate(&sleepQueue, 1000);

	return 0;
}
