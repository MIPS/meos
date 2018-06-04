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
*   Description:	Kernel timers
*
*************************************************************************/

#include "meos/config.h"
#include "meos/kernel/krn.h"

PARATYPE(Timr, KRN_TIMER_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

void _KRN_setExpiry(void);

/*
** FUNCTION:      KRN_setSoftTimer
**
** DESCRIPTION:   Set a timer
**
** RETURNS:       void
*/
void KRN_setSoftTimer(KRN_TIMER_T * timer, KRN_TIMERFUNC_T * timerFunction,
		      void *timerPar, int32_t delay, int32_t tolerance)
{
	PARACHECK();
	PARADEL(Timr, timer);

	KRN_TIMER_T *first, *successor;
	void *timerQ;
	IRQ_IPL_T oldipl;
	uint64_t now, then, delayj, dj;
	uint32_t clockspeed;

	timer->active = 0;	/* initially disable KRN_cancelTimeout */

	if (delay < 0) {
		PARACHECK();
		return;		/* infinite wait - do nothing */
	}

	timerQ = &_KRN_schedule->timers;
	timer->tfunc = timerFunction;
	timer->tpar = timerPar;
	oldipl = IRQ_raiseIPL();
	if (delay != 0) {
		/* Timers are queued in expiry time order with the tick count being
		   relative to the previous entry. This extra work at setup time means that the timer
		   tick processing is kept brief. */

		now = TMR_getMonotonic();
		then = _KRN_schedule->lastTimer;
		first = successor = DQ_next(timerQ);
		clockspeed = TMR_clockSpeed();
		delayj = delay * clockspeed;
		dj = (now - then) + delayj;
		tolerance *= clockspeed;

		timer->active = 1;

		while ((successor != timerQ) && (successor->deltaJiffy < dj)) {
			dj -= successor->deltaJiffy;
			successor = (KRN_TIMER_T *) DQ_next(successor);
		}
		timer->deltaJiffy = dj;
		PARAADD(Timr, timer);
		DQ_addBefore(successor, timer);

		if (successor != timerQ) {
			successor->deltaJiffy -= timer->deltaJiffy;
			if (successor->deltaJiffy < tolerance) {
				timer->deltaJiffy += successor->deltaJiffy;
				successor->deltaJiffy = 0;
			}
		}

		if (successor == first) {
			_KRN_schedule->timerExpire = now + delayj;
			_KRN_setExpiry();
		}
	} else {
		PARAADD(Timr, timer);
		KRN_putMbox(&_KRN_schedule->timerMailbox, timer);	/* instant expiry */
	}
	IRQ_restoreIPL(oldipl);
	PARACHECK();
}

/*
** FUNCTION:      KRN_cancelTimer
**
** DESCRIPTION:   Cancel a timer.
**
** RETURNS:       int32_t: 1 -  timer cancelled OK
**                     0 - timer already expired or cancelled
*/
int32_t KRN_cancelTimer(KRN_TIMER_T * timer)
{
	PARACHECK();

	IRQ_IPL_T oldipl;
	KRN_TIMER_T *first;
	KRN_TIMER_T *successor;
	int32_t status = 0;
	void *timerQ;

	timerQ = &_KRN_schedule->timers;
	oldipl = IRQ_raiseIPL();
	if (timer->active) {

		successor = (KRN_TIMER_T *) DQ_next(timer);
		DQ_remove(timer);
		if (successor != timerQ)	/* not end of queue */
			successor->deltaJiffy += timer->deltaJiffy;	/* successor gets unexpired ticks */
		timer->active = 0;
		first = DQ_first(timerQ);
		if (first) {
			_KRN_schedule->timerExpire =
			    _KRN_schedule->lastTimer + first->deltaJiffy;
			_KRN_setExpiry();
		}
		status = 1;
	}
	IRQ_restoreIPL(oldipl);

	PARACHECK();
	return status;
}
