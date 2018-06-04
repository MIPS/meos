/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Exception handlers
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/inttypes.h"

uintptr_t _mips_nextpc(struct gpctx *ctx, uint32_t one, uint32_t two);

#ifdef CONFIG_MVZ_BROKEN_TIMERS
/*
 * This function is used for fixing timers. It forces a context switch around
 * when a guest timer is due to go off. The guest context restore code actually
 * fires the timer.
 */
static int32_t doNothing(void)
{
	return 0;
}
#endif

void MVZ_mtc0(reg_t rt, reg_t rd, reg_t sel)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	reg_t value = 0;
	reg_t id = MIPS_C0_REGNAME(rd, sel);

	if (rt)
		value = c->r[rt - 1];

	switch (id) {
	case C0_ERRCTL:
		break;
	case C0_COUNT:
		mips32_set_gc0(C0_GTOFFSET, value - mips_getcount());
		break;
	case C0_COMPARE:
		{
			reg_t reg = mips32_get_gc0(C0_CAUSE);
			if (gt->ipti && ((reg & CR_TI) == CR_TI))
				MVZ_downInt(gt, gt->ipti);
			mips32_set_gc0(C0_CAUSE, reg & ~CR_TI);
			mips32_set_gc0(C0_COMPARE, value);
#ifdef CONFIG_MVZ_BROKEN_TIMERS
			KRN_cancelTimer(&gt->ccTimer);
			reg_t cs = TMR_clockSpeed();
			uint32_t interval =
			    ((value + mips32_get_c0(C0_GTOFFSET)) -
			     TMR_getMonotonic()) / cs;
			KRN_setTimer(&gt->ccTimer,
				     (KRN_TIMERFUNC_T *) & doNothing, NULL,
				     interval);
#endif
		}
		break;
	case C0_INTCTL:
		mips32_set_gc0(C0_INTCTL,
			       (mips32_get_gc0(C0_INTCTL) & 0xff800000) | (value
									   &
									   0x007fe1f0));
		break;
	case C0_CAUSE:
#ifdef CONFIG_MVZ_ACK_VIA_CAUSE
		mips32_set_gc0(C0_CAUSE,
			       (mips32_get_gc0(C0_CAUSE) & 0xf723007c) | (value
									  &
									  0x08c0ff00));
#else
		mips32_set_gc0(C0_CAUSE,
			       (mips32_get_gc0(C0_CAUSE) & 0xf723fc7c) | (value
									  &
									  0x08c00300));
#endif
		break;
	case C0_STATUS:
		mips32_set_gc0(C0_STATUS,
			       (value & ~SR_FR) | (mips32_getsr() & SR_FR));
		break;
	case C0_SRSCTL:
		mips32_set_gc0(C0_SRSCTL, value);
		break;
	case C0_SRSMAP:
		mips32_set_gc0(C0_SRSMAP, value);	// KEEP
		break;
	case C0_PRID:
		DBG_assert(0, "Can't set PRId!");
		break;
	case C0_LLADDR:
		mips32_set_gc0(C0_LLADDR, value);
		break;
	case C0_MAAR:
		mips32_set_gc0(C0_MAAR, value);
		break;
	case C0_MAARI:
		mips32_set_gc0(C0_MAARI, value);
		break;
	default:
		DBG_assert(0, "Gexc on invalid move to GCP0 register %x/%x\n",
			   rd, sel);
		MVZ_restart(gt);
	}
}

#if 0
void MVZ_mthc0(reg_t rt, reg_t rd, reg_t sel)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	reg64_t value = 0;
	reg_t id = rd | sel << 8;

	if (rt)
		value = ((reg64_t) c->r[rt - 1]) << 32;

	switch (id) {
	case C0_LLADDR:
		mips32_seth_gc0(C0_LLADDR, value);
		break;
	case C0_MAAR:
		mips32_seth_gc0(C0_MAAR, value);
		break;
	case C0_MAARI:
		mips32_seth_gc0(C0_MAARI, value);
		break;
	default:
		DBG_assert(0, "MTHC0 on short GCP0 register %x/%x\n", rd, sel);
		MVZ_restart(gt);
	}
}
#endif

void MVZ_mfc0(reg_t rt, reg_t rd, reg_t sel)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	reg64_t value = 0;
	reg_t id = MIPS_C0_REGNAME(rd, sel);

	switch (id) {
	case C0_CDMMBASE:
	case C0_CMGCRBASE:
	case C0_ERRCTL:
		value = 0;
		break;
	case C0_COUNT:
		value = mips32_get_gc0(C0_COUNT);
		break;
	case C0_COMPARE:
		value = mips32_get_gc0(C0_COMPARE);
		break;
	case C0_SRSCTL:
		value = mips32_get_gc0(C0_SRSCTL);
		break;
	case C0_SRSMAP:	// KEEP
		value = mips32_get_gc0(C0_SRSMAP);
		break;
	case C0_PRID:
		value = 0x019300;	/* MIPS 24K */
		break;
	case C0_LLADDR:
		value = mips32_get_gc0(C0_LLADDR);
		break;
	case C0_MAAR:
		value = mips32_get_gc0(C0_MAAR);
		break;
	case C0_MAARI:
		value = mips32_get_gc0(C0_MAARI);
		break;
	default:
		DBG_assert(0, "Gexc on invalid move from GCP0 register %x/%x\n",
			   rd, sel);
		MVZ_restart(gt);
	}
	if (rt && (_KRN_current->reason != KRN_DEAD))
		c->r[rt - 1] = (reg_t) value;
}

#if 0
void MVZ_mfhc0(reg_t rt, reg_t rd, reg_t sel)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	reg64_t value = 0;
	reg_t id = rd | sel << 8;

	switch (id) {
	case C0_CDMMBASE:
		value = 0;
		break;
	case C0_CMGCRBASE:
		value = 0;
		break;
	case C0_LLADDR:
		value = mips32_geth_gc0(C0_LLADDR);
	case C0_MAAR:
		value = mips32_geth_gc0(C0_MAAR);
	case C0_MAARI:
		value = mips32_geth_gc0(C0_MAARI);
	default:
		DBG_assert(0, "MFHC0 on short GCP0 register %x/%x\n", rd, sel);
		MVZ_restart(gt);
	}
	if (rt && (_KRN_current->reason != KRN_DEAD))
		c->r[rt - 1] = (reg_t) (value >> 32);
}
#endif

#define ureloff9(inst) ((((inst) & 0x1ff) ^ 0x100) - 0x100)
#define reloff16(inst) ((((inst) & 0xffff) ^ 0x8000) - 0x8000)
#define reloff9(inst) ((_mips32r2_ext((inst), 7, 9) ^ 0x100) - 0x100)
#define REGSET(C, I, V) do {reg_t __v = (V); reg_t __i = (I); if (i) (C)->r[__i - 1] = __v;} while (0)

/* This handles both GSPI and GSFC */
static inline void MVZ_emulate()
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	uint32_t instr;
	reg_t pc = c->epc;
	uint32_t i, i2;
	reg_t base, op, rt;
	sreg_t offset;

	(void)base;
	(void)op;
	(void)rt;
	(void)offset;

#ifdef CONFIG_ARCH_MIPS_MICRO
	uint16_t im;
	if (pc & 1) {
		/* microMIPS */
		MVZ_readGV((void *)pc - 1, &im, 2, 1, gt);
		instr = (uint32_t) im;

		if (c->cause & CR_BD) {
			/* In a delay slot */
			if (((instr & 0xc000) >= 0x1000)
			    && ((instr & 0xc000) <= 0x3000)) {
				/* This instruction is a 4 byte branch */
				/* Emulate next instruction */
				pc += 4;
				MVZ_readGV((void *)pc - 1, &im, 2, 1, gt);
				instr = (uint32_t) im;
			} else {
				/* This instruction is 2 bytes */
				/* Emulate next instruction */
				pc += 2;
				MVZ_readGV((void *)pc - 1, &im, 2, 1, gt);
				instr = (uint32_t) im;
			}
		}

		if (((instr & 0xc000) >= 0x1000)
		    && ((instr & 0xc000) <= 0x3000)) {
			/* Load the rest of a 4 byte instruction */
			instr <<= 16;
			MVZ_readGV((void *)pc + 1, &im, 2, 1, gt);
			instr |= (uint32_t) im;
		} else {
			/* All supported instructions are 32 bits long */
			DBG_assert(0,
				   "Guest '%s' caused unexpected gexc!\n",
				   KRN_taskName(NULL));
			MVZ_restart(gt);
			return;
		}

		if ((instr & 0xfc00ffff) == 0x0000937c) {
			/* WAIT */
			KRN_scheduleProtected();
		} else if (((instr & 0xfc00fe00) == 0x20006000)
			   || ((instr & 0xfc00fe00) == 0x6000a600)) {
			/* CACHE(E) */
			DBG_assert(0,
				   "Guest '%s' attempted illegal cacheop!\n",
				   KRN_taskName(NULL));
			MVZ_restart(gt);
			return;
		} else if ((instr & 0xfc00c7ff) == 0x000002fa) {
			/* MTC0 */
			/* (microMIPS) Convert 16-bit register encoding to 32-bit register encoding. */
			MVZ_mtc0(_mips32r2_ext(instr, 21, 5),
				 _mips32r2_ext(instr, 16, 5),
				 _mips32r2_ext(instr, 11, 3));
		} else if ((instr & 0xfc00c7ff) == 0x000000fa) {
			/* MFC0 */
			MVZ_mfc0(_mips32r2_ext(instr, 21, 5),
				 _mips32r2_ext(instr, 16, 5),
				 _mips32r2_ext(instr, 11, 3));
		} else if ((instr & 0xfc00c7ff) == 0x000002f8) {
			/* MTHC0 */
			/*
			   MVZ_mthc0(_mips32r2_ext(instr, 21, 5),
			   _mips32r2_ext(instr, 16, 5),
			   _mips32r2_ext(instr, 11, 3)); */
		} else if ((instr & 0xfc00c7ff) == 0x000000f8) {
			/* MFHC0 */
			/*MVZ_mfhc0(_mips32r2_ext(instr, 21, 5),
			   _mips32r2_ext(instr, 16, 5),
			   _mips32r2_ext(instr, 11, 3)); */
		} else if ((instr & 0xfc00c7ff) == 0x000001c0) {
			/* RDHWR */
			reg_t rs = _mips32r2_ext(instr, 21, 5);
			reg_t rt = _mips32r2_ext(instr, 16, 5);
			reg_t sel = _mips32r2_ext(instr, 11, 3);
			reg_t r;
			switch (rs) {
			case 0:
				REGSET(c, rt,
				       mips32_get_gc0(C0_EBASE) & EBASE_CPU);
				break;
			case 1:
			      asm("rdhwr %0, $1;":"=r"(r));
				REGSET(c, rt, r);
				break;
			case 2:
				REGSET(c, rt, mips32_get_gc0(C0_COUNT));
				break;
			case 3:
			      asm("rdhwr %0, $3;":"=r"(r));
				REGSET(c, rt, r);
				break;
			case 4:
				switch (sel) {
				case 0:
				      asm("rdhwr %0, $4,0;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 1:
				      asm("rdhwr %0, $4,1;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 2:
				      asm("rdhwr %0, $4,2;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 3:
				      asm("rdhwr %0, $4,3;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 4:
				      asm("rdhwr %0, $4,4;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 5:
				      asm("rdhwr %0, $4,5;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 6:
				      asm("rdhwr %0, $4,6;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 7:
				      asm("rdhwr %0, $4,7;":"=r"(r));
					REGSET(c, rt, r);
					break;
				}
				break;
			case 5:
				REGSET(c, rt,
				       (mips32_get_gc0(C0_CONFIG5) & CFG5_XNP)
				       >> CFG5_XNP_SHIFT);
				break;
			case 29:
				REGSET(c, rt, mips32_get_gc0(C0_USERLOCAL));
				break;
			}
		} else if ((instr & 0xfc3f) == 0x443b) {
			i = (instr & 0x03c0) >> 6;
			if (i == 1) {

/* SDBBP 1 */
				_MVZ->hyper(0, c);
			} else {
				DBG_assert(0,
					   "Guest '%s' took unexpected sdbbp %"
					   PRIu32 " at %08" PRIx32
					   "!\n",
					   KRN_taskName(NULL),
					   (instr & 0x03ffffc0) >> 6, (uint32_t)
					   _KRN_current->savedContext.gp.epc);
			}
		} else if ((instr & 0xfc00ffff) == 0x0000db7c) {
			i = (instr >> 16) & 0x03ff;
			if (i == 1) {
				/* SDBBP 1 */
				_MVZ->hyper(0, c);
			} else {
				DBG_assert(0,
					   "Guest '%s' took unexpected sdbbp %"
					   PRIu32 " at %08" PRIx32
					   "!\n",
					   KRN_taskName(NULL),
					   (instr & 0x03ffffc0) >> 6, (uint32_t)
					   _KRN_current->savedContext.gp.epc);
			}
		} else if ((instr & 0xfc00ffff) == 0x0000c37c) {
			/* HYPCALL x */
			_MVZ->hyper(0, c);
		} else {
			DBG_assert(0,
				   "Guest '%s' caused unexpected gexc!\n",
				   KRN_taskName(NULL));
			MVZ_restart(gt);
			return;
		}

	} else
#endif
	{
		/* macroMIPS */
		MVZ_readGV((void *)pc, &i, 4, 1, gt);
		instr = i;
		if (c->cause & CR_BD) {
			/* In a delay slot */
			pc += 4;
			MVZ_readGV((void *)pc, &i, 4, 1, gt);
			instr = i;
		}

		if ((instr & 0xfe00003f) == 0x42000020) {
			/* WAIT */
			_KRN_current->reason = KRN_RELEASE;
			KRN_scheduleProtected();
			return;
		} else if (
#ifdef CONFIG_ARCH_MIPS_R6
				  ((instr & 0xfc00007f)
				   == 0x7c000025)
#else
				  ((instr & 0xfc000000)
				   == 0xbc000000)
#endif
				  || ((instr & 0xfc00007f)
				      == 0x7c00001b)) {
			/* CACHE(E) */
			DBG_assert(0,
				   "Guest '%s' attempted illegal cacheop!\n",
				   KRN_taskName(NULL));
			MVZ_restart(gt);
			return;
		} else if ((instr & 0xffe007f8) == 0x40800000) {
			/* MTC0 */
			MVZ_mtc0(_mips32r2_ext
				 (instr, 16, 5),
				 _mips32r2_ext(instr, 11,
					       5), _mips32r2_ext(instr, 0, 3));
		} else if ((instr & 0xffe007f8) == 0x40000000) {
			/* MFC0 */
			MVZ_mfc0(_mips32r2_ext
				 (instr, 16, 5),
				 _mips32r2_ext(instr, 11,
					       5), _mips32r2_ext(instr, 0, 3));
		} else if ((instr & 0xffe007f8) == 0x40c00000) {
			/* MTHC0 */
			/*MVZ_mthc0(_mips32r2_ext(instr, 16, 5),
			   _mips32r2_ext(instr, 11, 5),
			   _mips32r2_ext(instr, 0, 3)); */
		} else if ((instr & 0xffe007f8) == 0x40400000) {
			/* MFHC0 */
			/*MVZ_mfhc0(_mips32r2_ext(instr, 16, 5),
			   _mips32r2_ext(instr, 11, 5),
			   _mips32r2_ext(instr, 0, 3)); */
		} else if ((instr & 0xffe0063f) == 0x7c00003b) {
			/* RDHWR */
			reg_t rt = _mips32r2_ext(instr, 16, 5);
			reg_t rd = _mips32r2_ext(instr, 11, 5);
#ifdef R6
			reg_t sel = _mips32r2_ext(instr, 6, 3);
#endif
			reg_t r;
			switch (rd) {
			case 0:
				REGSET(c, rt,
				       (mips32_get_gc0(C0_EBASE) & EBASE_CPU));
				break;
			case 1:
			      asm("rdhwr %0, $1;":"=r"(r));
				REGSET(c, rt, r);
				break;
			case 2:
				REGSET(c, rt, mips32_get_gc0(C0_COUNT));
				break;
			case 3:
			      asm("rdhwr %0, $3;":"=r"(r));
				REGSET(c, rt, r);
				break;
			case 4:
#ifdef R6
				switch (sel) {
				case 0:
				      asm("rdhwr %0, $4,0;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 1:
				      asm("rdhwr %0, $4,1;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 2:
				      asm("rdhwr %0, $4,2;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 3:
				      asm("rdhwr %0, $4,3;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 4:
				      asm("rdhwr %0, $4,4;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 5:
				      asm("rdhwr %0, $4,5;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 6:
				      asm("rdhwr %0, $4,6;":"=r"(r));
					REGSET(c, rt, r);
					break;
				case 7:
				      asm("rdhwr %0, $4,7;":"=r"(r));
					REGSET(c, rt, r);
					break;
				}
#endif
				break;
			case 5:
				REGSET(c, rt,
				       (mips32_get_gc0(C0_CONFIG5) & CFG_XNP)
				       >> CFG_XNP_SHIFT);
				break;
			case 29:
				REGSET(c, rt, mips32_get_gc0(C0_USERLOCAL));
				break;
			}
		} else if (((instr & 0xfa00003f) == 0x7000003f)
			   || ((instr & 0xfc00003f) == 0x000000e)) {
			i = (instr & 0x03ffffc0) >> 6;
			if (i == 1) {
				/* SDBBP 1 */
				_MVZ->hyper(0, c);
			} else {

				DBG_assert(0,
					   "Guest '%s' took unexpected sdbbp %"
					   PRIu32 " at %08" PRIx32
					   "!\n", KRN_taskName(NULL),
					   (instr & 0x03ffffc0) >> 6, (uint32_t)
					   _KRN_current->savedContext.gp.epc);
			}
		} else if ((instr & 0xffe007ff) == 0x42000028) {
			/* HYPCALL x */
			_MVZ->hyper(0, c);
		} else {
			DBG_logF
			    ("Guest '%s' caused unexpected gexc with instruction %08"
			     PRIx32 "!\n", KRN_taskName(NULL), instr);
			DBG_assert(0,
				   "Guest '%s' caused unexpected gexc with instruction %08"
				   PRIx32 "!\n", KRN_taskName(NULL), instr);
			MVZ_restart((MVZ_GUEST_T *)
				    _KRN_current);
			return;
		}
	}

	if (_KRN_current->reason != KRN_DEAD) {
		MVZ_readGV((void *)_KRN_current->savedContext.gp.epc, &i, 4, 1,
			   gt);
		MVZ_readGV((void *)_KRN_current->savedContext.gp.epc + 4, &i2,
			   4, 1, gt);
		c->epc = _mips_nextpc(&_KRN_current->savedContext.gp, i, i2);
	}
}

void MVZ_exception(int32_t intNum, struct gpctx *ctx)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	uint32_t gxc = (mips32_get_c0(C0_GUESTCTL0) & GUESTCTL0_GEXCCODE)
	    >> GUESTCTL0_GEXCCODE_SHIFT;
	switch (gxc) {
	case GEXC_GPSI:	/* Guest Privileged Sensitive instruction */
	case GEXC_GSFC:	/* Guest Software Field Change event */
	case GEXC_GRR:		/* Guest Reserved Instruction Redirect */
	case GEXC_HC:		/* Hypercall */
		MVZ_emulate();
		break;
	case GEXC_GVA:		/* Guest initiated TLB has GVA available */
		DBG_assert(0,
			   "Can't happen! Guest '%s' caused page fault in root context!\n",
			   KRN_taskName(NULL));
		MVZ_restart(gt);
		break;
	case GEXC_GHFC:	/* Guest Hardware Field Change event */
		if (gt->debugger) {
			reg_t sr = mips32_get_gc0(C0_SR);
			if ((sr & SR_EXL) == SR_EXL) {
				reg_t exc = mips32_get_gc0(C0_CR) & CR_X_MASK;
				if (exc == CR_XCPT(EXC_BP)) {
					ctx->epc = mips32_get_gc0(C0_EPC);
					mips32_set_gc0(C0_SR, sr & ~SR_EXL);
					gt->debugger(gt, gt->debugPriv);
				} else {
					MVZ_step(gt, ctx->epc);
				}
			} else {
				MVZ_step(gt, ctx->epc);
			}
		}
		break;
	case GEXC_GPA:		/* Guest initiated TLB has GPA available */
		DBG_assert(0,
			   "Can't happen! Guest '%s' caused page fault in root context!\n",
			   KRN_taskName(NULL));
		MVZ_restart(gt);
		break;
	default:
		DBG_assert(0,
			   "Unrecognised guest exception in root context!\n");
		MVZ_restart(gt);
	}
}

int MVZ_tlbxRead(void *paddr, void *buf, int size, int n, void *priv)
{
	if ((size & 0x80000000) == 0x80000000) {
		return MVZ_readGV(paddr, buf, size & ~0x80000000, n, priv);
	} else {
		MVZ_GUEST_T *gt = (MVZ_GUEST_T *) priv;
		MVZ_VREGS_T *handler = (MVZ_VREGS_T *) LST_first(&gt->mappings);
		return handler->read(paddr, buf, size, n, (void *)handler);
	}
}

int MVZ_tlbxWrite(void *paddr, void *buf, int size, int n, void *priv)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) priv;
	MVZ_VREGS_T *handler = (MVZ_VREGS_T *) LST_first(&gt->mappings);
	return handler->write(paddr, buf, size, n, (void *)handler);
}

extern int mips_emulate_ls(struct
			   gpctx *c, void
			   *addr, MVZ_XFERFUNC_T read, MVZ_XFERFUNC_T write, void
			   *rwpriv);
void MVZ_tlbx(int32_t intNum, struct gpctx *ctx)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	uintptr_t bad = ctx->badvaddr;
	MVZ_PTE_T *pte = (MVZ_PTE_T *) LST_first(&gt->mappings);
	while (pte) {
		if (pte->start & 0x1) {
			uintptr_t start = pte->start & ~1;
			MVZ_VREGS_T *handler = (MVZ_VREGS_T *) pte;
			if ((bad >= start)
			    && (bad <= handler->stop)) {
				uintptr_t addr = ctx->epc;
				uint32_t i, i2;
				LST_remove(&gt->mappings, handler);
				LST_addHead(&gt->mappings, handler);
				if (ctx->cause & CR_BD)
					addr += 4;	/* FIXME: handle 16 bit case - can do this by using final branch emulator */
				mips_emulate_ls(ctx, (void *)addr,
						MVZ_tlbxRead,
						MVZ_tlbxWrite, (void *)gt);
				if (_KRN_current->reason != KRN_DEAD) {
					MVZ_readGV((void *)ctx->epc, &i, 4, 1,
						   gt);
					MVZ_readGV((void *)ctx->epc + 4, &i2, 4,
						   1, gt);
					ctx->epc = _mips_nextpc(ctx, i, i2);
				}
				return;
			}
		}
#ifdef CONFIG_MVZ_TLB_REFILL
		else {
			/* Slow but memory efficient TLB refill */
			reg_t newgid = gt->gid;
			tlbhi_t hi = (tlbhi_t) mips32_get_c0(C0_ENTRYHI);
			if ((bad >= pte->start)
			    && (bad <= pte->stop)) {
				uintptr_t off =
				    (hi & C0_ENTRYHI_VPN2_MASK) - pte->start;
				reg_t gctl1 = mips32_get_c0(C0_GUESTCTL1);
				mips32_bcs_c0(C0_GUESTCTL1,
					      GUESTCTL1_RID,
					      newgid << GUESTCTL1_RID_SHIFT);
				if (!
				    (mips32_get_c0(C0_GUESTCTL0) &
				     GUESTCTL0_RAD))
					newgid = 0;
				const uintptr_t pm =
				    ((CONFIG_MVZ_PAGESIZE * 2) - 0x2000);
				reg_t pa =
				    (pte->paddr +
				     off) & ~((CONFIG_MVZ_PAGESIZE * 2) - 1);
				reg_t pa2 = pa + CONFIG_MVZ_PAGESIZE;
				hi = (hi & C0_ENTRYHI_VPN2_MASK) | newgid;
				reg_t lf =
				    (pte->flags & (ENTRYLO0_RI_MASK |
						   ENTRYLO0_D_MASK |
						   ENTRYLO0_XI_MASK)) ^
				    ENTRYLO0_D_MASK;
				reg_t l0 =
				    ((pa >> 6) & ENTRYLO0_PFN_MASK) | (5 <<
								       ENTRYLO0_C_SHIFT)
				    | lf | ENTRYLO0_V_MASK;
				reg_t l1 =
				    ((pa2 >> 6) & ENTRYLO1_PFN_MASK) | (5 <<
									ENTRYLO1_C_SHIFT)
				    | lf | ENTRYLO1_V_MASK;
				/* As long as pages are the same size, this won't alias */
				mips_tlbwr2(hi, l0, l1, pm);	/* GuestCtl1 already set */
				mips32_set_c0(C0_GUESTCTL1, gctl1);
				return;
			}
		}
#endif
		pte = (MVZ_PTE_T *) LST_next((LST_T *)
					     pte);
	}
	DBG_assert(0,
		   "Unhandled TLB exception! %s accessed %08"
		   PRIx32 "!\n", KRN_taskName(NULL), bad);
	MVZ_restart(gt);
}
