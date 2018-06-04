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
*   Description:	Remove task
*
*************************************************************************/

#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/ctx/ctx.h"

#ifdef CONFIG_DEBUG_PARANOIA
void _DBG_paranoidSweep(uintptr_t start, uintptr_t end);
extern DQ_T _paraItem_Dque;
extern DBG_PARATYPE_T _paraDesc_DqIt, _paraDesc_Dque;
#endif

/*
** FUNCTION:      KRN_removeTask
**
** DESCRIPTION:   Kill another task (or commit suicide if task = NULL)
**
** RETURNS:       void
*/
void KRN_removeTask(KRN_TASK_T * task)
{
	KRN_SCHEDULE_T *s;
	IRQ_IPL_T oldipl;
	KRN_TIMER_T *t;
	DQ_T *timers;

	s = _KRN_schedule;
	if (task == NULL)
		task = _KRN_current;

	/* cancel any pending timeouts for task - otherwise a killed task could be
	 * re-activated.
	 *
	 * I don't much like this code because it involves walking the entire timer queue
	 * with interrupts disabled. However, KRN_setTimer suffers the same problem, so
	 * there isn't much point in being efficient here, but not there, so we live with it.
	 */

	if (task != _KRN_current) {	/* current task can't have a pending timeout */
		/* search the timer queue to see if the task has a pending timeout - if so it must
		 * be cancelled, otherwise the task could be revived after removal ! */
		timers = &(s->timers);
		oldipl = IRQ_raiseIPL();
		t = (KRN_TIMER_T *) DQ_first(timers);
		if (t) {
			while (((DQ_T *) t) != timers) {
				if ((t->tpar == task)
				    && (t->tfunc = _KRN_timeoutWake)) {
					KRN_cancelTimer(t);
					break;	/* only ever one timeout active for a task */
				}
				t = (KRN_TIMER_T *) DQ_next(t);
			}
		}
		IRQ_restoreIPL(oldipl);
	}

	/* release task's statistics buffer slot */
#ifdef CONFIG_DEBUG_PROFILING
	if (task->statsPtr) {
		oldipl = IRQ_raiseIPL();
		task->statsPtr->name = NULL;
		_KRN_zeroStats(task->statsPtr);
		IRQ_restoreIPL(oldipl);
	}
#endif

	/* remove from the list of all tasks (which is only used by Codescape) */
	/* Bit 0 of the queue back pointer is set to 1 if Codescape needs to refresh */
	oldipl = IRQ_raiseIPL();
	void *item = &task->taskLink;
	{
		PARACHECK();
		PARADEL(DqIt, item);

		((DQ_LINKAGE_T *) item)->fwd->back =
		    ((DQ_LINKAGE_T
		      *) (((uintptr_t) (((DQ_LINKAGE_T *) item)->back))
			  & ~1));
		((DQ_LINKAGE_T
		  *) (((uintptr_t) (((DQ_LINKAGE_T *) item)->back)) &
		      ~1))->fwd = ((DQ_LINKAGE_T *) item)->fwd;

		/* make item linkages safe for "orphan" removes */
		((DQ_LINKAGE_T *) item)->fwd = (struct DQ_tag *)item;
		((DQ_LINKAGE_T *) item)->back = (struct DQ_tag *)item;

		/* Barrier between setting up and tweaking */
		asm volatile ("":::"memory");

		uintptr_t *loc = (uintptr_t *) (&
						(((DQ_LINKAGE_T *) &
						  _KRN_schedule->
						  liveTasks)->back));
		*loc |= 1;

		PARACHECK();
	}
	IRQ_restoreIPL(oldipl);

	if (task != _KRN_current) {
		/* just remove task from whatever queue it is waiting on */
		oldipl = IRQ_raiseIPL();
		DQ_remove(task);
		IRQ_restoreIPL(oldipl);
	}
#ifdef CONFIG_DEBUG_PARANOIA
	if ((task->stackStart) && (task->stackEnd - 1))
		DBG_paranoidSweep((uintptr_t) task->stackStart,
				  (uintptr_t) task->stackEnd);
	else {
#ifdef STACK_GROWS_UP
		DBG_paranoidSweep((uintptr_t) task->stackStart,
				  (uintptr_t) task->wm);
#else
		DBG_paranoidSweep((uintptr_t) task->wm,
				  (uintptr_t) task->stackEnd);
#endif
	}
#endif

	if (task == _KRN_current) {
		/* if we are stopping ourselves, set a NULL holding queue... */
		task->holdQueue = NULL;
		/* ... and with our dying gasps, force a reschedule ... */
		KRN_scheduleProtected();
		/* ... mark the context as gone, this may terminate us ... */
		CTX_remove(task);
		/* ...KRN_schedule() tests that the schedule int has actually happened */
		_KRN_current->reason = KRN_DEAD;
		KRN_schedule();
	} else
		CTX_remove(task);
}
