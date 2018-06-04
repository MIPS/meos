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
*   Description:	Linux context specialisation
*
*************************************************************************/

#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include "meos/debug/dbg.h"
#include "meos/kernel/krn.h"
#include "meos/irq/irq.h"
#include "meos/ctx/ctx.h"

PARATYPE(SCTX, KRN_CTX_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARAEXTERN(DqIt, DQ_LINKAGE_T);

pthread_t _CTX_self;
pthread_mutex_t _CTX_BFL;
pthread_cond_t _CTX_BFC;

extern volatile int32_t _KRN_schedNeeded;
__thread KRN_TASK_T *_CTX_current = NULL;
__thread _KRN_DISPOSABLE_CTX_T *_CTX_disp = NULL;
__thread int32_t _CTX_insig = 0;

void DBG_interruptSched(void);
void DBG_hotwire(KRN_CTX_T * ctx);

#ifdef CONFIG_DEBUG_PARANOIA
/*
** FUNCTION:	DBG_paranoiaAllowed
**
** DESCRIPTION:	Return true if we can be paranoid.
**
** RETURNS:	int32_t
*/
int32_t DBG_paranoiaAllowed(void)
    __attribute__ ((no_instrument_function));
int32_t DBG_paranoiaAllowed(void)
{
	if (_CTX_disp) {
		/* The task isn't under scheduler control, don't be paranoid, or we'll
		 * corrupt memory!
		 */
		return (!_CTX_disp->first) && (!_CTX_disp->die);
	} else
		return 0;
}
#endif

extern volatile int32_t _IRQ_needed;

void _IRQ_dispatch(void);
void _KRN_schedulerISR(void);
void _DBG_backtraceCtx(void);

/*
** FUNCTION:	_CTX_SIGUSR1
**
** DESCRIPTION:	interrupt/scheduling arbitration - do our best to make a pthread
**              look a little like a raw context.
**
** RETURNS:	void
*/
void _CTX_SIGUSR1(int32_t sigNum)
    __attribute__ ((no_instrument_function));
void _CTX_SIGUSR1(int32_t sigNum)
{
	/* report entry */
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, _CTX_SIGUSR1, 1, sigNum);
	DBG_walkExtra(&wp, 1);
#elif defined(DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(1);
	DBG_enter(&wp, _CTX_SIGUSR1, 1, sigNum);
#endif

	/* Bail out if we're already in "kernel" mode */
	if (_CTX_insig) {
#ifdef CONFIG_DEBUG_TRACE_EXTRA
		wp = DBG_openTrace(2);
		DBG_exit(&wp, _CTX_SIGUSR1, 0);
		DBG_walkExtra(&wp, 1);
#elif defined(DEBUG_TRACE_SOFT)
		wp = DBG_openTrace(1);
		DBG_exit(&wp, _CTX_SIGUSR1, 0);
#endif
		return;
	}
	do {
		/* Flag that we're in "kernel" mode */
		_CTX_insig++;

		/* Make multiple threads behave a bit like one instruction stream: idle
		 * until it's our turn.
		 */
		pthread_mutex_lock(&_CTX_BFL);
		for (;;) {
			/* If this pthread has finished being assimilated */
			if (!_CTX_disp->first) {
				/* If it's the timer interrupt emulation thread */
				if ((_KRN_current == _CTX_current)
				    || ((!_KRN_current)
					&& (_CTX_current ==
					    &_KRN_schedule->timerTCB))) {
					/* Timer interrupt? */
					if (_IRQ_needed) {
						pthread_mutex_unlock(&_CTX_BFL);
						_IRQ_dispatch();
						pthread_mutex_lock(&_CTX_BFL);
					}
					/* Schedule required? */
					if (_KRN_schedNeeded) {
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
						DBG_interruptSched();
#endif
						_KRN_schedulerISR();
					}
				}

				/* die if zombiefied */
				if (_CTX_disp->die) {
					uint32_t die = _CTX_disp->die;
					if (_KRN_current == _CTX_current) {
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
						DBG_interruptSched();
#endif
						_KRN_schedulerISR();
					}
					pthread_cond_signal(&_CTX_BFC);
					pthread_mutex_unlock(&_CTX_BFL);
					free(_CTX_disp);
					if (die == 1)
						pthread_exit(NULL);
					else
						return;
				}
			}

			/* is it our turn? */
			if (_KRN_current == _CTX_current) {
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
				/* trace activation */
				DBG_hotwire(&_KRN_current->savedContext);
#endif
				/* escape loop */
				break;
			}

			/* poke our brethren to keep things moving */
			pthread_cond_signal(&_CTX_BFC);

			if (_CTX_disp->first) {
				/* we're not yet assimilated - wait here until we are */
				_CTX_disp->first = 0;
				pthread_mutex_unlock(&_CTX_BFL);
				pthread_barrier_wait(&_CTX_disp->bar);
				pthread_mutex_lock(&_CTX_BFL);
			} else {
				/* sleep until something happens */
				pthread_cond_wait(&_CTX_BFC, &_CTX_BFL);
			}
		}
		pthread_mutex_unlock(&_CTX_BFL);

		/* die if zombified */
		if (_CTX_disp->die) {
			if (_KRN_current == _CTX_current) {
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
				DBG_interruptSched();
#endif
				_KRN_schedulerISR();
			}
			free(_CTX_disp);
			pthread_exit(NULL);
		}
#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
		/* trace activation */
		if (_CTX_current->savedContext.trace) {
			_CTX_current->savedContext.trace = 0;
			_DBG_backtraceCtx();
		}
#endif

		/* return back to user code */
		_CTX_self = _CTX_disp->thread;
		_CTX_insig--;
	} while (_IRQ_needed);

#ifdef CONFIG_DEBUG_TRACE_EXTRA
	wp = DBG_openTrace(2);
	DBG_exit(&wp, _CTX_SIGUSR1, 0);
	DBG_walkExtra(&wp, 1);
#elif defined(DEBUG_TRACE_SOFT)
	wp = DBG_openTrace(1);
	DBG_exit(&wp, _CTX_SIGUSR1, 0);
#endif
}

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

/*
** FUNCTION:	_CTX_startFunc
**
** DESCRIPTION:	Wrapper invoked when creating a new task. Assimilates itself
**				then invokes user code.
**
** RETURNS:	void *
*/
void *_CTX_startFunc(KRN_TASK_T * self)
{
	/* Initialise thread local data */
	_CTX_current = self;
	_CTX_disp = self->savedContext.disp;

	/* Linux threads might not run on the stack they've been told to. Discover
	 * where we're really running.
	 */
#ifdef CONFIG_DEBUG_PARANOIA
	DBG_walk(1, NULL, &self->wm);
#ifdef CONFIG_DEBUG_STACK_CHECKING
#ifdef STACK_GROWS_UP
	self->stackStart = (void *)self->wm;
#else
	self->stackEnd = (void *)self->wm;
#endif
#endif
#endif

	/* Block until first schedule */
	_IRQ_enable();
	while (_CTX_disp->first)
		KRN_schedulerInterrupt();

	PARACHECK();

	/* Run the task */
	self->savedContext.task_func();

	PARACHECK();

	/* Remove our self and die */
	KRN_removeTask(_CTX_current);
	return NULL;
}

/*
** FUNCTION:	_CTX_assimilateTask
**
** DESCRIPTION:	Assimilate any task.
**
** RETURNS:	void *
*/
void _CTX_assimilateTask(KRN_TASK_T * task)
{
	PARACHECK();

/* initialise locks, etc. used to assimilate threads into one instruction
 * stream.
 */
	struct sigaction sigusr1 = { };
/* Attach to TLS */
	_CTX_current = task;
	_CTX_disp = task->savedContext.disp;
	task->savedContext.disp->first = 0;
/* Initialise stack extent */
#ifdef CONFIG_DEBUG_STACK_CHECKING
	task->stackEnd = 0;
	task->stackStart = 0;
#endif
#ifdef CONFIG_DEBUG_PARANOIA
	DBG_walk(2, NULL, &task->wm);
#endif
/* Initialise thread identity data */
	task->savedContext.disp->thread = pthread_self();
/* Attach SIGUSR1 */
	sigusr1.sa_handler = _CTX_SIGUSR1;
	sigusr1.sa_flags = SA_NODEFER;
	sigaction(SIGUSR1, &sigusr1, NULL);

	PARACHECK();
}

/*
** FUNCTION:	CTX_attachBGTask
**
** DESCRIPTION:	Assimilate the initial task.
**
** RETURNS:	void *
*/
void CTX_attachBGTask(KRN_TASK_T * task)
{
	pthread_mutexattr_t mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
	pthread_mutex_init(&_CTX_BFL, &mta);
	pthread_mutexattr_destroy(&mta);
	pthread_cond_init(&_CTX_BFC, NULL);

	_CTX_assimilateTask(task);
}

/*
** FUNCTION:	CTX_removeNoWait
**
** DESCRIPTION:	Mark a thread for death, but don't wait for it to die.
**
** RETURNS:	void
*/
void CTX_removeNoWait(KRN_TASK_T * task, uint32_t die)
    __attribute__ ((no_instrument_function));
void CTX_removeNoWait(KRN_TASK_T * task, uint32_t die)
{
	/* Mark for death */
	task->savedContext.disp->die = die;
	task->savedContext.disp = NULL;
	task->savedContext.magic = 0x7e57ab1e;

	/* Commit suicide if necessary */
	if (task == _CTX_current) {
		KRN_schedule();
	}

	/* Wake it up */
	pthread_cond_signal(&_CTX_BFC);
}

/*
** FUNCTION:	CTX_startTask
**
** DESCRIPTION:	Create and assimilate a new pthread, and associate it with the
**				supplied task.
**
** RETURNS:	void
*/
void CTX_startTask(KRN_TASK_T * task, uintptr_t workspace,
		   size_t wssize, KRN_TASKFUNC_T * task_func)
{
	PARACHECK();

	PARADEL(SCTX, &task->savedContext);
	/* If it's likely this task existed before, poke it to death */
	if ((task->savedContext.magic == 0x7e57ab1e)
	    && (task->savedContext.disp)) {
		CTX_removeNoWait(task, 1);
	}
	/* Set magic */
	task->savedContext.magic = 0x7e57ab1e;
	/* Allocate disposable context */
	task->savedContext.disp = calloc(1, sizeof(_KRN_DISPOSABLE_CTX_T));
	task->savedContext.disp->first = 1;
	PARAADD(SCTX, &task->savedContext);
	/* Initialise task context */
	task->schedOut = NULL;
	task->schedIn = NULL;
	/* Start thread */
	if (task_func != NULL) {
		IRQ_IPL_T ipl;
		pthread_barrier_init(&task->savedContext.disp->bar, NULL, 2);
		pthread_attr_t attr;
		/* Initialise thread entry point */
		task->savedContext.task_func = task_func;
		/* Initialise stack extent */
		pthread_attr_init(&attr);
#ifdef CONFIG_DEBUG_STACK_CHECKING
		if (pthread_attr_setstack(&attr, (void *)workspace, wssize * 4)) {
			/* Stack extents can't be trusted */
			task->stackStart = 0;
			task->stackEnd = 0;
		}
#endif
		/* Create pthread */
		ipl = IRQ_raiseIPL();
		pthread_create(&(task->savedContext.disp->thread),
			       &attr, (void *(*)(void *))_CTX_startFunc,
			       (void *)task);
		IRQ_restoreIPL(ipl);
		/* Wait to be rescheduled after initialisation */
		pthread_barrier_wait(&task->savedContext.disp->bar);
		/* New pthread is assimilated - clean up */
		pthread_attr_destroy(&attr);
	}

	PARACHECK();
}

/*
** FUNCTION:	MEOS_join
**
** DESCRIPTION:	Assimilate a non-MEOS pthread into MEOS scheduler control.
**
** RETURNS:	KRN_TASK_T *
*/
KRN_TASK_T *MEOS_join(KRN_PRIORITY_T priority, const char *taskname)
{
	DBG_assert(_CTX_current == NULL,
		   "Thread already under MEOS control!\n");
	KRN_TASK_T *t = malloc(sizeof(KRN_TASK_T));
	KRN_startTask(NULL, t, NULL, 0, priority, NULL, taskname);
	_CTX_assimilateTask(t);
	pthread_kill(_CTX_disp->thread, SIGUSR1);
	return t;
}

/*
** FUNCTION:	CTX_remove
**
** DESCRIPTION:	Kill a thread.
**
** RETURNS:	void
*/
void CTX_remove(KRN_TASK_T * task)
    __attribute__ ((no_instrument_function));
void CTX_remove(KRN_TASK_T * task)
{
	/* Keep a local copy of important information */
	void *dummy;
	pthread_t thread = task->savedContext.disp->thread;

	PARACHECK();

	CTX_removeNoWait(task, 1);

	/* Wait until it's dead */
	IRQ_IPL_T before = IRQ_raiseIPL();
	pthread_join(thread, &dummy);

	IRQ_restoreIPL(before);

	PARACHECK();
}

/*
** FUNCTION:      MEOS_leave
**
** DESCRIPTION:   Sever a previous MEOS_join()ed thread from scheduler control.
**
** RETURNS:       void
*/
void MEOS_leave(void)
{
	DBG_assert(_CTX_current != NULL, "Thread not under MEOS control!\n");
	KRN_TASK_T *task = _CTX_current;
	IRQ_IPL_T oldipl;

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
						  _KRN_schedule->liveTasks)->
						 back));
		*loc |= 1;

		PARACHECK();
	}
	IRQ_restoreIPL(oldipl);

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

	/* if we are stopping ourselves, set a NULL holding queue... */
	task->holdQueue = NULL;
	/* ... and with our dying gasps, force a reschedule ... */
	KRN_scheduleProtected();
	/* ... mark the context as gone, without termination ... */
	CTX_removeNoWait(task, 2);
	/* ... KRN_schedule() tests that the schedule int has actually happened ... */
	_CTX_current->reason = KRN_DEAD;
	KRN_schedule();
	/* ... we should now be free of scheduler control. Clean up */
	free(task);
	_CTX_current = NULL;
	_CTX_disp = NULL;
	_CTX_insig = 0;
}

/*
** FUNCTION:	CTX_sleep
**
** DESCRIPTION:	Put everything to sleep.
**
** RETURNS:	void
*/
void CTX_sleep(void)
{
	PARACHECK();
	/* This will ensure every pthread is stuck waiting to be activated */
	_KRN_current = NULL;
	PARACHECK();
}

/*
** FUNCTION:	CTX_init
**
** DESCRIPTION:	No initialisation to do here.
**
** RETURNS:	void
*/
void CTX_init(void)
{
	PARACHECK();
}

/*
** FUNCTION:	CTX_verifyTask
**
** DESCRIPTION:	Context verification can't be done on Linux.
**
** RETURNS:	void
*/
void CTX_verifyTask(KRN_TASK_T * current)
{
	(void)current;
	PARACHECK();
}

/*
** FUNCTION:	CTX_verifySelf
**
** DESCRIPTION:	Context verification can't be done on Linux.
**
** RETURNS:	void
*/
void CTX_verifySelf(KRN_TASK_T * current)
    __attribute__ ((no_instrument_function));
void CTX_verifySelf(KRN_TASK_T * current)
{
	(void)current;
	PARACHECK();
}
