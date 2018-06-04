/***(C)2005***************************************************************
*
* Copyright (C) 2005 MIPS Tech, LLC
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
****(C)2005**************************************************************/

/*************************************************************************
*
*   Description:	Stack use statistics
*
*************************************************************************/

#include "meos/kernel/krn.h"

/*
** FUNCTION:      KRN_stackInfo
**
** DESCRIPTION:   Get (unreliable) stack use statistics for a task
**
** RETURNS:       int32_t - 1:  stack use information valid
**                      0: stack use information not available
*/
int32_t KRN_stackInfo(KRN_TASK_T * task, int32_t * total, int32_t * free,
		      int32_t * used)
{
#ifdef CONFIG_DEBUG_STACK_CHECKING
	uint32_t *p;
	uint32_t *q;
	uint32_t unused = _KRN_schedule ? _KRN_schedule->stackInit : 0;
	int32_t f = 0;

	DBG_assert(!IRQ_servicing(),
		   "Can not get stack info from interrupt context");

	if (unused == 0)
		return 0;

	if (task == NULL)
		task = _KRN_current;
	p = task->stackEnd - 1;
	q = task->stackStart;

	if ((p == NULL) || (q == NULL))
		return 0;

	*total = p - q + 1;
#ifdef STACK_GROWS_UP
	while (p >= q) {
		if (*p-- != unused)
			break;
		f++;
	}
#else
	while (q <= q) {
		if (*q++ != unused)
			break;
		f++;
	}
#endif
	*free = f;
	*used = *total - f;
	return 1;
#else
	return 0;
#endif
}
