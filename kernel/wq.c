/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*   Description:	Wakeup routines
*
*************************************************************************/

#include "meos/kernel/krn.h"

static const char *_wqw = "WQ worker";

static inline void KRN_dispatchOne(KRN_WQ_T * wq)
{
	KRN_JOB_T *job;
	KRN_TASKFUNC_T *fn;
	const char *old = _KRN_current->name;

	job = (KRN_JOB_T *) KRN_getMbox(&wq->mbox, KRN_INFWAIT);
	_KRN_current->name = job->name;
	_KRN_current->parameter = job->par;
	fn = job->fn;
	KRN_returnPool(job);
	fn();
	_KRN_current->name = old;
}

/*
** FUNCTION:      _KRN_wqDispatch
**
** DESCRIPTION:   Pull jobs from the work queue mailbox and execute them
**
** RETURNS:       void
*/
void KRN_dispatchWQ(KRN_WQ_T * wq, int32_t n)
{
	if (n >= 0) {
		int32_t i;
		for (i = 0; i < n; i++)
			KRN_dispatchOne(wq);
	} else
		for (;;)
			KRN_dispatchOne(wq);
}

static void _KRN_dispatchWQTask()
{
	_KRN_current->name = _wqw;
	KRN_dispatchWQ((KRN_WQ_T *) KRN_taskParameter(NULL), -1);
}

/*
** FUNCTION:      KRN_initWQ
**
** DESCRIPTION:   Initialise a work queue
**
** RETURNS:       void
*/
void KRN_initWQ(KRN_WQ_T * wq, KRN_TASK_T * tasks, uint32_t * stacks,
		uint32_t nTasks, size_t stackSize, KRN_PRIORITY_T prio,
		KRN_JOB_T * jobs, uint32_t nJobs)
{
	uint32_t i;
#ifdef CONFIG_FEATURE_IMPEXP
	wq->impexp.thread = (-1);
	wq->impexp.sId = 0;
	wq->impexp.objtype = KRN_OBJ_WQ;
	wq->impexp.unused = 0;
#endif
	wq->tasks = nTasks;
	wq->task = tasks;
	if (nJobs)
		KRN_initPool(&wq->jobs, jobs, nJobs, sizeof(KRN_JOB_T));
	KRN_initMbox(&wq->mbox);
	for (i = 0; i < nTasks; i++)
		KRN_startTask(_KRN_dispatchWQTask, &tasks[i],
			      &stacks[i * stackSize], stackSize, prio, wq,
			      _wqw);
}

/*
** FUNCTION:      KRN_removeWQ
**
** DESCRIPTION:   Stop a work queue and all related worker tasks
**
** RETURNS:       void
*/
void KRN_removeWQ(KRN_WQ_T * wq)
{
	uint32_t i;

#ifdef CONFIG_FEATURE_IMPEXP
	DBG_assert(wq->impexp.thread < 0,
		   " Can not remove imported work queue");
	if (wq->impexp.thread < 0)
		return;
#endif

	for (i = 0; i < wq->tasks; i++)
		KRN_removeTask(&wq->task[i]);
}

/*
** FUNCTION:      KRN_queueWQ
**
** DESCRIPTION:   Enqueue a job on a work queue
**
** RETURNS:       void
*/
int32_t KRN_queueWQ(KRN_WQ_T * wq, KRN_TASKFUNC_T fn, void *par,
		    const char *name, int32_t timeout)
{
	KRN_JOB_T *job;

#ifdef CONFIG_FEATURE_IMPEXP
	if (wq->impexp.thread >= 0) {
		IRQ_IPL_T oldipl;
		/* wq is imported so
		   1) we wait on the wq queue
		   2) we derive the message sequence number from the pool semaphore
		   value and send the appropriate queue message
		   3) the ACK will remove us from the queue
		   We wait for the ack to ensure desc can be reclaimed.
		 */
		oldipl = IRQ_raiseIPL();
		_KRN_waitOnSequence(_KRN_current, &wq->mbox.sem.waitq,
				    (uint32_t *) & wq->mbox.sem.value,
				    &wq->impexp, KRN_COMMAND_WQ_QUEUE, 0,
				    (uintptr_t) fn, (uintptr_t) par);
		KRN_scheduleUnprotect(oldipl);
		return _KRN_current->p2.ackStatus;
	}
#endif

	job = (KRN_JOB_T *) KRN_takePool(&wq->jobs, timeout);
	if (!job)
		return 0;
	job->fn = fn;
	job->par = par;
	job->name = name;
	KRN_putMbox(&wq->mbox, job);
	return 1;
}
