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
*   Description:	PIC32 debug specialisation
*
*************************************************************************/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"

#if defined(ARCH_MIPS_CHANNEL_FAST) && defined(__XC32)
#ifdef __cplusplus
extern "C" {
#endif
	void _mon_putc(char vData) __attribute__ ((weak));
	void _mon_putc(char vData) {
		asm("ssnop");
	}
#ifdef __cplusplus
}
#endif
#endif
uintptr_t _DBG_intStackStart = 0;
uintptr_t _DBG_intStackEnd = 0;

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

static inline SCANTHRU_T _DBG_backscan(uint32_t * mra, size_t * offset,
				       size_t * size)
    __attribute__ ((no_instrument_function));
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
			if (!*size)
				*size = abs((short)(*addr & 0xffff));
			break;

		case sw_ra_X_sp:
			if (!*offset)
				*offset = (short)(*addr & 0xffff);
			break;

		case jr_ra:
			return NONE;
			break;

		case lui_gp_X:
			return DONE;

		default:
			break;
		}
	}
	return NONE;
}

#ifdef __cplusplus
extern "C" {
#endif

	extern void _exception(void);
	extern void _exception_end(void);
	extern void _exception_nest(void);
	extern void _exception_nest_end(void);
	extern void _exception_root(void);
	extern void _exception_root_end(void);
	extern void _savenjump(void);
	extern void _savenjump_end(void);
	extern void _sleep(void);
	extern void _sleep_end(void);
#ifdef __cplusplus
}
#endif
static inline SCANTHRU_T _DBG_traceone(uint32_t * mpc, uint32_t ** mra,
				       uint32_t ** msp, KRN_CTX_T ** ctx)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_DBG_traceone
**
** DESCRIPTION:	Back trace 1 stack frame.
**
** RETURNS:	SCANTHRU_T
*/
static inline SCANTHRU_T _DBG_traceone(uint32_t * mpc, uint32_t ** mra,
				       uint32_t ** msp, KRN_CTX_T ** ctx)
{
#ifndef CONFIG_ARCH_MIPS_VZ
	register uint32_t realk0 asm("k0");
#else
	uint32_t realk0 = _m32c0_mfc0(31, 2);	/* Stick context in KSCRATCH1 */
#endif

	SCANTHRU_T r = NONE;
	size_t offset;
	size_t size;

	if ((*mpc >= (uint32_t) _sleep) && (*mpc < (uint32_t) _sleep_end))
		return DONE;

	if (_DBG_backscan(mpc, &offset, &size) == DONE)
		return DONE;

	if (!*mra)
		*mra = *(uint32_t **) ((uintptr_t) * msp + offset);
	*msp = (uint32_t *) ((uintptr_t) * msp + size);

	if ((*mra >= (uint32_t *) _exception_root)
	    && (*mra <= (uint32_t *) _exception_root_end)) {
		if (_DBG_backscan(*mra, &offset, &size) == DONE)
			return DONE;
		*mra = *(uint32_t **) ((uintptr_t) * msp + offset);
		*msp = (uint32_t *) ((uintptr_t) * msp + size);
		if ((*mra >= (uint32_t *) _exception)
		    && (*mra <= (uint32_t *) _exception_end)) {
			r = INT;
			*ctx = (KRN_CTX_T *) realk0;
		} else {
			r = NEST;
			*ctx = (KRN_CTX_T *) (((uintptr_t) * msp) + 16);	/* Does not include FP state, does include arg prep area */
		}
		if ((*ctx)->gp.epc == (*ctx)->gp.sp + 0x30)	/* Dirty dirty mips_xxc0 generated stub */
			*mra = (uint32_t *) (*ctx)->gp.t[7];
		else
			*mra = (uint32_t *) (*ctx)->gp.epc;
		*msp = (uint32_t *) (*ctx)->gp.sp;
	} else if ((*mra >= (uint32_t *) _savenjump)
		   && (*mra <= (uint32_t *) _savenjump_end)) {
		r = INT;
		*ctx = (KRN_CTX_T *) realk0;
		if ((*ctx)->gp.epc == (*ctx)->gp.sp + 0x30)	/* Dirty dirty mips_xxc0 generated stub */
			*mra = (uint32_t *) (*ctx)->gp.t[7];
		else
			*mra = (uint32_t *) (*ctx)->gp.epc;
		*msp = (uint32_t *) (*ctx)->gp.sp;
	}

	return r;
}
#endif

#if defined(CONFIG_DEBUG_TRACE_CTXSW_BT_HARD) || defined(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT)

#ifdef __cplusplus
extern "C" {
#endif
	extern void DBG_atrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i,
			       uintptr_t * ap);
#ifdef __cplusplus
}
#endif
void DBG_backtraceCtx(KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	DBG_backtraceCtx
**
** DESCRIPTION:	Back trace a context a configured number of frames, dumping the
** 				result to the trace log.
**
** RETURNS:	void
*/
void DBG_backtraceCtx(KRN_CTX_T * ctx)
{
	uint32_t depth;
	uint32_t *mpc;
	uint32_t *mra;
	uint32_t *msp;
#ifdef 	CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
	uintptr_t *results = (uintptr_t *)
	    alloca(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT * sizeof(uintptr_t));
	KRN_TRACE_T *wp;
#endif

	for (depth = 0, mpc = (uint32_t *) ctx->gp.epc, mra =
	     (uint32_t *) ctx->gp.ra, msp = (uint32_t *) ctx->gp.sp;
	     (mra && ((depth < HARD_DEPTH) || (depth < SOFT_DEPTH))
	      && (_DBG_traceone(mpc, &mra, &msp, &ctx) != DONE));
	     mpc = mra, mra = 0, depth++) {
#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_HARD
		if (depth == 0)
			DBG_RTTPair(DBG_TRACE_CTX_BT, mra);
		else if (depth < CONFIG_DEBUG_TRACE_CTXSW_BT_HARD)
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
#else
void DBG_backtraceCtx(KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
void DBG_backtraceCtx(KRN_CTX_T * ctx)
{
}
#endif

#if defined(CONFIG_DEBUG_TRACE_CTXSW_BT_HARD) || defined(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT) || defined(CONFIG_DEBUG_BT_DUMP) || defined(CONFIG_DEBUG_PARANOIA) || defined(CONFIG_DEBUG_TRACE_EXTRA)
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
	uint32_t *mra = NULL;
	uint32_t *mpc = realra;
	uint32_t *msp = realsp;
	size_t size;
	KRN_CTX_T *ctx;

	/* Get the stack frame for self */
	size = 0;

	for (addr = (uint32_t *) DBG_walk; !size; addr++) {
		if ((*addr & 0xffff0000) == addiu_sp_sp_X)
			size = abs((short)(*addr & 0xffff));
		else if (*addr == jr_ra)
			break;
	}

	msp = (uint32_t *) ((uintptr_t) msp + size);

	while (depth--) {
		if ((_DBG_traceone(mpc, &mra, &msp, &ctx) == DONE) && (depth))
			return 0;
		mpc = mra;
		mra = NULL;
	}

	if (ora)
		*ora = (uintptr_t) mra;
	if (osp)
		*osp = (uintptr_t) msp;

	return 1;

}
#else
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
    __attribute__ ((no_instrument_function));
int32_t DBG_walk(uint32_t depth, uintptr_t * ora, uintptr_t * osp)
{
	return 0;
}
#endif

#if defined(CONFIG_DEBUG_BT_DUMP) || defined(CONFIG_DEBUG_REG_DUMP)
void _DBG_dumpValue(const char *pre, uint32_t value, uint32_t digits,
		    const char *post)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_DBG_dumpValue
**
** DESCRIPTION:	Dump a hex value to the debug log without using printf to
**				format, which can cause cascading exceptions.
**
** RETURNS:	void
*/
void _DBG_dumpValue(const char *pre, uint32_t value, uint32_t digits,
		    const char *post)
{
	uint32_t mask = 0xf0000000, shift;
	char output[2] = { 'X', 0 };
	DBG_logF(pre);
	for (shift = ((digits - 1) * 4), mask = 0xf << shift; mask;
	     mask >>= 4, shift -= 4) {
		output[0] = "0123456789ABCDEF"[(value & mask) >> shift];
		DBG_logF(output);
	}
	DBG_logF(post);
}
#endif

#ifdef CONFIG_DEBUG_REG_DUMP
void _DBG_regdump(KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_DBG_regdump
**
** DESCRIPTION:	Dump registers to debug log.
**
** RETURNS:	void
*/
void _DBG_regdump(KRN_CTX_T * ctx)
{
	_DBG_dumpValue("at:\t", ctx->gp.at, 8, "\t");
	_DBG_dumpValue("v0:\t", ctx->gp.v[0], 8, "\t");
	_DBG_dumpValue("v1:\t", ctx->gp.v[1], 8, "\n");

	_DBG_dumpValue("a0:\t", ctx->gp.a[0], 8, "\t");
	_DBG_dumpValue("a1:\t", ctx->gp.a[1], 8, "\t");
	_DBG_dumpValue("a2:\t", ctx->gp.a[2], 8, "\t");
	_DBG_dumpValue("a3:\t", ctx->gp.a[3], 8, "\n");

	_DBG_dumpValue("t0:\t", ctx->gp.t[0], 8, "\t");
	_DBG_dumpValue("t1:\t", ctx->gp.t[1], 8, "\t");
	_DBG_dumpValue("t2:\t", ctx->gp.t[2], 8, "\t");
	_DBG_dumpValue("t3:\t", ctx->gp.t[3], 8, "\n");

	_DBG_dumpValue("t4:\t", ctx->gp.t[4], 8, "\t");
	_DBG_dumpValue("t5:\t", ctx->gp.t[5], 8, "\t");
	_DBG_dumpValue("t6:\t", ctx->gp.t[6], 8, "\t");
	_DBG_dumpValue("t7:\t", ctx->gp.t[7], 8, "\n");

	_DBG_dumpValue("s0:\t", ctx->gp.s[0], 8, "\t");
	_DBG_dumpValue("s1:\t", ctx->gp.s[1], 8, "\t");
	_DBG_dumpValue("s2:\t", ctx->gp.s[2], 8, "\t");
	_DBG_dumpValue("s3:\t", ctx->gp.s[3], 8, "\n");

	_DBG_dumpValue("s4:\t", ctx->gp.s[4], 8, "\t");
	_DBG_dumpValue("s5:\t", ctx->gp.s[5], 8, "\t");
	_DBG_dumpValue("s6:\t", ctx->gp.s[6], 8, "\t");
	_DBG_dumpValue("s7:\t", ctx->gp.s[7], 8, "\n");

	_DBG_dumpValue("t8:\t", ctx->gp.t2[0], 8, "\t");
	_DBG_dumpValue("t9:\t", ctx->gp.t2[1], 8, "\t");
	_DBG_dumpValue("k0:\t", ctx->gp.k[0], 8, "\t");
	_DBG_dumpValue("k1:\t", ctx->gp.k[1], 8, "\n");

	_DBG_dumpValue("gp:\t", ctx->gp.gp, 8, "\t");
	_DBG_dumpValue("sp:\t", ctx->gp.sp, 8, "\t");
	_DBG_dumpValue("fp:\t", ctx->gp.fp, 8, "\t");
	_DBG_dumpValue("ra:\t", ctx->gp.ra, 8, "\n");

	_DBG_dumpValue("hi:\t", (uint32_t) ctx->gp.hi, 8, "\t");
	_DBG_dumpValue("lo:\t", (uint32_t) ctx->gp.lo, 8, "\t");
	_DBG_dumpValue("epc:\t", (uint32_t) ctx->gp.epc, 8, "\t");
	_DBG_dumpValue("BadVAddr:\t", (uint32_t) _m32c0_mfc0(C0_BADVADDR, 0),
		       8, "\n");
}
#endif

#ifdef CONFIG_DEBUG_BT_DUMP
inline void _DBG_backtrace(void)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_DBG_backtrace
**
** DESCRIPTION:	Dump backtrace to debug log. This may terminate in another
**				exception, so do it last!
**
** RETURNS:	void
*/
inline void _DBG_backtrace(void)
{
	uint32_t *addr;
	register uint32_t *realra asm("ra");
	register uint32_t *realsp asm("sp");
	uint32_t *mra = realra, *nra = NULL;
	uint32_t *msp = realsp;
	size_t size;
	KRN_CTX_T *ctx;

	/* Get the stack frame for self */
	size = 0;

	for (addr = (uint32_t *) _DBG_backtrace; !size; addr++) {
		if ((*addr & 0xffff0000) == addiu_sp_sp_X)
			size = abs((short)(*addr & 0xffff));
		else if (*addr == jr_ra)
			break;
	}

	msp = (uint32_t *) ((uintptr_t) msp + size);

	/* repeat backward scanning */
	while (mra) {
		_DBG_dumpValue("BT PC:", (uint32_t) mra, 8, " ");
		_DBG_dumpValue("SP:", (uint32_t) msp, 8, "\n");

		switch (_DBG_traceone(mra, &nra, &msp, &ctx)) {
		case DONE:
			return;
		case INT:
			DBG_logF("INT\n");
#ifdef CONFIG_DEBUG_REG_DUMP
			_DBG_regdump(ctx);
#endif
			break;
		case NEST:
			DBG_logF("NEST\n");
#ifdef CONFIG_DEBUG_REG_DUMP
			_DBG_regdump(ctx);
#endif
		default:
			break;
		}
		mra = nra;
		nra = NULL;
	}

	return;
}
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
#ifdef CONFIG_DEBUG_REG_DUMP
	if (ctx) {
		DBG_logF("Context dump\n");
		_DBG_regdump(ctx);
	}
#endif
#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
	extern void _DBG_dumpTrace();
	_DBG_dumpTrace();
#endif
#ifdef CONFIG_DEBUG_PROFILING_DUMP
	extern void _KRN_dumpPerf();
	_KRN_dumpPerf();
#endif
#ifdef CONFIG_DEBUG_BT_DUMP
	DBG_logF("Context dump\n");
	_DBG_backtrace();
#endif
}

FILE *_DBG_file;
static void _DBG_safe(void)
    __attribute__ ((constructor, no_instrument_function));
static void _DBG_safe(void)
{
	_DBG_file = stdout;
}
