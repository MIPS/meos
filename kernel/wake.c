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
*   Description:	Wakeup routines
*
*************************************************************************/

#include "meos/kernel/krn.h"

/*
** FUNCTION:      KRN_wakeWithValue
**
** DESCRIPTION:   Wake the task at the head of a queue
**
** RETURNS:       KRN_TASK_T * handle of woken task or NULL if queue was empty
*/
KRN_TASK_T *KRN_wakeWithValue(KRN_TASKQ_T * queue, uint32_t value)
{
	KRN_TASK_T *t;
	IRQ_IPL_T oldipl;
	int32_t sched = 0;

	oldipl = IRQ_raiseIPL();

	/* Wake the first task from the queue */
	t = _KRN_wakeOneTask(queue, &sched);

	/* Set value */
	if (t)
		t->p1.testResult = value;

	/* Reschedule as necessary */
	if (sched)
		KRN_scheduleUnprotect(oldipl);
	else
		IRQ_restoreIPL(oldipl);

	return t;
}

/*
** FUNCTION:      KRN_wakeAll
**
** DESCRIPTION:   Wake all the tasks on a queue
**
** RETURNS:       void
*/
void KRN_wakeAll(KRN_TASKQ_T * queue)
{

	IRQ_IPL_T oldipl;
	KRN_TASK_T *t;
	int32_t sched = 0;

	/* Note: ALL tasks are awakened before ANY are scheduled */
	oldipl = IRQ_raiseIPL();

	/* Wake many tasks while there are tasks on the hold queue */
	do {
		t = _KRN_wakeOneTask(queue, &sched);
	} while (t);

	/* Reschedule as necessary */
	if (sched)
		KRN_scheduleUnprotect(oldipl);
	else
		IRQ_restoreIPL(oldipl);

	return;
}

/*
** FUNCTION:      KRN_resumeWithValue
**
** DESCRIPTION:   Resume the specified task, passing the specified value
**
** RETURNS:       void
*/
void KRN_resumeWithValue(KRN_TASK_T * t, uint32_t value)
{
	IRQ_IPL_T oldipl;
	int32_t sched = 0;

	oldipl = IRQ_raiseIPL();
	DQ_remove(t);

	/* Set value */
	t->p1.testResult = value;

	/* If there was nothing on the queue, do nothing */
	sched = _KRN_wakeTask(t);

	/* Reschedule as necessary */
	if (sched)
		KRN_scheduleUnprotect(oldipl);
	else
		IRQ_restoreIPL(oldipl);

	return;
}
