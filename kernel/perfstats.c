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
*   Description:	Performance statistics
*
*************************************************************************/

#include "meos/config.h"
#include "meos/inttypes.h"
#include "meos/kernel/krn.h"

/* everything in this module is conditionally compiled except for
* KRN_installPerfStats, which may be empty
*/
#ifdef CONFIG_DEBUG_PROFILING

/* globals for the external performance stats system */
KRN_STATS_T *_ThreadStatsArray;
int32_t _ThreadStatsArraySize;
int32_t _ThreadStatsCtrl;
int64_t _ThreadRunStart;
int64_t _ThreadRunTicks;

/*
** FUNCTION:      _KRN_zeroRawStats
**
** DESCRIPTION:   zero a raw performance statistics slot
**
** RETURNS:       void
*/

inline void _KRN_zeroRawStats(KRN_RAWSTATS_T * s)
{
	int32_t i;
	s->monotonics = 0;
	for (i = 0; i < TMR_PERFCOUNTERS; i++)
		s->perfCount[i] = 0;
}

/*
** FUNCTION:      _KRN_zeroStats
**
** DESCRIPTION:   zero a performance statistics slot
**
** RETURNS:       void
*/
void _KRN_zeroStats(KRN_STATS_T * s)
{
	_KRN_zeroRawStats(&s->stats);
	s->runCount = 0;
}

/*
** FUNCTION:      _KRN_grabStats
**
** DESCRIPTION:   grab raw statistics data from hardware
**                (as a processing optimisation for _KRN_deltaStats, the 24-bit items are left shifted by 8)
**
** RETURNS:       void
*/

void _KRN_grabStats(KRN_RAWSTATS_T * s)
{
	s->monotonics = TMR_getMonotonic();
#ifdef CONFIG_DEBUG_TASK_PERF
	int32_t i;
	for (i = 0; i < TMR_PERFCOUNTERS; i++)
		s->perfCount[i] = TMR_getPerfCount(i);
#endif
}

/*
** FUNCTION:      _KRN_deltaStats
**
** DESCRIPTION:   calculate difference between two lots of raw stats
**
** RETURNS:       void
*/
void _KRN_deltaStats(KRN_RAWSTATS_T * diff, KRN_RAWSTATS_T * a,
		     KRN_RAWSTATS_T * b)
{
#ifdef CONFIG_DEBUG_TASK_PERF
	int32_t i;
	for (i = 0; i < TMR_PERFCOUNTERS; i++)
		diff->perfCount[i] = a->perfCount[i] - b->perfCount[i];
#endif
	diff->monotonics = a->monotonics - b->monotonics;
}

/*
** FUNCTION:      _KRN_accStats
**
** DESCRIPTION:   accumulate raw delta stats into a stats slot
**
** RETURNS:       void
*/
void _KRN_accStats(KRN_STATS_T * acc, KRN_RAWSTATS_T * n, int32_t neg)
{
	int32_t i;
	(void)i;
	if (neg) {
#ifdef CONFIG_DEBUG_TASK_PERF
		for (i = 0; i < TMR_PERFCOUNTERS; i++)
			acc->stats.perfCount[i] -= n->perfCount[i];
#endif
		acc->stats.monotonics -= n->monotonics;
	} else {
#ifdef CONFIG_DEBUG_TASK_PERF
		for (i = 0; i < TMR_PERFCOUNTERS; i++)
			acc->stats.perfCount[i] += n->perfCount[i];
#endif
		acc->stats.monotonics += n->monotonics;
	}
}
#endif

/*
** FUNCTION:      KRN_installPerfStats
**
** DESCRIPTION:   Install performance counter collection stuff
**				  Assumption is that this is called after KRN_reset()
**                and before KRN_startOS()
**
** RETURNS:       void
*/
void KRN_installPerfStats(KRN_STATS_T * statsBuf, int32_t bufSize)
{
#ifdef CONFIG_DEBUG_PROFILING
	DBG_assert(bufSize > 4, "Perf stats buffer too small");	/* at least big enough for 4 specials, background and timer tasks */
	/* set up pointers and zero initial data */
	_ThreadStatsArray = statsBuf;
	_ThreadStatsArraySize = bufSize;
	/* Performance monitoring initially off (will be started by KRN_startTimerTask
	 * after the interrupt timer has been set up
	 */
	_ThreadStatsCtrl = 0;
	while (bufSize-- > 0) {
		statsBuf->name = NULL;
		_KRN_zeroStats(statsBuf++);
	}

	/* set up "special" (non-task) pointers */
	_KRN_schedule->nullStats = &_ThreadStatsArray[0];
	_KRN_schedule->tIntStats = &_ThreadStatsArray[1];
	_KRN_schedule->dIntStats = &_ThreadStatsArray[2];
	_KRN_schedule->schedStats = &_ThreadStatsArray[3];
	_ThreadStatsArray[0].name = "Idle";
	_ThreadStatsArray[1].name = "Timer Interrupts";
	_ThreadStatsArray[2].name = "QIO Device Interrupts";
	_ThreadStatsArray[3].name = "Scheduler interrupts";
#else
	/* suppress aggressive compiler warnings */
	(void)statsBuf;
	(void)bufSize;
#endif
}

#ifdef CONFIG_DEBUG_PROFILING_DUMP
void _KRN_dumpPerf(void)
{
	DBG_logF("Performance dump\n");
	if ((_ThreadStatsArray) && (_ThreadStatsArraySize)) {
		int32_t i, j;
		/* Dump table header, including perf counters if present */
		DBG_logF
		    ("Task                                        Run         Monotonics  ");
		for (j = 0; j < TMR_PERFCOUNTERS; j++)
			DBG_logF("Perf %3" PRIu32 "    ", j);
		DBG_logF("\n");
		/* Dump table rows */
		for (i = 0; i < _ThreadStatsArraySize; i++) {
			if (_ThreadStatsArray[i].name) {
				DBG_logF
				    ("[%40s]: %10" PRIu32 "  %10" PRIu32 "  ",
				     _ThreadStatsArray[i].name,
				     _ThreadStatsArray[i].runCount,
				     _ThreadStatsArray[i].stats.monotonics);
				for (j = 0; j < TMR_PERFCOUNTERS; j++)
					DBG_logF("%10" PRIu32 "  ",
						 _ThreadStatsArray[i].
						 stats.perfCount[j]);
				DBG_logF("\n");
			}
		}
	}
}
#endif
