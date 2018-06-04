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
*   Description:	MIPS HW interrupt specialisation
*
*************************************************************************/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"

#ifdef CONFIG_ARCH_MIPS_VZ
#define INTCTL_VS_X INTCTL_VS_64
#define VECSPACING 64
#else
#define INTCTL_VS_X INTCTL_VS_32
#define VECSPACING 32
#endif

#if defined(CONFIG_ARCH_MIPS_MICRO) || defined(CONFIG_ARCH_MIPS_MICRO_R6)
#define CLEAN_PTR(X) ((void*)(((uintptr_t)(X)) & ~1))
#define UOFFSET 2
#else
#define CLEAN_PTR(X) ((void*)(X))
#ifdef CONFIG_ARCH_MIPS_LITTLE_ENDIAN
#define UOFFSET 0
#else
#define UOFFSET 2
#endif
#endif

#ifndef CONFIG_ARCH_MIPS_HWSTAT_BASE
#define CONFIG_ARCH_MIPS_HWSTAT_BASE 0xa4000050
#endif

static volatile uint32_t *HWBASE = (uint32_t *) (CONFIG_ARCH_MIPS_HWSTAT_BASE);

#define HWCLEAR HWBASE[0x000/sizeof(uint32_t)]
#define HWVEC HWBASE[0x010/sizeof(uint32_t)]
#define HWSTAT HWBASE[0x060/sizeof(uint32_t)]

#define MIPS_SW_INT_0_HWSTAT 0x80000000
#define MIPS_SW_INT_1_HWSTAT 0x40000000
#define MIPS_TIMER_INT_HWSTAT 0x20000000

volatile int32_t _IRQ_ipl;
int32_t _IRQ_log;
int32_t _IRQ_numInts;
int32_t _IRQ_numExts;

static void _IRQ_dummyFunc(int32_t sig);

void *_IRQ_excTable[IRQ_EXCEPTIONS];
LST_T _IRQ_descTable[IRQ_EXCEPTIONS + IRQ_HARDINTS];	/* 32 exceptions, max 128 hard ints, 32 soft ints */
IRQ_DESC_T _IRQ_dummy = {.isrFunc = _IRQ_dummyFunc,
	.impSpec = {.pend = (uint32_t *) & _IRQ_dummy.impSpec.pend,
		    .bit = 0}
};

IRQ_DESC_T *_IRQ_timer = &_IRQ_dummy, *_IRQ_fdc = &_IRQ_dummy;	/* Quick access */

static void _IRQ_dummyFunc(int32_t sig)
{
	if (_IRQ_timer == &_IRQ_dummy) {
		/* Try to make sure we don't take a timer if it's dummied out */
		mips_setcompare(mips_getcount());
	}
	return;
}

volatile uint32_t _IRQ_softPend;
volatile uint32_t _IRQ_softMask;
uint32_t *_IRQ_intStack = NULL;

uint8_t *_EBASE = (uint8_t *) KSEG0_BASE;

#ifdef __cplusplus
extern "C" {
#endif

	extern void _toupee(void);
	extern void _template(void);
	extern void _templatea0(void);
	extern void _templates1h(void);
	extern void _templates1l(void);
	extern void _cache_routine(void);

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

IRQ_DESC_T _IRQ_exceptions[31];
#endif

/*
** FUNCTION:	IRQ_setBase
**
** DESCRIPTION:	Locate HW.
**
** RETURNS:	void
*/
void IRQ_setBase(void *hwBase)
{
	HWBASE = (uint32_t *) hwBase;
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
			_m32c0_mtc0(31, 2, 0);	/* Stick context in KSCRATCH1 */
#endif
		}
		return;
	}
#ifndef CONFIG_ARCH_MIPS_VZ
	asm volatile ("move $k0, $zero");
#else
	_m32c0_mtc0(31, 2, 0);	/* Stick context in KSCRATCH1 */
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
	_IRQ_timer = &_IRQ_dummy;
	_IRQ_fdc = &_IRQ_dummy;

	/* Do we support EBase? */
	if (((mips32_getconfig0() & CFG0_AR_MASK) >> CFG0_AR_SHIFT) > 0) {
		/* Find out where our vectors should go */
		uint32_t EBase = mips32_getebase();
		_EBASE = (uint8_t *) (EBase & EBASE_BASE);
		/* Also, which processor we are */
		_KRN_schedule->hwThread = EBase & EBASE_CPU;
	}
	/* Get the number of processors */
	_KRN_schedule->hwThreads = 1;
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
	int32_t i, j;

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
#else
	/*
	 * Explode on exceptions.
	 */
	for (i = 1; i < IRQ_EXCEPTIONS; i++)
		_IRQ_excTable[i] = NULL;
#endif
	/* Install base interrupt vectors */
	mips_bicsr(SR_IE);
	mips_biccr(CR_IV);
	memcpy(_EBASE, CLEAN_PTR(_toupee), 0x100);
	memcpy(_EBASE + 0x100, CLEAN_PTR(_cache_routine), 0x80);
	memcpy(_EBASE + 0x180, CLEAN_PTR(_toupee), 0x80);

	/* Get the number of interrupts */
	_IRQ_numExts = 128;
	_IRQ_numInts = 9;
	for (j = 0; i < _IRQ_numExts / 32; j++)
		(&HWCLEAR)[j++] = 0xffffffff;
	/* Patching the interrupt into the vectored handler is compatible with both
	   EIC and non-EIC mode */
	for (i = 0; i <= _IRQ_numInts; i++) {
		LST_init(&_IRQ_descTable[IRQ_EXCEPTIONS + i]);
		uint16_t *patch;
		memcpy(_EBASE + 0x200 + (i * VECSPACING), CLEAN_PTR(_template),
		       VECSPACING);
		patch =
		    (uint16_t *) (UOFFSET + (uintptr_t) _EBASE + 0x200 +
				  (i * VECSPACING) +
				  (((uintptr_t) CLEAN_PTR(_templatea0)) -
				   ((uintptr_t) CLEAN_PTR(_template))));
		*patch = (uint16_t) i;
	}
	/* Make sure all modified code is flushed */
	KRN_flushCache(_EBASE, 0x200 + (_IRQ_numInts * VECSPACING),
		       KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_syncCache(_EBASE, 0x200 + (_IRQ_numInts * VECSPACING));
	/* And activate them */
	mips32_setintctl((mips32_getintctl() & ~INTCTL_VS) | INTCTL_VS_X);
	mips_biscr(CR_IV);
	mips_bcssr(SR_BEV | SR_IMASK, SR_IE);

#ifdef CONFIG_ARCH_MIPS_PCINT
	mips_bissr(SR_IM0 <<
		   ((mips32_getintctl() & INTCTL_IPPCI) >> INTCTL_IPPCI_SHIFT));
#endif
}

/*
** FUNCTION:	_IRQ_remove
**
** DESCRIPTION:	Remove an interrupt from the list of interrupts associated with
**              a core interrupt number.
**
** RETURNS:	void
*/
inline static void _IRQ_remove(int32_t intNum, IRQ_DESC_T * irqDesc)
{
	int32_t dIntNum = irqDesc->intNum;
	int32_t extNum = irqDesc->impSpec.extNum;
	IRQ_DESC_T *candidate =
	    (IRQ_DESC_T *) LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
	while (candidate) {
		if ((candidate->intNum == dIntNum)
		    && (candidate->impSpec.extNum == extNum)) {
			LST_remove(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum],
				   candidate);
			break;
		}
		candidate = (IRQ_DESC_T *) LST_next(candidate);
	}
}

/*
** FUNCTION:	_IRQ_insert
**
** DESCRIPTION:	Insert an interrupt from the list of interrupts associated with
**              a core interrupt number.
**
** RETURNS:	void
*/
inline static void _IRQ_insert(int32_t intNum, IRQ_DESC_T * irqDesc,
			       IRQ_ISRFUNC_T * isr)
{
	uint16_t *patch;
	uintptr_t isrFunc = (uintptr_t) isr;
	/* Insert it into the list */
	LST_add(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum], irqDesc);
	/* Patch the entry point appropriately */
	patch = (uint16_t *) (UOFFSET +
			      (uintptr_t) _EBASE + 0x200 +
			      (intNum * VECSPACING) +
			      (((uintptr_t) CLEAN_PTR(_templates1l)) -
			       ((uintptr_t) CLEAN_PTR(_template))));
	*patch = (uint16_t) (isrFunc & 0xffff);
	patch = (uint16_t *) (UOFFSET +
			      (uintptr_t) _EBASE + 0x200 +
			      (intNum * VECSPACING) +
			      (((uintptr_t) CLEAN_PTR(_templates1h)) -
			       ((uintptr_t) CLEAN_PTR(_template))));
	*patch = (uint16_t) (isrFunc >> 16);
	/* Make sure all modified code is flushed */
	KRN_flushCache(_EBASE + 0x200 + (intNum * VECSPACING), VECSPACING,
		       KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_syncCache(_EBASE + 0x200 + (intNum * VECSPACING), VECSPACING);
}

/*
** FUNCTION:	_IRQ_sharedShunt
**
** DESCRIPTION:	Demultiplex and dispatch core and software interrupts.
**
** RETURNS:	void
*/
void _IRQ_sharedShunt(int32_t intNum)
{
	IRQ_DESC_T *candidate;
	uint32_t best;
	reg_t cr = mips_getcr();
	if (cr & CR_TI)
		_IRQ_timer->isrFunc(_IRQ_timer->intNum);
	if (cr & CR_FDCI)
		_IRQ_fdc->isrFunc(_IRQ_fdc->intNum);
	best = 1 << (31 - __builtin_clz(_IRQ_softPend & _IRQ_softMask));
	candidate =
	    (IRQ_DESC_T *) LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
	while (candidate) {
		if (candidate->impSpec.bit == best) {
			candidate->isrFunc(candidate->intNum);
			return;
		}
		candidate = (IRQ_DESC_T *) LST_next(candidate);
	}
}

/*
** FUNCTION:	_IRQ_routeException
**
** DESCRIPTION:	Route an exception
**
** RETURNS:	void
*/
inline static void _IRQ_routeException(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum & 0x7fff;
	if (irqDesc->isrFunc) {
		_IRQ_excTable[intNum] = (void *)irqDesc->isrFunc;
		LST_init(&_IRQ_descTable[intNum]);
		LST_add(&_IRQ_descTable[intNum], irqDesc);
	} else {
		_IRQ_excTable[intNum] = NULL;
		LST_init(&_IRQ_descTable[intNum]);
	}
	irqDesc->impSpec.pend = 0;
	irqDesc->impSpec.bit = 0;
}

/*
** FUNCTION:	_IRQ_enableSource
**
** DESCRIPTION:	Enable a core interrupt in non-EIC mode
**
** RETURNS:	void
*/
inline static void _IRQ_enableSource(int32_t intNum)
{
	if (intNum == 9)
		mips_bissr(SR_IM0 << 10);
	else
		mips_bissr(SR_IM0 << intNum);
}

/*
** FUNCTION:	_IRQ_disableSource
**
** DESCRIPTION:	Disable a core interrupt in non-EIC mode
**
** RETURNS:	void
*/
inline static void _IRQ_disableSource(int32_t intNum)
{
	if (LST_empty(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum])) {
		if (intNum == 9)
			mips_bicsr(SR_IM0 << 10);
		else
			mips_bicsr(SR_IM0 << intNum);
	}
}

/*
** FUNCTION:	_IRQ_routeTimer
**
** DESCRIPTION:	Route a timer interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeTimer(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	irqDesc->intNum = (mips32_getintctl() & INTCTL_IPTI)
	    >> INTCTL_IPTI_SHIFT;
	/* Make sure we don't double install */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Install interrupt */
	irqDesc->impSpec.set = NULL;
	irqDesc->impSpec.clear = NULL;
	irqDesc->impSpec.pend = (void *)-1;
	irqDesc->impSpec.bit = CR_TI;
	if (intNum == _IRQ_fdc->intNum)
		_IRQ_insert(intNum, irqDesc, _IRQ_sharedShunt);
	else
		_IRQ_insert(intNum, irqDesc, irqDesc->isrFunc);
	_IRQ_timer = irqDesc;
	_IRQ_enableSource(intNum);
}

/*
** FUNCTION:	_IRQ_unrouteTimer
**
** DESCRIPTION:	Unroute a timer interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteTimer(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	irqDesc->intNum = (mips32_getintctl() & INTCTL_IPTI)
	    >> INTCTL_IPTI_SHIFT;
	/* Remove it */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Disable it */
	_IRQ_disableSource(intNum);
}

/*
** FUNCTION:	_IRQ_routeFDC
**
** DESCRIPTION:	Route an FDC interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeFDC(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	irqDesc->intNum = (mips32_getintctl() & INTCTL_IPFDC)
	    >> INTCTL_IPFDC_SHIFT;
	/* Make sure we don't double install */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Install interrupt */
	irqDesc->impSpec.set = NULL;
	irqDesc->impSpec.clear = NULL;
	irqDesc->impSpec.pend = (void *)-1;
	irqDesc->impSpec.bit = CR_FDCI;
	if (intNum == _IRQ_timer->intNum)
		_IRQ_insert(intNum, irqDesc, _IRQ_sharedShunt);
	else
		_IRQ_insert(intNum, irqDesc, irqDesc->isrFunc);
	_IRQ_fdc = irqDesc;
	_IRQ_enableSource(intNum);
}

/*
** FUNCTION:	_IRQ_unrouteFDC
**
** DESCRIPTION:	Unroute an FDC interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteFDC(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	irqDesc->intNum = (mips32_getintctl() & INTCTL_IPFDC)
	    >> INTCTL_IPFDC_SHIFT;
	/* Remove it */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Disable it */
	_IRQ_fdc = &_IRQ_dummy;
	_IRQ_disableSource(intNum);
}

/*
** FUNCTION:  _IRQ_routeHard
**
** DESCRIPTION:       Route an external hardware interrupt
**
** RETURNS:   void
*/
inline static void _IRQ_routeHard(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	int32_t extNum = irqDesc->impSpec.extNum;
	uint32_t word = (extNum >> 5), bit = 1 << (extNum & 0x1f);
	/* Make sure we don't double install */
	_IRQ_remove(intNum, irqDesc);
	/* -1 means don't do HWSTAT magic */
	if (extNum >= 0) {
		/* Reconfigure HWVEC as per descriptor */
		(&HWVEC)[extNum >> 3] =
		    ((&HWVEC)[extNum >> 3] & ~(0x7 << (extNum & 0x7))) | (intNum
									  <<
									  (extNum
									   &
									   0x7));
		/* Install interrupt */
		irqDesc->impSpec.set = NULL;
		irqDesc->impSpec.clear = NULL;
		irqDesc->impSpec.pend = &(&HWSTAT)[word];
		irqDesc->impSpec.bit = bit;
	} else {
		irqDesc->impSpec.set = NULL;
		irqDesc->impSpec.clear = NULL;
		irqDesc->impSpec.pend = NULL;
		irqDesc->impSpec.bit = 0;
	}
	_IRQ_insert(intNum, irqDesc, irqDesc->isrFunc);
	_IRQ_enableSource(intNum);
}

/*
** FUNCTION:	_IRQ_unrouteHard
**
** DESCRIPTION:	Unroute an external hardware interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteHard(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	int32_t extNum = irqDesc->impSpec.extNum;
	if (extNum >= 0) {
		/* Reconfigure VEC as per descriptor */
		(&HWVEC)[extNum >> 3] &= ~(0x7 << (extNum & 0x7));
	}
	/* Remove it */
	_IRQ_remove(intNum, irqDesc);
	/* Disable it */
	_IRQ_disableSource(intNum);
}

/*
** FUNCTION:	_IRQ_wireSoft
**
** DESCRIPTION:	Install an ISR for the underlying soft interrupt
**
** RETURNS:	void
*/
inline void _IRQ_wireSoft(IRQ_DESC_T * irqDesc)
{
	/* Make sure we don't double install */
	_IRQ_remove(0, irqDesc);
	/* Install interrupt */
	_IRQ_insert(0, irqDesc, _IRQ_sharedShunt);
	mips_bissr(SR_IM0);
}

/*
** FUNCTION:	_IRQ_routeSoft
**
** DESCRIPTION:	Route a software interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeSoft(IRQ_DESC_T * irqDesc)
{
	irqDesc->impSpec.pend = (uint32_t *) & _IRQ_softPend;
	irqDesc->impSpec.bit = 1 << (irqDesc->intNum - IRQ_HARDINTS);
	/* Ensure demultiplexer is installed */
	_IRQ_wireSoft(irqDesc);
	/* Configure demultiplexer */
	__sync_or_and_fetch(&_IRQ_softMask,
			    1 << (irqDesc->intNum - IRQ_HARDINTS));
	/* There are pending soft interrupts - trigger them, in case the new handler can take it */
	if (_IRQ_softPend & _IRQ_softMask)
		mips_biscr(CR_SINT0);
}

/*
** FUNCTION:	_IRQ_unrouteSoft
**
** DESCRIPTION:	Unroute a software interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteSoft(IRQ_DESC_T * irqDesc)
{
	irqDesc->impSpec.pend = (uint32_t *) & _IRQ_softPend;
	irqDesc->impSpec.bit = 1 << (irqDesc->intNum - IRQ_HARDINTS);
	/* Deconfigure demultiplexer */
	__sync_and_and_fetch(&_IRQ_softMask, ~irqDesc->impSpec.bit);
	/* Disable underlying interrupt if no software interrupts enabled */
	_IRQ_remove(0, irqDesc);
	if (!_IRQ_softMask)
		mips_bicsr(SR_IM0);
	/* There are pending soft interrupts - trigger them */
	if (_IRQ_softPend & _IRQ_softMask)
		mips_biscr(CR_SINT0);
}

/*
** FUNCTION:	IRQ_route
**
** DESCRIPTION:	Work out which kind of interrupt it is, and call routing
**              function.
**
** RETURNS:	void
*/
void IRQ_route(IRQ_DESC_T * irqDesc)
{
	_IRQ_disable();
	int32_t intNum = irqDesc->intNum;
	if (intNum & 0x8000) {	/* exception */
		_IRQ_routeException(irqDesc);	/* route and unroute the same op */
	} else {
		if ((irqDesc->impSpec.extNum != IRQ_TIMER)
		    && ((intNum >= 0) && (intNum <= 1))) {
			DBG_insist(0,
				   "Do not directly route soft interrupts, use the appropriate factory function. Soft interrupts/IPI now broken.\n");
		}

		if (intNum < IRQ_HARDINTS) {	/* hard */
			switch (irqDesc->impSpec.extNum) {
			case IRQ_TIMER:
				if (irqDesc->isrFunc)
					_IRQ_routeTimer(irqDesc);
				else
					_IRQ_unrouteTimer(irqDesc);
				break;
			case IRQ_FDC:
				if (irqDesc->isrFunc)
					_IRQ_routeFDC(irqDesc);
				else
					_IRQ_unrouteFDC(irqDesc);
				break;
			default:
				if (irqDesc->isrFunc)
					_IRQ_routeHard(irqDesc);
				else
					_IRQ_unrouteHard(irqDesc);
			}
		} else {	/* soft */
			if (irqDesc->isrFunc)
				_IRQ_routeSoft(irqDesc);
			else
				_IRQ_unrouteSoft(irqDesc);
		}
	}

	_IRQ_enable();
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
	/* Search through the lists of descriptors for the best match */
	int32_t intNum = desc->intNum, dIntNum = intNum;
	int32_t extNum = desc->impSpec.extNum;
	IRQ_DESC_T *candidate;
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS) {
		if ((intNum - (IRQ_HARDINTS + IRQ_SOFTINTS))
		    != (int32_t) _KRN_schedule->hwThread) {
			DBG_assert(0,
				   "Can not IRQ_find IPI interrupts for other threads\n");
			return NULL;
		}
		intNum = 2;
	}
	if (intNum & 0x8000) {
		return (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[intNum & 0x7fff]);
	} else {
		if (intNum >= IRQ_HARDINTS)
			intNum = 0;
		candidate = (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
		while (candidate) {
			if ((candidate->intNum = dIntNum)
			    && (candidate->impSpec.extNum == extNum))
				return candidate;
			candidate = (IRQ_DESC_T *) LST_next(candidate);
		}
	}
	return NULL;
}

static uint32_t IRQ_pend(volatile uint32_t * pend)
{
	if (pend == (void *)-1)
		return mips_getcr();
	else
		return *pend;
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
	/* Search through the lists of descriptors for the one responsible */
	IRQ_DESC_T *best = NULL;
	IRQ_DESC_T *candidate;
	int32_t bestBit = 0;
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS) {
		if ((intNum - (IRQ_HARDINTS + IRQ_SOFTINTS))
		    != (int32_t) _KRN_schedule->hwThread) {
			DBG_assert(0,
				   "Can not IRQ_cause IPI interrupts for other threads\n");
			return NULL;
		}
		intNum = 2;
	}
	if (intNum & 0x8000) {
		return (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[intNum & 0x7fff]);
	} else {
		if (intNum >= IRQ_HARDINTS)
			intNum = 0;
		candidate = (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
		while (candidate) {
			if ((IRQ_pend(candidate->impSpec.pend) &
			     candidate->impSpec.bit)
			    && (candidate->impSpec.bit > bestBit)) {
				best = candidate;
				bestBit = candidate->impSpec.bit;
			}
			candidate = (IRQ_DESC_T *) LST_next(candidate);
		}
	}
	return best;
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

	int32_t intNum = irqDesc->intNum;
	if (irqDesc->impSpec.extNum == IRQ_TIMER) {
		mips_setcompare(mips_getcompare());
	}
	if ((intNum >= IRQ_HARDINTS)
	    && (intNum < IRQ_HARDINTS + IRQ_SOFTINTS)) {
		__sync_and_and_fetch(&_IRQ_softPend,
				     ~(1 << (irqDesc->intNum - IRQ_HARDINTS)));
		if (_IRQ_softPend & _IRQ_softMask)
			return irqDesc;
		intNum = 0;
	}
	switch (intNum) {
	case 0:
		mips_biccr(CR_SINT0);
		break;
	case 1:
		mips_biccr(CR_SINT1);
	default:
		if ((irqDesc->impSpec.extNum > 0)
		    && (irqDesc->impSpec.extNum < _IRQ_numExts)
		    && irqDesc->impSpec.clear)
			*irqDesc->impSpec.clear |= irqDesc->impSpec.bit;
		break;
	}
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
	if ((intNum >= IRQ_HARDINTS)
	    && (intNum < IRQ_HARDINTS + IRQ_SOFTINTS)) {
		/* Synthesize a multiplexed soft interrupt */
		__sync_or_and_fetch(&_IRQ_softPend,
				    1 << (irqDesc->intNum - IRQ_HARDINTS));
		if (!(_IRQ_softPend & _IRQ_softMask))
			return;
		intNum = 0;
	}
	switch (intNum) {
		/* Synthesize a raw soft interrupt */
	case 0:
		mips_biscr(CR_SINT0);
		break;
	case 1:
		mips_biscr(CR_SINT1);
		break;
	default:
		/* Synthesize a raw hard interrupt via the HWSTAT */
		if ((irqDesc->impSpec.extNum > 0)
		    && (irqDesc->impSpec.extNum < _IRQ_numExts)
		    && irqDesc->impSpec.set)
			*irqDesc->impSpec.set |= irqDesc->impSpec.bit;
		break;
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
	out->impSpec.extNum = 0;
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
	return 0;
}
