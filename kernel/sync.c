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
*   Description:	Kernel synchronisation point
*
*************************************************************************/

#include "meos/config.h"
#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"

PARATYPE(Sync, KRN_SYNC_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/*
** FUNCTION:	KRN_initSync
**
** DESCRIPTION:	Initialise a sync
**
** RETURNS:	void
*/
void KRN_initSync(KRN_SYNC_T * sync, uint32_t value)
{
	PARACHECK();
	PARADEL(Sync, sync);
#ifdef CONFIG_FEATURE_IMPEXP
	sync->impexp.thread = (-1);
	sync->impexp.sId = 0;
	sync->impexp.objtype = KRN_OBJ_SYNC;
	sync->impexp.unused = 0;
#endif
	sync->reset = value;
	sync->count = value;
	DQ_init(&sync->waitq);

	PARAADD(Sync, sync);
	PARACHECK();
}

/*
* Sync multiple threads
*/
KRN_TASK_T *_KRN_syncExported(KRN_SYNC_T * sync)
{
	PARACHECK();

#ifdef CONFIG_FEATURE_IMPEXP
	uint8_t sId;
	uint32_t notifyMask;
#endif

	if (sync->count > 1) {
		sync->count--;
	} else {
		sync->count = sync->reset;
		KRN_wakeAll(&sync->waitq);
		/*
		 * if  any other thread has a pending interest,
		 * send an asynchronous notify messages
		 */
#ifdef CONFIG_FEATURE_IMPEXP
		if (((sId = sync->impexp.sId) > 0) &&
		    (notifyMask =
		     _KRN_schedule->exportTable[sId - 1].notifyMask)) {
			/* the asynch notify function is accessed indirectly via a pointer.
			 * This is part of the system for avoiding linking unnecessary code
			 * in systems that don't use import/export
			 */
			_KRN_schedule->notifyFunc(notifyMask, sId, sync->count);
			_KRN_schedule->exportTable[sId - 1].notifyMask = 0;
		}
#endif
	}

	PARACHECK();

	return _KRN_current;
}

/*
* Sync multiple threads
*/
KRN_TASK_T *KRN_sync(KRN_SYNC_T * sync, int32_t timeout)
{
	KRN_TASK_T *t = _KRN_current;
	KRN_TIMER_T timer;
	IRQ_IPL_T oldipl;
#ifdef CONFIG_FEATURE_IMPEXP
	uint8_t sId;
	uint32_t notifyMask;
#endif

	PARACHECK();

	oldipl = _KRN_raiseIPLWithTimeout(&timer, timeout);

#ifdef CONFIG_FEATURE_IMPEXP
	if (sync->impexp.thread >= 0) {
		_KRN_waitOnSequence(t, &sync->waitq,
				    (uint32_t *) & sync->count, &sync->impexp,
				    KRN_COMMAND_SYNC, KRN_SUBCOMMAND_WAIT, 0,
				    0);
		KRN_scheduleUnprotect(oldipl);

		PARACHECK();
		return (t->timedOut == KRN_TIMEDOUT) ? NULL : t;
	}
#endif

	if (sync->count > 1) {
		_KRN_hibernateTask(t, &sync->waitq);
		/* An inter-thread race condition means that we must append to the sync
		 * queue immediately. We can't wait for the scheduler to do it for us */
		DQ_remove(t);
		DQ_addTail(&sync->waitq, t);

		sync->count--;
		KRN_scheduleUnprotect(oldipl);
	} else {
		sync->count = sync->reset;
		KRN_wakeAll(&sync->waitq);
		/*
		 * if  any other thread has a pending interest,
		 * send an asynchronous notify messages
		 */
#ifdef CONFIG_FEATURE_IMPEXP
		if (((sId = sync->impexp.sId) > 0) &&
		    (notifyMask =
		     _KRN_schedule->exportTable[sId - 1].notifyMask)) {
			/* the asynch notify function is accessed indirectly via a pointer.
			 * This is part of the system for avoiding linking unnecessary code
			 * in systems that don't use import/export
			 */
			_KRN_schedule->notifyFunc(notifyMask, sId, sync->count);
			_KRN_schedule->exportTable[sId - 1].notifyMask = 0;
		}
#endif
		_KRN_restoreIPLWithTimeout(oldipl, &timer, timeout);
	}

	PARACHECK();

	return (t->timedOut == KRN_TIMEDOUT) ? NULL : t;
}
