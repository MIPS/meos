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
*   Description:	MIPS _GIC interrupt specialisation
*
*************************************************************************/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"

#ifdef CONFIG_ARCH_MIPS_VZ
#define INTCTL_VS_X INTCTL_VS_128
#define VECSPACING 128
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

#if defined(CONFIG_ARCH_MIPS_NO_EIC)
#define EIC 0
#else
#if defined(CONFIG_ARCH_MIPS_YES_EIC)
#define EIC 1
#else
static uint32_t EIC = 0;
#endif
#endif

#ifndef CONFIG_ARCH_MIPS_GIC_BASE
#define CONFIG_ARCH_MIPS_GIC_BASE 0
#endif

volatile uint32_t *_GCR __attribute__ ((weak)) = NULL;
volatile uint32_t *_GIC __attribute__ ((weak)) = NULL;

#define GCR_BASE _GCR[0x8/sizeof(uint32_t)]
#define GCR_GIC_BASE _GCR[0x80/sizeof(uint32_t)]
#define GIC_EN	1
#define GCR_GIC_STATUS _GCR[0xd0/sizeof(uint32_t)]

#define GIC_SH_CONFIG		_GIC[0]
#define NUMINTERRUPTS_MSK	0x007f0000
#define NUMINTERRUPTS_SHF	16
#define PVPES_MSK 			0x000001ff
#define PVPES_SHF			0
#define	VZP			0x80000000
#define	VZE			0x80000000
#define GIC_SH_POL31_0		_GIC[0x100/sizeof(uint32_t)]
#define GIC_SH_TRIG31_0		_GIC[0x180/sizeof(uint32_t)]
#define GIC_SH_DUAL31_0		_GIC[0x200/sizeof(uint32_t)]
#define GIC_SH_WEDGE		_GIC[0x280/sizeof(uint32_t)]
#define GIC_SH_RMASK31_0	_GIC[0x300/sizeof(uint32_t)]
#define GIC_SH_SMASK31_0	_GIC[0x380/sizeof(uint32_t)]
#define GIC_SH_PEND31_0		_GIC[0x480/sizeof(uint32_t)]
#define GIC_SH_MAP0_PIN		_GIC[0x500/sizeof(uint32_t)]
#define GIC_SH_MAP0_CORE	_GIC[0x2000/sizeof(uint32_t)]
#define GIC_CORE_SPACER		(0x20/sizeof(uint32_t))

#define	GIC_COREi_CTL		_GIC[0x8000/sizeof(uint32_t)]
#define GIC_COREi_PEND		_GIC[0x8004/sizeof(uint32_t)]
#define	GIC_COREi_MASK		_GIC[0x8008/sizeof(uint32_t)]
#define	GIC_COREi_RMASK		_GIC[0x800C/sizeof(uint32_t)]
#define GIC_COREi_SMASK		_GIC[0x8010/sizeof(uint32_t)]
#define GIC_COREi_TIMER_MAP	_GIC[0x8048/sizeof(uint32_t)]
#define GIC_COREi_FDC_MAP	_GIC[0x804C/sizeof(uint32_t)]
#define GIC_COREi_PERFCTR_MAP	_GIC[0x8050/sizeof(uint32_t)]
#define GIC_COREi_SWInt0_MAP	_GIC[0x8054/sizeof(uint32_t)]
#define GIC_COREi_SWInt1_MAP	_GIC[0x8058/sizeof(uint32_t)]
#define GIC_COREi_IDENT		_GIC[0x8088/sizeof(uint32_t)]
#define GIC_COREi_EICVEC0	_GIC[0x8800/sizeof(uint32_t)]

#define SWINT0_MASK	0x10
#define SWINT1_MASK	0x20
#define TIMER_MASK	0x04
#define	FDC_MASK	0x40
#define	PERFCNT_MASK	0x8

#define SWINT_ROUTABLE	0x08
#define TIMER_ROUTABLE	0x02
#define FDC_ROUTABLE	0x10
#define EIC_MODE	0x01

#define TIMER_PEND	0x4
#define SWINT0_PEND	0x10
#define SWINT1_PEND	0x20
#define FDC_PEND	0x40
#define CR_PEND	(void*)-1

#define MAP_TO_PIN	0x80000000

#define INT_TO_MAP_LOCAL(X) (EIC ? (X) : ((X) - 2))
/*#define INT_TO_MAP_LOCAL(X) (EIC ? (X) : (1 << ((X) - 2))) */

/* Linux hwirqs are offset by this value */
#define GIC_NUM_LOCAL_INTRS	7

volatile int32_t _IRQ_ipl;
int32_t _IRQ_log;
int32_t _IRQ_numInts;
int32_t _IRQ_numExts;

static void _IRQ_dummyFunc(int32_t sig);

void *_IRQ_excTable[IRQ_EXCEPTIONS];
LST_T _IRQ_descTable[IRQ_EXCEPTIONS + IRQ_HARDINTS];	/* 32 exceptions, max 64 hard ints, 32 soft ints and ipi subsumed into main lists */
IRQ_DESC_T _IRQ_dummy = {.isrFunc = _IRQ_dummyFunc,
	.impSpec = {.pend = (uint32_t *) & _IRQ_dummy.impSpec.pend,
		    .bit = 0}
};

IRQ_DESC_T *_IRQ_ipi = &_IRQ_dummy, *_IRQ_timer = &_IRQ_dummy, *_IRQ_fdc = &_IRQ_dummy;	/* Quick access */

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
** FUNCTION:	IRQ_setBases
**
** DESCRIPTION:	Locate _GCR and GIC.
**
** RETURNS:	void
*/
void IRQ_setBases(void *gcrBase, void *gicBase)
{
	_GCR = (uint32_t *) gcrBase;
	_GIC = (uint32_t *) gicBase;
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
	_IRQ_ipi = &_IRQ_dummy;
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

	/* Locate _GCR */
	if ((mips32_getconfig0() & CFG0_M) && (mips32_getconfig1() & CFG1_M)
	    && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_CMGCR)) {
		if (_GCR) {
			/*
			 * We have a GCR, and know where it should be, so
			 * force it to be there.
			 */
			mips32_set_c0(C0_CMGCRBASE,
				      ((MEM_v2p((void *)_GCR) >> 4) &
				       0x0ffffc00));
		} else {
			/*
			 * We have a GCR, but don't know where it should be.
			 * Use its currently configured location, and hope it
			 * was set up by the bootloader.
			 */
			_GCR =
			    MEM_p2v(((mips32_get_c0(C0_CMGCRBASE) & 0x0ffffc00)
				     << 4), MEM_P2V_UNCACHED);
		}
	}

	/* Locate _GIC */
	if (_GIC) {
		if (_GCR) {
			/*
			 * We have a GCR, and know where the GIC should be, so
			 * force it to be there.
			 */
			GCR_GIC_BASE = MEM_v2p((void *)_GIC) | GIC_EN;
			KRN_barrier(KRN_BARRIER_FLAG_WRITE);
		}
	} else {
		if (_GCR) {
			/*
			 * We have a GCR, but don't know where the GIC should
			 * be. Use its currently configured location, and hope
			 * it was set up by the bootloader.
			 */
			_GIC = KSEG1_BASE + (GCR_GIC_BASE & ~GIC_EN);
			GCR_GIC_BASE = MEM_v2p((void *)_GIC) | GIC_EN;
			KRN_barrier(KRN_BARRIER_FLAG_WRITE);
		}
	}
	DBG_assert(_GIC, "No GIC address provided!\n");

	/* Get the number of processors */
	_KRN_schedule->hwThreads =
	    ((GIC_SH_CONFIG & PVPES_MSK) >> PVPES_SHF) + 1;
	if (_KRN_schedule->hwThreads > CONFIG_FEATURE_MAX_PROCESSORS)
		_KRN_schedule->hwThreads = CONFIG_FEATURE_MAX_PROCESSORS;
}

__attribute__ ((weak))
uint32_t _IRQ_reset = 1;

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
	int32_t VEIC = (mips32_getconfig0() & CFG0_M) &&
	    (mips32_getconfig1() & CFG1_M) &&
	    (mips32_getconfig2() & CFG2_M) && (mips32_getconfig3() & CFG3_VEIC);

	(void)VEIC;

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

	/* Probe _GIC */
#ifdef CONFIG_ARCH_MIPS_NO_EIC
	GIC_COREi_CTL &= ~EIC_MODE;
#else
	GIC_COREi_CTL |= EIC_MODE;
#endif
#if ((!defined(CONFIG_ARCH_MIPS_NO_EIC)) && (!defined(CONFIG_ARCH_MIPS_YES_EIC)))
	EIC = VEIC ? (GIC_COREi_CTL & EIC_MODE) : 0;
	if ((GIC_COREi_CTL & EIC_MODE) != EIC)
		GIC_COREi_CTL ^= EIC_MODE;
#endif

	GIC_COREi_RMASK = 0xffffffff;
	GIC_COREi_CTL |= SWINT_ROUTABLE | TIMER_ROUTABLE;
	/* Get the number of interrupts */
	_IRQ_numExts =
	    (((GIC_SH_CONFIG & NUMINTERRUPTS_MSK) >> NUMINTERRUPTS_SHF)
	     + 1) * 8;
	_IRQ_numInts = EIC ? 63 : 7;
	if (_IRQ_reset) {
		for (i = 0, j = 0; i < _IRQ_numExts / 32; i++) {
			(&GIC_SH_RMASK31_0)[j] = 0xffffffff;
			(&GIC_SH_TRIG31_0)[j++] = 0xffffffff;
		}
		for (i = 0; i < _IRQ_numExts; i++)
			GIC_SH_WEDGE = i;
	}
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
	GIC_COREi_PERFCTR_MAP = MAP_TO_PIN | INT_TO_MAP_LOCAL(2);
	GIC_COREi_SMASK = PERFCNT_MASK;
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
	if (GIC_SH_CONFIG & VZP)
		GIC_SH_CONFIG |= VZE;
	return GIC_SH_CONFIG & VZE;
}
#endif

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
** DESCRIPTION:	Demultiplex and dispatch core ints, IPI, and software interrupts.
**
** RETURNS:	void
*/
void _IRQ_sharedShunt(int32_t intNum)
{
	IRQ_DESC_T *candidate;
	reg_t cr = mips_getcr();
	if (cr & CR_TI)
		_IRQ_timer->isrFunc(_IRQ_timer->intNum);
	if (cr & CR_FDCI)
		_IRQ_fdc->isrFunc(_IRQ_fdc->intNum);
	if (*_IRQ_ipi->impSpec.pend & _IRQ_ipi->impSpec.bit)
		_IRQ_ipi->isrFunc(_IRQ_ipi->intNum);
	candidate =
	    (IRQ_DESC_T *) LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
	while (candidate) {
		uint32_t pend;
		if (candidate->impSpec.pend == CR_PEND)
			pend = mips_getcr();
		else
			pend = *candidate->impSpec.pend;
		if (pend & candidate->impSpec.bit)
			candidate->isrFunc(candidate->intNum);
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
	if ((!EIC) || (intNum < 2)) {
		if (intNum == 9)
			mips_bissr(SR_IM0 << 10);
		else
			mips_bissr(SR_IM0 << intNum);
	}
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
	if (((!EIC) || (intNum < 2))
	    && (LST_empty(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]))) {
		if (intNum == 9)
			mips_bicsr(SR_IM0 << 10);
		else
			mips_bicsr(SR_IM0 << intNum);
	}
}

/*
** FUNCTION:	_IRQ_cleanTimer
**
** DESCRIPTION:	Rewrite timer interrupt descriptor to fit available abilities.
**
** RETURNS:	void
*/
inline static void _IRQ_cleanTimer(IRQ_DESC_T * irqDesc)
{
	/* If the user has requested we pick an internal IRQ, or if the platform
	   doesn't support rerouting it, set the internal interrupt to the one
	   specified by IntCtl[IPTI]. */

	if (!(GIC_COREi_CTL & TIMER_ROUTABLE)) {
		irqDesc->intNum = (mips32_getintctl() & INTCTL_IPTI)
		    >> INTCTL_IPTI_SHIFT;
	} else if (irqDesc->intNum == 0) {
#if (CONFIG_ARCH_MIPS_GIC_OVERRIDE_TIMER==0)
		irqDesc->intNum = GIC_COREi_TIMER_MAP & ~MAP_TO_PIN;
		if (!EIC)
			irqDesc->intNum += 2;
#else
		irqDesc->intNum = CONFIG_ARCH_MIPS_GIC_OVERRIDE_TIMER;
#endif
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
	_IRQ_cleanTimer(irqDesc);
	/* Make sure we don't double install */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Reconfigure _GIC as per descriptor */
	GIC_COREi_TIMER_MAP = MAP_TO_PIN | INT_TO_MAP_LOCAL(intNum);
	GIC_COREi_SMASK = TIMER_MASK;
	/* Install interrupt */
	irqDesc->impSpec.pend = CR_PEND;
	irqDesc->impSpec.bit = CR_TI;
	if ((intNum == 2) || (intNum == _IRQ_fdc->intNum))
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
	_IRQ_cleanTimer(irqDesc);
	/* Remove it */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Disable it */
	GIC_COREi_RMASK = TIMER_MASK;
	_IRQ_timer = &_IRQ_dummy;
}

/*
** FUNCTION:	_IRQ_cleanFDC
**
** DESCRIPTION:	Rewrite FDC interrupt descriptor to fit available abilities.
**
** RETURNS:	void
*/
inline static void _IRQ_cleanFDC(IRQ_DESC_T * irqDesc)
{
	/* If the user has requested we pick an internal IRQ, or if the platform
	   doesn't support rerouting it, set the internal interrupt to the one
	   specified by IntCtl[IPFDC]. */

	if (!(GIC_COREi_CTL & FDC_ROUTABLE)) {
		irqDesc->intNum = (mips32_getintctl() & INTCTL_IPFDC)
		    >> INTCTL_IPFDC_SHIFT;
	} else if (irqDesc->intNum == 0) {
#if (CONFIG_ARCH_MIPS_GIC_OVERRIDE_FDC==0)
		irqDesc->intNum = GIC_COREi_FDC_MAP & ~MAP_TO_PIN;
		if (!EIC)
			irqDesc->intNum += 2;
#else
		irqDesc->intNum = CONFIG_ARCH_MIPS_GIC_OVERRIDE_FDC;
#endif
	}
}

/*
** FUNCTION:	_IRQ_routeFDC
**
** DESCRIPTION:	Route a FDC interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeFDC(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	_IRQ_cleanFDC(irqDesc);
	/* Make sure we don't double install */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Reconfigure _GIC as per descriptor */
	GIC_COREi_FDC_MAP = MAP_TO_PIN | INT_TO_MAP_LOCAL(intNum);
	GIC_COREi_SMASK = FDC_MASK;
	/* Install interrupt */
	irqDesc->impSpec.pend = CR_PEND;
	irqDesc->impSpec.bit = CR_FDCI;
	/* Always go via the shunt to cope with no timer ISR */
	_IRQ_insert(intNum, irqDesc, _IRQ_sharedShunt);
	_IRQ_fdc = irqDesc;
	_IRQ_enableSource(intNum);
}

/*
** FUNCTION:	_IRQ_unrouteFDC
**
** DESCRIPTION:	Unroute a FDC interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteFDC(IRQ_DESC_T * irqDesc)
{
	int32_t intNum;
	/* Make good the descriptor */
	_IRQ_cleanFDC(irqDesc);
	/* Remove it */
	intNum = irqDesc->intNum;
	_IRQ_remove(intNum, irqDesc);
	/* Disable it */
	GIC_COREi_RMASK = FDC_MASK;
	_IRQ_fdc = &_IRQ_dummy;
	_IRQ_disableSource(intNum);
}

/*
** FUNCTION:	_IRQ_routeHard
**
** DESCRIPTION:	Route an external hardware interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeHard(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = irqDesc->intNum;
	int32_t extNum = irqDesc->impSpec.extNum;
	int32_t core = irqDesc->impSpec.core;
#ifdef CONFIG_ARCH_MIPS_VZ
	int32_t guest = irqDesc->impSpec.guest;
#else
	const int32_t guest = 0;
#endif
	uint32_t word = (extNum >> 5), bit = 1 << (extNum & 0x1f);
	if (intNum == IRQ_MULTIPLEXED)
		irqDesc->intNum = intNum = 2;
	/* Make sure we don't double install */
	_IRQ_remove(intNum, irqDesc);
	/* Reconfigure GIC as per descriptor */
	(&GIC_SH_MAP0_CORE)[extNum * GIC_CORE_SPACER] =
	    core ? 1 << (core - 1) : 1 << GIC_COREi_IDENT;
	(&GIC_SH_MAP0_PIN)[extNum] =
	    MAP_TO_PIN | (guest << 8) | INT_TO_MAP_LOCAL(intNum);
	if (irqDesc->impSpec.polarity == IRQ_ACTIVE_HIGH)
		(&GIC_SH_POL31_0)[word] |= bit;
	else
		(&GIC_SH_POL31_0)[word] &= ~bit;
	if (irqDesc->impSpec.trigger == IRQ_LEVEL_SENSITIVE)
		(&GIC_SH_TRIG31_0)[word] &= ~bit;
	else
		(&GIC_SH_TRIG31_0)[word] |= bit;
	if (irqDesc->impSpec.trigger == IRQ_EDGE_DOUBLE_TRIGGERED)
		(&GIC_SH_DUAL31_0)[word] |= bit;
	else
		(&GIC_SH_DUAL31_0)[word] &= ~bit;
	(&GIC_SH_SMASK31_0)[word] = bit;
	/* Install interrupt */
	irqDesc->impSpec.pend = &(&GIC_SH_PEND31_0)[word];
	irqDesc->impSpec.bit = bit;
	if (intNum == 2)
		_IRQ_insert(intNum, irqDesc, _IRQ_sharedShunt);
	else
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
	uint32_t word = (extNum >> 5), bit = 1 << (extNum & 0x1f);
	/* Reconfigure _GIC as per descriptor */
	(&GIC_SH_RMASK31_0)[word] = bit;
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
	if (GIC_COREi_CTL & SWINT_ROUTABLE) {
		/* Make sure we don't double install */
		_IRQ_remove(2, irqDesc);
		/* Reconfigure _GIC */
		GIC_COREi_SWInt0_MAP = MAP_TO_PIN | INT_TO_MAP_LOCAL(2);
		GIC_COREi_SMASK = SWINT0_MASK;
		/* Install interrupt */
		_IRQ_insert(2, irqDesc, _IRQ_sharedShunt);
		_IRQ_enableSource(2);
	} else {
		/* Make sure we don't double install */
		_IRQ_remove(0, irqDesc);
		/* Install interrupt */
		_IRQ_insert(0, irqDesc, _IRQ_sharedShunt);
		mips_bissr(SR_IM0);
	}
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
	if (GIC_COREi_CTL & SWINT_ROUTABLE) {
		_IRQ_remove(2, irqDesc);
		if (!_IRQ_softMask)
			GIC_COREi_RMASK = SWINT0_MASK;
		_IRQ_disableSource(2);
	} else {
		_IRQ_remove(0, irqDesc);
		if (!_IRQ_softMask)
			mips_bicsr(SR_IM0);
	}
	/* There are pending soft interrupts - trigger them */
	if (_IRQ_softPend & _IRQ_softMask)
		mips_biscr(CR_SINT0);
}

/*
** FUNCTION:	_IRQ_routeSoft
**
** DESCRIPTION:	Route an interprocessor interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_routeIPI(IRQ_DESC_T * irqDesc)
{
	int32_t extNum = irqDesc->impSpec.extNum;
	uint32_t word = (extNum >> 5), bit = 1 << (extNum & 0x1f);
	/* Make sure we don't double install */
	_IRQ_remove(2, irqDesc);
	/* Install handler */
	_IRQ_insert(2, irqDesc, _IRQ_sharedShunt);
	/* Reconfigure _GIC as per descriptor */
	(&GIC_SH_MAP0_CORE)[extNum * GIC_CORE_SPACER] = 1 << GIC_COREi_IDENT;
	(&GIC_SH_MAP0_PIN)[extNum] = MAP_TO_PIN | INT_TO_MAP_LOCAL(2);
	(&GIC_SH_POL31_0)[word] |= bit;
	(&GIC_SH_TRIG31_0)[word] |= bit;
	(&GIC_SH_DUAL31_0)[word] &= ~bit;
	(&GIC_SH_SMASK31_0)[word] = bit;
	/* Install interrupt */
	irqDesc->impSpec.pend = &(&GIC_SH_PEND31_0)[word];
	irqDesc->impSpec.bit = bit;
	_IRQ_ipi = irqDesc;
	_IRQ_enableSource(2);
}

/*
** FUNCTION:	_IRQ_unrouteIPI
**
** DESCRIPTION:	Unroute an interprocessor interrupt
**
** RETURNS:	void
*/
inline static void _IRQ_unrouteIPI(IRQ_DESC_T * irqDesc)
{
	int32_t intNum = 2;
	int32_t extNum = irqDesc->impSpec.extNum;
	uint32_t word = (extNum >> 5), bit = 1 << (extNum & 0x1f);
	/* Reconfigure _GIC as per descriptor */
	(&GIC_SH_RMASK31_0)[word] = bit;
	/* Remove it */
	_IRQ_remove(intNum, irqDesc);
	_IRQ_ipi = &_IRQ_dummy;
	/* Disable it */
	_IRQ_disableSource(intNum);
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

	if ((intNum & 0xc000) == 0x8000) {	/* exception */
		_IRQ_routeException(irqDesc);	/* route and unroute the same op */
	} else {
		if ((irqDesc->impSpec.extNum != IRQ_TIMER)
		    && (irqDesc->impSpec.extNum != IRQ_FDC)
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
		} else if (intNum < IRQ_HARDINTS + IRQ_SOFTINTS) {	/* soft */
			if (irqDesc->isrFunc)
				_IRQ_routeSoft(irqDesc);
			else
				_IRQ_unrouteSoft(irqDesc);
		} else {	/* ipi */

			if (irqDesc->isrFunc)
				_IRQ_routeIPI(irqDesc);
			else
				_IRQ_unrouteIPI(irqDesc);
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
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS)
		intNum = 2;
	if (intNum & 0x8000) {
		return (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[intNum & 0x7fff]);
	} else {
		if (intNum >= IRQ_HARDINTS) {
			if (GIC_COREi_CTL & SWINT_ROUTABLE)
				intNum = 2;
			else
				intNum = 0;
		}
		candidate = (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
		while (candidate) {
			if ((candidate->intNum == dIntNum)
			    && (candidate->impSpec.extNum == extNum))
				return candidate;
			candidate = (IRQ_DESC_T *) LST_next(candidate);
		}
	}
	return NULL;
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
	if (intNum >= IRQ_HARDINTS + IRQ_SOFTINTS)
		intNum = 2;
	if (intNum & 0x8000) {
		return (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[intNum & 0x7fff]);
	} else {
		if (intNum >= IRQ_HARDINTS) {
			if (GIC_COREi_CTL & SWINT_ROUTABLE)
				intNum = 2;
			else
				intNum = 0;
		}
		candidate = (IRQ_DESC_T *)
		    LST_first(&_IRQ_descTable[IRQ_EXCEPTIONS + intNum]);
		while (candidate) {
			uint32_t pend;
			if (candidate->impSpec.pend == CR_PEND)
				pend = mips_getcr();
			else
				pend = *candidate->impSpec.pend;
			if ((pend & candidate->impSpec.bit)
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
	if (irqDesc == _IRQ_timer) {
		mips_setcompare(mips_getcompare());
	}
	if (irqDesc > 0) {
		if ((intNum >= IRQ_HARDINTS)
		    && (intNum < IRQ_HARDINTS + IRQ_SOFTINTS)) {
			__sync_and_and_fetch(&_IRQ_softPend,
					     ~(1 <<
					       (irqDesc->intNum -
						IRQ_HARDINTS)));
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
			if (irqDesc->impSpec.extNum < _IRQ_numExts) {
				GIC_SH_WEDGE = irqDesc->impSpec.extNum;
			}
			break;
		}
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
		/* Synthesize a raw hard interrupt with the _GIC */
		if (irqDesc->impSpec.extNum < _IRQ_numExts) {
			GIC_SH_WEDGE = 0x80000000 | irqDesc->impSpec.extNum;
		}
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
	out->intNum = IRQ_HARDINTS + IRQ_SOFTINTS;
	out->impSpec.extNum = ipi;
	return 1;
}
