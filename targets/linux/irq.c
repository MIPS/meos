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

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "meos/kernel/krn.h"

volatile int32_t _IRQ_needed;
volatile int32_t _IRQ_ipl;
IRQ_DESC_T *_IRQ_softTable[IRQ_INTERNAL_LAST];
uint64_t _IRQ_softPend, _IRQ_softMask;
extern pthread_t _CTX_self;
extern __thread int32_t _CTX_insig;
extern pthread_cond_t _CTX_BFC;

void DBG_interruptIn(int i);
void DBG_interruptOut(int i);

/*
** FUNCTION:	_IRQ_enable
**
** DESCRIPTION:	Enable interrupts - enable SIGUSR1.
**
** RETURNS:	void
*/
int32_t _IRQ_enable()
    __attribute__ ((no_instrument_function));
int32_t _IRQ_enable()
{
	if (_CTX_insig)
		return 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	return 0;
}

/*
** FUNCTION:	_IRQ_disable
**
** DESCRIPTION:	Disable interrupts - disable SIGUSR1.
**
** RETURNS:	void
*/
int32_t _IRQ_disable()
    __attribute__ ((no_instrument_function));
int32_t _IRQ_disable()
{
	if (_CTX_insig)
		return 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	return 0;
}

/*
** FUNCTION:	_IRQ_dispatch
**
** DESCRIPTION:	Dispatch the highest priority soft interrupt.
**
** RETURNS:	void
*/
void _IRQ_dispatch()
    __attribute__ ((no_instrument_function));
void _IRQ_dispatch()
{
	if (!(_IRQ_softPend & _IRQ_softMask))
		return;
	int32_t intNum = (63 - __builtin_clzll(_IRQ_softPend & _IRQ_softMask));
	IRQ_DESC_T *desc = _IRQ_softTable[intNum];
	DBG_assert(desc != NULL, "Soft interrupt with no handler!");
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
	DBG_interruptIn(intNum);
#endif
	desc->isrFunc(intNum);
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
	DBG_interruptOut(intNum);
#endif
}

/*
** FUNCTION:	IRQ_init
**
** DESCRIPTION:	Initialise the interrupt system.
**
** RETURNS:	void
*/
void IRQ_init(uint32_t * intStack, size_t issize)
{
	(void)intStack;
	(void)issize;

	if (intStack) {
#ifdef CONFIG_DEBUG_PARANOIA
		/* set the stack extent for interrupt handling code */
		extern uintptr_t _DBG_intStackStart;
		extern uintptr_t _DBG_intStackEnd;
		stack_t sigstack;
		sigaltstack(NULL, &sigstack);
		_DBG_intStackStart = (uintptr_t) sigstack.ss_sp;
		_DBG_intStackEnd =
		    _DBG_intStackStart + (uintptr_t) sigstack.ss_size;
#endif
		_IRQ_ipl = 0;
		if (_KRN_schedule) {
			_KRN_schedule->hwThread = 0;
			_KRN_schedule->hwThreads = 1;
		}
	}
}

/*
** FUNCTION:	IRQ_install
**
** DESCRIPTION:	Install the interrupt system.
**
** RETURNS:	void
*/
void IRQ_install()
{
	_IRQ_softMask = 0;
	_IRQ_softPend = 0;
	return;
}

/*
** FUNCTION:	_IRQ_service
**
** DESCRIPTION:	Make sure interrupts are handled.
**
** RETURNS:	void
*/
void _IRQ_service(void)
    __attribute__ ((no_instrument_function));
void _IRQ_service(void)
{
	pthread_cond_signal(&_CTX_BFC);
	if ((!_KRN_current) && (_KRN_schedule->timerTCB.reason != 0)) {
		pthread_kill(_KRN_schedule->timerTCB.savedContext.disp->thread,
			     SIGUSR1);
	} else {
		pthread_kill(_CTX_self, SIGUSR1);
	}
}

/*
** FUNCTION:	IRQ_route
**
** DESCRIPTION:	Route an interrupt by registering it in the soft interrupt array.
**
** RETURNS:	void
*/
void IRQ_route(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	if (irqDesc->isrFunc) {
		_IRQ_softTable[intNum] = irqDesc;
		_IRQ_softMask |= (((uint64_t) 1) << intNum);
		if ((_IRQ_softPend & _IRQ_softMask) && (!_IRQ_needed)) {
			_IRQ_needed = 1;
			_IRQ_service();
		}
	} else {
		_IRQ_softTable[intNum] = NULL;
		_IRQ_softMask &= ~(((uint64_t) 1) << intNum);
	}
}

/*
** FUNCTION:	IRQ_find
**
** DESCRIPTION:	Find the installed descriptor most like the supplied descriptor.
**
** RETURNS:	void
*/
IRQ_DESC_T *IRQ_find(IRQ_DESC_T * desc)
{
	return IRQ_cause(desc->intNum);
}

/*
** FUNCTION:	IRQ_find
**
** DESCRIPTION:	Find the installed descriptor for the supplied interrupt number.
**
** RETURNS:	void
*/
IRQ_DESC_T *IRQ_cause(int32_t intNum)
{
	return _IRQ_softTable[intNum];
}

/*
** FUNCTION:	IRQ_ack
**
** DESCRIPTION:	Acknowledge an interrupt so it doesn't happen again.
**
** RETURNS:	void
*/
IRQ_DESC_T *IRQ_ack(IRQ_DESC_T * irqDesc)
{
	if (!irqDesc)
		return NULL;
	_IRQ_softPend &= ~(((uint64_t) 1) << irqDesc->intNum);
	if (!(_IRQ_softPend & _IRQ_softMask))
		_IRQ_needed = 0;
	return irqDesc;
}

/*
** FUNCTION:	_IRQ_synthesizeNP
**
** DESCRIPTION:	Acknowledge an interrupt so it doesn't happen again, without
**              paranoia.
**
** RETURNS:	void
*/
INLINE void _IRQ_synthesizeNP(IRQ_DESC_T * irqDesc)
    __attribute__ ((no_instrument_function));
INLINE void _IRQ_synthesizeNP(IRQ_DESC_T * irqDesc)
{
	_IRQ_softPend |= (((uint64_t) 1) << irqDesc->intNum);
	if (_IRQ_softPend & _IRQ_softMask) {
		_IRQ_needed = 1;
		_IRQ_service();
	}
}

/*
** FUNCTION:	IRQ_synthesize
**
** DESCRIPTION:	Acknowledge an interrupt so it doesn't happen again.
**
** RETURNS:	void
*/
void IRQ_synthesize(IRQ_DESC_T * irqDesc)
{
	_IRQ_synthesizeNP(irqDesc);
	while (_IRQ_needed) ;
}

/*
** FUNCTION:	IRQ_soft
**
** DESCRIPTION:	Fill out a descriptor for a soft interrupt.
**
** RETURNS:	int32_t
*/
int32_t IRQ_soft(int32_t index, IRQ_DESC_T * out)
{
	if (index < 32) {
		out->intNum = index;
		return 1;
	}
	return 0;
}

/*
** FUNCTION:	IRQ_ipi
**
** DESCRIPTION:	Fill out a descriptor for an IPI.
**
** RETURNS:	int32_t
*/
int32_t IRQ_ipi(int32_t hwThread, IRQ_DESC_T * out)
{
	/* IRQ_INTERNAL_IPM is different */
	return 0;
}
