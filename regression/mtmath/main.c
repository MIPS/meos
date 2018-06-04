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
*   Description:	Time basic operations test
*
*************************************************************************/

#include "MEOS.h"
#include <stdlib.h>

#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)
#define STACKSIZE 2000
#define MAXIMP 32
#define MAXEXP 32

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

uint32_t istack[STACKSIZE];

/* Sample data types */
uint32_t result0[CONFIG_TEST_MTMATH_N * CONFIG_FEATURE_MAX_PROCESSORS + 1];
uint32_t *result = result0;

typedef struct {
	KRN_POOLLINK;
	volatile uint32_t *content;
} POOL_ITEM_T;

static KRN_SYNC_T sync;
static POOL_ITEM_T poolItems[CONFIG_FEATURE_MAX_PROCESSORS];
static KRN_POOL_T pool;
static KRN_MAILBOX_T mb;

static KRN_EXPORT_T expTable[MAXEXP];
static KRN_IMPORT_T impTable[MAXIMP];
static uint8_t maxImps[] = { 0, MAXIMP, 0, 0 };	/* T1 exports */

/*
 * Given sample data representing testing overheads, extract the average
 * so that it may be subtracted from subsequent tests to compensate for said
 * overheads.
 */
int64_t computeOverhead(uint32_t * res, int32_t first, int32_t last)
{
	int32_t i, n = 0;
	int64_t overhead = 0;
	for (i = first; i < last + 1; i++) {
		/* Reject negative results */
		if (res[i] < 0xffff0000) {
			overhead += res[i];
			n++;
		}
	}
	if (n)
		return (overhead * 1000) / n;
	else
		return 0;
}

/* Take sample data, compensate for overheads, compute and display statistics.*/
void
printResult(uint32_t * res, int32_t first, int32_t last, const char *title,
	    int64_t overhead)
{
	int64_t min = 0xffffffff, max = 0, cycles, average = 0, avgdev = 0;
	int32_t confavg = 0, confmin = 0, used = 0, i;

	KRN_flushCache(&res[first], sizeof(res[0]) * (last - first),
		       KRN_FLUSH_FLAG_D);

	for (i = first; i < last + 1; i++) {
		cycles = res[i] * 1000;
		if (cycles < overhead)
			res[i] = 0;
		else {
			res[i] = (uint32_t) cycles - overhead;
			used++;
		}
	}

	for (i = first; i < last + 1; i++) {
		cycles = res[i];
		if (cycles) {
			average += cycles;
			if (cycles < min)
				min = cycles;
			if (cycles > max)
				max = cycles;
		}
	}

	if (used)
		average /= used;

	for (i = first; i < last + 1; i++) {
		cycles = res[i] - average;
		if (cycles < 0)
			cycles = -cycles;
		avgdev += cycles;
	}
	avgdev /= (1 + last - first);

	if (avgdev == 0) {
		confavg = 100;
		confmin = 100;
	} else {
		for (i = first; i < last + 1; i++) {
			cycles = res[i];
			if ((cycles < (average + avgdev))
			    && (cycles >= (average - avgdev)))
				confavg++;
			if ((cycles < (min + avgdev))
			    && (cycles >= (min - avgdev)))
				confmin++;
		}
		confavg = (confavg * 100) / (1 + last - first);
		confmin = (confmin * 100) / (1 + last - first);
	}

	DBG_logF
	    ("%7" PRIu32 ".%02" PRIu32 " %7" PRIu32 ".%02" PRIu32 " %7" PRIu32
	     ".%02" PRIu32 " %7" PRIu32 ".%02" PRIu32 " %3" PRIu32 " %3" PRIu32
	     " %s\n", (uint32_t) (average / 8000),
	     (uint32_t) ((average % 8000) / 10), (uint32_t) (min / 8000),
	     (uint32_t) ((min % 8000) / 10), (uint32_t) (max / 8000),
	     (uint32_t) ((max % 8000) / 10), (uint32_t) (avgdev / 8000),
	     (uint32_t) ((avgdev % 8000) / 10), confavg / 8, confmin / 8,
	     title);
}

/* Define math tests */
#define TEST(TYPE, OPNAME, OP) \
void test ## TYPE ## OPNAME (int32_t i) { \
	register TYPE r, v0, v1, v2, v3, v4, v5, v6, v7;\
	/* Random values to defeat optimisation */ \
	r = (((TYPE)(rand() % 256)) / 16) + 1; \
	v0 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v1 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v2 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v3 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v4 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v5 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v6 = (((TYPE)(rand() % 256)) / 16) + 1; \
	v7 = (((TYPE)(rand() % 256)) / 16) + 1; \
	/* Time performing the op 8 times */ \
	TMR_startCycleCount(); \
	r = r OP v0; \
	r = r OP v1; \
	r = r OP v2; \
	r = r OP v3; \
	r = r OP v4; \
	r = r OP v5; \
	r = r OP v6; \
	r = r OP v7; \
	result[i] = TMR_stopCycleCount(); \
	/* Bleed out result to prevent optimisation */ \
	asm(""::"r"(r)); \
}

/* *INDENT-OFF* */
TEST(reg_t, add, +)
TEST(reg_t, sub, -)
TEST(reg_t, mul, *)
TEST(reg_t, div, /)
TEST(reg_t, mod, %)
TEST(float, add, +)
TEST(float, sub, -)
TEST(float, mul, *)
TEST(float, div, /)
TEST(double, add, +)
TEST(double, sub, -)
TEST(double, mul, *)
TEST(double, div, /)
/* *INDENT-ON* */

#undef TEST

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int test()
{
	int32_t i = 0, ps, pe;
	int64_t so = 0;

	/* Precompute range for this processor */
	ps = KRN_proc() * CONFIG_TEST_MTMATH_N;
	pe = ps + CONFIG_TEST_MTMATH_N + 1;

	/* Simplifying assumption: homogenous overheads */
	KRN_sync(&sync, KRN_INFWAIT);
	if (KRN_proc() == 0) {
		/* Wait for over processors to block */
		KRN_delay(1000);
		TMR_resetCycleCount();
		for (i = 0; i < CONFIG_TEST_MTMATH_N + 1; i++) {
			TMR_startCycleCount();
			result[i] = TMR_stopCycleCount();
		}
		so = computeOverhead(result, 1, CONFIG_TEST_MTMATH_N);
		DBG_logF("Active samples:                            %10u\n",
			 CONFIG_TEST_MTMATH_N);
		DBG_logF("Processors:                           %10" PRIu32
			 "\n", KRN_procs());
		/* Non-perfect OK, may be differences in compiler code for different stores */
		DBG_logF("Test overhead (cycles):           %7" PRIu32
			 ".%02" PRIu32 " %s\n", (uint32_t) (so / 1000),
			 (uint32_t) ((so % 1000) / 10),
			 so == 0 ? "(perfect TMR compensation)" : "");
		DBG_logF
		    ("                                            Cnfdnc\n");
		DBG_logF
		    ("AverageCyc     MinCyc     MaxCyc   Avg.Dev. Avg Min Functionality\n");
		DBG_logF
		    ("---------- ---------- ---------- ---------- --- --- -------------\n");
	}
	KRN_sync(&sync, KRN_INFWAIT);

#define RUN(FUNC, NAME) do {\
	/* Sync */ \
	KRN_sync(&sync, KRN_INFWAIT); \
	/* Clear down */ \
	TMR_resetCycleCount(); \
	/* Prime cache */ \
	testreg_tadd(ps); \
	/* Test */ \
	for (i = ps; i < pe; i++) \
		FUNC(i); \
	/* Buff out first */ \
	result[ps] = 0; \
	/* Sync */ \
	KRN_sync(&sync, KRN_INFWAIT); \
	/* Flush */ \
	KRN_flushCache(&result[ps], sizeof(result[0]) * (pe - ps), \
		KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D); \
	/* Sync */ \
	KRN_sync(&sync, KRN_INFWAIT); \
	/* Report */ \
	if (KRN_proc() == 0) \
		printResult(result, 0, CONFIG_TEST_MTMATH_N * KRN_procs(), NAME, so); \
	/* Sync */ \
	KRN_sync(&sync, KRN_INFWAIT); \
} while (0)

	RUN(testreg_tadd, "reg_t +");
	RUN(testreg_tsub, "reg_t -");
	RUN(testreg_tmul, "reg_t *");
	RUN(testreg_tdiv, "reg_t /");
	RUN(testreg_tmod, "reg_t %");
	RUN(testfloatadd, "float +");
	RUN(testfloatsub, "float -");
	RUN(testfloatmul, "float *");
	RUN(testfloatdiv, "float /");
	RUN(testdoubleadd, "double +");
	RUN(testdoublesub, "double -");
	RUN(testdoublemul, "double *");
	RUN(testdoublediv, "double /");
#undef RUN
	return 0;
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE,
		  NULL, 0);
	KRN_installImpExp(maxImps, MAXEXP, impTable, expTable);
	KRN_startOS("Background Task");

	DBG_logF("%" PRIu32 ": Multi-threaded math performance test\n",
		 KRN_proc());

	BSP_init();

	KRN_initSync(&sync, KRN_procs());
	KRN_initPool(&pool, poolItems, CONFIG_FEATURE_MAX_PROCESSORS,
		     sizeof(POOL_ITEM_T));
	KRN_initMbox(&mb);

	if (KRN_proc() == 1) {
		/* Processor 1 exports, allowing 0 to be Linux */
		KRN_export(1, &sync);
		KRN_export(2, &pool);
		KRN_export(3, &mb);

	} else if (KRN_procs() != 1) {
		KRN_import(1, 1, &sync);
		KRN_import(1, 2, &pool);
		KRN_import(1, 3, &mb);
	}

	if (KRN_procs() > 1) {
		if (KRN_proc() == 1) {
			int32_t i;
			for (i = 0; i < CONFIG_FEATURE_MAX_PROCESSORS; i++) {
				POOL_ITEM_T *packet =
				    (POOL_ITEM_T *) KRN_takePool(&pool,
								 KRN_INFWAIT);
				packet->content = (volatile uint32_t *)&result0;
				KRN_putMbox(&mb, packet);
			}
		} else {
			POOL_ITEM_T *packet =
			    (POOL_ITEM_T *) KRN_getMbox(&mb, KRN_INFWAIT);
			result = (uint32_t *) packet->content;
			KRN_returnPool(packet);
		}
	}

	test();

	return 0;
}
