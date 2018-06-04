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
*   Description:	Kernel scheduler and ISR (Common)
*
*************************************************************************/

#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"

/*
* Global variables:
* These are kept to a minimum by putting most of the scheduler's
* control data inside the user-supplied KRN_SCHEDULE_T object
*/

KRN_SCHEDULE_T *_KRN_schedule;

KRN_TASK_T *_KRN_current;

/* IPL management state */
volatile int32_t _KRN_schedNeeded;

/*
** FUNCTION:	KRN_me
**
** DESCRIPTION:	Provide current pointer to TCB
**
** RETURNS:	void
*/
KRN_TASK_T *KRN_me()
{
	DBG_assert(!IRQ_servicing(), "No current task in interrupt context");
	return _KRN_current;
}

/*
** FUNCTION:	KRN_release
**
** DESCRIPTION:	Voluntary processor release
**
** RETURNS:	void
*/
void KRN_release(void)
{
	DBG_assert(!IRQ_servicing(),
		   "Can not release current task from interrupt context");
	_KRN_current->reason = KRN_RELEASE;
	KRN_schedule();
}

/*
** FUNCTION:	KRN_taskName
**
** DESCRIPTION:	Get a task's name
**
** RETURNS:	int8_t * pointer to task name string or NULL if task unnamed
*/
const char *KRN_taskName(KRN_TASK_T * task)
{
	if (task == NULL)
		task = _KRN_current;
	if (task == NULL)
		return "Unknown task";
	if (task->name != NULL)
		return task->name;
	else
		return "Unnamed task";
}

/*
** FUNCTION:	KRN_taskParameter
**
** DESCRIPTION:	Get a task's startup parameter
**
** RETURNS:	void * task parameter
*/
void *KRN_taskParameter(KRN_TASK_T * task)
{

	DBG_assert(!IRQ_servicing()
		   || (task != NULL),
		   "Can not fetch current task parameter from interrupt context");

	if (task == NULL)
		task = _KRN_current;
	return task->parameter;
}
