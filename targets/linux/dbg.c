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
*   Description:	Linux debug specialisation
*
*************************************************************************/

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include "meos/config.h"
#include "meos/debug/dbg.h"
#include "meos/kernel/krn.h"

#ifndef CONFIG_DEBUG_TRACE_CTXSW_BT_HARD
#define HARD_DEPTH 0
#else
#define HARD_DEPTH CONFIG_DEBUG_TRACE_CTXSW_BT_HARD
#endif
#ifndef CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
#define SOFT_DEPTH 0
#else
#define SOFT_DEPTH CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
#endif

#if defined(CONFIG_DEBUG_TRACE_CTXSW_BT_HARD) || defined(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT) || defined(CONFIG_DEBUG_BT_DUMP) || defined(CONFIG_DEBUG_PARANOIA) || defined(CONFIG_DEBUG_TRACE_EXTRA)

uintptr_t _DBG_intStackStart = 0;
uintptr_t _DBG_intStackEnd = 0;

extern __thread KRN_TASK_T *_CTX_current;

#ifdef CONFIG_ARCH_LINUX_X86
/*
** FUNCTION:	DBG_walk
**
** DESCRIPTION:	Walk up activation records to get call sites.
**
** RETURNS:	int32_t
*/
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
    __attribute__ ((no_instrument_function));
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
{
	volatile register uintptr_t **realebp asm("ebp");
	uintptr_t *fakeebp = (uintptr_t *) * realebp;

	depth--;
	while ((depth--) && (fakeebp != NULL))
		fakeebp = (uintptr_t *) fakeebp[0];

	if (fakeebp == NULL)
		return 0;

	if (ora)
		*ora = fakeebp[1];
	if (osp)
		*osp = (uintptr_t) fakeebp;

	return 1;
}
#endif

#ifdef CONFIG_ARCH_LINUX_X64
/*
** FUNCTION:	DBG_walk
**
** DESCRIPTION:	Walk up activation records to get call sites.
**
** RETURNS:	int32_t
*/
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
    __attribute__ ((no_instrument_function));
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
{
	volatile register uintptr_t **realrbp asm("rbp");
	uintptr_t *fakerbp = (uintptr_t *) * realrbp;

	depth--;
	while ((depth--) && (fakerbp != NULL))
		fakerbp = (uintptr_t *) fakerbp[0];

	if (fakerbp == NULL)
		return 0;

	if (ora)
		*ora = fakerbp[1];
	if (osp)
		*osp = (uintptr_t) fakerbp;

	return 1;
}
#endif

#ifdef CONFIG_ARCH_LINUX_MIPS
#define sw_ra_X_sp 0xafbf0000
#define addiu_sp_sp_X 0x27bd0000
#define lui_gp_X 0x3c1c0000
#define jr_ra 0x03e00008

#define abs(s) ((s) < 0 ? -(s) : (s))

typedef enum {
	NONE = 0,
	INT,
	NEST,
	DONE
} SCANTHRU_T;

/*
** FUNCTION:	_DBG_backscan
**
** DESCRIPTION:	Code read backwards from specified PC and work out call site.
**
** RETURNS:	SCANTHRU_T
*/
static inline SCANTHRU_T _DBG_backscan(uint32_t * mra, size_t * offset,
				       size_t * size)
{
	uint32_t *addr;
	*offset = 0;
	*size = 0;
	for (addr = mra; !*offset || !*size; --addr) {
		switch (*addr & 0xffff0000) {
		case addiu_sp_sp_X:
			*size = abs((short)(*addr & 0xffff));
			break;

		case sw_ra_X_sp:
			*offset = (short)(*addr & 0xffff);
			break;

		case lui_gp_X:
			return DONE;

		default:
			break;
		}
	}
	return NONE;
}

/*
** FUNCTION:	_DBG_traceone
**
** DESCRIPTION:	Back trace 1 stack frame.
**
** RETURNS:	SCANTHRU_T
*/
static inline SCANTHRU_T _DBG_traceone(uint32_t ** mra, uint32_t ** msp)
{
	SCANTHRU_T r = NONE;
	size_t offset;
	size_t size;

	if (_DBG_backscan(*mra, &offset, &size) == DONE)
		return DONE;

	*mra = *(uint32_t **) ((uintptr_t) * msp + offset);
	*msp = (uint32_t *) ((uintptr_t) * msp + size);

	return r;
}

/*
** FUNCTION:	DBG_walk
**
** DESCRIPTION:	Walk up activation records to get call sites.
**
** RETURNS:	int32_t
*/
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
    __attribute__ ((no_instrument_function));
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
{
	uint32_t *addr;
	register uint32_t *realra asm("ra");
	register uint32_t *realsp asm("sp");
	uint32_t *mra = realra;
	uint32_t *msp = realsp;
	size_t size;

	/* Get the stack frame for self */
	size = 0;

	for (addr = (uint32_t *) DBG_walk; !size; addr++) {
		if ((*addr & 0xffff0000) == addiu_sp_sp_X)
			size = abs((short)(*addr & 0xffff));
		else if (*addr == jr_ra)
			break;
	}

	msp = (uint32_t *) ((uintptr_t) msp + size);

	while (depth--)
		if ((_DBG_traceone(&mra, &msp) == DONE) && (depth))
			return 0;

	if (ora)
		*ora = (uintptr_t) mra;
	if (osp)
		*osp = (uintptr_t) msp;

	return 1;

}
#endif

#if defined(CONFIG_DEBUG_TRACE_CTXSW_BT_HARD) || defined(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT)
/*
** FUNCTION:	_DBG_backtraceCtx
**
** DESCRIPTION:	Helper to back trace to the specified depth, and write it to the
**				trace log.
**
** RETURNS:	void
*/
void _DBG_backtraceCtx(void)
    __attribute__ ((no_instrument_function));
void _DBG_backtraceCtx(void)
{
	extern void DBG_atrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i,
			       uintptr_t * ap);
	int32_t depth;
	uintptr_t mra;
	uintptr_t msp;
#ifdef 	CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
	uintptr_t *results = alloca(SOFT_DEPTH * sizeof(uintptr_t));
	KRN_TRACE_T *wp;
#endif

	_CTX_current->savedContext.trace = 0;

	for (depth = 1; DBG_walk(depth, &mra, &msp); depth++) {

#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_HARD
		if (depth < CONFIG_DEBUG_TRACE_CTXSW_BT_HARD)
			DBG_RTTValue(mra);
#endif
#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
		if (depth < CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT)
			results[depth] = (uintptr_t) mra;
#endif
	}
#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
	wp = DBG_openTrace((depth + 3) / 3);
	DBG_atrace(&wp, DBG_TRACE_CTX_BT, depth, (uintptr_t *) results);
#endif
}

/*
** FUNCTION:	_DBG_backtraceCtx
**
** DESCRIPTION:	Mark a thread so that it will backtrace itself to the log at the
**				next opportunity.
**
** RETURNS:	void
*/
void DBG_backtraceCtx(KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
void DBG_backtraceCtx(KRN_CTX_T * ctx)
{
	ctx->trace = 1;
}

#endif

/* List of known pointers - simple, not efficient. This is only for extreme
 * debug, so shouldn't be problematic
 */
pthread_mutex_t _DBG_mPtr;
size_t _DBG_nPtrs = 0;
size_t _DBG_lPtr = 0;
uintptr_t *_DBG_ptrs = NULL;

/*
** FUNCTION:	DBG_badPtr
**
** DESCRIPTION:	Check the list to see if a pointer is known.
**
** RETURNS:	void
*/
int32_t DBG_badPtr(uintptr_t p)
    __attribute__ ((no_instrument_function));
int32_t DBG_badPtr(uintptr_t p)
{
	int32_t r = 1;
	size_t i;
	pthread_mutex_lock(&_DBG_mPtr);
	for (i = 0; i < _DBG_lPtr; i++)
		if (_DBG_ptrs[i] == p)
			r = 0;
	pthread_mutex_unlock(&_DBG_mPtr);
	return r;
}

/*
** FUNCTION:	DBG_goodPtr
**
** DESCRIPTION:	Add a pointer to the known good list.
**
** RETURNS:	void
*/
void DBG_goodPtr(uintptr_t p)
    __attribute__ ((no_instrument_function));
void DBG_goodPtr(uintptr_t p)
{
	size_t i;
	if (!_DBG_nPtrs) {
		_DBG_nPtrs = 256;
		_DBG_ptrs = malloc(_DBG_nPtrs * sizeof(uintptr_t));
		pthread_mutexattr_t mta;
		pthread_mutexattr_init(&mta);
		pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
		pthread_mutex_init(&_DBG_mPtr, &mta);
		pthread_mutexattr_destroy(&mta);
	}
	pthread_mutex_lock(&_DBG_mPtr);
	for (i = 0; i < _DBG_lPtr; i++)
		if (_DBG_ptrs[i] == p) {
			pthread_mutex_unlock(&_DBG_mPtr);
			return;
		}
	_DBG_ptrs[_DBG_lPtr++] = p;
	if (_DBG_lPtr == _DBG_nPtrs) {
		_DBG_nPtrs *= 2;
		_DBG_ptrs = realloc(_DBG_ptrs, _DBG_nPtrs * sizeof(uintptr_t));
	}
	pthread_mutex_unlock(&_DBG_mPtr);
}

#ifdef CONFIG_DEBUG_TRACE_EXTRA
/*
** FUNCTION:	DBG_extra
**
** DESCRIPTION:	Get some extra information for debug traces - the currently
**				executing task. This might be different from the schedulers view
**				if assimilation is wobbling.
**
** RETURNS:	void
*/
uintptr_t DBG_extra(void)
    __attribute__ ((no_instrument_function));
uintptr_t DBG_extra(void)
{
	return (uintptr_t) _CTX_current;
}
#endif

#endif

/*
** FUNCTION:	_DBG_dump
**
** DESCRIPTION:	Dump any interesting debug data to human readable output.
**
** RETURNS:	void
*/
void _DBG_dump(KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
void _DBG_dump(KRN_CTX_T * ctx)
{
#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
	extern void _DBG_dumpTrace();
	_DBG_dumpTrace();
#endif
#ifdef CONFIG_DEBUG_PROFILING_DUMP
	extern void _KRN_dumpPerf();
	_KRN_dumpPerf();
#endif
}
