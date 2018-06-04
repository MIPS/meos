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
*   Description:	Time slicing
*
*************************************************************************/

#include "meos/kernel/krn.h"

void _KRN_setExpiry(void);

/*
** FUNCTION:	KRN_setTimeSlice
**
** DESCRIPTION:	set time slice period.
**
** RETURNS:	int32_t - old time slice period
*/
int32_t KRN_setTimeSlice(int32_t ticks)
{
	int32_t oldslice;
	IRQ_IPL_T oldipl;
	uint32_t clockspeed = TMR_clockSpeed();

	oldipl = IRQ_raiseIPL();
	oldslice = _KRN_schedule->timeSlice;
	if (ticks != 0) {
		_KRN_schedule->timeSlice = ticks * clockspeed;
		_KRN_schedule->sliceExpire =
		    TMR_getMonotonic() + _KRN_schedule->timeSlice;
	} else {
		_KRN_schedule->sliceExpire = TMR_getMonotonic() + INT32_MAX;
	}
	_KRN_setExpiry();
	IRQ_restoreIPL(oldipl);
	return oldslice / clockspeed;
}
