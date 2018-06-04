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
*   Description:	MIPS baseline interrupt specialisation
*
*************************************************************************/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
#include <mips/mt.h>
#endif

#if defined(CONFIG_ARCH_MIPS_MICRO) || defined(CONFIG_ARCH_MIPS_MICRO_R6)
#define CLEAN_PTR(X) ((void*)(((uintptr_t)(X)) & ~1))
#else
#define CLEAN_PTR(X) ((void*)(X))
#endif

volatile int32_t _IRQ_ipl;
int32_t _IRQ_log;

void *_IRQ_intTable[IRQ_HARDINTS];
void *_IRQ_excTable[IRQ_EXCEPTIONS];
/* 32 exceptions, 8 hard ints, 32 soft ints mapped to SINT0, ipi subsumed by SINT1 */
IRQ_DESC_T *_IRQ_descTable[IRQ_EXCEPTIONS + IRQ_HARDINTS + IRQ_SOFTINTS];
volatile uint32_t _IRQ_softPend;
volatile uint32_t _IRQ_softMask;
uint32_t *_IRQ_intStack = NULL;

uint8_t *_EBASE = (uint8_t *) KSEG0_BASE;

#ifdef __cplusplus
extern "C" {
#endif
	extern void _toupee(void);
	extern void _cache_routine(void);
	const uint32_t _IPTI __attribute__ ((weak)) = 0;

#ifdef __cplusplus
}
#endif
extern uintptr_t _DBG_intStackStart;
extern uintptr_t _DBG_intStackEnd;
extern void _DBG_dumpValue(const char *pre, uint32_t value,
			   uint32_t digits, const char *post);
extern void _DBG_dump(KRN_CTX_T * ctx);

#ifdef CONFIG_DEBUG_POSTMORTEM
enum _IRQ_EXC {
	MOD = 1,
	TLBL = 2,
	TLBS = 3,
	ADEL = 4,
	ADES = 5,
	IBE = 6,
	DBE = 7,
	SYS = 8,
	BP = 9,
	RI = 10,
	CPU = 11,
	OV = 12,
	TR = 13,
	FPE = 15,
	IS1 = 16,
	CEU = 17,
	C2E = 18,
	TLBRI = 19,
	TLBXI = 20,
	WATCH = 23,
	MCHK = 24,
	THREAD = 25,
	DSPDIS = 26,
	CACHEERR = 30,
	XCPT = 0x8000
};

void _IRQ_exception(int32_t exception, KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_IRQ_exception
**
** DESCRIPTION:	Unhandled exception handler. Dumps human readable description of
**				cause.
**
** RETURNS:	void
*/
void _IRQ_exception(int32_t exception, KRN_CTX_T * ctx)
{
	static int32_t entries = 0;
	if (entries++)
		_DBG_stop(__FILE__, __LINE__);
	switch (exception) {
	case XCPT | MOD:
		DBG_logF("TLB modification exception\n\n");
		break;
	case XCPT | TLBL:
		_DBG_dumpValue("TLB error on load from 0x",
			       (uint32_t) _m32c0_mfc0(C0_BADVADDR, 0), 8, "");
		_DBG_dumpValue("@0x", (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | TLBS:
		_DBG_dumpValue("TLB error on store to 0x",
			       (uint32_t) _m32c0_mfc0(C0_BADVADDR, 0), 8, "");
		_DBG_dumpValue("@0x", (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | ADEL:
		_DBG_dumpValue("Address error on load from 0x",
			       (uint32_t) _m32c0_mfc0(C0_BADVADDR, 0), 8, "");
		_DBG_dumpValue("@0x", (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | ADES:
		_DBG_dumpValue("Address error on store to 0x",
			       (uint32_t) _m32c0_mfc0(C0_BADVADDR, 0), 8, "");
		_DBG_dumpValue("@0x", (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | IBE:
		DBG_logF("Instruction bus error\n\n");
		break;
	case XCPT | DBE:
		DBG_logF("Data bus error\n\n");
		break;
	case XCPT | SYS:
		DBG_logF("Unexpected syscall\n\n");
		break;
	case XCPT | BP:
		_DBG_dumpValue("Breakpoint @0x",
			       (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | RI:
		_DBG_dumpValue("Illegal instruction @0x",
			       (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | CPU:
		DBG_logF("Coprocessor unusable\n\n");
		break;
	case XCPT | OV:
		DBG_logF("Overflow\n\n");
		break;
	case XCPT | TR:
		DBG_logF("Trap\n\n");
		break;
	case XCPT | FPE:
		DBG_logF("Floating point error\n\n");
		break;
	case XCPT | IS1:
		DBG_logF("Coprocessor 2 implementation specific exception\n\n");
		break;
	case XCPT | CEU:
		DBG_logF("CorExtend unusable\n\n");
		break;
	case XCPT | C2E:
		DBG_logF("Precise Coprocessor 2 exception\n\n");
		break;
	case XCPT | TLBRI:
		DBG_logF("TLB read inhibit exception\n\n");
		break;
	case XCPT | TLBXI:
		DBG_logF("TLB execute inhibit exception\n\n");
		break;
	case XCPT | WATCH:
		_DBG_dumpValue("Watchpoint @0x",
			       (uint32_t) ctx->gp.epc, 8, "\n\n");
		break;
	case XCPT | MCHK:
		DBG_logF("Machine check error\n\n");
		break;
	case XCPT | THREAD:
		DBG_logF("Thread exception\n\n");
		break;
	case XCPT | DSPDIS:
		DBG_logF("DSP availability exception\n\n");
		break;
	case XCPT | CACHEERR:
		DBG_logF("Cache error\n\n");
		break;
	default:
		if (exception & XCPT)
			_DBG_dumpValue("Exception 0x",
				       (uint32_t) exception & 0x7fff, 2,
				       "\n\n");
		else
			_DBG_dumpValue("Unexpected interrupt 0x",
				       (uint32_t) exception, 2, "\n\n");
		break;
	}
	_DBG_dump(ctx);
	_DBG_stop(__FILE__, __LINE__);
}

IRQ_DESC_T _IRQ_exceptions[IRQ_EXCEPTIONS];
#endif

static void _IRQ_dispatchSoft(int32_t intNum, KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
/*
** FUNCTION:	_IRQ_dispatchSoft
**
** DESCRIPTION:	Demultiplex and dispatch software interrupts.
**
** RETURNS:	void
*/
static void _IRQ_dispatchSoft(int32_t intNum, KRN_CTX_T * ctx)
{
	int32_t sintNum =
	    IRQ_HARDINTS + (31 - __builtin_clz(_IRQ_softPend & _IRQ_softMask));
	IRQ_DESC_T *desc = _IRQ_descTable[IRQ_EXCEPTIONS + sintNum];
#ifdef CONFIG_DEBUG_POSTMORTEM
	if (desc == NULL)
		_IRQ_exception(intNum, ctx);
#else
	DBG_assert(desc != NULL, "Soft interrupt with no handler!");
#endif
	desc->isrFunc(sintNum);
}

/*
** FUNCTION:	IRQ_init
**
** DESCRIPTION:	Initialise the interrupt system.
**
** RETURNS:	void
*/
void IRQ_init(uint32_t * intStack, size_t issize)
    __attribute__ ((no_instrument_function));
void IRQ_init(uint32_t * intStack, size_t issize)
{
	if (!intStack) {
		if (!_IRQ_intStack) {
#ifndef CONFIG_ARCH_MIPS_VZ
			asm volatile ("move $k0, $zero");
#else
			mips32_set_c0(C0_KSCRATCH1, 0);
#endif
		}
		return;
	}
#ifndef CONFIG_ARCH_MIPS_VZ
	asm volatile ("move $k0, $zero");
#else
	mips32_set_c0(C0_KSCRATCH1, 0);
#endif
	/* Align intstack */
	uintptr_t uintStack = (uintptr_t) intStack;
	if (uintStack & 7) {
		issize -= 8 - (uintStack & 7);
		intStack = (uint32_t *) ((uintStack & ~7) + 8);
	}
	issize &= ~7;

	/* Initialise interrupt control data */
	_IRQ_intStack = intStack + issize - 4;
	_DBG_intStackStart = (uintptr_t) intStack;
	_DBG_intStackEnd = (uintptr_t) _IRQ_intStack;
	_IRQ_ipl = 0;
	_IRQ_softPend = 0;
	_IRQ_softMask = 0;

	/* Do we support EBase? */
	if (((mips32_getconfig0() & CFG0_AR_MASK) >> CFG0_AR_SHIFT) > 0) {
		/* Find out where our vectors should go */
		uint32_t EBase = mips32_getebase();
		_EBASE = (uint8_t *) (EBase & EBASE_BASE);
		/* Also, which processor we are */
		_KRN_schedule->hwThread = EBase & EBASE_CPU;
	}
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
	/* Get the number of VPEs */
	mips32_setmvpcontrol((mips32_getmvpcontrol() & ~MVPCONTROL_EVP)
			     | MVPCONTROL_VPC);
	_KRN_schedule->hwThreads =
	    ((mips32_getmvpconf0() & MVPCONF0_PVPE) >> MVPCONF0_PVPE_SHIFT) + 1;
	mips32_setmvpcontrol((mips32_getmvpcontrol() & ~MVPCONTROL_VPC)
			     | MVPCONTROL_EVP);
	if (_KRN_schedule->hwThreads > CONFIG_FEATURE_MAX_PROCESSORS)
		_KRN_schedule->hwThreads = CONFIG_FEATURE_MAX_PROCESSORS;
#else
	_KRN_schedule->hwThreads = 1;
#endif
}

/*
** FUNCTION:	_IRQ_deferTimer
**
** DESCRIPTION:	Used to make timers go away when unhandled. This is needed
**				because timers can never be truly turned off independently of
**				FDC/PC.
**
** RETURNS:	void
*/
static void _IRQ_deferTimer(int num, KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
static void _IRQ_deferTimer(int num, KRN_CTX_T * ctx)
{
	mips_setcompare(mips_getcompare());
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
	int32_t i;

	/* Initialise k0 to a safe value */
	if (_KRN_current)
		asm volatile ("move $k0, %0"::
			      "r" (&(_KRN_current->savedContext)));

#if defined(CONFIG_ARCH_MIPS_MICRO) || defined(CONFIG_ARCH_MIPS_MICRO_R6)
	/* Ensure exceptions are encoded correctly! */
	_m32c0_mtc0(C0_CONFIG, 3, mips32_getconfig3() | CFG3_IOE);
#endif

	_IRQ_excTable[0] = NULL;
#ifdef CONFIG_DEBUG_POSTMORTEM
	/* Install unhandled exception handler */
	for (i = 1; i < IRQ_EXCEPTIONS; i++) {
		_IRQ_exceptions[i - 1].intNum = 0x8000 + i;
		_IRQ_exceptions[i - 1].isrFunc =
		    (IRQ_ISRFUNC_T *) _IRQ_exception;
		IRQ_route(&_IRQ_exceptions[i - 1]);
	}
	for (i = 1; i < IRQ_HARDINTS - 2; i++)
		_IRQ_intTable[i] = (void *)_IRQ_exception;
	_IRQ_intTable[IRQ_TIMER] = _IRQ_deferTimer;
	_IRQ_intTable[IRQ_FDC] = NULL;
#else
	/*
	 * Explode on exceptions.
	 */
	for (i = 1; i < IRQ_EXCEPTIONS; i++)
		_IRQ_excTable[i] = NULL;
	for (i = 1; i < IRQ_HARDINTS - 2; i++)
		_IRQ_intTable[i] = NULL;
	_IRQ_intTable[IRQ_TIMER] = _IRQ_deferTimer;
	_IRQ_intTable[IRQ_FDC] = NULL;
#endif
	/* Install software interrupt demultiplexer */
	_IRQ_intTable[0] = (void *)_IRQ_dispatchSoft;
	/* Install base interrupt vectors */
	mips_bicsr(SR_IE);
	mips_biccr(CR_IV);
	memcpy(_EBASE, CLEAN_PTR(_toupee), 0x100);
	memcpy(_EBASE + 0x100, CLEAN_PTR(_cache_routine), 0x80);
	memcpy(_EBASE + 0x180, CLEAN_PTR(_toupee), 0x80);
	KRN_flushCache(_EBASE, 0x200,
		       KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_syncCache(_EBASE, 0x200);
	/* And activate them */
	mips_bcssr(SR_BEV, SR_IM0 | SR_IE);

#ifdef CONFIG_ARCH_MIPS_PCINT
	/* Switch off PC int triggers */
	_m32c0_mtc0(C0_PERFCNT, 0, 0);
	_m32c0_mtc0(C0_PERFCNT, 2, 0);
#ifdef CONFIG_ARCH_MIPS_QPC
	_m32c0_mtc0(C0_PERFCNT, 4, 0);
	_m32c0_mtc0(C0_PERFCNT, 6, 0);
#endif
	/* Switch on PC ints so they go off once configured. */
	mips_bissr(SR_IM0 <<
		   ((mips32_getintctl() & INTCTL_IPPCI) >> INTCTL_IPPCI_SHIFT));
#endif
}

#ifdef CONFIG_ARCH_MIPS_VZ
/*
** FUNCTION:	_IRQ_VZE
**
** DESCRIPTION:	Enable VZE.
**
** RETURNS:	int32_t
*/
int32_t _IRQ_VZE(void)
{
	return 0;
}
#endif

/*
** FUNCTION:	IRQ_route
**
** DESCRIPTION:	Route an interrupt
**
** RETURNS:	void
*/
void IRQ_route(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	int32_t ipti, ipfdc;
#ifdef CONFIG_ARCH_MIPS_PCINT
	int32_t ippci;
#endif
	if (intNum & 0x8000) {	/* exception */
		intNum &= 0x7fff;
		if (irqDesc->isrFunc) {
			_IRQ_excTable[intNum] = (void *)irqDesc->isrFunc;
			_IRQ_descTable[intNum] = irqDesc;
		} else {
			_IRQ_excTable[intNum] = NULL;
			_IRQ_descTable[intNum] = NULL;
		}
	} else {
		if ((intNum >= 0) && (intNum <= 1)) {
			DBG_insist(0,
				   "Do not directly route soft interrupts, use the appropriate factory function. Soft interrupts/IPI now broken.\n");
		}
		if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS)
			intNum = 1;
		if (intNum < IRQ_HARDINTS) {	/* hard */
			ipti = _IPTI ? _IPTI :
			    ((mips32_getintctl() & INTCTL_IPTI) >>
			     INTCTL_IPTI_SHIFT);
			if (irqDesc->isrFunc) {
				_IRQ_intTable[intNum] =
				    (void *)irqDesc->isrFunc;
				_IRQ_descTable[IRQ_EXCEPTIONS + intNum] =
				    irqDesc;
				switch (intNum) {
				case IRQ_TIMER:
					mips_bissr(SR_IM0 << ipti);
					break;
				case IRQ_FDC:
					mips_bissr(SR_IM0 << ipti);
					break;
#ifdef CONFIG_ARCH_MIPS_MCU_ASE
				case 9:
					mips_bissr(SR_IM0 << 10);
					break;
#endif
				default:
					mips_bissr(SR_IM0 << intNum);
				}
			} else {
				switch (intNum) {
				case IRQ_TIMER:
					_IRQ_intTable[intNum] = _IRQ_deferTimer;
					break;
				case IRQ_FDC:
					_IRQ_intTable[intNum] = NULL;
					break;
				default:
#ifdef CONFIG_DEBUG_POSTMORTEM
					_IRQ_intTable[intNum] = _IRQ_exception;
#else
					_IRQ_intTable[intNum] = NULL;
#endif
				}
				ipfdc =
				    ((mips32_getintctl() & INTCTL_IPFDC) >>
				     INTCTL_IPFDC_SHIFT);
#ifdef CONFIG_ARCH_MIPS_PCINT
				ippci =
				    ((mips32_getintctl() & INTCTL_IPPCI) >>
				     INTCTL_IPPCI_SHIFT);
#endif
				switch (intNum) {
				case IRQ_TIMER:
#ifdef CONFIG_DEBUG_POSTMORTEM
					if ((ipti == ipfdc
					     && _IRQ_intTable[IRQ_FDC] ==
					     (void *)_IRQ_exception)
					    && (_IRQ_intTable[ipti] ==
						(void *)_IRQ_exception)
#ifdef CONFIG_ARCH_MIPS_PCINT
					    && (ipti == ippci
						&& (_IRQ_intTable[ippci] ==
						    (void *)_IRQ_exception))
#endif
					    )
#else
					if ((ipti == ipfdc
					     && _IRQ_intTable[IRQ_FDC] == NULL)
					    && (_IRQ_intTable[ipti] == NULL)
#ifdef CONFIG_ARCH_MIPS_PCINT
					    && (ipti == ippci
						&& (_IRQ_intTable[ippci] ==
						    NULL))
#endif
					    )
#endif
						mips_bicsr(SR_IM0 << ipti);
					break;
				case IRQ_FDC:
#ifdef CONFIG_DEBUG_POSTMORTEM
					if ((ipti == ipfdc
					     && _IRQ_intTable[IRQ_TIMER] ==
					     (void *)_IRQ_deferTimer)
					    && (_IRQ_intTable[ipfdc] ==
						(void *)_IRQ_exception)
#ifdef CONFIG_ARCH_MIPS_PCINT
					    && (ipfdc == ippci
						&& (_IRQ_intTable[ippci] ==
						    (void *)_IRQ_exception))
#endif
					    )
#else
					if ((ipti == ipfdc
					     && _IRQ_intTable[IRQ_TIMER] ==
					     (void *)_IRQ_deferTimer)
					    && (_IRQ_intTable[ipfdc] == NULL)
#ifdef CONFIG_ARCH_MIPS_PCINT
					    && (ipfdc == ippci
						&& (_IRQ_intTable[ippci] ==
						    NULL))
#endif
					    )
#endif
						mips_bicsr(SR_IM0 << ipfdc);
					break;
#ifdef CONFIG_ARCH_MIPS_MCU_ASE
				case 9:
					mips_bicsr(SR_IM0 << 10);
					break;
#endif
				default:
					mips_bicsr(SR_IM0 << intNum);
				}
				_IRQ_descTable[IRQ_EXCEPTIONS + intNum] = NULL;
			}
		} else {	/* soft */
			if (irqDesc->isrFunc) {
				_IRQ_descTable[IRQ_EXCEPTIONS + intNum] =
				    irqDesc;
				__sync_or_and_fetch(&_IRQ_softMask,
						    1 << (intNum -
							  IRQ_HARDINTS));
				if (_IRQ_softPend & _IRQ_softMask)
					mips_biscr(CR_SINT0);
			} else {
				__sync_and_and_fetch(&_IRQ_softMask,
						     ~(1 <<
						       (intNum -
							IRQ_HARDINTS)));
				_IRQ_descTable[IRQ_EXCEPTIONS + intNum]
				    = NULL;
			}
		}
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
	int32_t intNum = desc->intNum;
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS)
		intNum = 1;
	return IRQ_cause(intNum);
}

/*
** FUNCTION:	IRQ_cause
**
** DESCRIPTION:	Find the installed descriptor for the supplied core interrupt
**				number.
**
** RETURNS:	void
*/
IRQ_DESC_T *IRQ_cause(int32_t intNum)
{
	return _IRQ_descTable[(intNum & 0x8000)
			      ? (intNum & 0x7fff)
			      : (IRQ_EXCEPTIONS + intNum)];
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
	IRQ_IPL_T ipl;
	if (!irqDesc)
		return NULL;
	int32_t intNum = irqDesc->intNum;
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS)
		intNum = 1;
	ipl = IRQ_raiseIPL();
	if (intNum >= IRQ_HARDINTS) {
		_IRQ_softPend &= ~(1 << (intNum - IRQ_HARDINTS));
		if (_IRQ_softPend == 0)
			mips_biccr(CR_SINT0);
	} else {
		switch (intNum) {
		case IRQ_TIMER:
			mips_setcompare(mips_getcompare());
			break;
		case 0:
			mips_biccr(CR_SINT0);
			break;
		case 1:
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
			mips_mt_dvpe();
#endif
			mips_biccr(CR_SINT1);
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
			mips_mt_evpe();
#endif
		default:
			break;
		}
	}
	IRQ_restoreIPL(ipl);
	return irqDesc;
}

/*
** FUNCTION:	IRQ_synthesize
**
** DESCRIPTION:	Synthesize an interrupt.
**
** RETURNS:	void
*/
void IRQ_synthesize(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	if (intNum < IRQ_HARDINTS) {	/* hard */
		switch (irqDesc->intNum) {
		case 0:
			mips_biscr(CR_SINT0);
			break;
		case 1:
			mips_biscr(CR_SINT1);
		default:
			/* Can't synthesize anything else */
			break;
		}
	} else if (intNum < IRQ_HARDINTS + IRQ_SOFTINTS) {	/* soft */
		__sync_or_and_fetch(&_IRQ_softPend,
				    1 << (intNum - IRQ_HARDINTS));
		if (_IRQ_softPend & _IRQ_softMask)
			mips_biscr(CR_SINT0);
	} else {		/* ipi */
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
		/*
		 * Switch into single threaded configuration mode, set the soft int bit
		 * on another VPE, then switch back to multithreaded mode.
		 */
		IRQ_IPL_T ipl;
		int32_t target = intNum - (IRQ_HARDINTS + IRQ_SOFTINTS);
		if (target >= _KRN_schedule->hwThreads)
			return;
		ipl = IRQ_raiseIPL();
		mips_mt_dvpe();
		int32_t previous = mips32_mt_gettarget();
		mips32_mt_settarget(target);
		/* Work around: if the core is offline due to debug, hold off */
		while (_m32c0_mftc0(23, 0) & 0x80) {
			mips32_mt_settarget(previous);
			mips_mt_evpe();
			mips_mt_yield(-2);
			mips_mt_dvpe();
			mips32_mt_settarget(target);
		}
		mips32_mt_setc0cause(mips32_mt_getc0cause() | CR_SINT1);
		mips32_mt_settarget(previous);
		mips_mt_evpe();
		IRQ_restoreIPL(ipl);
#endif
	}
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
	out->intNum = index + IRQ_HARDINTS;
	return 1;
}

/*
** FUNCTION:	IRQ_ipi
**
** DESCRIPTION:	Fill out a descriptor for an IPI.
**
** RETURNS:	int32_t
*/
int32_t IRQ_ipi(int32_t ipi, IRQ_DESC_T * out)
{
	(void)ipi;
	out->intNum = ipi + IRQ_HARDINTS + IRQ_SOFTINTS;
	return 1;
}
