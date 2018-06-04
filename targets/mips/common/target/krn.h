/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	MIPS common kernel specialisation
*
*************************************************************************/

#ifndef COMMON_KRN_H
#define COMMON_KRN_H

#ifndef __ASSEMBLER__

#define INLINE inline

static inline void KRN_schedulerInterrupt(void)
{
	DBG_assert(IRQ_getIPL() == 0, "Can not schedule from interrupt context");
	asm ("move $v0, $zero; syscall 0":::"v0");
}

/*
* Call the scheduler from a non-interruptible context. Restore the requested IPL, and wait
* for the schedule event to occur if we are now back at interruptible level.
*/
static inline void KRN_scheduleUnprotect(IRQ_IPL_T oldIPL)
{
	extern volatile int32_t _KRN_schedNeeded;
        _KRN_schedNeeded = 1;
        IRQ_restoreIPL(oldIPL);
        if (IRQ_bg())
        {
        	KRN_schedulerInterrupt();
        }
}

static inline void KRN_schedule()
{
	extern volatile int32_t _KRN_schedNeeded;
        _KRN_schedNeeded = 1;
        KRN_schedulerInterrupt();
}

/*
* Call the scheduler from a non-interruptible context. Do not wait for the underlying KICK to complete
* or we will deadlock. Rely on the fact that a enough code will be executed to get back to interruptible
* level to let the KICK filter through and arrive in a timely manner.
*/
static inline void KRN_scheduleProtected()
{
	extern volatile int32_t _KRN_schedNeeded;
	_KRN_schedNeeded = 1;
}

#include "meos/target/krnspec.h"
#endif

#endif
