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
*   Description:	MIPS timer specialisation
*
*************************************************************************/

#ifndef TARGET_TMR_H
#define TARGET_TMR_H

#include <limits.h>
#include <stdint.h>
#include <mips/cpu.h>

inline static void TMR_startCycleCount() __attribute__((optimize("O0"))) __attribute__ ((no_instrument_function));
inline static void TMR_startCycleCount()
{
	extern uint32_t _TMR_start;
	_TMR_start = _m32c0_mfc0(C0_PERFCNT, 1);
}

inline static uint32_t TMR_stopCycleCount() __attribute__((optimize("O0"))) __attribute__ ((no_instrument_function));
inline static uint32_t TMR_stopCycleCount()
{
	extern uint32_t _TMR_overhead, _TMR_start;
	uint32_t _TMR_stop = _m32c0_mfc0(C0_PERFCNT, 1);
	asm("ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop; ssnop;");
	if (_TMR_stop > _TMR_start)
		return (_TMR_stop - _TMR_start) - _TMR_overhead;
	else
		return (UINT32_MAX - _TMR_start) + _TMR_stop - _TMR_overhead;
}

#define TMR_getMonotonic() mips_getcount()

#ifdef CONFIG_ARCH_MIPS_PCINT
#ifdef __cplusplus
extern "C" {
#endif
	extern uint32_t _TMR_overflow0, _TMR_overflow1;
#ifdef CONFIG_ARCH_MIPS_QPC
	extern uint32_t _TMR_overflow2, _TMR_overflow3;
#endif
#ifdef __cplusplus
}
#endif
#endif

#ifdef CONFIG_ARCH_MIPS_QPC
#define TMR_PERFCOUNTERS 4
#else
#define TMR_PERFCOUNTERS 2
#endif

#ifdef CONFIG_ARCH_MIPS_QPC
#ifdef CONFIG_ARCH_MIPS_PCINT
#define TMR_getPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register reg_t before;\
	register uint32_t hi;\
	register uint32_t lo;\
	before = mips_bicsr(SR_IE);\
	switch (X)\
	{\
		case 0:\
			hi = _TMR_overflow0;\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			break;\
		case 1:\
			hi = _TMR_overflow1;\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			break;\
		case 2:\
			hi = _TMR_overflow2;\
			lo = _m32c0_mfc0(C0_PERFCNT, 5);\
			break;\
		case 3:\
			hi = _TMR_overflow3;\
			lo = _m32c0_mfc0(C0_PERFCNT, 7);\
			break;\
		default:\
			hi = 0;\
			lo = 0;\
	}\
	if (before & SR_IE)\
		mips_bissr(SR_IE);\
	(((uint64_t) hi) << 31) + (uint64_t) lo;\
})
#define TMR_resetPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register reg_t before;\
	register uint32_t hi;\
	register uint32_t lo;\
	before = mips_bicsr(SR_IE);\
	switch (X)\
	{\
		case 0:\
			hi = _TMR_overflow0;\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			_TMR_overflow0 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 1, 0);\
			break;\
		case 1:\
			hi = _TMR_overflow1;\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			_TMR_overflow1 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 3, 0);\
			break;\
		case 2:\
			hi = _TMR_overflow2;\
			lo = _m32c0_mfc0(C0_PERFCNT, 5);\
			_TMR_overflow2 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 5, 0);\
			break;\
		case 3:\
			hi = _TMR_overflow3;\
			lo = _m32c0_mfc0(C0_PERFCNT, 7);\
			_TMR_overflow3 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 6, 0);\
			break;\
		default:\
			hi = 0;\
			lo = 0;\
	}\
	if (before & SR_IE)\
		mips_bissr(SR_IE);\
	(((uint64_t) hi) << 31) + (uint64_t) lo;\
})
#define TMR_configPerfCount(X, E) __extension__ ({\
		switch (X)\
	{\
		case 0:\
			_m32c0_mtc0(C0_PERFCNT, 0, (E) << 5 | 0x1f);\
			break;\
		case 1:\
			_m32c0_mtc0(C0_PERFCNT, 2, (E) << 5 | 0x1f);\
			break;\
		case 2:\
			_m32c0_mtc0(C0_PERFCNT, 4, (E) << 5 | 0x1f);\
			break;\
		case 3:\
			_m32c0_mtc0(C0_PERFCNT, 6, (E) << 5 | 0x1f);\
			break;\
		default:\
			break;\
	}\
})
#else
#define TMR_getPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register uint32_t lo;\
	switch (X)\
	{\
		case 0:\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			break;\
		case 1:\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			break;\
		case 2:\
			lo = _m32c0_mfc0(C0_PERFCNT, 5);\
			break;\
		case 3:\
			lo = _m32c0_mfc0(C0_PERFCNT, 7);\
			break;\
		default:\
			lo = 0;\
	}\
	(uint64_t) lo;\
})
#define TMR_resetPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register uint32_t lo;\
	switch (X)\
	{\
		case 0:\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			_m32c0_mtc0(C0_PERFCNT, 1, 0);\
			break;\
		case 1:\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			_m32c0_mtc0(C0_PERFCNT, 3, 0);\
			break;\
		case 2:\
			lo = _m32c0_mfc0(C0_PERFCNT, 5);\
			_m32c0_mtc0(C0_PERFCNT, 5, 0);\
			break;\
		case 3:\
			lo = _m32c0_mfc0(C0_PERFCNT, 7);\
			_m32c0_mtc0(C0_PERFCNT, 7, 0);\
			break;\
		default:\
			lo = 0;\
	}\
	(uint64_t) lo;\
})
#define TMR_configPerfCount(X, E) __extension__ ({\
		switch (X)\
	{\
		case 0:\
			_m32c0_mtc0(C0_PERFCNT, 0, (E) << 5 | 0xf);\
			break;\
		case 1:\
			_m32c0_mtc0(C0_PERFCNT, 2, (E) << 5 | 0xf);\
			break;\
		case 2:\
			_m32c0_mtc0(C0_PERFCNT, 4, (E) << 5 | 0xf);\
			break;\
		case 3:\
			_m32c0_mtc0(C0_PERFCNT, 6, (E) << 5 | 0xf);\
			break;\
		default:\
			break;\
	}\
})
#endif
#else
#ifdef CONFIG_ARCH_MIPS_PCINT
#define TMR_getPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register reg_t before;\
	register uint32_t hi;\
	register uint32_t lo;\
	before = mips_bicsr(SR_IE);\
	switch (X)\
	{\
		case 0:\
			hi = _TMR_overflow0;\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			break;\
		case 1:\
			hi = _TMR_overflow1;\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			break;\
		default:\
			hi = 0;\
			lo = 0;\
	}\
	if (before & SR_IE)\
		mips_bissr(SR_IE);\
	(((uint64_t) hi) << 31) + (uint64_t) lo;\
})
#define TMR_resetPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register reg_t before;\
	register uint32_t hi;\
	register uint32_t lo;\
	before = mips_bicsr(SR_IE);\
	switch (X)\
	{\
		case 0:\
			hi = _TMR_overflow0;\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			_TMR_overflow0 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 1, 0);\
			break;\
		case 1:\
			hi = _TMR_overflow1;\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			_TMR_overflow1 = 0;\
			_m32c0_mtc0(C0_PERFCNT, 3, 0);\
			break;\
		default:\
			hi = 0;\
			lo = 0;\
	}\
	if (before & SR_IE)\
		mips_bissr(SR_IE);\
	(((uint64_t) hi) << 31) + (uint64_t) lo;\
})
#define TMR_configPerfCount(X, E) __extension__ ({\
	switch (X)\
	{\
		case 0:\
			_m32c0_mtc0(C0_PERFCNT, 0, (E) << 5 | 0x1f);\
			break;\
		case 1:\
			_m32c0_mtc0(C0_PERFCNT, 2, (E) << 5 | 0x1f);\
			break;\
		default:\
			break;\
	}\
})
#else
#define TMR_getPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register uint32_t lo;\
	switch (X)\
	{\
		case 0:\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			break;\
		case 1:\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			break;\
		default:\
			lo = 0;\
	}\
	(uint64_t) lo;\
})
#define TMR_resetPerfCount(X) __extension__ ({\
	/* Don't use IRQ to do this - we don't want the trace noise */\
	register uint32_t lo;\
	switch (X)\
	{\
		case 0:\
			lo = _m32c0_mfc0(C0_PERFCNT, 1);\
			_m32c0_mtc0(C0_PERFCNT, 1, 0);\
			break;\
		case 1:\
			lo = _m32c0_mfc0(C0_PERFCNT, 3);\
			_m32c0_mtc0(C0_PERFCNT, 3, 0);\
			break;\
		default:\
			lo = 0;\
	}\
	(uint64_t) lo;\
})
#define TMR_configPerfCount(X, E) __extension__ ({\
	switch (X)\
	{\
		case 0:\
			_m32c0_mtc0(C0_PERFCNT, 0, (E) << 5 | 0xf);\
			break;\
		case 1:\
			_m32c0_mtc0(C0_PERFCNT, 2, (E) << 5 | 0xf);\
			break;\
		default:\
			break;\
	}\
})
#endif
#endif

#endif
