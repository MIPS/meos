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
*   Description:	Set task priority
*
*************************************************************************/

#include "meos/kernel/krn.h"

/*
** FUNCTION:	KRN_priority
**
** DESCRIPTION:	Set a task's priority
**
** RETURNS:	KRN_PRIORITY_T previous priority
*/
KRN_PRIORITY_T KRN_priority(KRN_TASK_T * task, KRN_PRIORITY_T priority)
{
	KRN_PRIORITY_T oldpri, maxpri;
	IRQ_IPL_T oldipl;

	DBG_assert(!IRQ_servicing(),
		   "Can not set task priority from interrupt context");

	if (task == NULL)
		task = _KRN_current;

	if (task == &(_KRN_schedule->timerTCB))
		return -1;	/* not allowed to change priority of timer task */

	/* trap priority in valid range - not allowed to go to highest priority level,
	 * which is reserved for the timer task */
	maxpri = _KRN_schedule->maxPriority - 1;
	if (priority < 0)
		priority = 0;
	if (priority > maxpri)
		priority = maxpri;

	oldipl = IRQ_raiseIPL();

	/* If the task is runnable, adjust its hold queue ... */
	if (task->holdQueue == (_KRN_schedule->priQueues + task->priVal)) {
		task->holdQueue = _KRN_schedule->priQueues + priority;
		/*... and if its not the current task, change queues */
		if (task != _KRN_current) {
			DQ_remove(task);
			DQ_addTail(task->holdQueue, task);
		}
	}

	oldpri = task->priVal;
	task->priVal = priority;
	task->priFlags = _KRN_schedule->priFlags + (priority >> 5);
	task->priBit = priority & 31;
	/* activate new "queue in use" flag */
	*(task->priFlags) |= 1 << task->priBit;
	/* don't need to clear old flag - the scheduler cleans up flags belonging
	 * to unused queues
	 */

	KRN_scheduleUnprotect(oldipl);
	return oldpri;
}
