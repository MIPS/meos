/***(C)2003***************************************************************
*
* Copyright (C) 2003 MIPS Tech, LLC
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
****(C)2003**************************************************************/

/*************************************************************************
*
*   Description:	Import/export
*
*************************************************************************/

#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"

#ifndef CONFIG_FEATURE_IMPEXP

int32_t KRN_export(uint8_t exportId, void *object)
{
	DBG_assert(0, "Import/Export disabled\n");
	return 0;
}

int32_t KRN_import(int32_t thread, uint8_t exportId, void *object)
{
	DBG_assert(0, "Import/Export disabled\n");
	return 0;
}

void KRN_installImpExp(uint8_t maxImpIds[CONFIG_FEATURE_MAX_PROCESSORS],
		       uint8_t maxExpId, KRN_IMPORT_T * impTable,
		       KRN_EXPORT_T * expTable)
{
	DBG_assert(0, "Import/Export disabled\n");
}

int32_t KRN_resetExport(uint8_t exportId)
{
	DBG_assert(0, "Import/Export disabled\n");
	return 0;
}

int32_t KRN_resetImport(uint8_t threadId, uint8_t exportId)
{
	DBG_assert(0, "Import/Export disabled\n");
	return 0;
}

#else

#include "meos/ipm/ipm.h"

PARATYPE(MSGT, KRN_MSG_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(IMPT, KRN_IMPORT_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(EXPT, KRN_EXPORT_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/* local macros */
#define _KRN_IPM_ACK(CMD, SUBCMD, p64a) _KRN_IPM_ACKorNACK((CMD), (SUBCMD),KRN_RESPONSE_ACK, (p64a))
#define _KRN_IPM_NACK(CMD) _KRN_IPM_ACKorNACK((CMD), KRN_SUBCOMMAND_FAIL, KRN_RESPONSE_NACK, 0)

/* local types */
typedef void FLAGFUNC_T(KRN_FLAGCLUSTER_T * cluster, uint32_t mask);
typedef void APIFUNC_T(KRN_MSG_T * message);
typedef APIFUNC_T *APIFUNCPTR_T;

/* prototype for function borrowed from efc.c - check the definition matches */
int32_t _KRN_flgTest(uint32_t flags, uint32_t mask, KRN_FLAGOP_T test);

/* externally accessible prototypes */
void _KRN_recv(KRN_MSG_T * message);
KRN_TASK_T *_KRN_syncExported(KRN_SYNC_T * sync);

/* local prototypes */
void _KRN_notify(uint32_t threadMask, uint8_t exportId, uint32_t objValue);
void _KRN_request(KRN_IMPEXP_T * impexp, uint32_t command, uint32_t subcmd,
		  uint32_t seq, uint64_t p64a, uint64_t p64b);

static void _KRN_IPM_ack(KRN_MSG_T * message);
static void _KRN_IPM_nack(KRN_MSG_T * message);
static void _KRN_IPM_notify(KRN_MSG_T * message);
static void _KRN_IPM_semTest(KRN_MSG_T * message);
static void _KRN_IPM_semSet(KRN_MSG_T * message);
static void _KRN_IPM_efcTest(KRN_MSG_T * message);
static void _KRN_IPM_efcSet(KRN_MSG_T * message);
static void _KRN_IPM_sync(KRN_MSG_T * message);
static void _KRN_IPM_wqQueue(KRN_MSG_T * message);
static void _KRN_IPM_memAnnounce(KRN_MSG_T * message);
static void _KRN_IPM_importProbe(KRN_MSG_T * message);

static void _KRN_decodeResponse(KRN_MSG_T * message, uint32_t * apiFunc,
				uint32_t * subCmd, uint32_t * thread,
				uint32_t * sId, void **obj, int32_t * success,
				uint32_t * seq);
static void _KRN_IPM_ACKorNACK(KRN_MSG_T * message, uint32_t subCmd,
			       uint32_t ACKorNACK, uint32_t p64a);
static void *_KRN_locateExport(KRN_MSG_T * message, int32_t * serverId);

/* local variables constant api function lookup table */
static const APIFUNCPTR_T apiTable[] = {
	_KRN_IPM_ack,
	_KRN_IPM_nack,
	_KRN_IPM_notify,
	_KRN_IPM_semTest,
	_KRN_IPM_semSet,
	_KRN_IPM_semTest,
	_KRN_IPM_semSet,
	_KRN_IPM_efcTest,
	_KRN_IPM_efcSet,
	_KRN_IPM_efcSet,
	_KRN_IPM_efcSet,
	_KRN_IPM_semTest,
	_KRN_IPM_semSet,
	_KRN_IPM_semTest,
	_KRN_IPM_semSet,
	_KRN_IPM_sync,
	_KRN_IPM_wqQueue,
	_KRN_IPM_memAnnounce,
	_KRN_IPM_importProbe
};

/*
** FUNCTION:      KRN_installImpExp
**
** DESCRIPTION:   Prepare the import/export system for use by
**                setting up pointers to the tables and initial values
**
** RETURNS:       void
*/
void KRN_installImpExp(uint8_t maxImpIds[CONFIG_FEATURE_MAX_PROCESSORS],
		       uint8_t maxExpId, KRN_IMPORT_T * impTable,
		       KRN_EXPORT_T * expTable)
{
	int32_t n;
	int32_t t;

	PARACHECK();

	/* Install import tables into _KRN_schedule */
	for (t = 0; t < CONFIG_FEATURE_MAX_PROCESSORS; t++) {
		if ((maxImpIds[t] > 0) && (t != _KRN_schedule->hwThread)) {
			_KRN_schedule->importTables[t] = impTable;
			_KRN_schedule->maxImpIds[t] = maxImpIds[t];
			for (n = 0; n < maxImpIds[t]; n++, impTable++) {
				PARADEL(IMPT, impTable);
				impTable->objPtr = NULL;
				impTable->owner = 0;
				PARAADD(IMPT, impTable);
			}
		}
	}
	/* And export tables if needed */
	if (maxExpId > 0) {
		_KRN_schedule->exportTable = expTable;
		_KRN_schedule->maxExpId = maxExpId;
		for (n = 0; n < maxExpId; n++, expTable++) {
			PARADEL(EXPT, expTable);
			expTable->objPtr = NULL;
			expTable->notifyMask = 0;
			PARAADD(EXPT, expTable);
		}
	}
	/* Hook import/export into the kernel */
	_KRN_schedule->notifyFunc = _KRN_notify;
	_KRN_schedule->requestFunc = _KRN_request;
	_KRN_schedule->initFunc = IPM_start;

	PARACHECK();
}

/*
** FUNCTION:      KRN_import
**
** DESCRIPTION:   Import an object from a server thread
**
** RETURNS:       1: Success
**                0: Failure
*/
int32_t KRN_import(int32_t thread, uint8_t exportId, void *object)
{
	int32_t status = 0;
	KRN_SEMAPHORE_T *o = object;
	KRN_IMPORT_T *i;
	uint32_t seq = 0;
	KRN_TASKQ_T *wq;

	PARACHECK();

	if (((KRN_SEMAPHORE_T *) object)->impexp.objtype == KRN_OBJ_EFC)
		wq = &((KRN_FLAGCLUSTER_T *) object)->waitq;
	else if (((KRN_SEMAPHORE_T *) object)->impexp.objtype == KRN_OBJ_SYNC)
		wq = &((KRN_SYNC_T *) object)->waitq;
	else if (((KRN_WQ_T *) object)->impexp.objtype == KRN_OBJ_WQ)
		wq = &((KRN_WQ_T *) object)->mbox.sem.waitq;
	else
		wq = &((KRN_SEMAPHORE_T *) object)->waitq;

	/* sanity checks */
	if ((thread >= 0) && (thread <= CONFIG_FEATURE_MAX_PROCESSORS) &&
	    (thread != _KRN_schedule->hwThread) &&
	    (exportId > 0) && (exportId <= _KRN_schedule->maxImpIds[thread]) &&
	    (o->impexp.thread == (-1)) && (o->impexp.sId == 0)) {
		i = _KRN_schedule->importTables[thread] + (exportId - 1);
		if (i->objPtr == NULL)	/* not already imported to some other object */
			switch (o->impexp.objtype) {
			case KRN_OBJ_SEM:
			case KRN_OBJ_LOCK:
			case KRN_OBJ_MBX:
			case KRN_OBJ_EFC:
			case KRN_OBJ_POOL:
			case KRN_OBJ_SYNC:
			case KRN_OBJ_WQ:
				o->impexp.thread = thread;
				o->impexp.sId = exportId;
				i->objPtr = o;
				i->owner = thread;
				status = 1;

			default:
				/* do nothing */ ;
			}
	}

	if (status) {
		_KRN_current->p2.ackStatus = 0;
		IRQ_IPL_T ipl = IRQ_raiseIPL();
		_KRN_waitOnSequence(_KRN_current, wq, &seq, &o->impexp,
				    KRN_COMMAND_IMPORT_PROBE,
				    KRN_SUBCOMMAND_WAIT, 0, 0);
		KRN_scheduleUnprotect(ipl);
	}

	PARACHECK();
	return status;
}

/*
** FUNCTION:      KRN_export
**
** DESCRIPTION:   Export an object to client threads thread
**
** RETURNS:       void
*/
int32_t KRN_export(uint8_t exportId, void *object)
{
	int32_t status = 0;
	KRN_SEMAPHORE_T *o = object;
	KRN_EXPORT_T *e;

	PARACHECK();

	/* sanity checks */
	if ((exportId > 0) && (exportId <= _KRN_schedule->maxExpId) &&	/* valid export Id                   */
	    (o->impexp.sId == 0) &&	/* not already imported or exported          */
	    (o->impexp.thread == (-1))) {
		e = _KRN_schedule->exportTable + (exportId - 1);
		if (e->objPtr == NULL)	/* not already imported to some other object */
			switch (o->impexp.objtype) {
			case KRN_OBJ_SEM:
			case KRN_OBJ_LOCK:
			case KRN_OBJ_MBX:
			case KRN_OBJ_EFC:
			case KRN_OBJ_POOL:
			case KRN_OBJ_SYNC:
			case KRN_OBJ_WQ:
				o->impexp.sId = exportId;
				e->objPtr = o;
				e->notifyMask = 0;
				status = 1;
				if (o->impexp.objtype == KRN_OBJ_POOL) {
					/* put export info into all pool items so they can find
					   their way home */
					KRN_POOLLINK_T *item;
					item = LST_first(&(((KRN_POOL_T *)
							    o)->freeList));
					while (item) {
						item->expinfo.sId = exportId;
						item->expinfo.thread =
						    _KRN_schedule->hwThread;
						item = LST_next(item);
					}
				}
				_KRN_notify((uint32_t) - 1, exportId, 0);
			default:
				/* do nothing */ ;
			}
	}

	PARACHECK();
	return status;
}

/*
** FUNCTION:      KRN_resetImport
**
** DESCRIPTION:   Reset an imported object, allowing it to be reused
**
** RETURNS:       1: Success
**                0: Failure
*/
int32_t KRN_resetImport(uint8_t threadId, uint8_t exportId)
{
	PARACHECK();

	KRN_IMPORT_T *i = _KRN_schedule->importTables[threadId] + exportId;
	if (i) {
		i->objPtr = NULL;
		i->owner = 0;

		PARACHECK();
		return 1;
	}

	PARACHECK();
	return 0;
}

/*
** FUNCTION:      KRN_resetExport
**
** DESCRIPTION:   Reset an exported object, allowing it to be reused
**
** RETURNS:       1: Success
**                0: Failure
*/
int32_t KRN_resetExport(uint8_t exportId)
{
	PARACHECK();

	KRN_EXPORT_T *e = _KRN_schedule->exportTable + exportId;
	if (e) {
		e->objPtr = NULL;
		e->notifyMask = 0;

		PARACHECK();
		return 1;
	}

	PARACHECK();
	return 0;
}

/*
** FUNCTION:      _KRN_notify
**
** DESCRIPTION:   Send asynchronous notify messages to interested threads
**
** RETURNS:       void
*/
void _KRN_notify(uint32_t threadMask, uint8_t exportId, uint32_t objValue)
{
	KRN_MSG_T message;
	int32_t thread;

	PARACHECK();

	message.from = _KRN_schedule->hwThread;
	message.seq = 0;
	message.cmd = KRN_RESPONSE_NOTIFY;
	message.subCmd = KRN_SUBCOMMAND_OK;
	message.cID = 0;
	message.sID = exportId;
	message.p64a = objValue;
	message.p64b = 0;

	PARAADD(MSGT, &message);

	for (thread = 0; thread < CONFIG_FEATURE_MAX_PROCESSORS;
	     thread++, threadMask >>= 1) {
		if ((thread != _KRN_schedule->hwThread) && (threadMask & 1)) {
			message.to = thread;
			IPM_send(&message);
		}
	}

	PARADEL(MSGT, &message);
	PARACHECK();
}

/*
** FUNCTION:      _KRN_request
**
** DESCRIPTION:   Send request to server thread
**
** RETURNS:       void
*/
void _KRN_request(KRN_IMPEXP_T * impexp, uint32_t command, uint32_t subcmd,
		  uint32_t seq, uint64_t p64a, uint64_t p64b)
{
	KRN_MSG_T message;
	int32_t thread;
	KRN_IMPORT_T *i;

	PARACHECK();

	thread = impexp->thread;
	i = _KRN_schedule->importTables[thread] + (impexp->sId - 1);
	message.from = _KRN_schedule->hwThread;
	message.to = i->owner;
	message.seq = seq;
	message.cmd = command;
	message.subCmd = subcmd;
	message.cID = 0;
	message.sID = impexp->sId;
	message.p64a = p64a;
	message.p64b = p64b;

	PARAADD(MSGT, &message);

	IPM_send(&message);

	PARADEL(MSGT, &message);
	PARACHECK();
}

/*
** FUNCTION:      _KRN_decodeIPMResponse
**
** DESCRIPTION:   Helper for common parts of IPM response decode
**
** RETURNS:       void
*/
static void _KRN_decodeResponse(KRN_MSG_T * message, uint32_t * apiFunc,
				uint32_t * subCmd, uint32_t * thread,
				uint32_t * sId, void **obj, int32_t * success,
				uint32_t * seq)
{
	KRN_IMPORT_T *i;

	PARACHECK();

	*apiFunc = message->p64b & 0xffff;
	*subCmd = (message->p64b >> 16) & 0xffff;
	*thread = message->from;
	*sId = message->sID;
	i = _KRN_schedule->importTables[*thread] + (*sId - 1);

	*obj = i->objPtr;
	*success = message->subCmd == KRN_SUBCOMMAND_OK;
	*seq = message->seq;

	PARACHECK();
}

/*
** FUNCTION:      _KRN_handleMessage
**
** DESCRIPTION:   Deal with incoming IPM messages
**
** RETURNS:       void
*/
void _KRN_recv(KRN_MSG_T * message)
{
	PARACHECK();

	_IPM_impexpDebug("RX", message);

	(*apiTable[message->cmd]) (message);

	PARACHECK();
}

/*
** FUNCTION:      _KRN_IPM_ack
**
** DESCRIPTION:   Process ACK response
**                - could come from almost any command
**
** RETURNS:       void
*/
static void _KRN_IPM_ack(KRN_MSG_T * message)
{
	uint32_t thread;
	uint32_t sId;
	uint32_t apiFunc;
	uint32_t subCmd;
	uint32_t seq;
	int32_t success;
	int32_t woken;
	void *obj;
	KRN_TASK_T *t;
	KRN_TASK_T *st;
	KRN_TASKQ_T *wq;

	_KRN_decodeResponse(message, &apiFunc, &subCmd, &thread, &sId, &obj,
			    &success, &seq);

	DBG_assert(obj != NULL, "Got ACK for NULL!\n");

	/*
	 * 1) search the list of tasks waiting on the object for the one with the right
	 *    request sequence number.
	 * 2) If  SUCCESS || (FAILURE && NO WAIT)
	 *         wake up the task and inform it of success/failure
	 *    ELSE (FAILURE && WAIT)
	 *         clobber the sequence number so we don't consider the task for any future ACKS.
	 *         It will now sit on the queue until a suitable NOTIFY comes along
	 */
	if (((KRN_SEMAPHORE_T *) obj)->impexp.objtype == KRN_OBJ_EFC)
		wq = &((KRN_FLAGCLUSTER_T *) obj)->waitq;
	else if (((KRN_SEMAPHORE_T *) obj)->impexp.objtype == KRN_OBJ_SYNC)
		wq = &((KRN_SYNC_T *) obj)->waitq;
	else if (((KRN_WQ_T *) obj)->impexp.objtype == KRN_OBJ_WQ)
		wq = &((KRN_WQ_T *) obj)->mbox.sem.waitq;
	else
		wq = &((KRN_SEMAPHORE_T *) obj)->waitq;
	t = DQ_first(wq);
	woken = 0;
	if (t != NULL) {
		while ((void *)t != (void *)wq) {
			st = DQ_next(t);	/* successor task */
			if (t->p2.seqNum == seq) {
				if ((apiFunc == KRN_COMMAND_FLAGCLUSTER_TEST) ||
				    (success
				     && ((apiFunc == KRN_COMMAND_MAILBOX_GET)
					 || (apiFunc ==
					     KRN_COMMAND_POOL_TAKE))))
					/* ACK carried a significant value in p64a */
					t->p1.testResult = message->p64a;
				if (success || !(subCmd & KRN_SUBCOMMAND_WAIT)) {
					DQ_remove(t);	/* remove from semaphore queue */
					t->p2.ackStatus = success;
					woken |= _KRN_wakeTask(t);
				} else {
					/* Leave task where it is (to be woken by a future NOTIFY).
					   However, we "disable" the sequence number so that if we
					   wait long enough for sequence numbers to wrap around, we won't
					   target this task again

					   Note: ackStatus and seqNum are "unioned" together.
					 */
					t->p2.ackStatus = 0xffffffff;
				}
				break;	/* break out of while - no need to look further */
			}
			t = st;
		}
	}
	if (woken)
		KRN_scheduleProtected();

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_nack
**
** DESCRIPTION:   Process NACK response
**                - could come from almost any command
**
** RETURNS:       void
*/
static void _KRN_IPM_nack(KRN_MSG_T * message)
{
	PARACHECK();

	/* in MEOS the only reason for a NACK is a design error
	   probably due to not matching imports and exports properly
	 */
	DBG_insist(0, "Unexpected NACK");
	(void)message;		/* suppress aggressive compiler warnings */

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_notify
**
** DESCRIPTION:   Process asynchronous NOTIFY response.
**                These are not related to any particular command.
**
** RETURNS:       void
*/
static void _KRN_IPM_notify(KRN_MSG_T * message)
{
	KRN_IMPORT_T *i;
	uint32_t s;
	uint32_t thread;
	void *obj;
	int32_t woken = 0;
	KRN_TASK_T *t;
	KRN_TASK_T *st;
	KRN_TASKQ_T *wq;

	PARACHECK();

	thread = message->from;
	s = message->sID;
	i = _KRN_schedule->importTables[thread] + (s - 1);
	obj = i->objPtr;

	if (!obj)
		return;

	/*
	 * Search the list of tasks waiting on the object
	 * and wake them all up. Note that we don't make the wakeup conditional
	 * on the object's "value" (included in the NOTIFY message), because we need
	 * the task to test again in order to set the thread bit in the server's
	 * notify mask. Without this, further NOTIFYs would not be issued.
	 *
	 * This crude and simple approach is what is behind the efficiency
	 * guidelines in the MEOS kernel users' guide
	 *
	 */
	if (((KRN_SEMAPHORE_T *) obj)->impexp.objtype == KRN_OBJ_EFC)
		wq = &((KRN_FLAGCLUSTER_T *) obj)->waitq;
	else if (((KRN_SEMAPHORE_T *) obj)->impexp.objtype == KRN_OBJ_SYNC)
		wq = &((KRN_SYNC_T *) obj)->waitq;
	else if (((KRN_WQ_T *) obj)->impexp.objtype == KRN_OBJ_WQ)
		wq = &((KRN_WQ_T *) obj)->mbox.sem.waitq;
	else
		wq = &((KRN_SEMAPHORE_T *) obj)->waitq;
	t = DQ_first(wq);
	if (t != NULL) {
		while ((void *)t != (void *)wq) {
			st = DQ_next(t);	/* successor task */
			DQ_remove(t);	/* remove from  queue */
			t->p2.ackStatus = 0;	/* failure */
			woken |= _KRN_wakeTask(t);
			t = st;
		}
	}
	if (woken)
		KRN_scheduleProtected();

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_semTest
**
** DESCRIPTION:   Process a "semaphore test" command
**
** RETURNS:       void
*/
static void _KRN_IPM_semTest(KRN_MSG_T * message)
{
	int32_t sId;
	uint32_t testVal;
	uintptr_t p;
	KRN_OBJID_T objtype;
	KRN_SEMAPHORE_T *s;

	PARACHECK();

	if ((s = _KRN_locateExport(message, &sId)) != NULL) {
		/* can't use KRN_testSemaphore at interrupt level, so we have to
		   poke around in the works */
		objtype = s->impexp.objtype;
		if ((objtype == KRN_OBJ_MBX) || (objtype == KRN_OBJ_POOL))
			testVal = 1;
		else
			testVal = message->p64a;
		if (s->value >= testVal) {
			if (objtype == KRN_OBJ_MBX)
				p = (uintptr_t)
				    LST_removeHead(&((KRN_MAILBOX_T *)
						     s)->items);
			else if (objtype == KRN_OBJ_POOL)
				p = (uintptr_t)
				    LST_removeHead(&((KRN_POOL_T *)
						     s)->freeList);
			else
				p = 0;
			/* seize OK */
			s->value -= testVal;
			_KRN_IPM_ACK(message, KRN_SUBCOMMAND_OK, p);
		} else {
			if (message->subCmd == KRN_SUBCOMMAND_WAIT) {
				/* client will be waiting so set notify mask */
				_KRN_schedule->exportTable[sId -
							   1].notifyMask |=
				    1 << message->from;
			}
			_KRN_IPM_ACK(message, KRN_SUBCOMMAND_FAIL, 0);
		}
	} else {
		_KRN_IPM_NACK(message);
	}

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_semSet
**
** DESCRIPTION:   Process a "semaphore set" command
**
** RETURNS:       void
*/

static void _KRN_IPM_semSet(KRN_MSG_T * message)
{
	int32_t sId;
	KRN_SEMAPHORE_T *s;
	KRN_OBJID_T objtype;
	uintptr_t p64a;

	PARACHECK();

	if ((s = _KRN_locateExport(message, &sId)) != NULL) {
		objtype = s->impexp.objtype;
		p64a = message->p64a;
		if (objtype == KRN_OBJ_MBX)
			KRN_putMbox((KRN_MAILBOX_T *) s, (void *)p64a);
		else if (objtype == KRN_OBJ_POOL)
			/* KRN_returnPool refreshes the cache for pool linkage fields! */
			KRN_returnPool((void *)p64a);
		else
			KRN_setSemaphore(s, p64a);
		_KRN_IPM_ACK(message, KRN_SUBCOMMAND_OK, 0);
	} else
		_KRN_IPM_NACK(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_sync
**
** DESCRIPTION:   Process a "sync" command
**
** RETURNS:       void
*/

static void _KRN_IPM_sync(KRN_MSG_T * message)
{
	int32_t sId;
	KRN_SYNC_T *s;

	PARACHECK();

	if ((s = _KRN_locateExport(message, &sId)) != NULL) {
		if (message->subCmd == KRN_SUBCOMMAND_WAIT) {
			/* client will be waiting so set notify mask */
			_KRN_schedule->exportTable[sId -
						   1].notifyMask |=
			    1 << message->from;
		}
		/* False fail - syncs always end with a notify */
		_KRN_IPM_ACK(message, KRN_SUBCOMMAND_FAIL, 0);
		_KRN_syncExported(s);

	} else
		_KRN_IPM_NACK(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_wqQueue
**
** DESCRIPTION:   Process a "queue" command
**
** RETURNS:       void
*/

static void _KRN_IPM_wqQueue(KRN_MSG_T * message)
{
	int32_t sId;
	KRN_WQ_T *s;

	PARACHECK();

	if ((s = _KRN_locateExport(message, &sId)) != NULL) {
		_KRN_IPM_ACK(message,
			     KRN_queueWQ(s, (KRN_TASKFUNC_T *) (uintptr_t)
					 message->p64a,
					 (void *)(uintptr_t) message->p64b,
					 "Remote job",
					 0) ? KRN_SUBCOMMAND_OK :
			     KRN_SUBCOMMAND_FAIL, 0);
	} else
		_KRN_IPM_NACK(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_efcTest
**
** DESCRIPTION:   Process a "event flag cluster test" command
**
** RETURNS:       void
*/
static void _KRN_IPM_efcTest(KRN_MSG_T * message)
{
	int32_t sId;
	int32_t testMask;
	KRN_FLAGOP_T testType;
	int32_t clearFlags;
	uint32_t flags;
	uint32_t subcmd;
	KRN_FLAGCLUSTER_T *cluster;

	PARACHECK();

	if ((cluster = _KRN_locateExport(message, &sId)) != NULL) {
		/* can't use KRN_testFlags at interrupt level, so we have to
		   poke around in the works */
		testMask = message->p64a;
		subcmd = message->subCmd;
		testType =
		    (subcmd & KRN_SUBCOMMAND_FLAGCLUSTER_ANY) ? KRN_ANY :
		    KRN_ALL;
		clearFlags =
		    (subcmd & KRN_SUBCOMMAND_FLAGCLUSTER_CLEAR) ? 1 : 0;
		flags = cluster->flags;
		if (_KRN_flgTest(flags, testMask, testType)) {
			/* success */
			if (clearFlags)
				cluster->flags &= ~testMask;
			_KRN_IPM_ACK(message, KRN_SUBCOMMAND_OK, flags);
		} else {
			if (subcmd & KRN_SUBCOMMAND_WAIT) {
				/* client will be waiting so set notify mask */
				_KRN_schedule->exportTable[sId -
							   1].notifyMask |=
				    1 << message->from;
			}
			_KRN_IPM_ACK(message, KRN_SUBCOMMAND_FAIL, flags);
		}
	} else
		_KRN_IPM_NACK(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_efcSet
**
** DESCRIPTION:   Process a event flag cluster setting command
**
** RETURNS:       void
*/
static void _KRN_IPM_efcSet(KRN_MSG_T * message)
{
	int32_t sId;
	KRN_FLAGCLUSTER_T *cluster;
	uint32_t mask;

	PARACHECK();

	if ((cluster = _KRN_locateExport(message, &sId)) != NULL) {
		mask = message->p64a;
		switch (message->cmd) {
		case KRN_COMMAND_FLAGCLUSTER_SET:
			KRN_setFlags(cluster, mask);
			break;
		case KRN_COMMAND_FLAGCLUSTER_CLEAR:
			KRN_clearFlags(cluster, mask);
			break;
		case KRN_COMMAND_FLAGCLUSTER_TOGGLE:
			KRN_toggleFlags(cluster, mask);
			break;
		default:
			/* nothing */ ;
		}
		_KRN_IPM_ACK(message, KRN_SUBCOMMAND_OK, 0);
	} else
		_KRN_IPM_NACK(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_memAnnounce
**
** DESCRIPTION:   Process a "mem announce" command
**
** RETURNS:       void
*/

static void _KRN_IPM_memAnnounce(KRN_MSG_T * message)
{
	PARACHECK();

	if (message->p64a)
		MEM_map((((uint64_t) message->sID) << 32) | message->cID, (size_t) message->p64a, (void *)(uintptr_t) message->p64b);	/* P, L, V */

	PARACHECK();

	return;
}

/*
** FUNCTION:      _KRN_IPM_importProbe
**
** DESCRIPTION:   Process an "import probe" command
**
** RETURNS:       void
*/
static void _KRN_IPM_importProbe(KRN_MSG_T * message)
{
	int32_t sId;
	KRN_SEMAPHORE_T *s;

	PARACHECK();

	if ((s = _KRN_locateExport(message, &sId)) != NULL)
		_KRN_IPM_ACK(message, KRN_SUBCOMMAND_OK, 0);
	else
		_KRN_IPM_ACK(message, KRN_SUBCOMMAND_FAIL, 0);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_IPM_ACKorNACK
**
** DESCRIPTION:   Helper for constructing ACK and NACK responses
**
** RETURNS:       void
*/
static void _KRN_IPM_ACKorNACK(KRN_MSG_T * message, uint32_t subCmd,
			       uint32_t ACKorNACK, uint32_t p64a)
{
	uint32_t oldCmd;
	uint32_t oldSubCmd;

	PARACHECK();

	/* CmdId is (int32_t) so take care with right shifts... */
	message->to = message->from;
	message->from = _KRN_schedule->hwThread;
	oldCmd = message->cmd;
	oldSubCmd = message->subCmd;
	/* overwrite p64a if appropriate. 0 is never a valid value in this circumstance
	 * so the test is quite simple */
	if ((subCmd == KRN_SUBCOMMAND_OK) && (p64a != 0))
		message->p64a = p64a;
	message->p64b = (oldSubCmd << 16) | oldCmd;
	message->cmd = ACKorNACK;
	message->subCmd = subCmd;
	IPM_send(message);

	PARACHECK();
	return;
}

/*
** FUNCTION:      _KRN_locateExport
**
** DESCRIPTION:   Validates that an incoming command references an object
**                of the correct type and looks up its address and serverId
**
** RETURNS:       void * -     NULL: fail
**                         non-NULL: points to object
*/
static void *_KRN_locateExport(KRN_MSG_T * message, int32_t * serverId)
{
	void *obj = NULL;
	int32_t sId;

	PARACHECK();

	sId = message->sID;
	if ((sId <= _KRN_schedule->maxExpId) && (sId > 0)) {
		obj = _KRN_schedule->exportTable[sId - 1].objPtr;
		if (obj) {
			switch (message->cmd) {
			case KRN_COMMAND_IMPORT_PROBE:
				break;
			case KRN_COMMAND_SEMAPHORE_TEST:
			case KRN_COMMAND_SEMAPHORE_SET:
				if ((((KRN_SEMAPHORE_T *) obj)->impexp.
				     objtype) != KRN_OBJ_SEM)
					obj = NULL;
				break;
			case KRN_COMMAND_UNLOCK:
			case KRN_COMMAND_LOCK:
				if ((((KRN_SEMAPHORE_T *) obj)->impexp.
				     objtype) != KRN_OBJ_LOCK)
					obj = NULL;
				break;
			case KRN_COMMAND_FLAGCLUSTER_TEST:
			case KRN_COMMAND_FLAGCLUSTER_SET:
			case KRN_COMMAND_FLAGCLUSTER_TOGGLE:
			case KRN_COMMAND_FLAGCLUSTER_CLEAR:
				if ((((KRN_FLAGCLUSTER_T *) obj)->impexp.
				     objtype) != KRN_OBJ_EFC)
					obj = NULL;
				break;
			case KRN_COMMAND_MAILBOX_GET:
			case KRN_COMMAND_MAILBOX_PUT:
				if ((((KRN_SEMAPHORE_T *) obj)->impexp.
				     objtype) != KRN_OBJ_MBX)
					obj = NULL;
				break;
			case KRN_COMMAND_POOL_TAKE:
			case KRN_COMMAND_POOL_RETURN:
				if ((((KRN_SEMAPHORE_T *) obj)->impexp.
				     objtype) != KRN_OBJ_POOL)
					obj = NULL;
				break;
			case KRN_COMMAND_SYNC:
				if ((((KRN_SYNC_T *) obj)->impexp.objtype) !=
				    KRN_OBJ_SYNC)
					obj = NULL;
				break;
			case KRN_COMMAND_WQ_QUEUE:
				if ((((KRN_WQ_T *) obj)->impexp.objtype) !=
				    KRN_OBJ_WQ)
					obj = NULL;
				break;
			default:
				obj = NULL;
			}
		}
	}
	if (obj)
		*serverId = sId;
	else
		*serverId = 0;

	PARACHECK();
	return obj;
}
#endif
