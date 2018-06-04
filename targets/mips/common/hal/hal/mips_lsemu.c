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

#include <mips/cpu.h>
#include <mips/endian.h>
#include <mips/mips32.h>
#include <mips/hal.h>
#include <stddef.h>
#include <assert.h>

typedef int (_mips_xfermem_fn) (void *address, void *buffer, int size, int n,
				void *priv);
/* This probably wants to be the same implementation, but with a better interface */
#define _XFER_FETCH(SIZE) ((SIZE) | 0x80000000)

#define NO_CP2_CONTEXT_DEFINED 0	/* Trip errors when encountering CP2 instrs */

#define mips32_seb(x) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("seb %0,%1" \
                      : "=r" (__r) \
                      : "r" (x)); \
  __r; \
})

#define mips32_seh(x) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("seh %0,%1" \
                      : "=r" (__r) \
                      : "r" (x)); \
  __r; \
})

#define BIG (BYTE_ORDER == BIG_ENDIAN)

/* Search a chain of contexts for an FPU context */
static inline struct fpctx *getfpctx(struct gpctx *ctx)
    __attribute__ ((always_inline));
static inline struct fpctx *getfpctx(struct gpctx *ctx)
{
	struct fpctx *fctx = (struct fpctx *)ctx->link;
	while (fctx && (fctx->link.id != LINKCTX_TYPE_FP32)
	       && (fctx->link.id != LINKCTX_TYPE_FP64)
	       && (fctx->link.id != LINKCTX_TYPE_FMSA))
		fctx = (struct fpctx *)fctx->link.next;
	return fctx;
}

static inline void _lwl(reg_t * r, uintptr_t va, _mips_xfermem_fn * load,
			void *priv) __attribute__ ((always_inline));
static inline void _lwl(reg_t * r, uintptr_t va, _mips_xfermem_fn * load,
			void *priv)
{
	reg_t w, J, K, L;
	load((void *)(va & ~3), &w, 4, 1, priv);
	J = (w >> 16) & 0xff;
	K = (w >> 8) & 0xff;
	L = w & 0xff;

	switch ((BIG ? va : ~va) & 3) {
	case 0:
		*r = w;
	case 1:
		*r = (J << 24) | (K << 16) | (L << 8) | (*r & 0xff);
	case 2:
		*r = (K << 24) | (L << 16) | (*r & 0xffff);
	case 3:
		*r = (L << 24) | (*r & 0xffffff);
	}

}

static inline void _lwr(reg_t * r, uintptr_t va, _mips_xfermem_fn * load,
			void *priv) __attribute__ ((always_inline));
static inline void _lwr(reg_t * r, uintptr_t va, _mips_xfermem_fn * load,
			void *priv)
{
	reg_t w, I, J, K;
	load((void *)(va & ~3), &w, 4, 1, priv);
	I = (w >> 24) & 0xff;
	J = (w >> 16) & 0xff;
	K = (w >> 8) & 0xff;

	switch ((BIG ? va : ~va) & 3) {
	case 0:
		*r = (*r & 0xffffff00) | I;
	case 1:
		*r = (*r & 0xffff0000) | (I << 8) | J;
	case 2:
		*r = (*r & 0xff000000) | (I << 16) | (J << 8) | K;
	case 3:
		*r = w;
	}

}

static inline void _swl(reg_t r, uintptr_t va, _mips_xfermem_fn * load,
			_mips_xfermem_fn * store, void *priv)
    __attribute__ ((always_inline));
static inline void _swl(reg_t r, uintptr_t va, _mips_xfermem_fn * load,
			_mips_xfermem_fn * store, void *priv)
{
	reg_t w, E, F, G;
	load((void *)(va & ~3), &w, 4, 1, priv);
	E = (r >> 24) & 0xff;
	F = (r >> 16) & 0xff;
	G = (r >> 8) & 0xff;
	switch ((BIG ? va : ~va) & 3) {
	case 0:
		w = r;
	case 1:
		w = (w & 0xff000000) | (E << 16) | (F << 8) | G;
	case 2:
		w = (w & 0xffff0000) | (E << 8) | F;
	case 3:
		w = (w & 0xffffff00) | E;
	}
	store((void *)(va & ~3), &w, 4, 1, priv);
}

static inline void _swr(reg_t r, uintptr_t va, _mips_xfermem_fn * load,
			_mips_xfermem_fn * store, void *priv)
    __attribute__ ((always_inline));
static inline void _swr(reg_t r, uintptr_t va, _mips_xfermem_fn * load,
			_mips_xfermem_fn * store, void *priv)
{
	reg_t w, F, G, H;
	load((void *)(va & ~3), &w, 4, 1, priv);
	F = (r >> 16) & 0xff;
	G = (r >> 8) & 0xff;
	H = r & 0xff;
	switch ((BIG ? va : ~va) & 3) {
	case 0:
		w = (H << 24) | (w & 0x00ffffff);
	case 1:
		w = (G << 24) | (H << 16) | (w & 0x0000ffff);
	case 2:
		w = (F << 24) | (G << 16) | (H << 8) | (w & 0x000000ff);
	case 3:
		w = r;
	}
	store((void *)(va & ~3), &w, 4, 1, priv);
}

static inline void _xb(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
    __attribute__ ((always_inline));
static inline void _xb(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
{
	if (clr)
		*r = 0;
	xfer((void *)va, (void *)((uintptr_t) r + (BIG ? 3 : 0)), 1, 1, priv);
	if (ext)
		*r = mips32_seb(*r);
}

static inline void _xh(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
    __attribute__ ((always_inline));
static inline void _xh(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
{
	if (clr)
		*r = 0;
	xfer((void *)va, (void *)((uintptr_t) r + (BIG ? 2 : 0)), 1, 2, priv);
	if (ext)
		*r = mips32_seh(*r);
}

static inline void _xw(reg_t * r, uintptr_t va, _mips_xfermem_fn * xfer,
		       void *priv) __attribute__ ((always_inline));
static inline void _xw(reg_t * r, uintptr_t va, _mips_xfermem_fn * xfer,
		       void *priv)
{
	if (va & 3)
		xfer((void *)va, r, 1, 4, priv);
	else
		xfer((void *)va, r, 4, 1, priv);
}

static inline void _fh(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
    __attribute__ ((always_inline));
static inline void _fh(reg_t * r, uintptr_t va, uint32_t clr, uint32_t ext,
		       _mips_xfermem_fn * xfer, void *priv)
{
	if (clr)
		*r = 0;
	xfer((void *)va, (void *)((uintptr_t) r + (BIG ? 2 : 0)),
	     _XFER_FETCH(1), 2, priv);
	if (ext)
		*r = mips32_seh(*r);
}

static inline void _fw(reg_t * r, uintptr_t va, _mips_xfermem_fn * xfer,
		       void *priv) __attribute__ ((always_inline));
static inline void _fw(reg_t * r, uintptr_t va, _mips_xfermem_fn * xfer,
		       void *priv)
{
	if (va & 3)
		xfer((void *)va, r, _XFER_FETCH(1), 4, priv);
	else
		xfer((void *)va, r, _XFER_FETCH(4), 1, priv);
}

static inline void _xwc1(void *fctx, uint32_t ft, uintptr_t va, uint32_t clr,
			 _mips_xfermem_fn * xfer, void *priv)
    __attribute__ ((always_inline));
static inline void _xwc1(void *fctx, uint32_t ft, uintptr_t va, uint32_t clr,
			 _mips_xfermem_fn * xfer, void *priv)
{
	if (!(mips_getsr() & SR_FR)) {
		if (va & 3)
			xfer((void *)va,
			     (void *)&FP32CTX_SGL((struct fp32ctx *)fctx, ft),
			     1, 4, priv);
		else
			xfer((void *)va,
			     (void *)&FP32CTX_SGL((struct fp32ctx *)fctx, ft),
			     4, 1, priv);
	} else {
		if (clr)
			FP64CTX_DBL((struct fp64ctx *)fctx, ft) = 0.0;
		if (va & 3)
			xfer((void *)va,
			     (void *)&FP64CTX_SGL((struct fp64ctx *)fctx, ft),
			     1, 4, priv);
		else
			xfer((void *)va,
			     (void *)&FP64CTX_SGL((struct fp64ctx *)fctx, ft),
			     4, 1, priv);
	}
}

static inline void _xdc1(void *fctx, uint32_t ft, uintptr_t va,
			 _mips_xfermem_fn * xfer, void *priv)
    __attribute__ ((always_inline));
static inline void _xdc1(void *fctx, uint32_t ft, uintptr_t va,
			 _mips_xfermem_fn * xfer, void *priv)
{
	if (!(mips_getsr() & SR_FR)) {
		if (va & 7)
			xfer((void *)va,
			     (void *)&FP32CTX_DBL((struct fp32ctx *)fctx, ft),
			     1, 8, priv);
		else
			xfer((void *)va,
			     (void *)&FP32CTX_DBL((struct fp32ctx *)fctx, ft),
			     8, 1, priv);
	} else {
		if (va & 7)
			xfer((void *)va,
			     (void *)&FP64CTX_DBL((struct fp64ctx *)fctx, ft),
			     1, 8, priv);
		else
			xfer((void *)va,
			     (void *)&FP64CTX_DBL((struct fp64ctx *)fctx, ft),
			     8, 1, priv);
	}
}

/* Return microMIPS op code */
#define uop(i) ((i) >> 10)
static const signed char U2M[8] = { 16, 17, 2, 3, 4, 5, 6, 7 };

#define uoff(x) (((x) == 0xf) ? -1 : (x))

/* Decode microMIPS relative offsets */
#define ureloff19(inst) ((_mips32r2_ext((inst), 0, 19) ^ 0x40000) - 0x40000)
#define ureloff16(inst) mips32_seh((inst) & 0xffff)
#define ureloff12(inst) ((((inst) & 0xfff) ^ 0x800) - 0x800)
#define ureloff11(inst) ((((inst) & 0x7ff) ^ 0x400) - 0x400)
#define ureloff9(inst) ((((inst) & 0x1ff) ^ 0x100) - 0x100)
#define ureloff7(inst) (((((inst) & 0x7f) ^ 0x40) - 0x40) << 2)
/* Decode MIPS relative offsets */
#define reloff19(inst) (((_mips32r2_ext((inst), 0, 19) ^ 0x40000) - 0x40000) << 2)
#define reloff16(inst) ((((inst) & 0xffff) ^ 0x8000) - 0x8000)
#define reloff9(inst) ((_mips32r2_ext((inst), 7, 9) ^ 0x100) - 0x100)

#define REGDEC(C, I) ((I) == 0 ? 0 : ((C)->r[(I) - 1]))
#define PREGDEC(C, I) ((I) == 0 ? (&zero) : (&(C)->r[(I) - 1]))

void mips_emulate_ls(struct gpctx *c, void *addr, _mips_xfermem_fn read,
		     _mips_xfermem_fn write, void *rwpriv)
{
	reg_t zero = 0;
	uintptr_t va;
	reg_t base, rd, rs, rt, index;
	sreg_t offset;
	reg_t inst;
	va = (uintptr_t) addr;
	if (va & 1) {		/* microMIPS */
		/* Read inst */
		va = va & ~1;
		_fh(&inst, va, 1, 0, read, rwpriv);
		if (((uop(inst) & 0x4) == 0x4) || ((uop(inst) & 0x7) == 0x0)) {
			inst <<= 16;
			_fh(&inst, va + 2, 0, 0, read, rwpriv);
		}
		/* Decode */
		switch (_mips32r2_ext(inst, 26, 6)) {
		case 0x00:	/* 16 bit */
			switch (_mips32r2_ext(inst, 11, 4)) {
			case 0x02:	/* lbu16: 000010tttbbboooo */
				offset = uoff(inst & 0xf);
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);
				break;
			case 0x0a:	/* lhu16: 001010tttbbboooo */
				offset = uoff(inst & 0xf);
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);

				break;
			case 0x11:
				switch (inst & 0xf) {
				case 0x2:	/* lwm16: 010001rroooo0010 */
					offset = _mips32r2_ext(inst, 4, 4) << 2;
					va = c->sp + offset;
					rd = _mips32r2_ext(inst, 8, 2);
					switch (rd) {
					case 3:	/* r16, r17, r18, r19, r31 */
						_xw(&c->r[18], va + 12, read,
						    rwpriv);
						break;
					case 2:	/* r16, r17, r18, r31 */
						_xw(&c->r[17], va + 8, read,
						    rwpriv);
						break;
					case 1:	/* r16, r17, r31 */
						_xw(&c->r[16], va + 4, read,
						    rwpriv);
						break;
					case 0:	/* r16, r31 */
						_xw(&c->r[15], va, read,
						    rwpriv);
						break;
					default:
						break;
					}
					_xw(&c->r[30], va + 4 + (rd * 4), read,
					    rwpriv);
					break;
				case 0xa:	/* swm16: 010001rroooo1010 */
					offset = _mips32r2_ext(inst, 4, 4) << 2;
					va = c->sp + offset;
					rd = _mips32r2_ext(inst, 8, 2);
					switch (rd) {

					case 3:	/* r16, r17, r18, r19, r31 */
						_xw(&c->r[18], va + 12, write,
						    rwpriv);
						break;
					case 2:	/* r16, r17, r18, r31 */
						_xw(&c->r[17], va + 8, write,
						    rwpriv);
						break;
					case 1:	/* r16, r17, r31 */
						_xw(&c->r[16], va + 4, write,
						    rwpriv);
						break;
					case 0:	/* r16, r31 */
						_xw(&c->r[15], va, write,
						    rwpriv);
						break;
					default:
						break;
					}
					_xw(&c->r[30], va + 4 + (rd * 4), write,
					    rwpriv);
					break;
				default:
					return;
				}
				break;
			case 0x12:	/* lwsp: 010010tttttooooo */
				rt = _mips32r2_ext(inst, 7, 3);
				offset = _mips32r2_ext(inst, 0, 5) << 2;
				va = c->sp + offset;
				_xw(PREGDEC(c, rt), va, read, rwpriv);
				break;
			case 0x19:	/* lwgp: 011001tttooooooo */
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				offset = ureloff7(inst);
				va = c->sp + offset;
				_xw(PREGDEC(c, rt), va, read, rwpriv);
				break;
			case 0x1a:	/* lw16: 011010tttbbboooo */
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				offset = _mips32r2_ext(inst, 0, 4) << 2;
				va = REGDEC(c, base) + offset;
				_xw(PREGDEC(c, rt), va, read, rwpriv);
				break;
			case 0x22:	/* sb16: 100010tttbbboooo */
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				offset = _mips32r2_ext(inst, 0, 4) << 2;
				va = REGDEC(c, base) + offset;
				_xb(PREGDEC(c, rt), va, 0, 0, write, rwpriv);
				break;
			case 0x2a:	/* sh16: 101010tttbbboooo */
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				offset = _mips32r2_ext(inst, 0, 4) << 2;
				va = REGDEC(c, base) + offset;
				_xh(PREGDEC(c, rt), va, 0, 0, write, rwpriv);
				break;
			case 0x32:	/* swsp: 110010tttttooooo */
				rt = _mips32r2_ext(inst, 7, 3);
				offset = _mips32r2_ext(inst, 0, 5) << 2;
				va = c->sp + offset;
				_xw(PREGDEC(c, rt), va, write, rwpriv);
				break;
			case 0x3a:	/* sw16: 111010tttbbboooo */
				rt = U2M[_mips32r2_ext(inst, 7, 3)];
				base = U2M[_mips32r2_ext(inst, 4, 3)];
				offset = _mips32r2_ext(inst, 0, 4) << 2;
				va = REGDEC(c, base) + offset;
				_xw(PREGDEC(c, rt), va, write, rwpriv);
				break;
			default:
				return;
			}
			break;
		case 0x05:	/* lbu: 000101tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1, 0,
			    read, rwpriv);
			break;
		case 0x07:	/* lb: 000111tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1, 1,
			    read, rwpriv);
			break;
		case 0x08:
			rd = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			switch (_mips32r2_ext(inst, 11, 4)) {
			case 0x0:	/* lwc2: 001000tttttbbbbb00000ooooooooooo */
				if (!(inst & 0x800))
					return;
				offset = ureloff11(inst);
				if (offset & 0x400)
					offset |= ~0x7ff;
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x1:	/* lwp: 001000dddddbbbbb0001oooooooooooo */
				offset = ureloff12(inst);
				va = REGDEC(c, base) + offset;
				_xw(PREGDEC(c, rd), va, read, rwpriv);
				_xw(&c->r[rd], va + 4, read, rwpriv);
				break;
			case 0x2:	/* ldc2: 001000tttttbbbbb00100ooooooooooo */
				if (!(inst & 0x800))
					return;
				offset = ureloff11(inst);
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x5:	/* lwm32: 001000rrrrrbbbbb0101oooooooooooo */
				offset = ureloff12(inst);
				va = REGDEC(c, base) + offset;
				switch (rd & 0xf) {
				case 9:
					_xw(&c->r[30], va + 32, read, rwpriv);
				case 8:
					_xw(&c->r[23], va + 28, read, rwpriv);
				case 7:
					_xw(&c->r[22], va + 24, read, rwpriv);
				case 6:
					_xw(&c->r[21], va + 20, read, rwpriv);
				case 5:
					_xw(&c->r[20], va + 16, read, rwpriv);
				case 4:
					_xw(&c->r[19], va + 12, read, rwpriv);
				case 3:
					_xw(&c->r[18], va + 8, read, rwpriv);
				case 2:
					_xw(&c->r[17], va + 4, read, rwpriv);
				case 1:
					_xw(&c->r[16], va, read, rwpriv);
				case 0:
					if (rd & 0x10)
						break;
				default:
					return;
				}
				if (rd & 0x10)
					_xw(&c->r[31], va + ((rd & 0xf) * 4),
					    read, rwpriv);
				break;
			case 0x8:	/* swc2: 001000tttttbbbbb10000ooooooooooo */
				if (!(inst & 0x800))
					return;
				offset = ureloff11(inst);
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x9:	/* swp: 001000rrrrrbbbbb1001oooooooooooo */
				if ((inst & 0xf000) != 0x9000)
					return;
				offset = ureloff12(inst);
				va = REGDEC(c, base) + offset;
				_xw(PREGDEC(c, rd), va, read, rwpriv);
				_xw(&c->r[rd], va + 4, read, rwpriv);
				break;
			case 0xd:	/* swm32: 001000rrrrrbbbbb1101oooooooooooo */
				offset = ureloff12(inst);
				va = REGDEC(c, base) + offset;
				switch (rd & 0xf) {
				case 9:
					_xw(&c->r[30], va + 32, write, rwpriv);
				case 8:
					_xw(&c->r[23], va + 28, write, rwpriv);
				case 7:
					_xw(&c->r[22], va + 24, write, rwpriv);
				case 6:
					_xw(&c->r[21], va + 20, write, rwpriv);
				case 5:
					_xw(&c->r[20], va + 16, write, rwpriv);
				case 4:
					_xw(&c->r[19], va + 12, write, rwpriv);
				case 3:
					_xw(&c->r[18], va + 8, write, rwpriv);
				case 2:
					_xw(&c->r[17], va + 4, write, rwpriv);
				case 1:
					_xw(&c->r[16], va, write, rwpriv);
				case 0:
					if (rd & 0x10)
						break;
				default:
					return;
				}
				if (rd & 0x10)
					_xw(&c->r[31], va + ((rd & 0xf) * 4),
					    write, rwpriv);
				break;
			default:
				return;
			}
		case 0x0d:	/* lhu: 001101tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1, 0,
			    read, rwpriv);
			break;
		case 0x0e:	/* sh: 001110tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 0, 0,
			    write, rwpriv);
			break;
		case 0x0f:	/* lh: 001111tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1, 1,
			    read, rwpriv);
			break;
		case 0x18:
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff9(inst);
			switch (_mips32r2_ext(inst, 9, 7)) {
			case 0x08:	/* llx: 011000tttttbbbbb0001000ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x18:	/* ll: 011000tttttbbbbb0011000ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x30:	/* lbue: 011000tttttbbbbb0110000ooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);
				break;
			case 0x31:	/* lhue: 011000tttttbbbbb0110001ooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);
				break;
			case 0x32:	/* llxe: 011000tttttbbbbb0110010ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x34:	/* lbe: 011000tttttbbbbb0110100ooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    1, read, rwpriv);
				break;
			case 0x35:	/* lhe: 011000tttttbbbbb0110101ooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    1, read, rwpriv);
				break;
			case 0x36:	/* lle: 011000tttttbbbbb0110110ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x37:	/* lwe: 011000tttttbbbbb0110111ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x48:	/* scx: 011000tttttbbbbb1001000ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x50:	/* scxe: 011000tttttbbbbb1010000ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x53:	/* sce: 011000tttttbbbbb1010110ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x54:	/* sbe: 011000tttttbbbbb1010100ooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 0,
				    0, write, rwpriv);
				break;
			case 0x55:	/* she: 011000tttttbbbbb1010101ooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 0,
				    1, write, rwpriv);
				break;
			case 0x56:	/* sce: 011000tttttbbbbb1010110ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x57:	/* swe: 011000tttttbbbbb1010111ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x58:	/* sc: 011000tttttbbbbb1011000ooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			default:
				return;
			}
		case 0x1e:	/* lwpc: 011110ttttt01ooooooooooooooooooo */
			if (_mips32r2_ext(inst, 19, 2) != 1)
				return;
			rt = _mips32r2_ext(inst, 21, 5);
			offset = ureloff19(inst);
			_xw(PREGDEC(c, rt), (va & ~3) + offset, read, rwpriv);
			break;
		case 0x26:	/* swc1: 100110tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xwc1(getfpctx(c), rt, REGDEC(c, base) + offset, 0,
			      write, rwpriv);
			break;
		case 0x27:	/* lwc1: 100111tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xwc1(getfpctx(c), rt, REGDEC(c, base) + offset, 1,
			      read, rwpriv);
			break;
		case 0x28:	/* sb: 101000tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 0, 0,
			    write, rwpriv);
			break;
		case 0x2e:	/* sdc1: 101110tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xdc1(getfpctx(c), rt, REGDEC(c, base) + offset, write,
			      rwpriv);
			break;
		case 0x2f:	/* ldc1: 101111tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xdc1(getfpctx(c), rt, REGDEC(c, base) + offset, read,
			      rwpriv);
			break;
		case 0x36:	/* sdc2: 110110tttttbbbbb10100ooooooooooo */
			assert(NO_CP2_CONTEXT_DEFINED);
		case 0x3e:	/* sw: 111110tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xw(PREGDEC(c, rt), REGDEC(c, base) + offset, write,
			    rwpriv);
			break;
		case 0x3f:	/* lw: 111111tttttbbbbboooooooooooooooo */
			rt = _mips32r2_ext(inst, 21, 5);
			base = _mips32r2_ext(inst, 16, 5);
			offset = ureloff16(inst);
			_xw(PREGDEC(c, rt), REGDEC(c, base) + offset, read,
			    rwpriv);
			break;
		default:
			return;
		}
	} else {		/* MIPS32 */
		/* Read inst */
		_fw(&inst, va, read, rwpriv);
		/* Decode */
		switch (_mips32r2_ext(inst, 26, 6)) {
		case 0x1f:	/*  0b011111: special */
			base = _mips32r2_ext(inst, 21, 5);
			rt = _mips32r2_ext(inst, 16, 5);
			offset = reloff9(inst);
			switch (inst & 0x7f) {
			case 0x19:	/* lwle:011111bbbbbtttttooooooooo0011001 */
				_lwl(&rt, REGDEC(c, base) + offset, read,
				     rwpriv);
				break;
			case 0x1a:	/* lwre:011111bbbbbtttttooooooooo0011010 */
				_lwr(&rt, REGDEC(c, base) + offset, read,
				     rwpriv);
				break;
			case 0x28:	/* lbue: 011111bbbbbtttttooooooooo0101000 */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);
				break;
			case 0x29:	/* lhue: 011111bbbbbtttttooooooooo0101001 */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    0, read, rwpriv);
				break;
			case 0x2c:	/* lbe: 011111bbbbbtttttooooooooo0101100 */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    1, read, rwpriv);
				break;
			case 0x2d:	/* lhe: 011111bbbbbtttttooooooooo0101101 */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 1,
				    1, read, rwpriv);
				break;
			case 0x2e:	/* lle: 011111bbbbbtttttooooooooo0101110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x2f:	/* lwe: 011111bbbbbtttttooooooooo0101111 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x36:	/* ll(6): 011111bbbbbtttttooooooooo0110110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x3e:	/* llxe: 011111bbbbbtttttooooooooo1101110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x76:	/* llx: 011111bbbbbtttttooooooooo1110110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x26:	/* sc(6): 011111bbbbbtttttooooooooo0100110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x66:	/* scx: 011111bbbbbtttttooooooooo1100110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x1c:	/* sbe: 011111bbbbbtttttooooooooo0011100 */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset, 0,
				    0, write, rwpriv);
				break;
			case 0x1e:	/* sce: 011111bbbbbtttttooooooooo0011110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x5e:	/* scxe: 011111bbbbbtttttooooooooo1011110 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x1d:	/* she: 011111bbbbbtttttooooooooo0011101 */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset, 0,
				    1, write, rwpriv);
				break;
			case 0x1f:	/* swe: 011111bbbbbtttttooooooooo0011111 */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x21:	/* swle: 011111bbbbbtttttooooooooo0100001 */
				_swl(rt, base + offset, read, write, rwpriv);
				break;
			case 0x22:	/* swre: 011111bbbbbtttttooooooooo0100010} */
				_swr(rt, base + offset, read, write, rwpriv);
				break;
			default:
				return;
			}
		case 0x13:	/* cop1x */
			if (inst & 0xf800)
				break;
			base = _mips32r2_ext(inst, 21, 5);
			index = _mips32r2_ext(inst, 16, 5);
			rs = _mips32r2_ext(inst, 11, 5);
			rd = _mips32r2_ext(inst, 6, 5);
			switch (inst & 0x3f) {
			case 0x01:	/* ldxc1: 010011bbbbbiiiii00000ddddd000001 */
				_xdc1(getfpctx(c), rd,
				      REGDEC(c, base) + REGDEC(c, index), read,
				      rwpriv);
				break;
			case 0x03:	/* luxc1: 010011bbbbbiiiii00000ddddd000101 */
				/* fallthrough */
			case 0x00:	/* lwxc1: 010011bbbbbiiiii00000ddddd000000 */
				_xwc1(getfpctx(c), rd,
				      REGDEC(c, base) + REGDEC(c, index), 1,
				      read, rwpriv);
				break;
			case 0x09:	/*  sdxc1: 010011 bbbbbiiiiisssss00000001001 */
				_xdc1(getfpctx(c), rs,
				      REGDEC(c, base) + REGDEC(c, index), write,
				      rwpriv);
				break;
			case 0x0e:	/* suxc1: 010011bbbbbiiiiisssss00000001101 */
				/* fall through */
			case 0x08:	/* swxc1: 010011bbbbbiiiiisssss00000001000 */
				_xwc1(getfpctx(c), rs,
				      REGDEC(c, base) + REGDEC(c, index), 0,
				      write, rwpriv);
				break;
			default:
				return;
			}
			break;
		case 0x12:	/* cop2 */
			switch (_mips32r2_ext(inst, 21, 5)) {
			case 0x0e:	/* ldc2(6): 01001001110tttttbbbbbooooooooooo */
			case 0x0a:	/* lwc2(6): 01001001010tttttbbbbbooooooooooo */
			case 0x0f:	/* sdc2(6): 01001001111tttttbbbbbooooooooooo */
			case 0x0d:	/* swc2(6): 01001001011tttttbbbbbooooooooooo */
				assert(NO_CP2_CONTEXT_DEFINED);
			default:
				return;
			}
			break;
		case 0x3b:	/* pcrel */
			if (_mips32r2_ext(inst, 19, 2) == 1) {	/* lwpc: 111011sssss01ooooooooooooooooooo */
				rs = _mips32r2_ext(inst, 21, 5);
				offset = reloff19(inst);
				_xw(PREGDEC(c, rs), (va & ~3) + offset, read,
				    rwpriv);
			} else
				return;
			break;
		default:
			base = _mips32r2_ext(inst, 21, 5);
			rt = _mips32r2_ext(inst, 16, 5);
			offset = reloff16(inst);
			switch (_mips32r2_ext(inst, 26, 6)) {
			case 0x20:	/* lb: 100000bbbbbtttttoooooooooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    1, 1, read, rwpriv);
				break;
			case 0x21:	/* lh: 100001bbbbbtttttoooooooooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    1, 1, read, rwpriv);
				break;
			case 0x23:	/* lw: 100011bbbbbtttttoooooooooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x28:	/* sb: 101000bbbbbtttttoooooooooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    0, 0, write, rwpriv);
				break;
			case 0x29:	/* sh: 101001bbbbbtttttoooooooooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    0, 0, write, rwpriv);
				break;
			case 0x2b:	/* sw: 101011bbbbbtttttoooooooooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x24:	/* lbu: 100100bbbbbtttttoooooooooooooooo */
				_xb(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    1, 0, read, rwpriv);
				break;
			case 0x25:	/* lhu: 100101bbbbbtttttoooooooooooooooo */
				_xh(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    1, 0, read, rwpriv);
				break;
			case 0x22:	/* lwl: 100010bbbbbtttttoooooooooooooooo */
				_lwl(&rt, REGDEC(c, base) + offset, read,
				     rwpriv);
				break;
			case 0x26:	/* lwr: 100110bbbbbtttttoooooooooooooooo */
				_lwr(&rt, REGDEC(c, base) + offset, read,
				     rwpriv);
				break;
			case 0x2a:	/* swl: 101010bbbbbtttttoooooooooooooooo */
				_swl(rt, REGDEC(c, base) + offset, read, write,
				     rwpriv);
				break;
			case 0x2e:	/* swr: 101110bbbbbtttttoooooooooooooooo */
				_swr(rt, REGDEC(c, base) + offset, read, write,
				     rwpriv);
				break;
			case 0x30:	/* ll(5): 110000bbbbbtttttoooooooooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    read, rwpriv);
				break;
			case 0x31:	/* lwc1: 110001bbbbbtttttoooooooooooooooo */
				_xwc1(getfpctx(c), rt, REGDEC(c, base) + offset,
				      1, read, rwpriv);
				break;
			case 0x35:	/* ldc1: 110101bbbbbtttttoooooooooooooooo */
				_xdc1(getfpctx(c), rt, REGDEC(c, base) + offset,
				      read, rwpriv);
				break;
			case 0x36:	/* ldc2(5): 110110bbbbbtttttoooooooooooooooo */
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x32:	/* lwc2(5): 110010bbbbbtttttoooooooooooooooo */
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x38:	/* sc(5): 111000bbbbbtttttoooooooooooooooo */
				_xw(PREGDEC(c, rt), REGDEC(c, base) + offset,
				    write, rwpriv);
				break;
			case 0x39:	/* swc1: 111001bbbbbtttttoooooooooooooooo */
				_xwc1(getfpctx(c), rt, REGDEC(c, base) + offset,
				      0, write, rwpriv);
				break;
			case 0x3a:	/* swc2(5): 111010bbbbbtttttoooooooooooooooo */
				assert(NO_CP2_CONTEXT_DEFINED);
				break;
			case 0x3e:	/* sdc1: 111101bbbbbtttttoooooooooooooooo */
				_xdc1(getfpctx(c), rt, REGDEC(c, base) + offset,
				      write, rwpriv);
				break;
			case 0x3f:	/* sdc2(5): 111110bbbbbtttttoooooooooooooooo */
				assert(NO_CP2_CONTEXT_DEFINED);
			default:
				return;
			}
		}

	}
}
