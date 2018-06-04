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
*   Description:	Linux interrupt specialisation
*
*************************************************************************/

#ifndef TARGET_IRQ_H
#define TARGET_IRQ_H

#include <sched.h>
#include <pthread.h>
#include <signal.h>

extern volatile int32_t _IRQ_ipl;
extern __thread int32_t _CTX_insig;
extern pthread_t _CTX_self;
extern volatile int32_t _IRQ_needed;


#define IRQ_getIPL() __extension__ ({ \
	_IRQ_ipl; \
})
int32_t _IRQ_enable();
int32_t _IRQ_disable();

extern int8_t *_IRQ_file;
extern uint32_t _IRQ_line;

#define IRQ_raiseIPL()  __extension__ ({ \
	if (!_IRQ_ipl) \
		_IRQ_disable(); \
	DBG_raise(_IRQ_ipl); \
	_IRQ_ipl++; \
})

#define IRQ_restoreIPL(X)  __extension__ ({ \
	int32_t to = (X); \
	DBG_lower(_IRQ_ipl, to); \
	_IRQ_ipl = to; \
	if (!to) { \
		_IRQ_enable(); \
		if ((_IRQ_needed) && (!_CTX_insig)) \
			pthread_kill(_CTX_self, SIGUSR1); \
	} \
})

#define IRQ_wait() sched_yield()

#define IRQ_bg() ((_IRQ_ipl == 0) && (_CTX_insig == 0))
#define IRQ_servicing() (_CTX_insig != 0)
#define IRQ_critical() (_IRQ_ipl != 0)

#define IRQ_HARDINTS 0
#define IRQ_SOFTINTS 32

#define IRQ_INTERNAL_TIMER 32
#define IRQ_INTERNAL_IPM 33
#define IRQ_INTERNAL_STEAL 34
#define IRQ_INTERNAL_LAST 35

#endif
