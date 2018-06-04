/*
 * Copyright 2015MIPS Tech, LLC and/or its
 *		   affiliated group companies.
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

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>
#include <mips/hal.h>
#include <mips/endian.h>

.set virt

//#ifdef __mips_micromips
#define NOHI
//#endif

#if (_MIPS_SIM==_ABIO32) || (_MIPS_SIM==_ABIN32)

/* 32 bit */
#define PTR_MFGC0	mfgc0	/* access CP0 pointer width register */
#define PTR_MTGC0	mtgc0	/* access CP0 pointer width register */
#ifdef NOHI
#define XPTR_MFGC0(R1, R2, S) mfgc0 R1, S
#define XPTR_MTGC0(R1, R2, D) mtgc0 R1, D
#else
#define XPTR_MFGC0(R1, R2, S) \
	mfgc0		R1, S; \
	mfhgc0		R2, S
#define XPTR_MTGC0(R1, R2, D) \
	mthgc0		R2, D; \
	ehb; \
	mtgc0		R1, D
#endif
#if (BYTE_ORDER==LITTLE_ENDIAN) && defined(NOHI)
#define XPTR_S(R1, R2, O, D) \
	sw		R1, (O)(D); \
	sw		zero, ((O)+4)(D)
#define XPTR_L(R1, R2, O, S) \
	lw		R1, (O)(S); \
	lw		zero, ((O)+4)(S)
#elif (BYTE_ORDER==LITTLE_ENDIAN) && !defined(NOHI)
#define XPTR_S(R1, R2, O, D) \
	sw		R1, (O)(D); \
	sw		R2, ((O)+4)(D)
#define XPTR_L(R1, R2, O, S) \
	lw		R1, (O)(S); \
	lw		R2, ((O)+4)(S)
#elif  (BYTE_ORDER==BIG_ENDIAN) && defined(NOHI)
#define XPTR_S(R1, R2, O, D) sw  R1, ((O)+4)(D)
#define XPTR_L(R1, R2, O, S) lw  R1, ((O)+4)(S)
#elif  (BYTE_ORDER==BIG_ENDIAN) && !defined(NOHI)
#define XPTR_S(R1, R2, O, D) \
	sw		R2, (O)(D); \
	sw		R1, ((O)+4)(D)
#define XPTR_L(R1, R2, O, S) \
	lw		R2, (O)(S); \
	lw		R1, ((O)+4)(S)
#endif

#elif (_MIPS_SIM==_ABIO64) || (_MIPS_SIM==_ABI64)

/* 64 bit */
#define PTR_MFGC0	dmfgc0	/* access CP0 pointer width register */
#define PTR_MTGC0	dmtgc0	/* access CP0 pointer width register */
#define XPTR_MFGC0(R1, R2, S) dmfgc0 R1, S
#define XPTR_MTGC0(R1, R2, D) dmfgc0 R1, D
#define XPTR_S(R1, R2, O, D) sd R1, (O)(D)
#define XPTR_L(R1, R2, O, D) ld R1, (O)(D)

#endif

#
# FUNCTION:	_vzrootctx_save
#
# DESCRIPTION:	save VZ root registers to memory starting at a0
#
# RETURNS:	int
#			0:	No context saved
#			CTX_*:	Type of context stored
#
LEAF(_vzrootctx_save)
	# Check we have VZ, bail if not
	mfc0		v0, C0_CONFIG3
	ext		v0, v0, CFG3_VZ_SHIFT, 1
	beqz		v0, 1f

	# Save root VZ context
	mfc0		v0, C0_GUESTCTL0
	sw		v0, VZROOTCTX_GUESTCTL0(a0)
	mfc0		v0, C0_GUESTCTL1
	sw		v0, VZROOTCTX_GUESTCTL1(a0)
	mfc0		v0, C0_GUESTCTL2
	sw		v0, VZROOTCTX_GUESTCTL2(a0)
	mfc0		v0, C0_GUESTCTL3
	sw		v0, VZROOTCTX_GUESTCTL3(a0)
	mfc0		v0, C0_GUESTCTL0EXT
	sw		v0, VZROOTCTX_GUESTCTL0EXT(a0)
	mfc0		v0, C0_GTOFFSET
	sw		v0, VZROOTCTX_GTOFFSET(a0)

	# Set type
	li		v0, LINKCTX_TYPE_VZROOT

	# Save link and zero next pointer
1:	PTR_S		v0, LINKCTX_ID(a0)
	PTR_S		zero, LINKCTX_NEXT(a0)
	jr		ra
END(_vzrootctx_save)

#
# FUNCTION:	_vzrootctx_load
#
# DESCRIPTION:	load VZ root registers from memory starting at a0
#
# RETURNS:	int
#		0:	Unrecognised context
#		CTX_*:	Type of context restored
#
LEAF(_vzrootctx_load)
	# Check we have VZ, bail if not
	mfc0		v0, C0_CONFIG3
	ext		v0, v0, CFG3_VZ_SHIFT, 1
	beqz		v0, 1f

	# Check link, bail if wrong type
	PTR_L		t0, LINKCTX_ID(a0)
	li		t1, LINKCTX_TYPE_VZROOT
	move		v0, zero
	bne		t0, t1, 1f

	# Load root VZ context
	lw		v0, VZROOTCTX_GUESTCTL0(a0)
	mtc0		v0, C0_GUESTCTL0
	ehb
	lw		v0, VZROOTCTX_GUESTCTL1(a0)
	mtc0		v0, C0_GUESTCTL1
	ehb
	lw		v0, VZROOTCTX_GUESTCTL2(a0)
	mtc0		v0, C0_GUESTCTL2
	ehb
	lw		v0, VZROOTCTX_GUESTCTL3(a0)
	mtc0		v0, C0_GUESTCTL3
	ehb
	lw		v0, VZROOTCTX_GUESTCTL0EXT(a0)
	mtc0		v0, C0_GUESTCTL0EXT
	ehb
	lw		v0, VZROOTCTX_GTOFFSET(a0)
	mtc0		v0, C0_GTOFFSET
	ehb

	li		v0, LINKCTX_TYPE_VZROOT
1:	jr		ra
END(_vzrootctx_load)

#
# FUNCTION:	_vzguestctx_save
#
# DESCRIPTION:	save VZ guest registers to memory starting at a0
#
# RETURNS:	int
#			0:	No context saved
#			CTX_*:	Type of context stored
#
LEAF(_vzguestctx_save)
	# Check we have VZ, bail if not
	mfc0		a1, C0_CONFIG3
	ext		v0, a1, CFG3_VZ_SHIFT, 1
	beqz		v0, 9f

	# Save guest context
	mfgc0		t0, C0_INDEX
	sw		t0, VZGUESTCTX_INDEX(a0)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_S		(t0, t1, VZGUESTCTX_ENTRYLO0, a0)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO1)
	XPTR_S		(t0, t1, VZGUESTCTX_ENTRYLO1, a0)
	PTR_MFGC0	t0, C0_CONTEXT
	PTR_S		t0, VZGUESTCTX_CONTEXT(a0)
	mfgc0		t0, C0_CONTEXTCONF
	sw		t0, VZGUESTCTX_CONTEXTCONFIG(a0)
	PTR_MFGC0	t0, C0_PAGEMASK
	PTR_S		t0, VZGUESTCTX_PAGEMASK(a0)
	mfgc0		t0, C0_PAGEGRAIN
	sw		t0, VZGUESTCTX_PAGEGRAIN(a0)
	mfgc0		t0, C0_WIRED
	sw		t0, VZGUESTCTX_WIRED(a0)
	mfgc0		t0, C0_HWRENA
	sw		t0, VZGUESTCTX_HWRENA(a0)
	PTR_MFGC0	t0, C0_BADVADDR
	PTR_S		t0, VZGUESTCTX_BADVADDR(a0)
	PTR_MFGC0	t0, C0_ENTRYHI
	PTR_S		t0, VZGUESTCTX_ENTRYHI(a0)
	mfgc0		t0, C0_COUNT
	sw		t0, VZGUESTCTX_COUNT(a0)
	mfgc0		t0, C0_COMPARE
	sw		t0, VZGUESTCTX_COMPARE(a0)
	mfgc0		t0, C0_STATUS
	sw		t0, VZGUESTCTX_STATUS(a0)
	mfgc0		t0, C0_INTCTL
	sw		t0, VZGUESTCTX_INTCTL(a0)
	mfgc0		t0, C0_SRSCTL
	sw		t0, VZGUESTCTX_SRSCTL(a0)
	mfgc0		t0, C0_SRSMAP
	sw		t0, VZGUESTCTX_SRSMAP(a0)
	mfgc0		t0, C0_CAUSE
	sw		t0, VZGUESTCTX_CAUSE(a0)
	PTR_MFGC0	t0, C0_EPC
	PTR_S		t0, VZGUESTCTX_EPC(a0)
	PTR_MFGC0	t0, C0_EBASE
	PTR_S		t0, VZGUESTCTX_EBASE(a0)
	PTR_MFGC0	t0, C0_USERLOCAL
	PTR_S		t0, VZGUESTCTX_USERLOCAL(a0)
	PTR_MFGC0	t0, C0_ERRPC
	PTR_S		t0, VZGUESTCTX_ERROREPC(a0)

	mfgc0		a1, C0_CONFIG3
	ext		v0, a1, CFG3_BI_SHIFT, 1
	beqz		v0, 1f
	mfgc0		t0, C0_BADINSTR
	sw		t0, VZGUESTCTX_BADINSTR(a0)
1:	ext		v0, a1, CFG3_BP_SHIFT, 1
	beqz		v0, 1f
	mfgc0		t0, C0_BADPINSTR
	sw		t0, VZGUESTCTX_BADINSTRP(a0)

1:	mfgc0		a1, C0_CONFIG5
	ext		v0, a1, CFG5_NF_SHIFT, 1
	beqz		v0, 1f
	mfgc0		t0, C0_NESTEDEXC
	sw		t0, VZGUESTCTX_NESTEDEXC(a0)
	mfgc0		t0, C0_NEPC
	sw		t0, VZGUESTCTX_NESTEDEPC(a0)

1:	ext		v0, a1, CFG5_LLB_SHIFT, 1
	beqz		v0, 1f
	PTR_MFGC0	t0, C0_LLADDR
	PTR_S		t0, VZGUESTCTX_LLADDR(a0)

1:	mfgc0		t0, C0_CONFIG
	sw		t0, VZGUESTCTX_CONFIG(a0)
	bgez		t0, 11f
	mfgc0		t1, C0_CONFIG1
	sw		t1, VZGUESTCTX_CONFIG1(a0)
	bgez		t1, 12f
	mfgc0		t2, C0_CONFIG2
	sw		t2, VZGUESTCTX_CONFIG2(a0)
	bgez		t2, 13f
	mfgc0		t3, C0_CONFIG3
	sw		t3, VZGUESTCTX_CONFIG3(a0)
	bgez		t3, 14f
	mfgc0		ta0, C0_CONFIG4
	sw		ta0, VZGUESTCTX_CONFIG4(a0)
	bgez		ta0, 15f
	mfgc0		ta1, C0_CONFIG5
	sw		ta1, VZGUESTCTX_CONFIG5(a0)
10:	mfgc0		ta2, C0_CONFIG6
	sw		ta2, VZGUESTCTX_CONFIG6(a0)
	mfgc0		ta3, C0_CONFIG7
	sw		ta3, VZGUESTCTX_CONFIG7(a0)

	ext		t0, t3, CFG3_PW_SHIFT, 1
	beqz		t0, 1f
	PTR_MFGC0	t0, C0_PWBASE
	PTR_S		t0, VZGUESTCTX_PWBASE(a0)
	PTR_MFGC0	t0, C0_PWFIELD
	PTR_S		t0, VZGUESTCTX_PWFIELD(a0)
	PTR_MFGC0	t0, C0_PWSIZE
	PTR_S		t0, VZGUESTCTX_PWSIZE(a0)
	mfgc0		t0, C0_PWCTL
	sw		t0, VZGUESTCTX_PWCTL(a0)

1:	ext		t0, t3, CFG3_SC_SHIFT, 1
	beqz		t0, 1f
	PTR_MFGC0	t0, C0_SEGCTL0
	PTR_S		t0, VZGUESTCTX_SEGCTL0(a0)
	PTR_MFGC0	t0, C0_SEGCTL1
	PTR_S		t0, VZGUESTCTX_SEGCTL1(a0)
	PTR_MFGC0	t0, C0_SEGCTL2
	PTR_S		t0, VZGUESTCTX_SEGCTL2(a0)

1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 1, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH1
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH1(a0)
	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 2, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH2
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH2(a0)
	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 3, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH3
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH3(a0)
	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 4, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH4
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH4(a0)
	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 5, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH5
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH5(a0)
	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 6, 1
	move		t0, zero
	beqz		a1, 1f
	PTR_MFGC0	t0, C0_KSCRATCH6
1:	PTR_S		t0, VZGUESTCTX_KSCRATCH6(a0)

	ext		v0, t1, CFG1_WR_SHIFT, 1
	beqz		v0, 21f
	PTR_MFGC0	t0, C0_WATCHLO
	PTR_S		t0, VZGUESTCTX_WATCHLO0(a0)
	mfgc0		t0, C0_WATCHHI
	sw		t0, VZGUESTCTX_WATCHHI0(a0)
	bgez		t0, 22f
	PTR_MFGC0	t0, C0_WATCHLO, 1
	PTR_S		t0, VZGUESTCTX_WATCHLO1(a0)
	mfgc0		t0, C0_WATCHHI, 1
	sw		t0, VZGUESTCTX_WATCHHI1(a0)
	bgez		t0, 23f
	PTR_MFGC0	t0, C0_WATCHLO, 2
	PTR_S		t0, VZGUESTCTX_WATCHLO2(a0)
	mfgc0		t0, C0_WATCHHI, 2
	sw		t0, VZGUESTCTX_WATCHHI2(a0)
	bgez		t0, 24f
	PTR_MFGC0	t0, C0_WATCHLO, 3
	PTR_S		t0, VZGUESTCTX_WATCHLO3(a0)
	mfgc0		t0, C0_WATCHHI, 3
	sw		t0, VZGUESTCTX_WATCHHI3(a0)
	bgez		t0, 25f
	PTR_MFGC0	t0, C0_WATCHLO, 4
	PTR_S		t0, VZGUESTCTX_WATCHLO4(a0)
	mfgc0		t0, C0_WATCHHI, 4
	sw		t0, VZGUESTCTX_WATCHHI4(a0)
	bgez		t0, 26f
	PTR_MFGC0	t0, C0_WATCHLO, 5
	PTR_S		t0, VZGUESTCTX_WATCHLO5(a0)
	mfgc0		t0, C0_WATCHHI, 5
	sw		t0, VZGUESTCTX_WATCHHI5(a0)
	bgez		t0, 27f
	PTR_MFGC0	t0, C0_WATCHLO, 6
	PTR_S		t0, VZGUESTCTX_WATCHLO6(a0)
	mfgc0		t0, C0_WATCHHI, 6
	sw		t0, VZGUESTCTX_WATCHHI6(a0)
	bgez		t0, 28f
	PTR_MFGC0	t0, C0_WATCHLO, 7
	PTR_S		t0, VZGUESTCTX_WATCHLO7(a0)
	mfgc0		t0, C0_WATCHHI, 7
	sw		t0, VZGUESTCTX_WATCHHI7(a0)

	# Set type
20:	li		v0, LINKCTX_TYPE_VZGUEST
	# Skip if no MAARs
	ext		t0, ta1, CFG5_MRP_SHIFT, 1
	beqz		t0, 9f

	# Put number of MAARs in a1
	di		t2
	PTR_MFC0	t0, C0_MAARI
	li		t1, -1
	PTR_MTC0	t1, C0_MAARI
	ehb
	PTR_MFC0	a1, C0_MAARI
	PTR_MTC0	t0, C0_MAARI
	ehb
	mtc0 		t2, C0_SR
	addiu		a1, 1

	# Save guest MAARI via a2/a3
	XPTR_MFGC0	(a2, a3, C0_MAARI)
	XPTR_S		(a2, a3, VZGUESTCTX_MAARI, a0)

	# Compute initial address in t0
	li		t0, VZGUESTCTX_MAAR
	add		t0, a0

	# Loop over all guest MAARs
	move		t1, zero
	# Save MARR[t1]
1:	XPTR_MTGC0	(t1, zero, C0_MAARI)
	ehb
	XPTR_MFGC0	(t2, t3, C0_MAAR)
	XPTR_S		(t2, t3, 0, t0)
	# Increment
	addiu		t0, 8
	addiu		t1, 1
	blt		t1, a1, 1b

	# Restore guest MAARI
	XPTR_MTGC0	(a2, a3, C0_MAARI)
	ehb

	# Save link and zero next pointer
9:	PTR_S		v0, LINKCTX_ID(a0)
	PTR_S		zero, LINKCTX_NEXT(a0)
	jr		ra

11:	sw		zero, VZGUESTCTX_CONFIG1(a0)
12:	sw		zero, VZGUESTCTX_CONFIG2(a0)
13:	sw		zero, VZGUESTCTX_CONFIG3(a0)
	move		t3, zero
14:	sw		zero, VZGUESTCTX_CONFIG4(a0)
15:	sw		zero, VZGUESTCTX_CONFIG5(a0)
	move		ta1, zero
	b		10b

21:	sw		zero, VZGUESTCTX_WATCHHI0(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO0(a0)
22:	sw		zero, VZGUESTCTX_WATCHHI1(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO1(a0)
23:	sw		zero, VZGUESTCTX_WATCHHI2(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO2(a0)
24:	sw		zero, VZGUESTCTX_WATCHHI3(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO3(a0)
25:	sw		zero, VZGUESTCTX_WATCHHI4(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO4(a0)
26:	sw		zero, VZGUESTCTX_WATCHHI5(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO5(a0)
27:	sw		zero, VZGUESTCTX_WATCHHI6(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO6(a0)
28:	sw		zero, VZGUESTCTX_WATCHHI7(a0)
	PTR_S		zero, VZGUESTCTX_WATCHLO7(a0)
	b		20b
END(_vzguestctx_save)

#
# FUNCTION:	_vzguestctx_load
#
# DESCRIPTION:	load VZ guest registers from memory starting at a0
#
# RETURNS:	int
#		0:	Unrecognised context
#		CTX_*:	Type of context restored
#
LEAF(_vzguestctx_load)
	# Check we have VZ, bail if not
	mfc0		a1, C0_CONFIG3
	ext		v0, a1, CFG3_VZ_SHIFT, 1
	beqz		v0, 9f

	# Check link, bail if wrong type
	PTR_L		t0, LINKCTX_ID(a0)
	li		v1, LINKCTX_TYPE_VZGUEST
	move		v0, zero
	bne		t0, v1, 9f
	move		v0, v1

	# Load guest context
	lw		t0, VZGUESTCTX_INDEX(a0)
	mtgc0		t0, C0_INDEX
	XPTR_L		(t0, t1, VZGUESTCTX_ENTRYLO0, a0)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_L		(t0, t1, VZGUESTCTX_ENTRYLO1, a0)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO1)
	PTR_L		t0, VZGUESTCTX_CONTEXT(a0)
	PTR_MTGC0	t0, C0_CONTEXT
	lw		t0, VZGUESTCTX_CONTEXTCONFIG(a0)
	mtgc0		t0, C0_CONTEXTCONF
	PTR_L		t0, VZGUESTCTX_PAGEMASK(a0)
	PTR_MTGC0	t0, C0_PAGEMASK
	lw		t0, VZGUESTCTX_PAGEGRAIN(a0)
	mtgc0		t0, C0_PAGEGRAIN
	lw		t0, VZGUESTCTX_WIRED(a0)
	mtgc0		t0, C0_WIRED
	lw		t0, VZGUESTCTX_HWRENA(a0)
	mtgc0		t0, C0_HWRENA
	PTR_L		t0, VZGUESTCTX_BADVADDR(a0)
	PTR_MTGC0	t0, C0_BADVADDR
	PTR_L		t0, VZGUESTCTX_ENTRYHI(a0)
	PTR_MTGC0	t0, C0_ENTRYHI
	lw		t0, VZGUESTCTX_COMPARE(a0)
	mtgc0		t0, C0_COMPARE
	lw		t0, VZGUESTCTX_STATUS(a0)
	mtgc0		t0, C0_STATUS
	lw		t0, VZGUESTCTX_INTCTL(a0)
	mtgc0		t0, C0_INTCTL
	lw		t0, VZGUESTCTX_SRSCTL(a0)
	mtgc0		t0, C0_SRSCTL
	lw		t0, VZGUESTCTX_SRSMAP(a0)
	mtgc0		t0, C0_SRSMAP
	lw		t0, VZGUESTCTX_CAUSE(a0)
	mtgc0		t0, C0_CAUSE
	PTR_L		t0, VZGUESTCTX_EPC(a0)
	PTR_MTGC0	t0, C0_EPC
	PTR_L		t0, VZGUESTCTX_EBASE(a0)
	PTR_MTGC0	t0, C0_EBASE
	PTR_L		t0, VZGUESTCTX_USERLOCAL(a0)
	PTR_MTGC0	t0, C0_USERLOCAL
	PTR_L		t0, VZGUESTCTX_ERROREPC(a0)
	PTR_MTGC0	t0, C0_ERRPC

	lw		a1, VZGUESTCTX_CONFIG3(a0)
	ext		v0, a1, CFG3_BI_SHIFT, 1
	beqz		v0, 1f
	lw		t0, VZGUESTCTX_BADINSTR(a0)
	mtgc0		t0, C0_BADINSTR
1:	ext		v0, a1, CFG3_BP_SHIFT, 1
	beqz		v0, 1f
	lw		t0, VZGUESTCTX_BADINSTRP(a0)
	mtgc0		t0, C0_BADPINSTR

1:	lw		a1, VZGUESTCTX_CONFIG5(a0)
	ext		v0, a1, CFG5_NF_SHIFT, 1
	beqz		v0, 1f
	lw		t0, VZGUESTCTX_NESTEDEXC(a0)
	mtgc0		t0, C0_NESTEDEXC
	lw		t0, VZGUESTCTX_NESTEDEPC(a0)
	mtgc0		t0, C0_NEPC

1:	ext		v0, a1, CFG5_LLB_SHIFT, 1
	beqz		v0, 1f
	PTR_L		t0, VZGUESTCTX_LLADDR(a0)
	PTR_MTGC0	t0, C0_LLADDR

1:	lw		t0, VZGUESTCTX_CONFIG(a0)
	mtgc0		t0, C0_CONFIG
	bgez		t0, 11f
	lw		t1, VZGUESTCTX_CONFIG1(a0)
	mtgc0		t1, C0_CONFIG1
	bgez		t1, 12f
	lw		t2, VZGUESTCTX_CONFIG2(a0)
	mtgc0		t2, C0_CONFIG2
	bgez		t2, 13f
	lw		t3, VZGUESTCTX_CONFIG3(a0)
	mtgc0		t3, C0_CONFIG3
	bgez		t3, 14f
	lw		ta0, VZGUESTCTX_CONFIG4(a0)
	mtgc0		ta0, C0_CONFIG4
	bgez		ta0, 15f
	lw		ta1, VZGUESTCTX_CONFIG5(a0)
	mtgc0		ta1, C0_CONFIG5
10:	lw		ta2, VZGUESTCTX_CONFIG6(a0)
	mtgc0		ta2, C0_CONFIG6
	lw		ta3, VZGUESTCTX_CONFIG7(a0)
	mtgc0		ta3, C0_CONFIG7

	ext		t0, t3, CFG3_PW_SHIFT, 1
	beqz		t0, 1f
	PTR_L		t0, VZGUESTCTX_PWBASE(a0)
	PTR_MTGC0	t0, C0_PWBASE
	PTR_L		t0, VZGUESTCTX_PWFIELD(a0)
	PTR_MTGC0	t0, C0_PWFIELD
	PTR_L		t0, VZGUESTCTX_PWSIZE(a0)
	PTR_MTGC0	t0, C0_PWSIZE
	lw		t0, VZGUESTCTX_PWCTL(a0)
	mtgc0		t0, C0_PWCTL

1:	ext		t0, t3, CFG3_SC_SHIFT, 1
	beqz		t0, 1f
	PTR_L		t0, VZGUESTCTX_SEGCTL0(a0)
	PTR_MTGC0	t0, C0_SEGCTL0
	PTR_L		t0, VZGUESTCTX_SEGCTL1(a0)
	PTR_MTGC0	t0, C0_SEGCTL1
	PTR_L		t0, VZGUESTCTX_SEGCTL2(a0)
	PTR_MTGC0	t0, C0_SEGCTL2

1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 1, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH1(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH1
1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 2, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH2(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH2
1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 3, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH3(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH3
1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 4, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH4(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH4
1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 5, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH5(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH5
1:	ext		a1, ta0, CFG4_KSCREXIST_SHIFT + 6, 1
	PTR_L		t0, VZGUESTCTX_KSCRATCH6(a0)
	beqz		a1, 1f
	PTR_MTGC0	t0, C0_KSCRATCH6

1:	ext		v0, t1, CFG1_WR_SHIFT, 1
	beqz		v0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO0(a0)
	PTR_MTGC0	t0, C0_WATCHLO
	lw		t0, VZGUESTCTX_WATCHHI0(a0)
	mtgc0		t0, C0_WATCHHI
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO1(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 1
	lw		t0, VZGUESTCTX_WATCHHI1(a0)
	mtgc0		t0, C0_WATCHHI, 1
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO2(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 2
	lw		t0, VZGUESTCTX_WATCHHI2(a0)
	mtgc0		t0, C0_WATCHHI, 2
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO3(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 3
	lw		t0, VZGUESTCTX_WATCHHI3(a0)
	mtgc0		t0, C0_WATCHHI, 3
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO4(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 4
	lw		t0, VZGUESTCTX_WATCHHI4(a0)
	mtgc0		t0, C0_WATCHHI, 4
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO5(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 5
	lw		t0, VZGUESTCTX_WATCHHI5(a0)
	mtgc0		t0, C0_WATCHHI, 5
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO6(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 6
	lw		t0, VZGUESTCTX_WATCHHI6(a0)
	mtgc0		t0, C0_WATCHHI, 6
	bgez		t0, $Ldone_wr
	PTR_L		t0, VZGUESTCTX_WATCHLO7(a0)
	PTR_MTGC0	t0, C0_WATCHLO, 7
	lw		t0, VZGUESTCTX_WATCHHI7(a0)
	mtgc0		t0, C0_WATCHHI, 7
	bgez		t0, $Ldone_wr

$Ldone_wr:
	ext		t0, ta1, CFG5_MRP_SHIFT, 1
	beqz		t0, 8f

	# Put number of MAARs in a1
	di		t2
	PTR_MFC0	t0, C0_MAARI
	li		t1, -1
	PTR_MTC0	t1, C0_MAARI
	ehb
	PTR_MFC0	a1, C0_MAARI
	PTR_MTC0	t0, C0_MAARI
	ehb
	mtc0 		t2, C0_SR
	addiu		a1, 1

	# Compute initial address in t0
	li		t0, VZGUESTCTX_MAAR
	add		t0, a0

	# Loop over all guest MAARs
	move		t1, zero
	# Load MARR[t1]
1:	XPTR_MTGC0	(t1, zero, C0_MAARI)
	ehb
	XPTR_L		(t2, t3, 0, t0)
	XPTR_MTGC0	(t2, t3, C0_MAAR)
	ehb
	# Increment
	addiu		t0, 8
	addiu		t1, 1
	blt			t1, a1, 1b

	# Load guest MAARI via a2/a3
	XPTR_L		(a2, a3, VZGUESTCTX_MAARI, a0)
	XPTR_MTGC0	(a2, a3, C0_MAARI)
	ehb

	# Restore link type
8:	move		v0, v1

	# Return
9:	jr		ra

11:	mtgc0		zero, C0_CONFIG1
12:	mtgc0		zero, C0_CONFIG2
13:	mtgc0		zero, C0_CONFIG3
	move		t3, zero
14:	mtgc0		zero, C0_CONFIG4
15:	mtgc0		zero, C0_CONFIG5
	move		ta1, zero
	b		10b
END(_vzguestctx_load)

#
# FUNCTION:	_vztlbctx_save
#
# DESCRIPTION:	save VZ guest TLBs to memory starting at a0
#
# RETURNS:	int
#			0:	No context saved
#			CTX_*:	Type of context stored
#
NESTED(_vztlbctx_save, (VZTLBCTXENTRY_SIZE + 4), ra)
	addiu		sp, sp, -(VZTLBCTXENTRY_SIZE + 4)

	# Check we have VZ, bail if not
	mfc0		v0, C0_CONFIG3
	ext		v0, v0, CFG3_VZ_SHIFT, 1
	beqz		v0, 9f

	# Stash guest TLB state
	mfgc0		t0, C0_INDEX
	sw		t0, 0(sp)
	PTR_MFGC0	t0, C0_ENTRYHI
	PTR_S		t0, 4 + VZTLBCTXENTRY_ENTRYHI(sp)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_S		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO0, sp)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO1)
	XPTR_S		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO1, sp)
	PTR_MFGC0	t0, C0_PAGEMASK
	PTR_S		t0, 4 + VZTLBCTXENTRY_PAGEMASK(sp)
	mfc0		t0, C0_GUESTCTL1
	sw		t0, 4 + VZTLBCTXENTRY_GUESTCTL1(sp)

	# Figure out if we are on a pre-R6 or R6 ISA.
	mfgc0		t0, C0_CONFIG
	ext		a1, t0, CFG0_AR_SHIFT, CFG0_AR_BITS
	xori		a1, a1, 2
	bnez		a1, 1f

	# R6: Check Config_MT
	mfgc0		t0, C0_CONFIG
	ext		a1, t0, CFG0_MT_SHIFT, CFG0_MT_BITS
	li		a2, 1
	beq		a1, a2, 2f
	li		a2, 4
	beq		a1, a2, 2f
	b		3f

	# pre-R6: Check Config4_MMUExtDef
1:	mfgc0		t0, C0_CONFIG4
	ext		a1, t0, CFG4_MMUED_SHIFT, CFG4_MMUED_BITS
	li		a2, 3
	bne		a1, a3, 3f

2:	mfgc0		t0, C0_CONFIG4
	ext		a1, t0, CFG4_VTLBSEXT_SHIFT, CFG4_VTLBSEXT_BITS
	sll		a1, a1, CFG1_MMUS_BITS
	mfgc0		t0, C0_CONFIG1
	ext		a2, t0, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS
	or		a1, a1, a2
	b		1f

      # Put the number of TLB entries in a1
3:	mfgc0		t0, C0_CONFIG1
	ext		a1, t0, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS
1:	addiu		a1, 1

	# Compute initial address in t0
	li		t0, VZTLBCTX_ENTRIES
	add		t0, a0

	# Loop over all guest TLBs
	move		t1, zero
	# Save TLB[t1]
1:	mtgc0		t1, C0_INDEX
	ehb
	tlbgr
	ehb
	PTR_MFGC0	t2, C0_ENTRYHI
	PTR_S		t2, VZTLBCTXENTRY_ENTRYHI(t0)
	XPTR_MFGC0	(t2, t3, C0_ENTRYLO0)
	XPTR_S		(t2, t3, VZTLBCTXENTRY_ENTRYLO0, t0)
	XPTR_MFGC0	(t2, t3, C0_ENTRYLO1)
	XPTR_S		(t2, t3, VZTLBCTXENTRY_ENTRYLO1, t0)
	PTR_MFGC0	t2, C0_PAGEMASK
	PTR_S		t2, VZTLBCTXENTRY_PAGEMASK(t0)
	mfc0		t2, C0_GUESTCTL1
	sw		t2, VZTLBCTXENTRY_GUESTCTL1(t0)
	# Increment
	addiu		t0, VZTLBCTXENTRY_SIZE
	addiu		t1, 1
	blt		t1, a1, 1b

	# Restore guest TLB state
	lw		t0, 0(sp)
	mtgc0		t0, C0_INDEX
	PTR_L		t0, 4 + VZTLBCTXENTRY_ENTRYHI(sp)
	PTR_MTGC0	t0, C0_ENTRYHI
	XPTR_L		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO0, sp)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_L		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO1, sp)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO1)
	PTR_L		t0, 4 + VZTLBCTXENTRY_PAGEMASK(sp)
	PTR_MTGC0	t0, C0_PAGEMASK
	lw		t0, 4 + VZTLBCTXENTRY_GUESTCTL1(sp)
	mtc0		t0, C0_GUESTCTL1

	# Set type
	li		v0, LINKCTX_TYPE_VZTLBCTX

	# Save link and zero next pointer
9:	PTR_S		v0, LINKCTX_ID(a0)
	PTR_S		zero, LINKCTX_NEXT(a0)
	addiu		sp, sp, (VZTLBCTXENTRY_SIZE + 4)
	jr.hb		ra
END(_vztlbctx_save)

#
# FUNCTION:	_vztlbctx_load
#
# DESCRIPTION:	load VZ guest TLBs from memory starting at a0
#
# RETURNS:	int
#		0:	Unrecognised context
#		CTX_*:	Type of context restored
#
NESTED(_vztlbctx_load, (VZTLBCTXENTRY_SIZE + 4), ra)
	addiu		sp, sp, -(VZTLBCTXENTRY_SIZE + 4)

	# Check we have VZ, bail if not
	mfc0		v0, C0_CONFIG3
	ext		v0, v0, CFG3_VZ_SHIFT, 1
	beqz		v0, 9f

	# Check link, bail if wrong type
	PTR_L		t0, LINKCTX_ID(a0)
	li		v1, LINKCTX_TYPE_VZTLBCTX
	move		v0, zero
	bne		t0, v1, 9f
	move		v0, v1

	# Stash guest TLB state
	mfgc0		t0, C0_INDEX
	sw		t0, 0(sp)
	PTR_MFGC0	t0, C0_ENTRYHI
	PTR_S		t0, 4 + VZTLBCTXENTRY_ENTRYHI(sp)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_S		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO0, sp)
	XPTR_MFGC0	(t0, t1, C0_ENTRYLO1)
	XPTR_S		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO1, sp)
	PTR_MFGC0	t0, C0_PAGEMASK
	PTR_S		t0, 4 + VZTLBCTXENTRY_PAGEMASK(sp)
	mfc0		t0, C0_GUESTCTL1
	sw		t0, 4 + VZTLBCTXENTRY_GUESTCTL1(sp)

	# Figure out if we are on a pre-R6 or R6 ISA.
	mfgc0		t0, C0_CONFIG
	ext		a1, t0, CFG0_AR_SHIFT, CFG0_AR_BITS
	xori		a1, a1, 2
	bnez		a1, 1f

	# R6: Check Config_MT
	mfgc0		t0, C0_CONFIG
	ext		a1, t0, CFG0_MT_SHIFT, CFG0_MT_BITS
	li		a2, 1
	beq		a1, a2, 2f
	li		a2, 4
	beq		a1, a2, 2f
	b		3f

	# pre-R6: Check Config4_MMUExtDef
1:	mfgc0		t0, C0_CONFIG4
	ext		a1, t0, CFG4_MMUED_SHIFT, CFG4_MMUED_BITS
	li		a2, 3
	bne		a1, a3, 3f

2:
	mfgc0		t0, C0_CONFIG4
	ext		a1, t0, CFG4_VTLBSEXT_SHIFT, CFG4_VTLBSEXT_BITS
	sll		a1, a1, CFG1_MMUS_BITS
	mfgc0		t0, C0_CONFIG1
	ext		a2, t0, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS
	or		a1, a1, a2
	b		1f

      # Put the number of TLB entries in a1
3:	mfgc0		t0, C0_CONFIG1
	ext		a1, t0, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS
1:	addiu		a1, 1

	# Compute initial address in t0
	li		t0, VZTLBCTX_ENTRIES
	add		t0, a0

	# Loop over all guest TLBs
	move		t1, zero
	li		ta0, 0x80000000
	# Load TLB[t1]
1:	mtgc0		t1, C0_INDEX
	PTR_L		t2, VZTLBCTXENTRY_ENTRYHI(t0)
	PTR_MTGC0	t2, C0_ENTRYHI
	ehb
	tlbgp
	ehb
	mfgc0		ta1, C0_INDEX
	bltz		ta1, 3f
2:	mtgc0		ta1, C0_INDEX
	mtgc0		ta0, C0_ENTRYHI
	mtgc0		zero, C0_ENTRYLO0
	mtgc0		zero, C0_ENTRYLO1
	mtgc0		zero, C0_PAGEMASK
	mtc0		zero, C0_GUESTCTL1
	ehb
	addiu		ta0, ta0, 0x2000
	tlbgp
	ehb
	mfgc0		ta2, C0_INDEX
	bgez		ta2, 2b
	tlbgwi
	b		1b

3:	XPTR_L		(t2, t3, VZTLBCTXENTRY_ENTRYLO0, t0)
	XPTR_MTGC0	(t2, t3, C0_ENTRYLO0)
	XPTR_L		(t2, t3, VZTLBCTXENTRY_ENTRYLO1, t0)
	XPTR_MTGC0	(t2, t3, C0_ENTRYLO1)
	PTR_L		t2, VZTLBCTXENTRY_PAGEMASK(t0)
	PTR_MTGC0	t2, C0_PAGEMASK
	lw		t2, VZTLBCTXENTRY_GUESTCTL1(t0)
	mtc0		t2, C0_GUESTCTL1
	ehb
	tlbgwi
	ehb
	# Increment
	addiu		t0, VZTLBCTXENTRY_SIZE
	addiu		t1, 1
	blt		t1, a1, 1b

	# Restore guest TLB state
	lw		t0, 0(sp)
	mtgc0		t0, C0_INDEX
	PTR_L		t0, 4 + VZTLBCTXENTRY_ENTRYHI(sp)
	PTR_MTGC0	t0, C0_ENTRYHI
	XPTR_L		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO0, sp)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO0)
	XPTR_L		(t0, t1, 4 + VZTLBCTXENTRY_ENTRYLO1, sp)
	XPTR_MTGC0	(t0, t1, C0_ENTRYLO1)
	PTR_L		t0, 4 + VZTLBCTXENTRY_PAGEMASK(sp)
	PTR_MTGC0	t0, C0_PAGEMASK
	lw		t0, 4 + VZTLBCTXENTRY_GUESTCTL1(sp)
	mtc0		t0, C0_GUESTCTL1

	# Set exit code
	move		v0, v1
	# Return
9:	addiu		sp, sp, (VZTLBCTXENTRY_SIZE + 4)
	jr.hb		ra
END(_vztlbctx_load)
