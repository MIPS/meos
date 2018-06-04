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
*   Description:	Lazy context saving test
*
*************************************************************************/

/*
 * This test checks lazy context saves work.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define CONFIG_DEBUG_DIAGNOSTICS y

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

#ifdef CONFIG_ARCH_MIPS_DSP
typedef int64_t a64;
typedef short v2q15 __attribute__ ((vector_size(4)));

a64 __builtin_mips_maq_sa_w_phl(a64, v2q15, v2q15);
#endif

void test_func(void)
{
	register volatile float f0, f1, f2, f3, f4, f5;
#ifdef CONFIG_ARCH_MIPS_DSP
	register volatile v2q15 a, b, c, d;
	register volatile a64 ac0, ac1, ac2 = 0, ac3 = 0;
#endif
	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		f0 = 0.5f;
		f1 = 0.25f;
		f2 = 0.125f;
		f3 = 0.0625f;
		f4 = 0.03125f;
		f5 = 0.015625f;
		DBG_logF("f0: %f\tf1: %f\t f2: %f\tf3: %f\tf4: %f\tf5: %f\n",
			 (double)f0, (double)f1, (double)f2, (double)f3,
			 (double)f4, (double)f5);
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		f0 = 0.25f;
		f1 = 0.125f;
		f2 = 0.0625f;
		f3 = 0.03125f;
		f4 = 0.015625f;
		f5 = 0.0078125f;
		DBG_logF("f0: %f\tf1: %f\t f2: %f\tf3: %f\tf4: %f\tf5: %f\n",
			 (double)f0, (double)f1, (double)f2, (double)f3,
			 (double)f4, (double)f5);
	}

	KRN_sync(&sync, KRN_INFWAIT);

	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		DBG_assert((f0 == 0.5f) &&
			   (f1 == 0.25f) &&
			   (f2 == 0.125f) &&
			   (f3 == 0.0625f) &&
			   (f4 == 0.03125f) &&
			   (f5 == 0.015625f), "Floats in oneTask wrong");
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		DBG_assert((f0 == 0.25f) &&
			   (f1 == 0.125f) &&
			   (f2 == 0.0625f) &&
			   (f3 == 0.03125f) &&
			   (f4 == 0.015625f) &&
			   (f5 == 0.0078125f), "Floats in twoTask wrong");
	}
#ifdef CONFIG_ARCH_MIPS_DSP
	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		a = (v2q15) {
		(short)0xac1e, (short)0x0b57};
		b = (v2q15) {
		(short)0x7a1e, (short)0x7e11};
		c = (v2q15) {
		(short)0xc7ed, (short)0xde7e};
		d = (v2q15) {
		(short)0xf007, (short)0xf1a7};
		DBG_logF("1b a: %08" PRIx32 "\tb: %08" PRIx32 "\tc: %08" PRIx32
			 "\td: %08" PRIx32 "\t", (uint32_t) a, (uint32_t) b,
			 (uint32_t) c, (uint32_t) d);
		ac0 = (a64) 0xba5e1e55b0a710ad;
		ac1 = (a64) 0xbea7ab1ecab00d1e;
		ac2 = __builtin_mips_maq_sa_w_phl(ac0, a, b);
		ac3 = __builtin_mips_maq_sa_w_phl(ac1, c, d);
		DBG_logF
		    ("1b ac0: %016" PRIx64 "\tac1: %016" PRIx64 "\tac2: %016"
		     PRIx64 "\t:ac3: %016" PRIx64 "\n", ac0, ac1, ac2, ac3);

	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		a = (v2q15) {
		(short)0x11ed, (short)0x1abe};
		b = (v2q15) {
		(short)0xfa57, (short)0x57ed};
		c = (v2q15) {
		(short)0xca57, (short)0x7e1e};
		d = (v2q15) {
		(short)0x55ed, (short)0xacce};
		DBG_logF("2b a: %08" PRIx32 "\tb: %08" PRIx32 "\tc: %08" PRIx32
			 "\td: %08" PRIx32 "\t", (uint32_t) a, (uint32_t) b,
			 (uint32_t) c, (uint32_t) d);
		ac0 = (a64) 0x5ca1ab1e5caff01d;
		ac1 = (a64) 0x0b501e7ea5be5705;
		ac2 = __builtin_mips_maq_sa_w_phl(ac0, a, b);
		ac3 = __builtin_mips_maq_sa_w_phl(ac1, c, d);
		DBG_logF
		    ("2b ac0: %016" PRIx64 "\tac1: %016" PRIx64 "\tac2: %016"
		     PRIx64 "\t:ac3: %016" PRIx64 "\n", ac0, ac1, ac2, ac3);
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &oneTask) {
		DBG_logF("1a a: %08" PRIx32 "\tb: %08" PRIx32 "\tc: %08" PRIx32
			 "\td: %08" PRIx32 "\t", (uint32_t) a, (uint32_t) b,
			 (uint32_t) c, (uint32_t) d);
		DBG_logF
		    ("1a ac0: %016" PRIx64 "\tac1: %016" PRIx64 "\tac2: %016"
		     PRIx64 "\t:ac3: %016" PRIx64 "\n", ac0, ac1, ac2, ac3);
		DBG_assert(((uint32_t) a == 0x0b57ac1e) &&
			   ((uint32_t) b == 0x7e117a1e) &&
			   ((uint32_t) c == 0xde7ec7ed) &&
			   ((uint32_t) d == 0xf1a7f007),
			   "Vectors in oneTask wrong");
		DBG_assert((ac0 == (a64) 0xba5e1e55b0a710ad)
			   && (ac1 == (a64) 0xbea7ab1ecab00d1e)
			   && (ac2 == (a64) 0xba5e1e55bbd2363b)
			   && (ac3 == (a64) 0xbea7ab1ece719182),
			   "Accumulators in oneTask wrong");
	}

	if (KRN_sync(&sync, KRN_INFWAIT) == &twoTask) {
		DBG_logF("2a a: %08" PRIx32 "\tb: %08" PRIx32 "\tc: %08" PRIx32
			 "\td: %08" PRIx32 "\t", (uint32_t) a, (uint32_t) b,
			 (uint32_t) c, (uint32_t) d);
		DBG_logF
		    ("2a ac0: %016" PRIx64 "\tac1: %016" PRIx64 "\tac2: %016"
		     PRIx64 "\t:ac3: %016" PRIx64 "\n", ac0, ac1, ac2, ac3);
		DBG_assert(((uint32_t) a == 0x1abe11ed) &&
			   ((uint32_t) b == 0x57edfa57) &&
			   ((uint32_t) c == 0x7e1eca57) &&
			   ((uint32_t) d == 0xacce55ed),
			   "Vectors in twoTask wrong");
		DBG_assert((ac0 == (a64) 0x5ca1ab1e5caff01d)
			   && (ac1 == (a64) 0x0b501e7ea5be5705)
			   && (ac2 == (a64) 0x5ca1ab1e6f0e97e9)
			   && (ac3 == (a64) 0xffffffff80000000),
			   "Accumulators in twoTask wrong");
	}
#endif

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
	DBG_logF("Lazy context save Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	BSP_init();

	KRN_initSync(&sync, 3);
	KRN_startTask(test_func, &oneTask, oneStack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "1");
	KRN_startTask(test_func, &twoTask, twoStack, STACKSIZE,
		      KRN_LOWEST_PRIORITY, NULL, "2");

	test_func();

	return passed;
}
