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
*   Description:	Kernel scheduler and ISR
*
*************************************************************************/

#include "meos/config.h"

#ifndef CONFIG_DEBUG_PARANOIA
#define DQ_INLINE
#define LST_INLINE
#endif

#include <stdint.h>
#include <string.h>
#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"
#include "meos/ctx/ctx.h"
#include "meos/tmr/tmr.h"

extern volatile int32_t _KRN_schedNeeded;
void _KRN_setExpiry(void);

#ifndef CONFIG_DEBUG_TRACE_SOFT
#define DBG_ctxSw(REASON, FROM, TO)
#elif !defined(MEOS_NO_FUNCS)
#ifdef __cplusplus
extern "C" {
#endif
	void DBG_ctxSw(int32_t reason, void *from, void *to);
#ifdef __cplusplus
}
#endif
#endif
#ifdef CONFIG_DEBUG_PARANOIA
#include "meos/debug/dbg.h"
PARATYPE(Task, KRN_TASK_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(Schd, KRN_SCHEDULE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
extern DQ_T _paraItem_Dque;
extern DBG_PARATYPE_T _paraDesc_DqIt, _paraDesc_Dque;
#endif

void _KRN_scheduleInit(void);
void _KRN_scheduleSetInitialStack(KRN_TASK_T * task);
void _KRN_schedulePrepareContext(KRN_TASK_T * task, uintptr_t workspace,
				 size_t wssize, KRN_TASKFUNC_T * task_func);
void _KRN_scheduleNone(void);
void _KRN_scheduleSleep(void);

/* local prototypes */
void _KRN_schedulerISR(void) __attribute__ ((no_instrument_function));

#ifdef CONFIG_DEBUG_TRACE_SOFT
volatile KRN_TRACE_T *KRN_tracePtr;
KRN_TRACE_T *KRN_traceMin;
KRN_TRACE_T *KRN_traceMax;
size_t KRN_traceSize;
#endif

/*
** FUNCTION:	uffb
**
** DESCRIPTION:	"Unsigned Find First Bit" - helper function
**				for use in the optimised "search for runnable task"
**				code in the scheduler
**
** RETURNS:	number of the most significat "1" bit in n.
*/

inline static int32_t uffb(uint32_t n)
{
	return 31 - __builtin_clz(n);
}

/*
** FUNCTION:	KRN_reset
**
** DESCRIPTION:	Kernel reset - called once by each processor at startup
**
** RETURNS:		int32_t - 1 OK
**					0 Fail - due to bad priority value
*/
int32_t KRN_reset(KRN_SCHEDULE_T * sched, KRN_TASKQ_T * priQueues,
		  uint32_t maxPriority, uint32_t stackInit, uint32_t * intStack,
		  size_t issize, KRN_TRACE_T * traceBuf, int32_t traceBufSize)
{
	uint32_t n;

	PARACHECK();

#ifdef CONFIG_DEBUG_TRACE_SOFT
	/* initialise trace system */
	KRN_tracePtr = traceBuf;
	KRN_traceSize = traceBufSize;
	KRN_traceMin = traceBuf;
	KRN_traceMax = ((KRN_TRACE_T *) traceBuf) + traceBufSize - 1;
	if (traceBufSize)
		memset(KRN_traceMin, 0,
		       (uintptr_t) KRN_traceMax - (uintptr_t) KRN_traceMin);
#endif

#ifdef CONFIG_DEBUG_PROFILING
	/* initialise OS profiling system */
	_ThreadStatsArray = NULL;
	_ThreadStatsArraySize = 0;
	_ThreadStatsCtrl = 0;
#else
	(void)traceBuf;
	(void)traceBufSize;
#endif

	/* Accept a NULL scheduler as part of potentially shutting down */
	if (sched == NULL) {
		_KRN_schedule = NULL;
		return 1;
	}

	if ((maxPriority < 1)
	    || (maxPriority >= CONFIG_FEATURE_MAX_PRIORITIES))
		return 0;

	PARADEL(Schd, sched);

	/* quick zero everything in the scheduler data structure, so we only have to
	 * initialise any non-zero elements */
	memset(sched, 0, sizeof(KRN_SCHEDULE_T));

	/* global pointer to schedule data structure */
	_KRN_schedule = sched;

	/* initialise optimised priority search system */
	_KRN_schedule->maxPriority = maxPriority;
	_KRN_schedule->maxPriFlags =
	    _KRN_schedule->priFlags + (maxPriority / 32);

	/* initialise task queues */
	_KRN_schedule->priQueues = priQueues;
	for (n = 0; n <= maxPriority; n++)
		DQ_init(&priQueues[n]);

	/* timer system */
	_KRN_schedule->timeSlice = -1;
	KRN_initMbox(&_KRN_schedule->timerMailbox);
	DQ_init(&(_KRN_schedule->timers));
	/* Manual initialisation to keep it out of the paranoia system */
	DQ_T *queue = &(_KRN_schedule->liveTasks);
	{
		queue->DQ_link.fwd = (DQ_LINKAGE_T *) queue;
		queue->DQ_link.back = (DQ_LINKAGE_T *) queue;
	};

	/* basic scheduling */
	_KRN_schedule->stackInit = stackInit;

	/* initialise the interrupt stack */
	if (stackInit)
		for (n = 0; n < issize; n++)
			intStack[n] = stackInit;

	/* initialise interrupts */
	IRQ_init(intStack, issize);

	/* register with paranoia system */
	PARAADD(Schd, sched);
	PARACHECK();

	return 1;
}

extern void exit(int32_t);

/*
** FUNCTION:	KRN_startOS
**
** DESCRIPTION:	Start the OS. All code following this call will
**				execute at minimum priority as the first task in the system
**
** RETURNS:	KRN_TASK_T * Pointer to the initial task's TCB
*/
KRN_TASK_T *KRN_startOS(const char *taskName)
{
	PARACHECK();

	_KRN_schedNeeded = 0;

	/* set up the kernel's scheduler data */
	_KRN_current = &_KRN_schedule->startupTCB;

	/* initialise interrupts, contexts and timers */
	IRQ_install();
	CTX_init();
	TMR_init();

	/* fill in the initial TCB */
	KRN_startTask(NULL, &_KRN_schedule->startupTCB, NULL, 0,
		      KRN_LOWEST_PRIORITY, NULL, taskName);

	/* hook it to this instruction stream */
	CTX_attachBGTask(&_KRN_schedule->startupTCB);

	/* schedule to get the kernel data structures up and running */
	KRN_schedule();

#ifdef CONFIG_FEATURE_IMPEXP
	/* If the import/export system has been initialised, allow it to do the
	 * initialisation that was deferred because the kernel wasn't running.
	 */
	if (_KRN_schedule->initFunc)
		_KRN_schedule->initFunc();
#endif

	PARACHECK();
	return &_KRN_schedule->startupTCB;
}

KRN_TASK_T *MEOS_VERSION(const char *taskName)
    __attribute__ ((weak, alias("KRN_startOS")));

#ifdef CONFIG_DEBUG_PARANOIA
void DBG_paranoidSweep(uintptr_t start, uintptr_t end);
#endif

/*
** FUNCTION:	_KRN_prepareTask
**
** DESCRIPTION:	prepare a new task
**
** RETURNS:	void
*/
void _KRN_prepareTask(KRN_TASKFUNC_T * taskfunc, KRN_TASK_T * task,
		      uint32_t * uworkspace, size_t wssize,
		      KRN_PRIORITY_T * priority, void *parameter,
		      const char *taskname)
{
	KRN_PRIORITY_T maxpri;
	uintptr_t workspace = (uintptr_t) uworkspace, stackAddress;
	uint32_t *fill, *end;
	uint32_t stackFill = _KRN_schedule->stackInit;

	PARACHECK();
	PARADEL(Task, task);

#ifdef CONFIG_DEBUG_PARANOIA
	/* the stack might be recycled, make sure the paranoia system isn't tracking
	 * it's contents.
	 */
	DBG_paranoidSweep((uintptr_t) workspace, (uintptr_t) (workspace +
							      (wssize *
							       sizeof
							       (uint32_t))));
#endif

	/* force 64 bit (8 byte) alignment of task extra */
	stackAddress = (workspace + 7) & -8;
	if (stackAddress != workspace) {
		wssize -= (stackAddress - workspace);
		workspace = stackAddress;
	}
#ifdef CONFIG_DEBUG_STACK_CHECKING
	task->stackStart = (uint32_t *) workspace;
#endif
#ifdef CONFIG_DEBUG_PARANOIA
	task->wm = (uintptr_t) - 1;
#endif
	if (stackFill) {
		/* fill stack with unused marker value */
		end = (uint32_t *) stackAddress + wssize;
		if (workspace != 0)
			for (fill = (uint32_t *) workspace; fill < end; fill++)
				*fill = stackFill;
	}
	/* trap priority in valid range (and ensure timer task runs at maximum) */
	if (task == &(_KRN_schedule->timerTCB)) {
		*priority = _KRN_schedule->maxPriority;
	} else {
		maxpri = _KRN_schedule->maxPriority - 1;
		if (*priority < 0)
			*priority = 0;
		if (*priority > maxpri)
			*priority = maxpri;
	}
	DQ_init((DQ_T *) task);	/* set TCB linkages to safe initial state */
	task->priVal = *priority;
	task->priFlags = _KRN_schedule->priFlags + (*priority >> 5);
	task->priBit = *priority & 31;
	task->name = taskname;
	task->parameter = parameter;
	task->reason = KRN_OTHER;

	if (workspace != 0) {
#ifdef CONFIG_DEBUG_STACK_CHECKING
		/* not the startup task */
		task->stackEnd =
		    (uint32_t *) (workspace + (wssize * sizeof(uint32_t)));
#endif
	}

	/* Initialise the task's stack and entry point etc */
	CTX_startTask(task, workspace, wssize, taskfunc);

	PARAADD(Task, task);
	PARACHECK();

	return;
}

/*
** FUNCTION:	KRN_startTask
**
** DESCRIPTION:	Start a new task
**
** RETURNS:	void
*/
void KRN_startTask(KRN_TASKFUNC_T * taskfunc, KRN_TASK_T * task,
		   uint32_t * uworkspace, size_t wssize,
		   KRN_PRIORITY_T priority, void *parameter,
		   const char *taskname)
{
	PARACHECK();

	IRQ_IPL_T oldipl;
	uintptr_t workspace = (uintptr_t) uworkspace;

	DBG_assert(IRQ_bg()
		   || (taskfunc != NULL),
		   "Can not start task from interrupt context");

	task->holdQueue = NULL;
	_KRN_prepareTask(taskfunc, task,
			 uworkspace, wssize, &priority, parameter, taskname);
	task->holdQueue = _KRN_schedule->priQueues + priority;

#ifdef CONFIG_DEBUG_PROFILING
	/* if possible, locate and set up a task performance statistics slot */
	task->statsPtr = NULL;
	if (_ThreadStatsArraySize > 0) {
		int32_t n;
		for (n = 0; n < _ThreadStatsArraySize; n++) {
			oldipl = IRQ_raiseIPL();
			if (_ThreadStatsArray[n].name == NULL) {
				_ThreadStatsArray[n].name =
				    taskname ? taskname : "";
				task->statsPtr = &_ThreadStatsArray[n];
				_KRN_zeroStats(task->statsPtr);
				IRQ_restoreIPL(oldipl);
				break;
			}
			IRQ_restoreIPL(oldipl);
		}
		DBG_insist(n < _ThreadStatsArraySize, "No free perf stats slots!");	/* fails if no slot available */
	}
#endif

	oldipl = IRQ_raiseIPL();
	/* Manually add the task to the all tasks queue to keep it out of the
	 * paranoia system, and set the dirty bit so Codescape notices.
	 */
	DQ_T *queue = &_KRN_schedule->liveTasks;
	void *item = &task->taskLink;
	{
		PARACHECK();
		PARADEL(DqIt, item);

		((DQ_LINKAGE_T *) queue)->back =
		    (DQ_LINKAGE_T
		     *) (((uintptr_t) ((DQ_LINKAGE_T *) queue)->back) & ~1);
		((DQ_LINKAGE_T *) item)->fwd = (DQ_LINKAGE_T *) queue;
		((DQ_LINKAGE_T *) item)->back = ((DQ_LINKAGE_T *) queue)->back;
		((DQ_LINKAGE_T *) queue)->back->fwd = (DQ_LINKAGE_T *) item;
		((DQ_LINKAGE_T *) queue)->back =
		    (DQ_LINKAGE_T *) (((uintptr_t) item) | 1);

		PARAADDI(DqIt, item);
		PARACHECK();
	}
	IRQ_restoreIPL(oldipl);

	if (workspace != 0) {	/* not the startup task */
		/* link the new TCB onto a scheduler queue */
		oldipl = IRQ_raiseIPL();
		DQ_addTail(task->holdQueue, task);	/* activate task */
		*(task->priFlags) |= 1 << task->priBit;	/* mark queue as active for optimised search */
		KRN_scheduleUnprotect(oldipl);
	} else {
		oldipl = IRQ_raiseIPL();
		*(task->priFlags) |= 1 << task->priBit;	/* mark queue as active for optimised search */
		IRQ_restoreIPL(oldipl);
	}

	PARACHECK();
	return;
}

#ifdef CONFIG_DEBUG_PROFILING
/*
** FUNCTION:    _KRN_schedStats
**
** DESCRIPTION: Helper function for updating performance stats relating to the
**                              scheduler ISR itself
**
** RETURNS:     void
*/
void _KRN_schedStats(KRN_RAWSTATS_T * schedEntryStats)
{
	/* update perf counters attributable to the scheduler */
	KRN_RAWSTATS_T deltaStats;
	KRN_STATS_T *sp = _KRN_schedule->schedStats;
	_KRN_grabStats(&deltaStats);
	_KRN_deltaStats(&deltaStats, &deltaStats, schedEntryStats);
	_KRN_accStats(_KRN_schedule->schedStats, &deltaStats, 0);
	sp->runCount++;
	/* avoid double counting by subtracting from current task */
	sp = _KRN_current ? _KRN_current->statsPtr : _KRN_schedule->nullStats;
	_KRN_accStats(sp, &deltaStats, 1);
}
#endif

/*
** FUNCTION:	_KRN_scheduleTaskCore
**
** DESCRIPTION:	Core scheduler function
**
** RETURNS:	void
*/
static inline void _KRN_scheduleTaskCore(KRN_TASK_T * newTask)
{
	PARACHECK();
	KRN_SCHEDULE_T *s;
	KRN_TASK_T *oldTask;

	uint32_t *priFlgs;
	uint32_t *priMin;
	int32_t n;

	s = _KRN_schedule;

#ifdef CONFIG_DEBUG_PROFILING
	/* possibly reset and grab performance counters */
	KRN_STATS_T *sp;
	KRN_RAWSTATS_T stats;
	if (_ThreadStatsCtrl && _ThreadStatsArray) {	/* if performance counting installed and enabled */
		if (_ThreadStatsCtrl == 2) {	/* restart... */
			/* reset and enable */
			sp = _ThreadStatsArray;
			for (n = 0; n < _ThreadStatsArraySize; n++, sp++)
				if (sp->name)
					_KRN_zeroStats(sp);
			/* initialise "previous counter values */
			_KRN_grabStats(&(s->stats));
			_ThreadRunStart = s->stats.monotonics;
			_ThreadStatsCtrl = 1;
		}
		/* grab current counter values */
		_KRN_grabStats(&stats);
		_ThreadRunTicks = stats.monotonics - _ThreadRunStart;
	}
#endif

	/* work out where the old task should be queued */
	oldTask = _KRN_current;
	if (oldTask != NULL) {
		KRN_TASKQ_T *queue = oldTask->holdQueue;
		DQ_remove(oldTask);
		if (queue != NULL) {
			if ((queue ==
			     _KRN_schedule->priQueues + oldTask->priVal)
			    && (oldTask->reason == KRN_OTHER)) {
				/* Interrupted/preempted - make sure the next time we're running
				 * at this priority level, it gets a chance to finish.
				 */
				DQ_addHead(queue, oldTask);
			} else {
				/* Yielded/timesliced - round robin */
				DQ_addTail(queue, oldTask);
			}
		}
		/* else - interrupted task dies */
	}

	/*
	 * Locate the highest priority runnable task - we use an optimised
	 * search based on having an array of 32-bit flag sets which indicate
	 * which priority levels are active. This allows us to search a large
	 * number of priority levels very quickly, based on skipping groups of
	 * 32 empty levels and then using the equivalent of a find first bit
	 * instruction to locate the highest priority active queue in a group
	 * of 32
	 */
	if (!newTask) {
		priFlgs = s->maxPriFlags;
		priMin = s->priFlags;
		while (priFlgs >= priMin) {
			if (*priFlgs) {
				n = uffb(*priFlgs) + ((priFlgs - priMin) << 5);	/* maximum active priority */
				if ((newTask = (KRN_TASK_T *) DQ_removeHead(&s->priQueues[n])))	/* single = is deliberate ! */
					break;
				/* no task - queue has gone out of use - this should normally happen no
				 * more than once on any entry to the scheduler */
				*priFlgs ^= (1 << (n & 31));
			} else
				priFlgs--;
		}
	}

	/* If we've found a candidate task, swap to it, otherwise defer descheduling of
	 * the current task and go idle. If we get here with a dead task, then we
	 * must reactivate its context.
	 */
	if (newTask != NULL) {

		if (s->timeSlice > 0) {
			if ((oldTask != newTask)
			    || (oldTask->reason == KRN_TIMESLICE))
				s->sliceExpire =
				    TMR_getMonotonic() + s->timeSlice;
			_KRN_setExpiry();
		}

		if ((oldTask == newTask) && (oldTask->reason != KRN_DEAD)) {
			/* no context switch - fast return */
#ifdef CONFIG_DEBUG_PROFILING
			if (_ThreadStatsCtrl && _ThreadStatsArray)
				_KRN_schedStats(&stats);	/* update statistics for scheduler prior to exit */
#endif
			oldTask->reason = KRN_OTHER;
			CTX_verifyTask(oldTask);	/* Opportunity for stack checks */
			PARACHECK();
			return;
		}
#ifdef CONFIG_DEBUG_PROFILING
		if (_ThreadStatsCtrl && _ThreadStatsArray) {	/* performance monitoring installed and switched on */
			/* update OS Profiling statistics for outgoing task */
			sp = oldTask ? oldTask->statsPtr : s->nullStats;
			KRN_RAWSTATS_T deltaStats;
			_KRN_deltaStats(&deltaStats, &stats, &(s->stats));
			_KRN_accStats(sp, &deltaStats, 0);
			sp->runCount++;
			s->stats = stats;	/* structure copy ! */
		}
#endif

		if (oldTask == NULL)
			oldTask = s->deferred;	/* idle->active */
		DBG_assert(oldTask != NULL, "NULL outgoing task");

		/* trace this context switch */
#ifdef CONFIG_DEBUG_TRACE_CTXSW_HARD
		DBG_RTTPair(DBG_TRACE_CTX_SW, oldTask->reason);
		DBG_RTTValue(oldTask);
		DBG_RTTValue(newTask);
#endif
#ifdef CONFIG_DEBUG_TRACE_CTXSW_SOFT
		DBG_ctxSw(oldTask->reason, oldTask, newTask);
#endif

		/* switch the old task out */
		if (oldTask->reason != KRN_DEAD) {
			CTX_verifyTask(oldTask);
			if ((oldTask->schedOut)
			    && (oldTask->reason != KRN_DEAD)) {
				_KRN_current = oldTask;
				oldTask->schedOut();
			}
		}
		oldTask->reason = KRN_OTHER;

		/* switch the new task in */
		s->deferred = NULL;
		PARACHECK();
		_KRN_current = newTask;
		if (newTask->schedIn) {
#ifdef CONFIG_DEBUG_PROFILING
			if (_ThreadStatsCtrl && _ThreadStatsArray)
				/* update statistics for scheduler prior to exit */
				_KRN_schedStats(&stats);
#endif
			newTask->schedIn();	/* exit point */
		}
		return;		/* exit point */
	} else {
		/* no available task so wait for next interrupt */
		_KRN_current = NULL;
		/* idle */
		if (oldTask != NULL) {
#ifdef CONFIG_DEBUG_TRACE_CTXSW_HARD
			DBG_RTTPair(DBG_TRACE_CTX_SW, oldTask->reason);
			DBG_RTTValue(oldTask);
			DBG_RTTValue(NULL);
#endif
#ifdef CONFIG_DEBUG_TRACE_CTXSW_SOFT
			DBG_ctxSw(oldTask->reason, oldTask, NULL);
#endif

			CTX_verifyTask(oldTask);
			/* active -> idle */
			s->deferred = oldTask;
		}
		/* else idle->idle, nothing to do */
		PARACHECK();
#ifdef CONFIG_DEBUG_PROFILING
		if (_ThreadStatsCtrl && _ThreadStatsArray) {	/* performance monitoring installed and switched on */
			/* Update OS Profiling statistics for outgoing task */
			sp = oldTask ? oldTask->statsPtr : s->nullStats;
			KRN_RAWSTATS_T deltaStats;
			_KRN_deltaStats(&deltaStats, &stats, &(s->stats));
			_KRN_accStats(sp, &deltaStats, 0);
			sp->runCount++;
			s->stats = stats;	/* structure copy */
		}
#endif
		CTX_sleep();	/* exit point */
	}
#ifdef CONFIG_DEBUG_PROFILING
	if (_ThreadStatsCtrl && _ThreadStatsArray)
		/* update OS profiling statistics for scheduler prior to exit */
		_KRN_schedStats(&stats);
#endif
}

void _KRN_scheduleTask(KRN_TASK_T * newTask)
{
	_KRN_scheduleTaskCore(newTask);
}

/*
** FUNCTION:	_KRN_schedulerISR
**
** DESCRIPTION:	Scheduler interrupt handler - executes with interrupts disabled
**				so no additional protection needed (apart from inter-thread LOCKs)
**
** RETURNS:	void
*/
void _KRN_schedulerISR(void)
{

	/*  Refuse to schedule if IPL is raised */
	if ((_KRN_schedNeeded == 0) || (IRQ_getIPL()))
		return;		/* exit point */

	_KRN_schedNeeded = 0;

	_KRN_scheduleTaskCore(NULL);
}
