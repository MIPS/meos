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
*   Description:	MIPS baseline timer specialisation
*
*************************************************************************/

#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/tmr/tmr.h"
#include "meos/inttypes.h"

IRQ_DESC_T _TMR_timer
#ifndef __cplusplus
    = {
	.intNum = IRQ_TIMER
}
#endif
;

uint32_t _TMR_overhead, _TMR_start, _TMR_perus =
    CONFIG_ARCH_MIPS_JIFFIES_PER_US;

#ifdef __cplusplus
extern "C" {
#endif

	extern void _TMR_init_cbegin(void);

#ifdef CONFIG_ARCH_MIPS_PCINT
	uint32_t _TMR_overflow0, _TMR_overflow1;
#ifdef CONFIG_ARCH_MIPS_QPC
	uint32_t _TMR_overflow2, _TMR_overflow3;
#endif
#endif

#ifdef __cplusplus
}
#endif
/*
** FUNCTION:	TMR_resetCycleCount
**
** DESCRIPTION:	Reset cycle counter.
**
** RETURNS:	void
*/ void TMR_resetCycleCount(void) __attribute__ ((optimize("O0")));
void TMR_resetCycleCount(void)
{
	IRQ_init(NULL, 0);
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	_TMR_overhead = 0;
	/* Force first perf counter to count cycles */
	_m32c0_mtc0(C0_PERFCNT, 0, (0 << 5) | 0xf);
	/* Prevent overflow */
	_m32c0_mtc0(C0_PERFCNT, 1, 0);
	/* Ensure calibration code is in cache */
	KRN_preloadCache((void *)&_TMR_init_cbegin, 512, KRN_PRELOAD_FLAG_INST);
	/* Ensure cache load is complete */
	asm("ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop;");
	asm("_TMR_init_cbegin:");
	/* Prime pipeline */
	asm("ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop;");
	/* Measure overhead */
	TMR_startCycleCount();
	_TMR_overhead = TMR_stopCycleCount();
	/* Keep pipeline flowing until we're clear */
	asm("ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop;");
	IRQ_restoreIPL(ipl);
	/* Reset perf counter */
	_m32c0_mtc0(C0_PERFCNT, 1, 0);
}

/*
** FUNCTION:	TMR_init
**
** DESCRIPTION:	Initialise timer system.
**
** RETURNS:	void
*/
void TMR_init(void)
{
	mips_setcount(0xacce55ed);
#ifdef __cplusplus
	_TMR_timer.intNum = IRQ_TIMER;
#endif
#ifdef CONFIG_ARCH_MIPS_PCINT
	_TMR_overflow0 = 0;
	_TMR_overflow1 = 0;
#ifdef CONFIG_ARCH_MIPS_QPC
	_TMR_overflow2 = 0;
	_TMR_overflow3 = 0;
#endif
#endif
}

/*
** FUNCTION:	TMR_route
**
** DESCRIPTION:	Specify a timer interrupt handler. Start or kill a helper
**              thread appropriately to generate the interrupts .
**
** RETURNS:	void
*/ void TMR_route(IRQ_ISRFUNC_T isrFunc)
{
	mips_setcompare(mips_getcount());
	_TMR_timer.isrFunc = isrFunc;
	IRQ_route(&_TMR_timer);
}

/*
** FUNCTION:	TMR_set
**
** DESCRIPTION:	Fire a timer interrupt at a specified number of jiffies.
**
** RETURNS:	void
*/
void TMR_set(uint32_t abstime)
    __attribute__ ((no_instrument_function));
void TMR_set(uint32_t abstime)
{
	/*
	 * If there's a timer interrupt pending, don't do this, otherwise it will
	 * get lost! Rely on the timer rearming itself.
	 */
	if ((mips_getcr() & CR_TI) == 0)
		mips_setcompare(abstime);
	/* Make sure the timer goes off */
	sreg_t now = mips_getcount();
	if (((mips_getcr() & CR_TI) == 0)
	    && (now - (sreg_t) abstime >= 0)) {
		sreg_t add = _TMR_perus;
		do {
			/* Missed */
			abstime = mips_getcount() + add;
			mips_setcompare(abstime);
			add *= 2;
		} while (((mips_getcr() & CR_TI) == 0)
			 && ((sreg_t) mips_getcount() - (sreg_t) abstime >= 0));
	}
}

/*
** FUNCTION:	TMR_setClockSpeed
**
** DESCRIPTION:	Map hardware ticks to real time.
**
** RETURNS:	void
*/
void TMR_setClockSpeed(uint32_t perus)
{
	_TMR_perus = perus;
}

/*
** FUNCTION:	TMR_clockSpeed
**
** DESCRIPTION:	Map real time to hardware ticks.
**
** RETURNS:	void
*/
uint32_t TMR_clockSpeed()
{
	return _TMR_perus;
}
