/*
 * Copyright 2014-2015MIPS Tech, LLC and/or its
 *                      affiliated group companies.
 * All rights reserved.
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
 */

#include <mips/endian.h>
#include <mips/mips32.h>
#include <mips/hal.h>
#include <assert.h>
#include <stddef.h>

/* Interchangeable 16/32-bit type */
typedef union {
	uint32_t fat;
	struct __attribute__ ((__packed__)) {
#if BYTE_ORDER == BIG_ENDIAN
		uint16_t first;
		uint16_t second;
#else
		uint16_t second;
		uint16_t first;
#endif
	} thin;
} uinterch_t;

static inline reg_t addwilloverflow(reg_t a, reg_t b)
{
	sreg_t sa = (sreg_t) a, sb = (sreg_t) b;
	sreg_t r = a + b;
	return (sa < 0 && sb < 0 && r >= 0) || (sa >= 0 && sb >= 0 && r < 0);
}

static inline reg_t isr6()
{
	return (mips32_getconfig0() & CFG0_AR_MASK) == (2 << CFG0_AR_SHIFT);
}

/* The bbit instructions only exist on Cavium cores */
static inline reg_t isbbit(reg_t inst)
{
	reg_t op;
	if (_mips32r2_ext(mips32_getprid(), 16, 7) != 0xd0000)
		return 0;
	op = _mips32r2_ext(inst, 26, 6);
	return (op == 50 || op == 54 || op == 58 || op == 62);
}

/* Search a chain of contexts for a DSP context */
static inline struct dspctx *getdspctx(struct gpctx *ctx)
{
	struct dspctx *dctx = (struct dspctx *)ctx->link;
	while (dctx && (dctx->link.id != LINKCTX_TYPE_DSP))
		dctx = (struct dspctx *)dctx->link.next;
	return dctx;
}

/* Search a chain of contexts for an FPU context */
static inline struct fpctx *getfpctx(struct gpctx *ctx)
{
	struct fpctx *fctx = (struct fpctx *)ctx->link;
	while (fctx && (fctx->link.id != LINKCTX_TYPE_FP32)
	       && (fctx->link.id != LINKCTX_TYPE_FP64)
	       && (fctx->link.id != LINKCTX_TYPE_FMSA))
		fctx = (struct fpctx *)fctx->link.next;
	return fctx;
}

/* Search a chain of contexts for an MSA context */
static inline struct msactx *getmsactx(struct gpctx *ctx)
{
	struct msactx *mctx = (struct msactx *)ctx->link;
	while (mctx && (mctx->link.id != LINKCTX_TYPE_MSA)
	       && (mctx->link.id != LINKCTX_TYPE_FMSA))
		mctx = (struct msactx *)mctx->link.next;
	return mctx;
}

/* Helper functions to index GP registers in a gpctx */
static inline sreg_t getreg(struct gpctx *ctx, sreg_t i)
{
	if (i)
		return (sreg_t) ctx->r[i - 1];
	else
		return 0;
}

static inline reg_t getureg(struct gpctx *ctx, sreg_t i)
{
	if (i)
		return (reg_t) ctx->r[i - 1];
	else
		return 0;
}

/* Return if an MSA branch should be taken */
static inline reg_t takebc1msa(struct gpctx *ctx, reg_t op, reg_t wt)
{
	struct msactx *mctx = getmsactx(ctx);
	reg_t taken = -1;
	reg_t elsz;
	uint8_t *buf, *end, *elend;

	if (!mctx)
		return -1;

	buf = (uint8_t *) & mctx->w[wt];
	if ((op & 0x18) == 0x18) {
		elsz = 1 << (op & 0x3);
		taken = 0;
		for (end = buf + sizeof(_msareg); buf < end;) {
			taken = 1;
			for (elend = buf + elsz; buf < elend; buf++) {
				if (*buf) {
					taken = 0;
					break;
				}
			}
			if (taken)
				break;
			buf = elend;
		}

	} else if ((op & 0x1b) == 0x0b) {
		taken = 1;
		for (end = buf + sizeof(_msareg); buf < end; buf++) {
			if (*buf) {
				taken = 0;
				break;
			}
		}
	}

	if (op & 0x4)
		return !taken;
	else
		return taken;
}

/* Mapping from 16-bit to 32-bit register numberings */
static const signed char U2M[8] = { 16, 17, 2, 3, 4, 5, 6, 7 };

/* ---microMIPS--- */

/* Return microMIPS op code */
static inline reg_t uop(reg_t i)
{
	return i >> 10;
}

/* Return number of bytes for a microMIPS sequence */
static inline reg_t usz(reg_t first)
{
	if (isr6() && uop(first) == 0x1f)
		return 6;
	else if (((uop(first) & 0x4) == 0x4)
		 || ((uop(first) & 0x7) == 0x0))
		return 4;
	else
		return 2;
}

/* Decode microMIPS relative offsets */
static inline sreg_t ureloff26(reg_t inst)
{
	return ((_mips32r2_ext(inst, 0, 26) ^ 0x2000000) - 0x2000000) << 1;
}

static inline sreg_t ureloff21(reg_t inst)
{
	return ((_mips32r2_ext(inst, 0, 21) ^ 0x100000) - 0x100000) << 1;
}

static inline sreg_t ureloff16(reg_t inst)
{
	return (((inst & 0xffff) ^ 0x8000) - 0x8000) << 1;
}

static inline sreg_t ureloff10(reg_t inst)
{
	return (((inst & 0x3ff) ^ 0x200) - 0x200) << 1;
}

static inline sreg_t ureloff7(reg_t inst)
{
	return (((inst & 0x7f) ^ 0x40) - 0x40) << 1;
}

/*
 * Compute the next PC for a microMIPS R6 instruction.
 * May not want to be static.
 */
static inline uintptr_t _umipsr6_nextpc(struct gpctx *ctx, uinterch_t one)
{
	reg_t pc = ctx->epc;
	reg_t inst;
	reg_t val;
	reg_t rs, rt, vrs, vrt;
	struct dspctx *dctx;

	inst = one.thin.first;
	pc += 2;

	/* 32-bit instructions.  */
	if (((uop(inst) & 0x4) == 0x4) || ((uop(inst) & 0x7) == 0x0)) {
		inst <<= 16;
		inst |= one.thin.second;
		rt = _mips32r2_ext(inst, 21, 5);
		rs = _mips32r2_ext(inst, 16, 5);
		pc += 2;
		switch (uop(inst >> 16)) {
		case 0x00:	/* POOL32A */
			if ((inst & 0x3f) == 0x3c) {
				/* POOL32Axf:JALRC(.HB) */
				if ((_mips32r2_ext(inst, 6, 10) == 0x3c)
				    || (_mips32r2_ext(inst, 6, 10) == 0x7c))
					pc = getreg(ctx,
						    _mips32r2_ext(inst, 16, 5));
				break;
		case 0x10:	/* POOL32I */
				switch (_mips32r2_ext(inst, 21, 5)) {
				case 0x08:	/* BC1EQZC */
				case 0x09:	/* BC1NEZC */
					if (!getfpctx(ctx))
						break;
					val =
					    getreg(ctx,
						   _mips32r2_ext(inst, 16, 5));
					if (((_mips32r2_ext(inst, 21, 5) ==
					      0x08) && ((val & 0x1) == 0))
					    ||
					    ((_mips32r2_ext(inst, 21, 5) ==
					      0x09) && ((val & 0x1) != 0)))
						pc += ureloff16(inst);
					break;
				case 0x0a:	/* BC2EQZC */
				case 0x0b:	/* BC2NEZC */
					assert(0);
					break;
				case 0x19:	/* BPOSGE32 */
					dctx = getdspctx(ctx);
					if (!dctx)
						break;

					if ((dctx->dspc & 0x7f) >= 32)
						pc += ureloff16(inst);
					break;
				}
				break;
		case 0x20:	/* BEQZC/JIC */
				if (_mips32r2_ext(inst, 21, 5) == 0) {
					/* JIC */
					val =
					    getreg(ctx,
						   _mips32r2_ext(inst, 16, 5));
					pc = val + (((inst & 0xffff) ^ 0x8000) -
						    0x8000);
				} else {
					val =
					    getreg(ctx,
						   _mips32r2_ext(inst, 21, 5));
					if (val == 0)
						pc += ureloff21(inst);
				}
				break;
		case 0x28:	/* BNEZC/JIALC */
				if (_mips32r2_ext(inst, 21, 5) == 0) {
					/* JIALC */
					val =
					    getreg(ctx,
						   _mips32r2_ext(inst, 16, 5));
					pc = val + (((inst & 0xffff) ^ 0x8000) -
						    0x8000);
				} else {
					val =
					    getreg(ctx,
						   _mips32r2_ext(inst, 21, 5));
					if (val != 0)
						pc += ureloff21(inst);
				}
				break;
		case 0x30:	/* BLEZALC/BGEZALC/BGEUC */
				if ((((rt != 0) && (rs == 0))
				     && (getreg(ctx, rt) <= 0))
				    || (((rt == rs) && (rt != 0))
					&& (getreg(ctx, rt) >= 0))
				    || (((rs != rt) && (rt != 0) && (rs != 0))
					&& (getreg(ctx, rs) >=
					    getreg(ctx, rt))))
					pc += ureloff16(inst);
				break;
		case 0x38:	/* BGTZALC/BLTZALC/BLTUC */
				if ((((rt != 0) && (rs == 0))
				     && (getreg(ctx, rt) > 0))
				    || (((rt == rs) && (rt != 0))
					&& (getreg(ctx, rt) < 0))
				    || (((rs != rt) && (rt != 0) && (rs != 0))
					&& (getreg(ctx, rs) < getreg(ctx, rt))))
					pc += ureloff16(inst);
				break;
		case 0x1d:	/* BOVC/BEQC/BEQZALC */
				vrs = getreg(ctx, rs);
				vrt = getreg(ctx, rt);

				if (rs >= rt) {
					/* BOVC */
					if (addwilloverflow(vrs, vrt) == 1)
						pc += ureloff16(inst);
				} else if (((rs < rt) && (vrt == vrs))
					   || ((rt != 0) && (rs == 0)
					       && (vrt == 0))) {
					/* BEQC, BEQZALC */
					pc += ureloff16(inst);
				}
				break;
			}
		case 0x25:	/* BC */
		case 0x2d:	/* BALC */
			pc += ureloff26(inst);
			break;
		case 0x35:	/* BGTZC/BLTZC/BLTC */
			{
				if ((((rt != 0) && (rs == 0))
				     && (getreg(ctx, rt) > 0))
				    || (((rt == rs) && (rt != 0))
					&& (getreg(ctx, rt) < 0))
				    || (((rt != rs) && (rt != 0) && (rs != 0))
					&& (getreg(ctx, rs) < getreg(ctx, rt))))
					pc += ureloff16(inst);
				break;
			}
		case 0x3d:	/* BLEZC/BGEZC/BGEC */
			{
				if ((((rt != 0) && (rs == 0))
				     && (getreg(ctx, rt) <= 0))
				    || (((rt == rs) && (rt != 0))
					&& (getreg(ctx, rt) >= 0))
				    || (((rt != rs) && (rt != 0) && (rs != 0))
					&& (getreg(ctx, rs) >=
					    getreg(ctx, rt))))
					pc += ureloff16(inst);
				break;
			}
		case 0x1f:	/* BNVC/BNEC/BNEZALC */
			{
				reg_t vrs = getreg(ctx, rs);
				reg_t vrt = getreg(ctx, rt);

				if (rs >= rt) {
					/* BNVC */
					if (addwilloverflow(vrs, vrt) == 0)
						pc += ureloff16(inst);
					break;
				}

				if (((rs < rt) && (vrt != vrs))
				    || ((rt != 0) && (rs == 0) && (vrt != 0)))
					/* BNEC/BNEZALC */
					pc += ureloff16(inst);
				break;
			}
		}
	} else {
		/* 16-bit instructions.  */
		switch (uop(inst)) {
		case 0x11:	/* POOL16C */
			switch (inst & 0x1f) {
			case 0x03:	/* JRC16 */
			case 0x0b:	/* JALRC16 */
				pc = getreg(ctx, _mips32r2_ext(inst, 5, 5));
				break;
			case 0x13:	/* JRCADDIUSP */
				pc = ctx->ra;
				break;
			}
			break;
		case 0x23:	/* BEQZC16 */
			{
				rs = U2M[_mips32r2_ext(inst, 7, 3)];
				if (getreg(ctx, rs) == 0)
					pc += ureloff7(inst);
				break;
			}
		case 0x2b:	/* BNEZC16 */
			{
				rs = U2M[_mips32r2_ext(inst, 7, 3)];
				if (getreg(ctx, rs) != 0)
					pc += ureloff7(inst);
				break;
			}
		case 0x33:	/* BC16 */
			pc += ureloff10(inst);
			break;
		}
	}

	return pc;
}

/* Compute the PC a microMIPS FPU branch will go to */
uintptr_t inline ubc1pc(struct gpctx * ctx, reg_t inst, reg_t count,
			uintptr_t pc, size_t skip)
{
	struct fpctx *fctx = getfpctx(ctx);
	reg_t fcsr, cond;
	reg_t cnum = _mips32r2_ext(inst, 18, 3) & (count - 1);
	reg_t tf = _mips32r2_ext(inst, 21, 1);
	reg_t mask = (1 << count) - 1;

	if (!fctx)
		return pc;

	fcsr = fctx->fcsr;
	if (!fcsr)
		return pc;

	cond = ((fcsr >> 24) & 0xfe) | ((fcsr >> 23) & 1);
	if (((cond >> cnum) & mask) != (mask * !tf))
		pc += ureloff16(inst);
	else
		pc += skip;

	return pc;
}

/* Compute the PC a microMIPS MSA branch will go to */
static inline uintptr_t ubc1msapc(struct gpctx *ctx, reg_t inst, uintptr_t pc,
				  size_t skip)
{
	reg_t op = _mips32r2_ext(inst, 21, 5);
	reg_t wt = _mips32r2_ext(inst, 16, 5);
	reg_t taken;
	taken = takebc1msa(ctx, op, wt);
	if (taken > 0)
		pc += ureloff16(inst);
	else if (taken == 0)
		pc += skip;
	return pc;
}

/*
 * Compute the next PC for a microMIPS instruction.
 * May not want to be static.
 */
static inline uintptr_t _umips_nextpc(struct gpctx *ctx, uinterch_t one,
				      uinterch_t two)
{
	reg_t inst, pc;
	reg_t rs, val;
	struct dspctx *dctx;
	inst = one.thin.first;
	pc = ctx->epc + 2;
	if (uop(inst) == 0x1f)
		pc += 4;
	else if (((uop(inst) & 4) == 4) || (uop(inst) & 0x7) == 0) {
		inst <<= 16;
		inst = one.thin.second;
		pc += 2;
		switch (uop(inst >> 16)) {
		case 0x00:	/* POOL32A */
			/* POOL32Axf:JALR(S)(.HB) */
			if ((inst & 0xafff) == 0xf3c)
				pc = getreg(ctx, _mips32r2_ext(inst, 16, 5));
			break;
		case 0x10:	/* POOL32I */
			switch (_mips32r2_ext(inst, 21, 5)) {
			case 0x00:	/* BLTZ */
			case 0x01:	/* BLTZAL */
			case 0x11:	/* BLTZALS */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) < 0)
					pc += ureloff16(inst);
				else
					pc += usz(two.thin.first);
				break;
			case 0x02:	/* BGEZ */
			case 0x03:	/* BGEZAL */
			case 0x13:	/* BGEZALS */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) >=
				    0)
					pc += ureloff16(inst);
				else
					pc += usz(two.thin.first);
				break;
			case 0x04:	/* BLEZ */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) <=
				    0)
					pc += ureloff16(inst);
				else
					pc += usz(two.thin.first);
				break;
			case 0x05:	/* BNEZC */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) !=
				    0)
					pc += ureloff16(inst);
				break;
			case 0x06:	/* BGTZ */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) > 0)
					pc += ureloff16(inst);
				else
					pc += usz(two.thin.first);
				break;
			case 0x07:	/* BEQZC */
				if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) ==
				    0)
					pc += ureloff16(inst);
				break;
			case 0x14:	/* BC2F */
			case 0x15:	/* BC2T */
				assert(0);
				break;
			case 0x1a:	/* BPOSGE64 */
			case 0x1b:	/* BPOSGE32 */
				dctx = getdspctx(ctx);
				if (!dctx)
					break;
				val =
				    (_mips32r2_ext(inst, 21, 5) & 1) ? 32 : 64;
				if ((dctx->dspc & 0x7f) >= val)
					pc += ureloff16(inst);
				else
					pc += usz(two.thin.first);
				break;
			case 0x1c:	/* BC1F/BC1ANY2F */
			case 0x1d:	/* BC1T/BC1ANY2T */
				if (((inst >> 16) & 0x2) == 0x0)
					pc = ubc1pc(ctx, inst,
						    ((inst >> 16) & 0x1) + 1,
						    pc, usz(two.thin.first));
				break;
			case 0x1e:	/* BC1ANY4F */
			case 0x1f:	/* BC1ANY4T */
				if (((inst >> 16) & 0x3) == 0x1)
					pc = ubc1pc(ctx, inst, 4, pc,
						    usz(two.thin.first));
				break;
			}
			break;
		case 0x20:	/* POOL32D */
			if ((_mips32r2_ext(inst, 5, 5) & 0x18) == 0x18
			    || (_mips32r2_ext(inst, 5, 5) & 0x1b) == 0x0b)
				/* B(N)(Z).<df|V> */
				pc = ubc1msapc(ctx, inst, pc,
					       usz(two.thin.first));
			break;
		case 0x1d:	/* JALS */
		case 0x35:	/* J */
		case 0x3d:	/* JAL */
			pc = ((pc | 0x7fffffe) ^ 0x7fffffe) |
			    (_mips32r2_ext(inst, 0, 26) << 1);
			break;
		case 0x25:	/* BEQ */
			if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) ==
			    getreg(ctx, _mips32r2_ext(inst, 21, 5)))
				pc += ureloff16(inst);
			else
				pc += usz(two.thin.first);
			break;
		case 0x2d:	/* BNE */
			if (getreg(ctx, _mips32r2_ext(inst, 16, 5)) !=
			    getreg(ctx, _mips32r2_ext(inst, 21, 5)))
				pc += ureloff16(inst);
			else
				pc += usz(two.thin.first);
			break;
		case 0x3c:	/* JALX */
			pc = ((pc | 0xfffffff) ^ 0xfffffff) |
			    (_mips32r2_ext(inst, 0, 26) << 2);
			break;
		}
	} else {
		/* 16 bit */
		switch (uop(inst)) {
		case 0x11:	/* POOL16C */
			if ((_mips32r2_ext(inst, 5, 5) & 0x1c) == 0xc)
				/* JR16, JRC, JALR16, JALRS16 */
				pc = getreg(ctx, (inst & 0x1f));
			else if (_mips32r2_ext(inst, 5, 5) == 0x18)
				/* JRADDIUSP */
				pc = ctx->ra;
			break;
		case 0x23:	/* BEQZ16 */
			rs = U2M[_mips32r2_ext(inst, 7, 3)];
			if (getreg(ctx, rs) == 0)
				pc += ureloff7(inst);
			else
				pc += usz(one.thin.second);
			break;
		case 0x2b:	/* BNEZ16 */
			rs = U2M[_mips32r2_ext(inst, 7, 3)];
			if (getreg(ctx, rs) != 0)
				pc += ureloff7(inst);
			else
				pc += usz(one.thin.second);
			break;
		case 0x33:	/* B16 */
			pc += ureloff10(inst);
			break;
		}
	}

	return pc;
}

/* ---MIPS16--- */

/* Add a 16 bit offset with appropriate fudge factors */
static uintptr_t pcplus16(uintptr_t pc, int offset)
{
	return pc + (offset << 1) + 2;
}

/* Decode an offset from an extended instruction */
static inline uintptr_t xoffset(uintptr_t xi)
{
	uintptr_t r = xi & 0x1f;
	_mips32r2_ins(r, _mips32r2_ext(xi, 16, 5), 11, 5);
	_mips32r2_ins(r, _mips32r2_ext(xi, 21, 6), 5, 6);
	return r;
}

/*
 * Compute the next PC for a MIPS16 instruction.
 * May not want to be static.
 */
static inline uintptr_t _mips16_nextpc(struct gpctx *ctx, uinterch_t one)
{
	uintptr_t pc = ctx->epc, offset = 0;
	reg_t inst = one.thin.first, ext = 0;
	reg_t op = inst >> 11, op2 = 0;
	reg_t rx = 0, val;
	if (op == 30) {
		/* EXTEND */
		ext = inst;
		inst = one.thin.second;
		op = inst >> 11;
	}

	/* Prepare data */
	switch (op) {
	case 2:		/* B */
		if (ext) {
			offset = xoffset((ext << 16) | inst);
			offset = (offset ^ 0x8000) - 0x8000;
		} else {
			offset = inst & 0x7ff;
			offset = (offset ^ 0x400) - 0x400;
		}
		break;
	case 3:		/* JAL(X) */
		offset = ((inst & 0x1f) << 5) | ((inst >> 5) & 0x1f);
		offset = offset << 16;
		offset |= one.thin.second;
		break;
	case 4:		/* BEQZ */
	case 5:		/* BNEZ */
	case 12:		/* I8 */
		if (ext) {
			val = xoffset((ext << 16) | inst);
			val = (val ^ 0x8000) - 0x8000;
		} else {
			val = inst & 0xff;
			val = (val ^ 0x80) - 0x80;
		}
		offset = val;
		rx = (inst >> 8) & 0x07;
		break;
	case 29:		/* RR */
		op2 = inst & 0x1f;
		if (op2 == 0)
			rx = (inst >> 8) & 0x07;
		break;
	}

	/* Perform operation */
	switch (op) {
	case 2:		/* B */
		pc = pcplus16(pc, offset);
		break;
	case 3:		/* JAL(X) */
		pc = ((pc + 2) & (~(uintptr_t) 0x0fffffff)) | (offset << 2);
		if ((inst >> 10) & 0x01)	/* X */
			pc = pc & ~0x01;	/* ->MIPS32 */
		else
			pc |= 0x01;	/* ->MIPS16 */
		break;
	case 4:		/* BEQZ */
		val = getreg(ctx, U2M[rx]);
		if (val == 0)
			pc = pcplus16(pc, offset);
		else
			pc += 2;
		break;
	case 5:		/* BNEZ */
		val = getreg(ctx, U2M[rx]);
		if (val != 0)
			pc = pcplus16(pc, offset);
		else
			pc += 2;
		break;
	case 12:		/* I8 */
		val = ctx->t2[0];
		if (((rx == 0) && (val == 0)) || ((rx == 1) && (val != 0)))
			/* BT(N)EZ */
			pc = pcplus16(pc, offset);
		else
			pc += 2;
		break;
	case 29:		/* RR */
		if (op2 == 0) {
			if ((rx & 1) == 0)
				val = getreg(ctx, U2M[rx]);
			else
				val = 31;
			pc = getreg(ctx, U2M[val]);
		} else
			pc += 2;
		break;
	default:
		pc += 2;
	}

	return pc;
}

/* ---MIPS32--- */

/* Decode MIPS relative offsets */
static inline sreg_t reloff16(reg_t inst)
{
	return (((inst & 0xffff) ^ 0x8000) - 0x8000) << 2;
}

static inline sreg_t reloff21(reg_t inst)
{
	return ((_mips32r2_ext(inst, 0, 21) ^ 0x100000) - 0x100000) << 2;
}

static inline sreg_t reloff26(reg_t inst)
{
	return ((_mips32r2_ext(inst, 0, 26) ^ 0x2000000) - 0x2000000) << 2;
}

/* Compute the PC an FPU branch will go to */
static inline uintptr_t bc1pc(struct gpctx *ctx, reg_t inst, reg_t count,
			      uintptr_t pc)
{
	struct fpctx *fctx = getfpctx(ctx);
	reg_t fcsr, cond;
	reg_t rt = _mips32r2_ext(inst, 16, 5);
	int cnum = (rt >> 2) & (count - 1);
	int tf = rt & 1;
	reg_t mask = (1 << count) - 1;

	if (!fctx)
		return pc;

	fcsr = fctx->fcsr;
	if (!fcsr)
		return pc;

	cond = ((fcsr >> 24) & 0xfe) | ((fcsr >> 23) & 1);
	if (((cond >> cnum) & mask) != (mask * !tf))
		pc += reloff16(inst);
	else
		pc += 4;

	return pc;
}

/* Compute the PC an MSA branch will go to */
static inline uintptr_t bc1msapc(struct gpctx *ctx, reg_t inst, uintptr_t pc)
{
	reg_t op = _mips32r2_ext(inst, 21, 5);
	reg_t wt = _mips32r2_ext(inst, 16, 5);
	reg_t cond;
	cond = takebc1msa(ctx, op, wt);
	if (cond > 0)
		pc += reloff16(inst);
	else if (cond == 0)
		pc += 4;
	return pc;
}

/* Compute the PC an inequality dependant branch will go to */
static inline uintptr_t ineqbpc(struct gpctx *ctx, reg_t inst, uintptr_t pc,
				reg_t invert)
{
	reg_t rs = _mips32r2_ext(inst, 21, 5);
	reg_t rt = _mips32r2_ext(inst, 16, 5);
	sreg_t vrs = getreg(ctx, rs);
	sreg_t vrt = getreg(ctx, rt);
	reg_t uvrs = getureg(ctx, rs);
	reg_t uvrt = getureg(ctx, rt);
	reg_t cond = 0;
	if (rt == 0) {
		/* BLEZ(L), BGTZ(L) */
		cond = (vrs <= 0);
	} else if (isr6()) {
		if (rs == 0 && rt != 0)
			/* BLEZALC, BGTZALC */
			cond = (vrt <= 0);
		else if (rs == rt && rt != 0)
			/* BGEZALC, BLTZALC */
			cond = (vrt >= 0);
		else if (rs != rt && rs != 0 && rt != 0)
			/* BGEUC, BLTUC */
			cond = (uvrs >= uvrt);
	}

	if (invert ^ cond)
		return pc + reloff16(inst);
	else
		return pc + 4;
}

/* Compute the PC a BEQ will go to */
static inline uintptr_t beqpc(reg_t inst, reg_t pc, reg_t rs, reg_t rt,
			      reg_t invert)
{
	if (invert ^ (rs == rt))
		return pc + reloff16(inst) + 4;
	else
		return pc + 8;
}

/*
 * Compute the next PC for a MIPS32 instruction.
 * May not want to be static.
 */
static inline uintptr_t _mips32_nextpc(struct gpctx *ctx, uint32_t inst,
				       uint32_t next)
{
	reg_t val, cond = 0;
	reg_t pc = ctx->epc;
	reg_t op = _mips32r2_ext(inst, 26, 6);
	reg_t rs = _mips32r2_ext(inst, 21, 5);
	reg_t rt = _mips32r2_ext(inst, 16, 5);
	if ((inst & 0xe0000000) != 0) {
		if (((op >> 2) == 5) && (((op & 0x02) == 0) || (rt == 0))) {
			/* BEQL, BNEL, BLEZL, BGTZL */
			switch (op & 0x03) {
			case 0:	/* BEQL */
				pc = beqpc(inst, pc, getreg(ctx, rs),
					   getreg(ctx, rt), 0);
				break;
			case 1:	/* BNEL */
				pc = beqpc(inst, pc, getreg(ctx, rs),
					   getreg(ctx, rt), 1);
				break;
			case 2:	/* BLEZL */
				pc = ineqbpc(ctx, inst, pc + 4, 0);
				break;
			case 3:	/* BGTZL */
				pc = ineqbpc(ctx, inst, pc + 4, 1);
				break;
			default:
				pc += 4;
			}
		} else if ((op == 17) && (rs == 8)) {
			/* BC1F, BC1FL, BC1T, BC1TL */
			pc = bc1pc(ctx, inst, 1, pc + 4);
		} else if (!isr6() && (op == 17) && (rs == 9)
			   && ((rt & 2) == 0)) {
			/* BC1ANY2F, BC1ANY2T */
			pc = bc1pc(ctx, inst, 2, pc + 4);
		} else if (!isr6() && (op == 17) && (rs == 10)
			   && ((rt & 2) == 0)) {
			/* BC1ANY4F, BC1ANY4T */
			pc = bc1pc(ctx, inst, 4, pc + 4);
		} else if ((op == 17)
			   && (((rs & 0x18) == 0x18) || (rs & 0x1b) == 0x0b)) {
			/* B(N)Z.df, B(N)Z.V */
			pc = bc1msapc(ctx, inst, pc + 4);
		} else if (!isr6() && (op == 29)) {
			/* JALX */
			val = _mips32r2_ext(inst, 0, 26) << 2;
			pc = ((pc + 4) & ~(uintptr_t) 0x0fffffff) + val + 1;
		} else if (isbbit(op)) {
			cond = (op == 58) || (op == 62);
			val = rt;
			if ((op == 54) || (op == 62))
				val += 32;
			if (((getreg(ctx, rs) >> val) & 1) == cond)
				pc += reloff16(inst) + 4;
			else
				pc += 8;
		} else if (isr6()) {
			/* BOVC, BEQZALC, BEQC, BNVC, BNEZALC, BNEC */
			if ((op == 8) || (op == 24)) {
				reg_t vrs = getreg(ctx, rs);
				reg_t vrt = getreg(ctx, rt);
				/* BOVC/BNVC */
				if (rs >= rt)
					cond = addwilloverflow(vrs, vrt);
				/* BEQZALC/BNEZALC */
				else if (rs < rt && rs == 0)
					cond = (vrt == 0);
				/* BEQC/BNEC */
				else
					cond = (vrs == vrt);
				if ((op == 24) ^ cond)
					pc += reloff16(inst) + 4;
				else
					pc += 8;
			}

			else if (op == 17 && ((rs == 9) || (rs == 13))) {
				/* BC1EQZ, BC1NEZ */
				uint8_t status, *raw;
				struct fp64ctx *fctx =
				    (struct fp64ctx *)getfpctx(ctx);
				if (fctx) {
					raw = (uint8_t *) & fctx->d[rt];
					status =
					    raw[(BYTE_ORDER ==
						 BIG_ENDIAN) ? 3 : 0];
					if ((status & 0x1) ==
					    ((rs == 13) ? 1 : 0))
						pc += reloff16(inst) + 4;
					else
						pc += 8;
				}
			} else if ((op == 22) || (op == 23)) {
				/* BLEZC, BGEZC, BGEC, BGTZC, BLTZC, BLTC */
				reg_t vrs = getreg(ctx, rs);
				reg_t vrt = getreg(ctx, rt);
				if (rs == 0 && rt != 0)
					cond = (vrt <= 0);
				else if (rs == rt && rt != 0)
					cond = (vrt >= 0);
				else if (rs != rt && rs != 0 && rt != 0)
					cond = (vrs >= vrt);
				if ((op == 23) ^ cond)
					pc += reloff16(inst) + 4;
				else
					pc += 8;
			} else if ((op == 50) || (op == 58)) {
				/* BC, BALC */
				pc += reloff26(inst) + 4;
			} else if (((op == 54) || (op == 62)) && (rs == 0)) {
				/* JIC, JIALC */
				pc = getreg(ctx, rt);
				pc += ((inst & 0xffff) ^ 0x8000) - 0x8000;
			} else if ((op == 54) || (op == 62)) {
				/* BEQZC, BNEZC */
				int cond = (getreg(ctx, rs) == 0);
				if ((op == 62) ^ cond)
					pc += reloff21(inst) + 4;
				else
					pc += 8;
			} else {
				pc += 4;
			}
		} else {
			pc += 4;
		}
	} else {
		switch (op & 0x07) {
		case 0:	/* SPECIAL */
			op = inst & 0x3f;
			switch (op) {
			case 8:	/* JR */
			case 9:	/* JALR */
				pc = getreg(ctx, rs);
				break;
			case 12:	/* SYSCALL */
			default:
				pc += 4;
			}
			break;
		case 1:	/* REGIMM */
			op = rt;
			switch (op) {
			case 0:	/* BLTZ */
			case 2:	/* BLTZL */
			case 16:	/* BLTZAL */
			case 18:	/* BLTZALL */
				if (getreg(ctx, rs) < 0)
					pc += reloff16(inst) + 4;
				else
					pc += 8;
				break;
			case 1:	/* BGEZ */
			case 3:	/* BGEZL */
			case 17:	/* BGEZAL */
			case 19:	/* BGEZALL */
				if (getreg(ctx, rs) >= 0)
					pc += reloff16(inst) + 4;
				else
					pc += 8;
				break;
			case 0x1c:	/* BPOSGE32 */
			case 0x18:	/* BPOSGE32C */
			case 0x1e:	/* BPOSGE64 */
				pc += 4;
				if (rs == 0) {
					struct dspctx *dctx = getdspctx(ctx);

					if (!dctx)
						// barf
						break;
					if (op == 0x18 && !isr6())
						/* BPOSGE32C */
						break;

					val = (op & 2) ? 64 : 32;

					if ((dctx->dspc & 0x7f) >= val)
						pc += reloff16(inst);
					else
						pc += 4;
				}
				break;
			default:
				pc += 4;
			}
			break;
		case 2:	/* J */
		case 3:	/* JAL */
			val = _mips32r2_ext(inst, 0, 26) << 2;
			pc = val + ((pc + 4) & ~(uintptr_t) 0x0fffffff);
			break;
		case 4:	/* BEQ */
			pc = beqpc(inst, pc, getreg(ctx, rs), getreg(ctx, rt),
				   0);
			break;
		case 5:	/* BNE */
			pc = beqpc(inst, pc, getreg(ctx, rs), getreg(ctx, rt),
				   1);
			break;
		case 6:	/* BLEZ, BLEZALC, BGEZALC, BGEUC */
			pc = ineqbpc(ctx, inst, pc + 4, 0);
			break;
		case 7:	/* BGTZ, BGTZALC, BLTZALC, BLTUC */
		default:
			pc = ineqbpc(ctx, inst, pc + 4, 1);
			break;
		}
	}

	return pc;
}

/* ---all--- */

/* Compute the next PC for the specified instruction, using the appropriate emulator */
uintptr_t _mips_nextpc(struct gpctx * ctx, uint32_t one, uint32_t two)
{
	uinterch_t ich1, ich2;
	ich1.fat = one;
	ich2.fat = two;
	if (ctx->epc & 1) {
		if (mips32_getconfig1() & CFG1_CA)
			return _mips16_nextpc(ctx, ich1);
		else if (mips32_getconfig3() & CFG3_ISA_MASK) {
			if (isr6())
				return _umipsr6_nextpc(ctx, ich1);
			else
				return _umips_nextpc(ctx, ich1, ich2);
		}
	}
	return _mips32_nextpc(ctx, one, two);
}
