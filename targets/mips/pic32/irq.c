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
*   Description:	PIC32 interrupt specialisation
*
*************************************************************************/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/mem/mem.h"
#include "picpart.h"

/* These are required, regardless of what the P32PS lib thinks */
#define _INTCON_VS_POSITION                      0x00000010
#define _INTCON_VS_MASK                          0x007F0000
#define _INTCON_VS_LENGTH                        0x00000007

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
#define CLEAN_PTR(X) (X)
#ifdef CONFIG_ARCH_MIPS_LITTLE_ENDIAN
#define UOFFSET 0
#else
#define UOFFSET 2
#endif
#endif

volatile int32_t _IRQ_ipl;
int32_t _IRQ_log;

#ifndef CONFIG_ARCH_MIPS_MVEC
IRQ_ISRFUNC_T *_IRQ_intTable[IRQ_HARDINTS];
#endif
IRQ_ISRFUNC_T *_IRQ_excTable[IRQ_EXCEPTIONS];
/* 32 exceptions, 256 hard ints, 32 soft ints mapped to SINT0 */
IRQ_DESC_T *_IRQ_descTable[IRQ_EXCEPTIONS + IRQ_HARDINTS + IRQ_SOFTINTS];
volatile uint32_t _IRQ_softPend;
volatile uint32_t _IRQ_softMask;
uint32_t *_IRQ_intStack = NULL;

uint8_t *_EBASE = (uint8_t *) KSEG0_BASE;

#ifdef __cplusplus
extern "C" {
#endif

	extern void _toupee(void);
#ifdef CONFIG_ARCH_MIPS_MVEC
	extern void _template(void);
	extern void _templatea0(void);
	extern void _templates1h(void);
	extern void _templates1l(void);
#else
	extern void _svechead(void);
	extern void _svechead1h(void);
	extern void _svechead1l(void);
	extern void _svechead2h(void);
	extern void _svechead2l(void);
#endif
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
** FUNCTION:	_IRQ_wire
**
** DESCRIPTION:	Patch the entry point to call the provided ISR.
**
** RETURNS:	void
*/
inline static void _IRQ_wire(int32_t intNum, IRQ_ISRFUNC_T * isr)
{
#ifdef CONFIG_ARCH_MIPS_MVEC
	volatile uint16_t *patch;
	uintptr_t isrFunc = (uintptr_t) isr;
	/* Patch the entry point appropriately */
	patch = (volatile uint16_t *)(UOFFSET +
				      _EBASE + 0x200 + (intNum * VECSPACING) +
				      (((uintptr_t) CLEAN_PTR(_templates1l)) -
				       ((uintptr_t) CLEAN_PTR(_template))));
	*patch = (volatile uint16_t)(isrFunc & 0xffff);
	patch = (volatile uint16_t *)(UOFFSET +
				      _EBASE + 0x200 + (intNum * VECSPACING) +
				      (((uintptr_t) CLEAN_PTR(_templates1h)) -
				       ((uintptr_t) CLEAN_PTR(_template))));
	*patch = (volatile uint16_t)(isrFunc >> 16);
	/* Make sure all modified code is flushed */
	KRN_flushCache(_EBASE + 0x200 + (intNum * VECSPACING), VECSPACING,
		       KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_syncCache(_EBASE + 0x200 + (intNum * VECSPACING), VECSPACING);
#else
	_IRQ_intTable[intNum] = isr;
#endif
}

/*
** FUNCTION:	_IRQ_priority
**
** DESCRIPTION:	Set interrupt priority.
**
** RETURNS:	void
*/
inline static void _IRQ_priority(int32_t intNum, uint32_t priority)
{
	/* Compute registers */
	uint32_t wordNum = intNum >> 2;
	volatile uint32_t *creg =
	    &((volatile uint32_t *)&IPC0CLR)[wordNum << 2];
	volatile uint32_t *sreg =
	    &((volatile uint32_t *)&IPC0SET)[wordNum << 2];
	/* 0 would disable */
	if (!priority)
		priority = 1;
	/* Clear out current priority */
	*creg = 0x1f << ((intNum & 3) << 3);
	/* Set new priority */
	*sreg = (priority << 2) << ((intNum & 3) << 3);
}

/*
** FUNCTION:	_IRQ_polarity
**
** DESCRIPTION:	Set interrupt polarity if applicable.
**
** RETURNS:	void
*/
inline static void _IRQ_polarity(int32_t intNum, uint32_t polarity)
{
	switch (intNum) {
	case _EXTERNAL_0_VECTOR:
		if (polarity)
			INTCONCLR = _INTCON_INT0EP_MASK;
		else
			INTCONCLR = _INTCON_INT0EP_MASK;
		break;
	case _EXTERNAL_1_VECTOR:
		if (polarity)
			INTCONCLR = _INTCON_INT1EP_MASK;
		else
			INTCONCLR = _INTCON_INT1EP_MASK;
		break;
	case _EXTERNAL_2_VECTOR:
		if (polarity)
			INTCONCLR = _INTCON_INT2EP_MASK;
		else
			INTCONCLR = _INTCON_INT2EP_MASK;
		break;
	case _EXTERNAL_3_VECTOR:
		if (polarity)
			INTCONCLR = _INTCON_INT3EP_MASK;
		else
			INTCONCLR = _INTCON_INT3EP_MASK;
		break;
	case _EXTERNAL_4_VECTOR:
		if (polarity)
			INTCONCLR = _INTCON_INT4EP_MASK;
		else
			INTCONCLR = _INTCON_INT4EP_MASK;
		break;
	default:
		break;
	}
}

/*
** FUNCTION:	_IRQ_allow
**
** DESCRIPTION:	Enable interrupt.
**
** RETURNS:	void
*/
inline static void _IRQ_allow(int32_t intNum)
{
	uint32_t wordNum = intNum >> 5;
	((uint32_t *) & IEC0SET)[wordNum << 2] = 1 << (intNum & 31);
}

/*
** FUNCTION:	_IRQ_disallow
**
** DESCRIPTION:	Disable interrupt.
**
** RETURNS:	void
*/
inline static void _IRQ_disallow(int32_t intNum)
{
	uint32_t wordNum = intNum >> 5;
	((uint32_t *) & IEC0CLR)[wordNum << 2] = 1 << (intNum & 31);
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

			/* Put interrupt system into a good state for XC32 */
			/* Disable all interrupts on controller */
			IEC0CLR = 0xffffffff;
			((uint32_t *) & IEC0CLR)[4] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[8] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[12] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[16] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[20] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[24] = 0xffffffff;
			((uint32_t *) & IEC0CLR)[28] = 0xffffffff;
			/* Enable but mask interrupts */
			mips_bcssr(SR_BEV | SR_IMASK, SR_IE);
		}
		return;
	}
#ifndef CONFIG_ARCH_MIPS_VZ
	asm volatile ("move $k0, $zero");
#else
	_m32c0_mtc0(31, 2, 0);	/* Stick context in KSCRATCH1 */
#endif
	uint32_t EBase = mips32_getebase();

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

	/* We always support EBase */
	_EBASE = (uint8_t *) (EBase & EBASE_BASE);
	/* Also, which processor we are */
	_KRN_schedule->hwThread = EBase & EBASE_CPU;
	/* Hard code the number of processors */
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
	int32_t i;

	volatile uint16_t *patch;

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
	mips_bissr(SR_BEV);
	mips_biccr(CR_IV);
	memcpy(_EBASE, (const void *)CLEAN_PTR(_toupee), 0x100);
	memcpy(_EBASE + 0x100, (const void *)CLEAN_PTR(_cache_routine), 0x80);
	memcpy(_EBASE + 0x180, (const void *)CLEAN_PTR(_toupee), 0x80);

	/* Disable all interrupts on controller */
	IEC0CLR = 0xffffffff;
	((uint32_t *) & IEC0CLR)[4] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[8] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[12] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[16] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[20] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[24] = 0xffffffff;
	((uint32_t *) & IEC0CLR)[28] = 0xffffffff;
#ifdef CONFIG_ARCH_MIPS_MVEC
	/* Patch the interrupt number into the vectored handler */
	for (i = 0; i < 256; i++) {
		memcpy(_EBASE + 0x200 + (i * VECSPACING),
		       CLEAN_PTR(_template), VECSPACING);
		patch =
		    (volatile uint16_t *)(UOFFSET + (uintptr_t) _EBASE +
					  0x200 + (i * VECSPACING) +
					  (((uintptr_t)
					    CLEAN_PTR(_templatea0)) -
					   ((uintptr_t)
					    CLEAN_PTR(_template))));
		*patch = (volatile uint16_t)i;
	}
#else
	memcpy(_EBASE + 0x200, (const void *)CLEAN_PTR(_svechead), 128);
	patch =
	    (volatile uint16_t *)(UOFFSET + _EBASE + 0x200 +
				  (((uintptr_t) CLEAN_PTR(_svechead1h))
				   - ((uintptr_t) CLEAN_PTR(_svechead))));
	*patch = (volatile uint16_t)(((uintptr_t) & INTSTAT) >> 16);
	patch = (volatile uint16_t *)(UOFFSET + _EBASE + 0x200 + (((uintptr_t)
								   CLEAN_PTR
								   (_svechead1l))
								  - ((uintptr_t)
								     CLEAN_PTR
								     (_svechead))));
	*patch = (volatile uint16_t)(((uintptr_t) & INTSTAT) & 0xffff);
	patch =
	    (volatile uint16_t *)(UOFFSET + _EBASE + 0x200 +
				  (((uintptr_t) CLEAN_PTR(_svechead2h))
				   - ((uintptr_t) CLEAN_PTR(_svechead))));
	*patch = (volatile uint16_t)(((uintptr_t) _IRQ_intTable) >> 16);
	patch = (volatile uint16_t *)(UOFFSET + _EBASE + 0x200 + (((uintptr_t)
								   CLEAN_PTR
								   (_svechead2l))
								  - ((uintptr_t)
								     CLEAN_PTR
								     (_svechead))));
	*patch = (volatile uint16_t)(((uintptr_t) _IRQ_intTable) & 0xffff);
#endif
	/* Install software interrupt demultiplexer */
	_IRQ_wire(1, (IRQ_ISRFUNC_T *) _IRQ_dispatchSoft);
	_IRQ_priority(1, 7);
	_IRQ_allow(1);
	/* Make sure all modified code is flushed */
	KRN_flushCache(_EBASE, 0x200 + (256 * VECSPACING),
		       KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_syncCache(_EBASE, 0x200 + (256 * VECSPACING));
#ifdef CONFIG_ARCH_MIPS_MVEC
	/* Set offsets to match vector spacing */
	for (i = 0; i < 256; i++)
		OFF000[i] = 0x200 + (i * VECSPACING);
	/* Set vector spacing */
	mips32_setintctl((mips32_getintctl() & ~INTCTL_VS) | INTCTL_VS_X);
#ifdef _INTCON_VS_MASK
	INTCONCLR = _INTCON_VS_MASK;
	INTCONSET = (VECSPACING << _INTCON_VS_POSITION);
#endif
	INTCONSET = _INTCON_MVEC_MASK;
#else
	/* Set offsets to match vector spacing */
	for (i = 0; i < 256; i++)
		((uint32_t *) & OFF000)[i] = 0x200;
	/* Set vector spacing - VS required, interrupt controller will fix */
	mips32_setintctl((mips32_getintctl() & ~INTCTL_VS) | INTCTL_VS_X);
#ifdef _INTCON_VS_MASK
	INTCONCLR = _INTCON_VS_MASK | _INTCON_MVEC_MASK;
#else
	INTCONCLR = _INTCON_MVEC_MASK;
#endif
#endif
	/* And activate them */
	mips_biscr(CR_IV);
	mips_bcssr(SR_BEV | SR_IMASK, SR_IE);

#ifdef CONFIG_ARCH_MIPS_PCINT
	_IRQ_allow(104);
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
	return 1;
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
	if (intNum & 0x8000) {	/* exception */
		intNum &= 0x7fff;
		if (irqDesc->isrFunc) {
			_IRQ_excTable[intNum] = irqDesc->isrFunc;
			_IRQ_descTable[intNum] = irqDesc;
		} else {
			_IRQ_excTable[intNum] = NULL;
			_IRQ_descTable[intNum] = NULL;
		}
	} else {
		if (intNum == 1) {
			DBG_insist(0,
				   "Do not directly route soft interrupt 0, use the appropriate factory function. Soft interrupts now broken.\n");
		}
		if (intNum < IRQ_HARDINTS) {	/* hard */
			if (irqDesc->isrFunc) {
				_IRQ_disallow(intNum);
				_IRQ_wire(intNum, irqDesc->isrFunc);
				_IRQ_priority(intNum,
					      irqDesc->impSpec.priority);
				_IRQ_polarity(intNum,
					      irqDesc->impSpec.polarity);
				_IRQ_descTable[IRQ_EXCEPTIONS +
					       intNum] = irqDesc;
				_IRQ_allow(intNum);
			} else {
#if CONFIG_DEBUG_POSTMORTEM
				_IRQ_disallow(intNum);
				_IRQ_wire(intNum, _IRQ_exception);
#else
				_IRQ_disallow(intNum);
				_IRQ_wire(intNum, NULL);
#endif
				_IRQ_descTable[IRQ_EXCEPTIONS + intNum] = NULL;

			}
		} else {	/* soft */
			if (irqDesc->isrFunc) {
				_IRQ_descTable[IRQ_EXCEPTIONS +
					       intNum] = irqDesc;
				__sync_or_and_fetch
				    (&_IRQ_softMask,
				     1 << (intNum - IRQ_HARDINTS));
				if (_IRQ_softPend & _IRQ_softMask)
					IFS0SET = 2;
			} else {
				__sync_and_and_fetch
				    (&_IRQ_softMask,
				     ~(1 << (intNum - IRQ_HARDINTS)));
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
	return IRQ_cause(desc->intNum);
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
	if (!irqDesc)
		return NULL;
	int32_t intNum = irqDesc->intNum;
	uint32_t wordNum = intNum >> 5;
	if (intNum >= IRQ_HARDINTS) {
		_IRQ_softPend &= ~(1 << (intNum - IRQ_HARDINTS));
		if (_IRQ_softPend == 0) {
			IFS0CLR = _IFS0_CS0IF_MASK;
		}
	} else {
		switch (intNum) {
		case 0:	/* Timer */
			mips_setcompare(mips_getcompare());
			IFS0CLR = _IFS0_CTIF_MASK;
			break;
		case 1:
			IFS0CLR = _IFS0_CS0IF_MASK;
			break;
		case 2:
			IFS0CLR = _IFS0_CS1IF_MASK;
		default:
			((uint32_t *) & IFS0CLR)[wordNum << 2] =
			    1 << (intNum & 31);
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
	uint32_t wordNum = intNum >> 5;
	if (intNum < IRQ_HARDINTS) {	/* hard */
		((uint32_t *) & IFS0SET)[wordNum << 2] = 1 << (intNum & 31);

	} else if (intNum < IRQ_HARDINTS + IRQ_SOFTINTS) {	/* soft */
		__sync_or_and_fetch(&_IRQ_softPend,
				    1 << (intNum - IRQ_HARDINTS));
		if (_IRQ_softPend & _IRQ_softMask)
			IFS0SET = 2;
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
	return 0;
}
