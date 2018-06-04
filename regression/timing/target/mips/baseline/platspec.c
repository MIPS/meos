/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	MIPS baseline timing test specialisation
*
*************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEOS.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Import asm functions */
	uint32_t testOverhead(void);
	uint32_t testLatency(void);
	uint32_t testReturnOverhead(void);
	uint32_t testReturnLatency(void);
	uint32_t testTotalOverhead(void);
	uint32_t testTotalLatency(void);
	uint32_t testIsrOverhead(void);
	uint32_t testIsrLatency(void);
	uint32_t testIsrReturnOverhead(void);
	uint32_t testIsrReturnLatency(void);
	uint32_t testIsrTotalOverhead(void);
	uint32_t testIsrTotalLatency(void);
	void testIsr(int32_t sigNum);
	void testIsrRet(int32_t sigNum);
	void testIsrTot(int32_t sigNum);
#ifdef __cplusplus
}
#endif
IRQ_DESC_T isrDesc;		/* = {.intNum = 0 }; // Rely on BSS */

int32_t isrStamp;
uint32_t epcStow;

/* Inhibit warning, we know what evil we're perpetrating */
int32_t _DBG_insist(const char *file, const int line, const char *message, ...)
{
	(void)file;
	(void)line;
	(void)message;

	return 0;
}

static int64_t intavg = 0, isravg = 0, intretavg =
    0, isrretavg = 0, inttotavg = 0, isrtotavg = 0;

uint32_t iline = 32, dline = 32;

void TIMING_capturePlatformSpecific(void)
{
	int32_t i = 0;

	iline = ((mips32_getconfig0() & CFG0_M)
		 ? (2 <<
		    ((mips32_getconfig1() & CFG1_IL_MASK) >> CFG1_IL_SHIFT))
		 : 32);
	dline = ((mips32_getconfig0() & CFG0_M)
		 ? (2 <<
		    ((mips32_getconfig1() & CFG1_DL_MASK) >> CFG1_DL_SHIFT))
		 : 32);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testLatency();
		o = testOverhead();
		/* } while (l < o); */
		if (i)
			intavg += (l - o) * 1000;
		/*DBG_logF
		   ("Interrupt latency enter: (%" PRIu32 " - %" PRIu32
		   ") %" PRIu32 " cycles\n", l, o, l - o); */
	}
	intavg /= CONFIG_TEST_TIMING_N;

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testReturnLatency();
		o = testReturnOverhead();
		/* } while (l < o); */
		if (i)
			intretavg += (l - o) * 1000;
		/*DBG_logF
		   ("Interrupt return latency: (%" PRIu32 " - %" PRIu32
		   ") %" PRIu32 " cycles %08x\n", l, o, l - o,
		   _m32c0_mfc0(C0_PERFCNT, 0)); */
	}
	intretavg /= CONFIG_TEST_TIMING_N;

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testTotalLatency();
		o = testTotalOverhead();
		/* } while (l < o); */
		if (i)
			inttotavg += (l - o) * 1000;
		/*DBG_logF
		   ("Interrupt total latency: (%" PRIu32 " - %" PRIu32
		   ") %" PRIu32 " cycles\n", l, o, l - o); */
	}
	inttotavg /= CONFIG_TEST_TIMING_N;

	isrDesc.isrFunc = testIsr;
	IRQ_route(&isrDesc);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testIsrLatency();
		o = testIsrOverhead();
		/* } while (l < o); */
		if (i)
			isravg += (l - o) * 1000;
		/* DBG_logF */
		/*     ("ISR latency: (%" PRIu32 " - %" PRIu32 */
		/*      " ) %" PRIu32 " cycles\n", l, o, l - o); */
	}
	isravg /= CONFIG_TEST_TIMING_N;

	isrDesc.isrFunc = testIsrRet;
	IRQ_route(&isrDesc);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testIsrReturnLatency();
		o = testIsrReturnOverhead();
		/* } while (l < o); */
		if (i)
			isrretavg += (l - o) * 1000;
		/*  DBG_logF */
		/*      ("ISR latency: ((%" PRIu32 " + %" PRIu32 ") - (%" PRIu32 " + %" PRIu32 "))(%" PRIu32 " + %" PRIu32 ") %" PRIu32 " cycles\n", */
		/*       l.idle, l.active, o.idle, o.active, l.idle - o.idle, */
		/*       l.active - o.active, */
		/*       l - o); */
	}
	isrretavg /= CONFIG_TEST_TIMING_N;

	isrDesc.isrFunc = testIsrTot;
	IRQ_route(&isrDesc);
	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		uint32_t l, o;
		/* do { */
		l = testIsrTotalLatency();
		o = testIsrTotalOverhead();
		/* } while (l < o); */
		if (i)
			isrtotavg += (l - o) * 1000;
		/*  DBG_logF */
		/*      ("ISR latency: ((%" PRIu32 " + %" PRIu32 ") - (%" PRIu32 " + %" PRIu32 "))(%" PRIu32 " + %" PRIu32 ") %" PRIu32 " cycles\n", */
		/*       l.idle, l.active, o.idle, o.active, l.idle - o.idle, */
		/*       l.active - o.active, */
		/*       l - o); */
	}
	isrtotavg /= CONFIG_TEST_TIMING_N;

	isrDesc.isrFunc = NULL;
	IRQ_route(&isrDesc);
}

void TIMING_reportPlatformSpecific(void)
{
	DBG_logF("Raw interrupt entry latency (cycles):      %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (intavg / 1000),
		 (uint32_t) ((intavg % 1000) / 10));
	DBG_logF("Raw interrupt exit latency (cycles):       %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (intretavg / 1000),
		 (uint32_t) ((intretavg % 1000) / 10));
	DBG_logF("Raw interrupt total latency (cycles):      %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (inttotavg / 1000),
		 (uint32_t) ((inttotavg % 1000) / 10));
	DBG_logF("Vectored interrupt entry latency (cycles): %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (isravg / 1000),
		 (uint32_t) ((isravg % 1000) / 10));
	DBG_logF("Vectored interrupt exit latency (cycles):  %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (isrretavg / 1000),
		 (uint32_t) ((isrretavg % 1000) / 10));
	DBG_logF("Vectored interrupt total latency (cycles): %7" PRIu32 ".%02"
		 PRIu32 "\n", (uint32_t) (isrtotavg / 1000),
		 (uint32_t) ((isrtotavg % 1000) / 10));
}

#include <mips/hal.h>

struct vzrootctx rootCtx;
struct vzguestctxmax guestCtx;
struct vztlbctxmax guestTlb;
extern uint32_t result[CONFIG_TEST_TIMING_N + 1];
void
printResult(uint32_t * res, int32_t first, int32_t last, const char *title,
	    int64_t overhead);

void TIMING_platformAdditional(int64_t so)
{
#ifdef CONFIG_ARCH_MIPS_VZ
	int32_t i = 0;

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vzrootctx_save(&rootCtx);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ root context save",
		    so);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vzrootctx_load(&rootCtx);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ root context load",
		    so);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vzguestctx_save((struct vzguestctx *)&guestCtx);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ guest context save",
		    so);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vzguestctx_load((struct vzguestctx *)&guestCtx);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ guest context load",
		    so);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vztlbctx_save((struct vztlbctx *)&guestTlb);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ guest TLB save", so);

	for (i = 0; i < CONFIG_TEST_TIMING_N + 1; i++) {
		TMR_resetCycleCount();
		TMR_startCycleCount();
		_vztlbctx_load((struct vztlbctx *)&guestTlb);
		result[i] = TMR_stopCycleCount();
	}
	printResult(result, 1, CONFIG_TEST_TIMING_N, "VZ guest TLB load", so);
#endif
}
