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
*   Description:	Event flag clusters
*
*************************************************************************/

#include "meos/debug/dbg.h"
#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"

PARATYPE(Flag, KRN_FLAGCLUSTER_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/* local prototypes and inlines */
static void flagSchedule(KRN_FLAGCLUSTER_T * cluster);
#ifdef CONFIG_FEATURE_IMPEXP
static void setImport(KRN_FLAGCLUSTER_T * cluster, uint32_t mask, uint32_t cmd);
static uint32_t testImportedFlags(KRN_FLAGCLUSTER_T * cluster,
				  uint32_t mask, KRN_FLAGOP_T test,
				  int32_t clearFlags, IRQ_IPL_T * lowerIPL);
#endif

/* this one is used in impexp.c as well - make sure the prototype there matches */
/*
** FUNCTION:	_KRN_flgTest
**
** DESCRIPTION:	flag test function
**
** RETURNS:	void
**
*/
INLINE int32_t _KRN_flgTest(uint32_t flags, uint32_t mask, KRN_FLAGOP_T test)
{
	if (test == KRN_ANY)
		return (flags & mask) != 0;
	else
		return (flags & mask) == mask;
}

/*
** FUNCTION:	KRN_initFlags
**
** DESCRIPTION:	Initialise an event flag cluster
**
** RETURNS:	void
*/
void KRN_initFlags(KRN_FLAGCLUSTER_T * cluster)
{
	PARACHECK();
	PARADEL(Flag, cluster);
#ifdef CONFIG_FEATURE_IMPEXP
	cluster->impexp.thread = (-1);
	cluster->impexp.sId = 0;
	cluster->impexp.objtype = KRN_OBJ_EFC;
	cluster->impexp.unused = 0;
#endif
	cluster->flags = 0;
	DQ_init(&cluster->waitq);

	PARAADD(Flag, cluster);
	PARACHECK();
}

/*
** FUNCTION:	KRN_testFlags
**
** DESCRIPTION:	Test and optionally clear event flags
**
** RETURNS:	uint32_t - flag bitmask (prioir to clearing )
**
*/
uint32_t KRN_testFlags(KRN_FLAGCLUSTER_T * cluster, uint32_t mask,
		       KRN_FLAGOP_T test, int32_t clearFlags, int32_t timeout)
{
	PARACHECK();

	IRQ_IPL_T oldipl;
	uint32_t result;
	KRN_TIMER_T timer;

	oldipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	result =
	    KRN_testFlagsProtected(cluster, mask, test, clearFlags, &oldipl);
	_KRN_restoreIPLWithTimeout(oldipl, &timer, timeout);

	PARACHECK();

	return result;
}

/*
* A variant of KRN_testFlags for use at elevated IPL.
*
*/
uint32_t KRN_testFlagsProtected(KRN_FLAGCLUSTER_T * cluster,
				uint32_t mask, KRN_FLAGOP_T test,
				int32_t clearFlags, IRQ_IPL_T * lowerIPL)
{
	KRN_TASK_T *t;
	int32_t result = 0;
	int32_t status = 1;

	if (KRN_intsDisabled(*lowerIPL)) {	/* ints disabled or in an ISR... */
		/* either succeed immediately or fail since we are elevated IPL
		   we can do a quick non-blocking test */
		result = cluster->flags;
#ifdef CONFIG_FEATURE_IMPEXP
		if (cluster->impexp.thread >= 0)
			return 0;	/* can't test imp/exp semaphore at elevated IPL */
#endif
		/* Only bother checking the test if we need to clear the flags */
		if (clearFlags && _KRN_flgTest(cluster->flags, mask, test))
			cluster->flags &= ~mask;

		return result;
	}
#ifdef CONFIG_FEATURE_IMPEXP
	if (cluster->impexp.thread >= 0)	/* imp/exp with  another processor */
		return testImportedFlags(cluster, mask, test, clearFlags,
					 lowerIPL);
#endif

	t = _KRN_current;
	while (!_KRN_flgTest(cluster->flags, mask, test)) {
		if (t->timedOut) {
			status = 0;
			break;
		}
		/* set up details, then hold on cluster queue at lower IPL */
		_KRN_hibernateTask(t, &cluster->waitq);
		/* An inter-thread race condition means that we must append to the cluster queue
		 * immediately. We can't wait for the scheduler to do it for us */
		DQ_remove(t);
		DQ_addTail(&cluster->waitq, t);

		t->p1.testPar = mask;
		t->p2.testType = test;
		KRN_scheduleUnprotect(*lowerIPL);

		/* at this point we *may* be able to "claim" the flags. However, we must
		   first raise the IPL and test again.
		 */
		*lowerIPL = IRQ_raiseIPL();	/* current IPL may have bee manipulated by another task! */
	}
	result = cluster->flags;
	/* If we didn't time out, clear the flags as needed */
	if (status)
		if (clearFlags)
			cluster->flags &= ~mask;
	return result;
}

#ifdef CONFIG_FEATURE_IMPEXP
static uint32_t testImportedFlags(KRN_FLAGCLUSTER_T * cluster,
				  uint32_t mask, KRN_FLAGOP_T test,
				  int32_t clearFlags, IRQ_IPL_T * lowerIPL)
{
	KRN_TASK_T *t;
	int32_t status = 0;
	uint32_t subcmd;
	uint32_t result = 0;

	/* cluster is imported so
	   1) we wait on the cluster queue
	   2) we derive the message sequence number from the cluster flags
	   and send the appropriate "test flags" message
	   3) either an ACK or a later async NOTIFY will remove us from the queue
	   4) After removal, t->blockpars.seqStatus will contain success/fail indication and
	   if successful t->blockpars.testval will contain the cluster flags value (prior to clear)
	 */
	t = _KRN_current;
	while (!status) {
		subcmd = t->timedOut ? 0 : KRN_SUBCOMMAND_WAIT;
		if (clearFlags)
			subcmd |= KRN_SUBCOMMAND_FLAGCLUSTER_CLEAR;
		if (test == KRN_ANY)
			subcmd |= KRN_SUBCOMMAND_FLAGCLUSTER_ANY;

		_KRN_waitOnSequence(t, &cluster->waitq, &cluster->flags,
				    &cluster->impexp,
				    KRN_COMMAND_FLAGCLUSTER_TEST, subcmd, mask,
				    0);
		KRN_scheduleUnprotect(*lowerIPL);

		*lowerIPL = IRQ_raiseIPL();	/* current IPL might have been manipulated by another task */
		/* ackStatus will be one of
		 *                      1: semaphore test succeeded
		 *                      0: semaphore test failed, but a NOTIFY means it's worth trying again
		 *              other: semaphore test failed due to timeout while waiting for notify
		 */
		if (t->p2.ackStatus == 1) {	/* IPM request ACKed with success */
			status = 1;
		} else if (t->timedOut) {	/* Timed out */
			status = 0;
			break;
		}
	}
	if (status)		/* On success, report result */
		result = t->p1.testResult;
	return result;
}
#endif
/*
** FUNCTION:	KRN_setFlags
**
** DESCRIPTION:	Set event flags
**
** RETURNS:	void
**
*/
void KRN_setFlags(KRN_FLAGCLUSTER_T * cluster, uint32_t mask)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

#ifdef CONFIG_FEATURE_IMPEXP
	if (cluster->impexp.thread >= 0) {
		/* imported cluster */
		setImport(cluster, mask, KRN_COMMAND_FLAGCLUSTER_SET);
		return;
	}
#endif
	oldipl = IRQ_raiseIPL();

	cluster->flags |= mask;
	flagSchedule(cluster);	/* common core for set/clear/toggle flags */

	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:	KRN_toggleFlags
**
** DESCRIPTION:	 Invert event flags
**
** RETURNS:	void
**
*/
void KRN_toggleFlags(KRN_FLAGCLUSTER_T * cluster, uint32_t mask)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

#ifdef CONFIG_FEATURE_IMPEXP
	if (cluster->impexp.thread >= 0) {
		/* imported cluster */
		setImport(cluster, mask, KRN_COMMAND_FLAGCLUSTER_TOGGLE);
		return;
	}
#endif
	oldipl = IRQ_raiseIPL();

	cluster->flags ^= mask;
	flagSchedule(cluster);	/* common core for set/clear/toggle flags */

	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:	KRN_clearFlags
**
** DESCRIPTION:	 Set event flags
**
** RETURNS:	void
**
*/
void KRN_clearFlags(KRN_FLAGCLUSTER_T * cluster, uint32_t mask)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

#ifdef CONFIG_FEATURE_IMPEXP
	if (cluster->impexp.thread >= 0) {
		/* imported cluster */
		setImport(cluster, mask, KRN_COMMAND_FLAGCLUSTER_CLEAR);
		return;
	}
#endif
	oldipl = IRQ_raiseIPL();
	cluster->flags &= ~mask;

	/* At present KRN_clearFlags has no scheduling consequences, but it would if we were ever
	   to introduce a "test for flag clear" function. Uncomment the following if that ever happens
	 */
	/* flagSchedule(cluster); *//* common core for set/clear/toggle flags */

	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:	flagSchedule
**
** DESCRIPTION:	Common core for KRN_setFlags, KRN_clearFlags and KRN_toggleFlags
**
** RETURNS:	void
**
*/
static void flagSchedule(KRN_FLAGCLUSTER_T * cluster)
{
	KRN_TASK_T *t;
	KRN_TASK_T *st;
	KRN_TASKQ_T *wq;
	int32_t woken;

#ifdef CONFIG_FEATURE_IMPEXP
	uint8_t sId;
	uint32_t notifyMask;
#endif
	/* This runs with INTS disabled */

	wq = &cluster->waitq;

	/* Wake *all* waiting tasks for whom the cluster value is acceptable.
	   This allows the scheduler's priority system to arbitrate
	   over who actually gets the resource in the event of contention. (Event flag "resources" will be
	   granted according to task priority, not just queueing order). This is also part of
	   the process for making KRN_testFlagsProtected work properly. This could be inefficient
	   when several tasks are waiting on the same cluster, but this is rare. The extra code
	   for a more efficient approach actually ends up worse than this simpler approach (believe
	   me i've tried!). There are all kinds of potential problems around pre-emptive task removal
	   so I would strongly advise against trying to change this implementation */

	woken = 0;
	t = (KRN_TASK_T *) DQ_first(wq);
	if (t != NULL) {
		while ((void *)t != (void *)wq) {
			st = (KRN_TASK_T *) DQ_next(t);	/* successor task */
			if (_KRN_flgTest
			    (cluster->flags, t->p1.testPar,
			     (KRN_FLAGOP_T) t->p2.testType)) {
				DQ_remove(t);	/* remove from semaphore queue */
				woken |= _KRN_wakeTask(t);
			}
			t = st;
		}
	}
#ifdef CONFIG_FEATURE_IMPEXP
	/*
	 * if any other thread has a pending interest,
	 * send an asynchronous notify messages
	 */
	if (((sId = cluster->impexp.sId) > 0) &&
	    (notifyMask = _KRN_schedule->exportTable[sId - 1].notifyMask)) {
		/* the asynch notify function is accessed indirectly via a pointer.
		 * This is part of the system for avoiding linking unnecessary code
		 * in systems that don't use import/export
		 */
		_KRN_schedule->notifyFunc(notifyMask, sId, cluster->flags);
		_KRN_schedule->exportTable[sId - 1].notifyMask = 0;
	}
#endif
	if (woken)
		KRN_scheduleProtected();
}

/*
** FUNCTION:	setImport
**
** DESCRIPTION:	Common core for KRN_setFlags, KRN_clearFlags and KRN_toggleFlags
**				when cluster is imported
**
** RETURNS:	void
**
*/
#ifdef CONFIG_FEATURE_IMPEXP
static void setImport(KRN_FLAGCLUSTER_T * cluster, uint32_t mask, uint32_t cmd)
{
	IRQ_IPL_T oldipl;
	KRN_TASK_T *t;

	/* cluster is imported so
	   1) we wait on the cluster queue
	   2) we derive the message sequence number from the cluster value
	   and send the appropriate cluster message
	   3) the ACK will remove us from the queue
	   You might think we don't need to wait for the ack, but doing so
	   guarantees that the cluster has actually been set before we carry on
	 */
	oldipl = IRQ_raiseIPL();
	t = _KRN_current;
	_KRN_waitOnSequence(t, &cluster->waitq, &cluster->flags,
			    &cluster->impexp, cmd, 0, mask, 0);
	KRN_scheduleUnprotect(oldipl);
	return;
}

#endif
