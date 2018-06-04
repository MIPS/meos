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

#ifndef TARGET_IRQ2_H
#define TARGET_IRQ2_H

#include <mips/cpu.h>
#include "meos/target/ctx.h"

#ifndef __ASSEMBLER__
extern volatile int32_t _IRQ_ipl;
extern int32_t _IRQ_log;

typedef struct {
	intptr_t pAddr;
	uint32_t volatile *regs;
	uint32_t mask;
} XPSINTC_T;

void XPSINTC_init(XPSINTC_T * intc, IRQ_DESC_T * irq);
#endif

#define IRQ_getIPL() __extension__ ({ \
	_IRQ_ipl; \
})
#define _IRQ_disable() __asm__ __volatile__ ("di; ehb")
#define IRQ_raiseIPL()  __extension__ ({ \
	_IRQ_disable(); \
	DBG_raise(_IRQ_ipl); \
	_IRQ_ipl++; \
})

#ifdef CONFIG_ARCH_MIPS_VZ

#define _IRQ_enable() do { \
	register KRN_CTX_T *k0 = (KRN_CTX_T *)mips32_get_c0(C0_KSCRATCH1); \
	if (!_IRQ_log && !(_IRQ_ipl || (k0 && k0->nest))) \
		__asm__ __volatile__ ("ei; ehb");\
	} while (0)

#define IRQ_bg() __extension__ ({ \
	register KRN_CTX_T *k0 = (KRN_CTX_T *)mips32_get_c0(C0_KSCRATCH1); \
	k0 ? ((_IRQ_ipl == 0) && (k0->nest == 0)) : 1; \
})

#define IRQ_servicing() __extension__ ({ \
	register KRN_CTX_T *k0 = (KRN_CTX_T *)mips32_get_c0(C0_KSCRATCH1); \
	k0 && (k0->nest != 0); \
})

#else

#define _IRQ_enable() do { \
	register KRN_CTX_T *k0 asm ("k0"); \
	if (!_IRQ_log && !(_IRQ_ipl || (k0 && k0->nest))) \
		__asm__ __volatile__ ("ei; ehb"); \
	} while (0)

#define IRQ_bg() __extension__ ({ \
	register KRN_CTX_T *k0 asm ("k0"); \
	k0 ? ((_IRQ_ipl == 0) && (k0->nest == 0)) : 1; \
})

#define IRQ_servicing() __extension__ ({ \
	register KRN_CTX_T *k0 asm ("k0"); \
	k0 && (k0->nest != 0); \
})

#endif
#define IRQ_restoreIPL(X) do {int32_t to = (X); DBG_lower(_IRQ_ipl, to); _IRQ_ipl = to; _IRQ_enable();} while (0)
#define IRQ_wait() _mips_wait()

#define IRQ_critical() (_IRQ_ipl != 0)

#define IRQ_EXCEPTIONS 32
#define IRQ_SOFTINTS 32
#ifdef CONFIG_ARCH_MIPS_MCU_ASE
#define IRQ_CORE_HARDINTS 12
#else
#define IRQ_CORE_HARDINTS 10
#endif
#define XPSINTC_HARDINTS 8	/* FIXME: should probably be configurable? */
#define XPSINTC_FIRSTINT IRQ_CORE_HARDINTS
#define IRQ_HARDINTS (IRQ_CORE_HARDINTS + XPSINTC_HARDINTS)
#define IRQ_TIMER (IRQ_CORE_HARDINTS - 2)
#define IRQ_FDC (IRQ_CORE_HARDINTS - 1)
#endif
