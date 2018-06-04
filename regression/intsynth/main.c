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
*   Description:	Interrupt synthesis test
*
*************************************************************************/

/*
 * This test checks synthesised interrupts are routed correctly
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"


#define TSTACKSIZE 200		/* MEOS timer task stack size */
#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

#define TESTS 1024
int32_t passed = TESTS;		/* Zero success, non-zero fail */


void check(int32_t intNum)
{
	IRQ_DESC_T *desc = IRQ_ack(IRQ_cause(intNum));
	if ((desc->impSpec.extNum & 3) + 2 == intNum)
		passed--;
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
	DBG_logF("Interrupt synthesis Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	BSP_init();

	IRQ_DESC_T descs[CONFIG_TEST_INTSYNTH_MAX - CONFIG_TEST_INTSYNTH_MIN];
	int i, j;

	for (i = CONFIG_TEST_INTSYNTH_MIN; i < CONFIG_TEST_INTSYNTH_MAX; i++) {
		descs[i].intNum = (i & 3) + 2;
		descs[i].impSpec.extNum = i;
		descs[i].isrFunc = check;
		IRQ_route(&descs[i]);
	}

	for (i = 0; i < TESTS; i++) {
		j = (rand() % CONFIG_TEST_INTSYNTH_MAX -
		     CONFIG_TEST_INTSYNTH_MIN) + CONFIG_TEST_INTSYNTH_MIN;
		IRQ_synthesize(&descs[j]);
	}

	return passed;
}
