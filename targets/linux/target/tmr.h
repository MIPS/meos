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

#ifndef TARGET_TMR_H
#define TARGET_TMR_H

#include <limits.h>
#include <sys/time.h>
#include <time.h>

inline static void TMR_startCycleCount() __attribute__((optimize("O0"))) __attribute__ ((no_instrument_function));
inline static void TMR_startCycleCount()
{
}

inline static uint32_t TMR_stopCycleCount() __attribute__((optimize("O0"))) __attribute__ ((no_instrument_function));
inline static uint32_t TMR_stopCycleCount()
{
	return 0;
}

#define TMR_getMonotonic() __extension__ ({ \
	extern struct timeval _TMR_start; \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	timersub(&tv, &_TMR_start, &tv); \
	((tv.tv_sec * 1000000) + tv.tv_usec); \
})

#define TMR_PERFCOUNTERS 1
extern uint64_t _TMR_perfBase;
#define TMR_getPerfCount(X) __extension__ ({ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); ((((uint64_t)ts.tv_sec * 1000000000) + ts.tv_nsec) - _TMR_perfBase); })
#define TMR_resetPerfCount(X) __extension__ ({ struct timespec ts; uint64_t r; uint64_t i; clock_gettime(CLOCK_MONOTONIC, &ts); i = (((uint64_t)ts.tv_sec * 1000000000) + ts.tv_nsec)); r = i - _TMR_perfBase;  _TMR_perfBase = i; r;})
#define TMR_configPerfCount(X, E)

#endif
