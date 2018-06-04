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
*   Description:	Kernel semaphores
*
*************************************************************************/

#include "meos/config.h"
#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"

PARATYPE(Sema, KRN_SEMAPHORE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

#ifdef CONFIG_FEATURE_IMPEXP
static int32_t _KRN_testImportedSemaphore(KRN_SEMAPHORE_T * semaphore,
					  uint32_t testvalue,
					  IRQ_IPL_T * lowerIPL);

inline static int32_t _KRN_ccbTestCmd(KRN_OBJID_T objtype)
{
	return (objtype == KRN_OBJ_SEM) ? KRN_COMMAND_SEMAPHORE_TEST :
	    (objtype == KRN_OBJ_LOCK) ? KRN_COMMAND_LOCK :
	    (objtype == KRN_OBJ_MBX) ? KRN_COMMAND_MAILBOX_GET :
	    (objtype == KRN_OBJ_POOL) ? KRN_COMMAND_POOL_TAKE : 0;
}

inline static int32_t _KRN_ccbSetCmd(KRN_OBJID_T objtype)
{
	return (objtype == KRN_OBJ_SEM) ? KRN_COMMAND_SEMAPHORE_SET :
	    (objtype == KRN_OBJ_LOCK) ? KRN_COMMAND_UNLOCK :
	    (objtype == KRN_OBJ_MBX) ? KRN_COMMAND_MAILBOX_PUT :
	    (objtype == KRN_OBJ_POOL) ? KRN_COMMAND_POOL_RETURN : 0;
}
#endif
/*
** FUNCTION:	KRN_initSemaphore
**
** DESCRIPTION:	Initialise a semaphore
**
** RETURNS:	void
*/
void KRN_initSemaphore(KRN_SEMAPHORE_T * semaphore, uint32_t value)
{
	PARACHECK();
	PARADEL(Sema, semaphore);
#ifdef CONFIG_FEATURE_IMPEXP
	semaphore->impexp.thread = (-1);
	semaphore->impexp.sId = 0;
	semaphore->impexp.objtype = KRN_OBJ_SEM;
	semaphore->impexp.unused = 0;
#endif
	semaphore->value = value;
	DQ_init(&semaphore->waitq);

	PARAADD(Sema, semaphore);
	PARACHECK();
}

/*
** FUNCTION:	KRN_setSemaphore
**
** DESCRIPTION:	Increment a semaphore
**
** RETURNS:	void
**
*/
void KRN_setSemaphore(KRN_SEMAPHORE_T * semaphore, uint32_t increment)
{
	PARACHECK();

	KRN_TASK_T *st;
	KRN_TASKQ_T *wq;
	KRN_TASK_T *t;
	IRQ_IPL_T oldipl;
	int32_t woken;
#ifdef CONFIG_FEATURE_IMPEXP
	uint8_t sId;
	uint32_t notifyMask;
#endif

	oldipl = IRQ_raiseIPL();

	wq = &semaphore->waitq;
#ifdef CONFIG_FEATURE_IMPEXP
	if (semaphore->impexp.thread >= 0) {
		KRN_OBJID_T objtype;
		/* sempahore is imported so
		   1) we wait on the semaphore queue
		   2) we derive the message sequence number from the semaphore value
		   and send the appropriate set semaphore message
		   3) the ACK will remove us from the queue
		   You might think we don't need to wait for the ack, but doing so
		   guarantees that the semaphore has actually been set before we carry on
		 */
		objtype = semaphore->impexp.objtype;
		t = _KRN_current;
		_KRN_waitOnSequence(t, &semaphore->waitq,
				    (uint32_t *) & semaphore->value,
				    &semaphore->impexp, _KRN_ccbSetCmd(objtype),
				    0, ((objtype == KRN_OBJ_MBX)
					|| (objtype ==
					    KRN_OBJ_POOL)) ? t->
				    p1.testPar : (uint32_t) increment, 0);
		IRQ_restoreIPL(oldipl);
		return;
	}
#endif

	semaphore->value += increment;

	/* Wake all waiting tasks for whom the semaphore value is acceptable.
	   This allows the scheduler's priority system to arbitrate
	   over who actually gets the resource in the event of contention. (Semaphore resources will be
	   granted according to task priority, not just queuing order). This is also part of
	   the process for making KRN_testSemaphoreProtected work properly. This could be inefficient
	   when several tasks are waiting on the same semaphore, but this is rare. The extra code
	   for a more efficient approach actually ends up worse than this simpler approach (believe
	   me I've tried!). There are all kinds of potential problems around pre-emptive task removal
	   so I would strongly advise against trying to change this implementation */

	woken = 0;
	t = (KRN_TASK_T *) DQ_first(wq);
	if (t != NULL) {
		while ((void *)t != (void *)wq) {
			st = (KRN_TASK_T *) DQ_next(t);	/* successor task */
			if (t->p1.testPar <= semaphore->value) {
				DQ_remove(t);	/* remove from semaphore queue */
				woken |= _KRN_wakeTask(t);
			}
			t = st;
		}
	}

	/*
	 * if  any other thread has a pending interest,
	 * send an asynchronous notify messages
	 */
#ifdef CONFIG_FEATURE_IMPEXP
	if (((sId = semaphore->impexp.sId) > 0) &&
	    (notifyMask = _KRN_schedule->exportTable[sId - 1].notifyMask)) {
		/* the asynch notify function is accessed indirectly via a pointer.
		 * This is part of the system for avoiding linking unnecessary code
		 * in systems that don't use import/export
		 */
		_KRN_schedule->notifyFunc(notifyMask, sId, semaphore->value);
		_KRN_schedule->exportTable[sId - 1].notifyMask = 0;
	}
#endif

	/* If necessary, cause a scheduler event */
	if (woken)
		KRN_scheduleUnprotect(oldipl);
	else
		IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:	KRN_testSemaphore
**
** DESCRIPTION:	Test and decrement a semaphore
**
** RETURNS:	int32_t -	1:  success
**					0: failure
**
*/
int32_t KRN_testSemaphore(KRN_SEMAPHORE_T * semaphore, uint32_t testvalue,
			  int32_t timeout)
{
	PARACHECK();

	KRN_TIMER_T timer;
	IRQ_IPL_T oldipl;
	int32_t status;

	oldipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	status = KRN_testSemaphoreProtected(semaphore, testvalue, &oldipl);
	_KRN_restoreIPLWithTimeout(oldipl, &timer, timeout);

	PARACHECK();

	return status;
}

/*
* A variant of KRN_testSemaphore for use at elevated IPL.
*
*/
int32_t KRN_testSemaphoreProtected(KRN_SEMAPHORE_T * semaphore,
				   uint32_t testvalue, IRQ_IPL_T * lowerIPL)
{
	PARACHECK();

	KRN_TASK_T *t;
	int32_t status = 1;

	if (KRN_intsDisabled(*lowerIPL) || IRQ_servicing()) {	/* interrupts disabled or in an ISR... */
		/* either succeed immediately or fail - since we are at elevated IPL,
		   we can do a quick non-blocking test */
#ifdef CONFIG_FEATURE_IMPEXP
		if (semaphore->impexp.thread >= 0)
			status = 0;	/* test imp/exp semaphore always fails at elevated IPL */
		else
#endif
		if (semaphore->value >= testvalue)
			semaphore->value -= testvalue;
		else
			status = 0;
		PARACHECK();
		return status;
	}
#ifdef CONFIG_FEATURE_IMPEXP
	if (semaphore->impexp.thread >= 0) {
		status =
		    _KRN_testImportedSemaphore(semaphore, testvalue, lowerIPL);

		return status;
	}
#endif

	t = _KRN_current;
	while (semaphore->value < testvalue) {
		if (t->timedOut) {
			status = 0;
			break;
		}
		/* set up details, then hold on semaphore queue at lower IPL */
		_KRN_hibernateTask(t, &semaphore->waitq);
		/* An inter-thread race condition means that we must append to the semaphore queue
		 * immediately. We can't wait for the scheduler to do it for us */
		DQ_remove(t);
		DQ_addTail(&semaphore->waitq, t);

		t->p1.testPar = testvalue;
		KRN_scheduleUnprotect(*lowerIPL);

		/* at this point we *may* be able to claim the semaphore. However, we must
		   first raise the IPL and test again.
		 */
		*lowerIPL = IRQ_raiseIPL();	/* current IPL might have been manipulated by another task */
	}

	if (status)
		semaphore->value -= testvalue;

	PARACHECK();
	return status;
}

/*
* Test imported semaphore - invoked at elevated IPL.
*
*/
#ifdef CONFIG_FEATURE_IMPEXP
static int32_t _KRN_testImportedSemaphore(KRN_SEMAPHORE_T * semaphore,
					  uint32_t testvalue,
					  IRQ_IPL_T * lowerIPL)
{
	KRN_TASK_T *t;
	int32_t status = 0;

	/* sempahore is imported so
	   1) we wait on the semaphore queue
	   2) we derive the message sequence number from the semaphore value
	   and send the appropriate "test semaphore" message
	   3) either an ACK or a later asynch NOTIFY will remove us from the queue
	   4) After removal, t->blockpars.seqStatus will contain success/fail indication and
	   if successful t->blockpars.testVal will contain a pointer to the mailbox message or
	   pool item if relevant
	 */
	t = _KRN_current;
	while (!status) {
		_KRN_waitOnSequence(t, &semaphore->waitq,
				    (uint32_t *) & semaphore->value,
				    &semaphore->impexp,
				    _KRN_ccbTestCmd(semaphore->impexp.objtype),
				    t->timedOut ? 0 :
				    KRN_SUBCOMMAND_WAIT, testvalue, 0);
		KRN_scheduleUnprotect(*lowerIPL);

		*lowerIPL = IRQ_raiseIPL();	/* current IPL might have been manipulated by another task */
		/* ackStatus will be one of
		 *                      1: semaphore test succeeded
		 *                      0: semaphore test failed, but a NOTIFY means it's worth trying again
		 *              other: semaphore test failed due to timeout while waiting for notify
		 */
		if (t->p2.ackStatus == 1) {	/* IPM request ACKed with success */
			status = 1;
		} else if (t->timedOut) {
			status = 0;
			break;
		}
	}

	return status;
}
#endif
