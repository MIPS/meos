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
*   Description:	Timer task and ISR
*
*************************************************************************/

#include "meos/kernel/krn.h"
#include "meos/tmr/tmr.h"

#undef KRN_startTimerTask
KRN_TASK_T *KRN_startTimerTask(const char *taskName, uint32_t * ttStack,
			       int32_t ttStackSize);

/* an undocumented way for external performance analysis tools
* to splice arbitrary functions into the timer interrupt handler
*/
typedef void VOIDFUNC_T(void);

#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))

/*
** FUNCTION:	_KRN_setExpiry
**
** DESCRIPTION:	Set the appropriate timer interrupt expiry time.
**
** RETURNS:	void
*/
static inline void _KRN_setExpiryCore()
{
	int32_t delta = INT32_MAX / 2;
	uint32_t now = TMR_getMonotonic();
	uint32_t expire = now + delta;
	uint32_t timerExpire = _KRN_schedule->timerExpire;
	int32_t timerDelta = timerExpire - now;
	/* Timer? */
	if (timerDelta < delta) {
		delta = timerDelta;
		expire = timerExpire;
	}
	/* Timeslice? */
	if (_KRN_schedule->timeSlice > 0) {
		uint32_t sliceExpire = _KRN_schedule->sliceExpire;
		int32_t sliceDelta = sliceExpire - now;
		if (sliceDelta < delta) {
			delta = sliceDelta;
			expire = sliceExpire;
		}
	}
	TMR_set(expire);
}

void _KRN_setExpiry()
{
	_KRN_setExpiryCore();
}

/*
** FUNCTION:	KRN_timerISR
**
** DESCRIPTION:	Timer tick interrupt handler. executes with interrupts disabled
**
** RETURNS:	void
*/
void _KRN_timerISR(int32_t sigNum)
{
	uint64_t then, now, next;
	KRN_TIMER_T *t;
	DQ_T *timerQ = &_KRN_schedule->timers;
	int32_t schedule = 0;
#ifdef CONFIG_DEBUG_PROFILING
	KRN_RAWSTATS_T s1, s2;

	if (_ThreadStatsArray && _ThreadStatsCtrl)	/* performance monitoring installed and switched on */
		_KRN_grabStats(&s1);
#endif
	IRQ_ack(IRQ_cause(sigNum));

	/* timer processing */
	then = _KRN_schedule->lastTimer;
	now = TMR_getMonotonic();
	if (now < then)
		now |= 0x100000000;
	next = then;
	t = (KRN_TIMER_T *) DQ_first(timerQ);
	while ((t) && ((int32_t) now - (int32_t) (next += t->deltaJiffy) > 0)) {
		/* complete the timer by delivering it to the timer task */
		DQ_remove(t);
		t->active = 0;
		KRN_putMbox(&_KRN_schedule->timerMailbox, t);
		/* Try next timer */
		t = (KRN_TIMER_T *) DQ_first(timerQ);
	}
	if (t) {
		t->deltaJiffy = (next - now);
		_KRN_schedule->timerExpire = next;
	} else {
		_KRN_schedule->timerExpire = now + INT32_MAX / 2;
	}

	_KRN_schedule->lastTimer = now;

	/* timeslice processing */
	if ((_KRN_schedule->timeSlice > 0) && (now > _KRN_schedule->sliceExpire)) {	/* negative value disables time-slicing */
		schedule = 1;
		_KRN_schedule->sliceExpire = now + INT32_MAX;
		if (_KRN_current)
			_KRN_current->reason = KRN_TIMESLICE;
	}

	_KRN_setExpiryCore();

	/* scheduler interrupt needed if either
	 * 1) our processing has done something significant
	 * 2) the scheduler had no runnable task - in such cases we need to re-enter the
	 *      scheduler in order to sleep
	 */
	if (schedule || (_KRN_current == NULL))
		KRN_scheduleProtected();
#ifdef CONFIG_DEBUG_PROFILING
	if (_ThreadStatsArray && _ThreadStatsCtrl) {
		/* OS Profiling installed and switched on */
		KRN_SCHEDULE_T *s = _KRN_schedule;
		KRN_STATS_T *sp = s->tIntStats;
		_KRN_grabStats(&s2);
		_KRN_deltaStats(&s2, &s2, &s1);
		_KRN_accStats(sp, &s2, 0);
		sp->runCount++;
		/* time attributed to timer processing is removed from the current task
		 * (although interrupt overheads will remain with the interrupted task
		 */
		sp = _KRN_current ? _KRN_current->statsPtr : s->nullStats;
		_KRN_accStats(sp, &s2, 1);
	}
#endif
}

/*
** FUNCTION:	KRN_timerTask
**
** DESCRIPTION:	Timer processing task function
**
** RETURNS:	void
*/
static void KRN_timerTask(void)
{
	KRN_TIMER_T *t;
	KRN_MAILBOX_T *m;
	m = &_KRN_schedule->timerMailbox;
	for (;;) {
		t = (KRN_TIMER_T *) KRN_getMbox(m, KRN_INFWAIT);
		t->tfunc(t, t->tpar);
	}
}

/*
** FUNCTION:	KRN_startTimerTask
**
** DESCRIPTION:	Start the timer processing task
**
** RETURNS:	void
*/
KRN_TASK_T *KRN_startTimerTask(const char *taskName,
			       uint32_t * ttStack, int32_t ttStackSize)
{
	KRN_TASK_T *tcb;
	DBG_assert(!IRQ_servicing(),
		   "Can not start timer task from interrupt context");
	/* Start the timer task */
	tcb = &_KRN_schedule->timerTCB;
	KRN_startTask(KRN_timerTask, tcb, ttStack, ttStackSize,
		      _KRN_schedule->maxPriority, NULL, taskName);
	/* Register the timer ISR */
	TMR_route(_KRN_timerISR);
	/* If OS Profiling installed, switch it on */
#ifdef CONFIG_DEBUG_PROFILING
	if (_ThreadStatsArray && _ThreadStatsArraySize)
		_ThreadStatsCtrl = 2;
#endif
	/* return the timer task's TCB */
	return tcb;
}
