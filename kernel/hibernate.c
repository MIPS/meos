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
*   Description:	Task descheduling (Hibernation)
*
*************************************************************************/

#include "meos/kernel/krn.h"

/*
** FUNCTION:      KRN_hibernate
**
** DESCRIPTION:   Voluntary suspension on specified queue
**
** RETURNS:       void
*/
uint32_t KRN_hibernateWithValue(KRN_TASKQ_T * queue, int32_t timeout)
{
	IRQ_IPL_T oldipl;
	KRN_TASK_T *t;
	KRN_TIMER_T timer;

	DBG_assert(IRQ_bg(), "Can not hibernate from interrupt context");
	DBG_assert(queue->DQ_link.fwd != NULL,
		   "Can not hibernate on uninitialised queue");

	if (timeout == 0)
		return 0;

	t = _KRN_current;

	oldipl = IRQ_raiseIPL();

	t->p1.testResult = 0;

	/* Place the task on the wait queue */
	_KRN_hibernateTask(t, queue);

	KRN_setTimer(&timer, _KRN_timeoutWake, t, timeout);

	/* perform the descheduling proper */
	KRN_scheduleUnprotect(oldipl);

	/* at this point we've either been woken by another task or the timer has expired */
	KRN_cancelTimer(&timer);	/* cancel timer in case it hasn't expired */
	return t->p1.testResult;
}
