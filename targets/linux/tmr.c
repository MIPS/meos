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
*   Description:	Linux timer specialisation
*
*************************************************************************/

#include "meos/kernel/krn.h"
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

KRN_ISRFUNC_T *_TMR_timerISR;

struct timeval _TMR_start;
uint32_t _TMR_expire;
pthread_t _TMR_task;
pthread_mutex_t _TMR_BFL;
pthread_cond_t _TMR_BFC;
uint64_t _TMR_perfBase;

extern int32_t _CTX_log;

/*
** FUNCTION:	TMR_resetCycleCount
**
** DESCRIPTION:	Reset cycle counter.
**
** RETURNS:	void
*/
void TMR_resetCycleCount(void) __attribute__ ((optimize("O0")));
void TMR_resetCycleCount(void)
{
	/* Can't - do nothing */
}

IRQ_DESC_T _TMR_IRQ = {.intNum = IRQ_INTERNAL_TIMER };

extern volatile int32_t _IRQ_needed;
extern uint32_t _IRQ_softPend, _IRQ_softMask;
extern pthread_t _CTX_self;
extern __thread int32_t _CTX_insig;
extern void _IRQ_synthesizeNP(IRQ_DESC_T * irqDesc);

/*
** FUNCTION:	_TMR_timerFunc
**
** DESCRIPTION:	Simulate a timer interrupt.
**
** RETURNS:	void *
*/
void *_TMR_timerFunc(void *dummy)
    __attribute__ ((no_instrument_function));
void *_TMR_timerFunc(void *dummy)
{
	int err = 0;
	struct timespec sleepytime;
	struct timeval now, delta, era = { 4294, 967296 };
	uint32_t nowTick;
	uint64_t tick;
	sigset_t block;
	/* intialise */
	sigfillset(&block);
	pthread_sigmask(SIG_BLOCK, &block, NULL);
	pthread_mutex_lock(&_TMR_BFL);

	/* forever */
	for (;;) {
		tick = _TMR_expire;
		/* work out when we should wake up */
		gettimeofday(&now, NULL);
		/* Compensate 32 bit usec wrap in source timer */
		timersub(&now, &_TMR_start, &delta);
		if (!timercmp(&delta, &era, <))
			timeradd(&now, &era, &now);
		/* Now in ticks */
		nowTick = ((delta.tv_sec * 1000000) + delta.tv_usec);
		/* Convert 32-bit jiffy to timeval, fixing up wrap */
		err = ETIMEDOUT;
		do {
			int32_t dt = (int32_t) nowTick - (int32_t) tick;

			/* Still good */
			/* Fix up */
			if ((tick < nowTick) && (dt < 0)) {
				tick += 0x100000000;
			}
			/* Sleep */
			sleepytime.tv_sec = tick / 1000000;
			sleepytime.tv_nsec =
			    tick - (sleepytime.tv_sec * 1000000);
			/* Add start and convert to timespec for expiry time */
			sleepytime.tv_sec += _TMR_start.tv_sec;
			sleepytime.tv_nsec += _TMR_start.tv_usec;
			if (sleepytime.tv_nsec > 1000000) {
				sleepytime.tv_sec++;
				sleepytime.tv_nsec -= 1000000;
			}
			sleepytime.tv_nsec *= 1000;
			/* wait until then */
			err =
			    pthread_cond_timedwait(&_TMR_BFC, &_TMR_BFL,
						   &sleepytime);
		} while (0);
		/* if a timer ISR is registered */
		if ((err == ETIMEDOUT) && (_TMR_IRQ.isrFunc)) {
			/* synthesize an interrupt if the pthread cond timed out,
			 *  i.e. the requested timer interrupt expired.
			 */
			pthread_mutex_unlock(&_TMR_BFL);
			IRQ_synthesize(&_TMR_IRQ);
			pthread_mutex_lock(&_TMR_BFL);
		}
	}

	return NULL;
}

/*
** FUNCTION:	TMR_route
**
** DESCRIPTION:	Specify a timer interrupt handler. Start or kill a helper
**              thread appropriately to generate the interrupts .
**
** RETURNS:	void
*/
void TMR_route(IRQ_ISRFUNC_T isrFunc)
{
	IRQ_ISRFUNC_T *old = _TMR_IRQ.isrFunc;
	/* fill and route a descriptor */
	_TMR_IRQ.isrFunc = isrFunc;
	IRQ_route(&_TMR_IRQ);
	if (!isrFunc) {
		/* No ISR, kill helper */
		IRQ_IPL_T before = IRQ_raiseIPL();
		void *dummy;
		pthread_cancel(_TMR_task);
		pthread_join(_TMR_task, &dummy);
		pthread_mutex_unlock(&_TMR_BFL);
		IRQ_restoreIPL(before);
	} else if (!old) {
		/* New ISR, wasn't one previous, start helper */
		gettimeofday(&_TMR_start, NULL);
		pthread_create(&_TMR_task, NULL, (void *(*)(void *))
			       _TMR_timerFunc, NULL);
	}
}

/*
** FUNCTION:	TMR_set
**
** DESCRIPTION:	Fire a timer interrupt in the specified number of microseconds.
**
** RETURNS:	void
*/
void TMR_set(uint32_t abstime)
    __attribute__ ((no_instrument_function));
void TMR_set(uint32_t abstime)
{
	/* set the timer interrupt expiry time, and kick the helper */
	pthread_mutex_lock(&_TMR_BFL);
	_TMR_expire = abstime;
	pthread_cond_broadcast(&_TMR_BFC);
	pthread_mutex_unlock(&_TMR_BFL);
}

/*
** FUNCTION:	TMR_init
**
** DESCRIPTION:	Initialise timer system.
**
** RETURNS:	void
*/
void TMR_init()
{
	/* initialise control data for communicating with helper */
	pthread_mutexattr_t mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
	pthread_mutex_init(&_TMR_BFL, &mta);
	pthread_mutexattr_destroy(&mta);
	pthread_cond_init(&_TMR_BFC, NULL);
}

/*
** FUNCTION:	TMR_setClockSpeed
**
** DESCRIPTION:	Map hardware ticks to real time.
**
** RETURNS:	void
*/
void TMR_setClockSpeed(uint32_t perus)
{
	/* underlying clock is real time, ignore */
	(void)perus;
}

/*
** FUNCTION:	TMR_clockSpeed
**
** DESCRIPTION:	Map real time to hardware ticks.
**
** RETURNS:	void
*/
uint32_t TMR_clockSpeed()
{
	return 1;
}
