/***(C)2001***************************************************************
*
* Copyright (C) 2001 MIPS Tech, LLC
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
****(C)2001**************************************************************/

/*************************************************************************
*
*   Description:	Timer ISRs
*
*************************************************************************/

#include "MEOS.h"

TIMISR_T *_TIMISR;

PARATYPE(TmrI, KRN_TIMERISR_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

void TIMISR_run(KRN_TIMER_T * timer, void *par)
{
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	KRN_TIMERISR_T *t = (KRN_TIMERISR_T *) DQ_first(&_TIMISR->timerISRQ);
	KRN_TIMERISR_T *last = (KRN_TIMERISR_T *) DQ_last(&_TIMISR->timerISRQ);
	if (t)
		do {
			t->timisr();
			t = (KRN_TIMERISR_T *) DQ_next(t);
		} while (t != last);
	IRQ_restoreIPL(ipl);
}

/*
** FUNCTION:      KRN_initTimerISR
**
** DESCRIPTION:   Initialise a timer ISR descriptor.
**
** RETURNS:       void
*/
void KRN_initTimerISR(KRN_TIMERISR_T * isr, KRN_TIMERISRFUNC_T * timerISRFunc)
{
	PARACHECK();
	PARADEL(TmrI, isr);

	DQ_init((DQ_T *) isr);	/* set linkage fields to "safe" values */
	isr->timisr = timerISRFunc;

	PARAADD(TmrI, isr);
	PARACHECK();
}

/*
** FUNCTION:      KRN_activateTimerISR
**
** DESCRIPTION:   Activate a timer ISR.
**
** RETURNS:       void
*/
void KRN_activateTimerISR(KRN_TIMERISR_T * isr)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

	DBG_assert(!IRQ_servicing(),
		   "Can not activate timer ISR from interrupt context");

	oldipl = IRQ_raiseIPL();
	/* adding at the head of the queue allows a timer ISR to activate
	   another without interference */
	if (DQ_first(&_TIMISR->timerISRQ) == NULL)
		KRN_setTimer(&_TIMISR->timer, TIMISR_run, NULL, _TIMISR->rearm);
	DQ_addHead(&_TIMISR->timerISRQ, isr);
	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:      KRN_deactivateTimerISR
**
** DESCRIPTION:   Activate a timer ISR.
**
** RETURNS:       void
*/
void KRN_deactivateTimerISR(KRN_TIMERISR_T * isr)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

	DBG_assert(!IRQ_servicing(),
		   "Can not deactivate timer ISR from interrupt context");

	oldipl = IRQ_raiseIPL();
	DQ_remove(isr);
	if (DQ_first(&_TIMISR->timerISRQ) == NULL)
		KRN_cancelTimer(&_TIMISR->timer);
	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

void TIMISR_init(TIMISR_T * timisr, uint32_t rearm)
{
	_TIMISR = timisr;
	DQ_init(&timisr->timerISRQ);
	timisr->rearm = rearm;
}
