/***(C)2016***************************************************************
*
* Copyright (C) 2016 MIPS Tech, LLC
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
****(C)2016**************************************************************/

/*************************************************************************
*
*   Description:	SRTC library
*
*************************************************************************/

#include "MEOS.h"

static SRTC_T *_srtc;

time_t SRTC_now()
{
	uint64_t diff = TMR_getMonotonic() - _srtc->epochj;
	return _srtc->epoch + (time_t) (diff / (1000000ULL * TMR_clockSpeed()));
}

static void SRTC_update(KRN_TIMER_T * timer, void *timerPar)
{
	time_t now, diff;
	uint64_t jiffies;
	/* Compute seconds since last update */
	now = SRTC_now();
	diff = now - _srtc->epoch;
	/* Convert to jiffies */
	jiffies = diff * 1000000 * TMR_clockSpeed();
	/* Increment seconds */
	_srtc->epoch += diff;
	/* Increment and wrap jiffies */
	_srtc->epochj += (uint32_t) jiffies;
	/* And do it again next time */
	KRN_setSoftTimer(timer, SRTC_update, NULL, 0x7ffffff, 0x100);
}

void SRTC_mark(time_t now)
{
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	_srtc->epochj = TMR_getMonotonic();
	_srtc->epoch = now;
	IRQ_restoreIPL(ipl);
	KRN_setSoftTimer(&_srtc->timer, SRTC_update, NULL, 0x7ffffff, 0x100);
}

void SRTC_init(SRTC_T * srtc)
{
	_srtc = srtc;
}

#ifdef CONFIG_SNTP

static uint64_t SRTC_rcSample()
{
	int32_t r;
	uint64_t startn, startj, endn, endj, diffn, diffj;
	KRN_TASKQ_T q;

	DQ_init((DQ_T *) & q);
	do {
		r = SNTP_getDetailedTime(NULL, &startn);
		startj = TMR_getMonotonic();
		KRN_hibernate(&q, 1);
		r |= SNTP_getDetailedTime(NULL, &endn);
		endj = TMR_getMonotonic();
		/* Repeat until we don't hit a wrap */
	} while (r || (endj < startj));

	diffn = endn - startn;
	diffj = endj - startj;
	/* Convert diffn to uSecs */
	diffn *= 1000000;
	diffn += 0x80000000;
	diffn >>= 32;
	/* Divide diffj by diffn, and set clockspeed */
	return diffj / diffn;
}

void SRTC_reverseCalibrate()
{
	uint32_t i;
	uint64_t a = 0;
	/* Rough cut */
	for (i = 0; i < 4; i++)
		TMR_setClockSpeed(SRTC_rcSample());
	/* Average */
	for (i = 0; i < 16; i++)
		a += SRTC_rcSample();
	TMR_setClockSpeed(a >> 4);
}

static void SRTC_netUpdate()
{
	time_t t;
	if (!SNTP_getTime(NULL, &t))
		SRTC_mark(t);
}

static void SRTC_netUpReq(KRN_TIMER_T * timer, void *timerPar)
{
	KRN_queueWQ(LOPRI, (KRN_TASKFUNC_T *) SRTC_netUpdate, NULL,
		    "SRTC NTP resync", 0);
	KRN_setSoftTimer(timer, SRTC_netUpReq, NULL, 0x7ffffff, 0x100);

}

void SRTC_ntp()
{
	SRTC_netUpdate();
	KRN_setSoftTimer(&_srtc->ntimer, SRTC_netUpReq, NULL, 0x7ffffff, 0x100);
}

#endif
