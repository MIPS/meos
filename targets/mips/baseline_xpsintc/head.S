####(C)2013###############################################################
#
# Copyright (C) 2013 MIPS Tech, LLC
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
####(C)2013###############################################################
#
#   Description:	MIPS baseline root interrupt code
#
##########################################################################

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include "meos/config.h"
#include "meos/target/ctx.h"
#include "meos/target/irq2.h"

#ifndef SR_CU1_SHIFT
#define  SR_CU1_SHIFT		29
#endif
#ifndef CFG5_MSAEN_SHIFT
#define  CFG5_MSAEN_SHIFT	27
#endif
#ifndef SR_IE_SHIFT
#define  SR_IE_SHIFT		 0
#endif

#if defined(CONFIG_ARCH_MIPS_R6) || defined(CONFIG_ARCH_MIPS_MICRO_R6)
#define	SELNEZ(RD, RS, RT) selnez RD, RS, RT
#define	SELEQZ(RD, RS, RT) seleqz RD, RS, RT
#else
#define SELNEZ(RD, RS, RT) \
movn	RD, RS, RT ;\
movz	RD, zero, RT

#define SELEQZ(RD, RS, RT) \
movz	RD, RS, RT ;\
movn	RD, zero, RT
#endif

#define DBG_TRACE_EXIT_ISR 0x80000000
#define DBG_TRACE_SCHED_ISR 0x80000001
#define DBG_TRACE_HOTWIRE_ISR 0x80000002

# Context size, adjusted for parameter area
#define	ADJ 16
#ifdef CONFIG_ARCH_MIPS_DSP
#define CTX_SIZEADJ (CTX_SIZE + DSPCTX_SIZE + ADJ)
#else
#define CTX_SIZEADJ (CTX_SIZE + ADJ)
#endif

# DSP context offset
#ifdef CONFIG_ARCH_MIPS_DSP
#ifdef CONFIG_ARCH_MIPS_VZ
#define DOFFSET 8
#else
#define DOFFSET 4
#endif
#endif

# reent offset
#ifdef CONFIG_ARCH_MIPS_REENT
#ifdef CONFIG_ARCH_MIPS_DSP
#ifdef CONFIG_ARCH_MIPS_VZ
#define ROFFSET (8 + DSPCTX_SIZE)
#else
#define ROFFSET (4 + DSPCTX_SIZE)
#endif
#else
#ifdef CONFIG_ARCH_MIPS_VZ
#define ROFFSET 8
#else
#define ROFFSET 4
#endif
#endif
#endif

#define	e_CTX	s0
#define	e_SIG	s1
#define e_ISR	s2
#define e_CR	s3
#define e_IST	s4
#define e_SR	s5
#define e_EPC	s6
#define e_RA	s7

#define IST_IE	0
#define IST_MSA	1
#define IST_FP	2

#
# FUNCTION:	_sleep
#
# DESCRIPTION: Wait forever - we will get context switched away.
#
LEAF(_sleep)
	wait
	di
	ehb
	ei
	j 	_sleep
.globl _sleep_end
_sleep_end:
END(_sleep)

#if !defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6)
#
# FUNCTION:	_toupee
#
# DESCRIPTION: Interrupt entry vector - chain forwards to _head or _nest.
#
#ifdef CONFIG_ARCH_MIPS_VZ
NESTED(_toupee, 0, ra)
	.set	push
	.set	noat
	.set	noreorder
	mtc0	k0, C0_KSCRATCH2
	mfc0	k0, C0_KSCRATCH1
	mtc0	k1, C0_KSCRATCH1
	lw	k1, CTX_NEST(k0)
	bnez	k1, _toupee_nest
		mfc0	k1, C0_KSCRATCH2
	sw	k1, CTX_K0(k0)
	mfc0	k1, C0_KSCRATCH1
	sw	k1, CTX_K1(k0)
	la	k1, _head
	jr	k1
		mtc0	k0, C0_KSCRATCH1
_toupee_nest:
	sw	k1, (-CTX_SIZEADJ + ADJ + CTX_K0)(sp)
	mfc0	k1, C0_KSCRATCH1
	sw	k1, (-CTX_SIZEADJ + ADJ + CTX_K1)(sp)
	la	k1, _nest
	jr	k1
		mtc0	k0, C0_KSCRATCH1
	.set	pop
END(_toupee)
#else
NESTED(_toupee, 0, ra)
	.set	push
	.set	noat
	.set	noreorder
	lw	k1, CTX_NEST(k0)
	bnez	k1, _toupee_nest
		lui	k1, %hi(_head)
	addiu	k1, %lo(_head)
	jr	k1
_toupee_nest:
		lui	k1, %hi(_nest)
	addiu	k1, %lo(_nest)
	jr	k1
		nop
	.set	pop
END(_toupee)
#endif
#else
#
# FUNCTION:	_toupee
#
# DESCRIPTION: Interrupt entry vector - chain forwards to _head or _nest.
#
#ifdef CONFIG_ARCH_MIPS_VZ
NESTED(_toupee, 0, ra)
	.set	push
	.set	noat
	.set	noreorder
	mtc0	k0, C0_KSCRATCH2
	mfc0	k0, C0_KSCRATCH1
	mtc0	k1, C0_KSCRATCH1
	lw	k1, CTX_NEST(k0)
	bnez	k1, _toupee_nest
	mfc0	k1, C0_KSCRATCH2
	sw	k1, CTX_K0(k0)
	mfc0	k1, C0_KSCRATCH1
	sw	k1, CTX_K1(k0)
	mtc0	k0, C0_KSCRATCH2
	la	k1, _head
	jr	k1
_toupee_nest:
	mfc0	k1, C0_KSCRATCH2
	sw	k1, (-CTX_SIZEADJ + ADJ + CTX_K0)(sp)
	mfc0	k1, C0_KSCRATCH1
	sw	k1, (-CTX_SIZEADJ + ADJ + CTX_K1)(sp)
	mtc0	k0, C0_KSCRATCH1
	la	k1, _nest
	jr	k1
	.set	pop
END(_toupee)
#else
NESTED(_toupee, 0, ra)
	.set	push
	.set	noat
	.set	noreorder
	lw	k1, CTX_NEST(k0)
	bnez	k1, _toupee_nest
	la	k1, _head
	jr	k1
_toupee_nest:
	la	k1, _nest
	jr	k1
	.set	pop
END(_toupee)
#endif
#endif

#
# FUNCTION:	_cache_routine
#
# DESCRIPTION:	Dummy cache error handler.
#
NESTED(_cache_routine, 0, ra)
	.set	push
	.set	noat
	sdbbp	0
	.set	pop
END(_cache_routine)

#
# FUNCTION:	_dret
#
# DESCRIPTION:	dret.
#
LEAF(_dret)
	.set	push
	.set	noat
	deret
	.set	pop
END(_dret)

#ifdef CONFIG_ARCH_MIPS_PCINT
# Cause in r0
.macro dopci r0:req, r1:req
	ext	\r1, \r0, CR_PCI_SHIFT, 1
	beqz	\r1, 92f
	mfc0	\r0, C0_PERFCNT, 1
	ext	\r1, \r0, 31, 1
	beqz	\r1, 91f
	ins	\r0, zero, 31, 1
	mtc0	\r0, C0_PERFCNT, 1
	lui	\r0, %hi(_TMR_overflow0)
	lw	\r1, %lo(_TMR_overflow0)(\r0)
	addi	\r1, 1
	sw	\r1, %lo(_TMR_overflow0)(\r0)
91:	mfc0	\r0, C0_PERFCNT, 3
	ext	\r1, \r0, 31, 1
	beqz	\r1, 91f
	ins	\r0, zero, 31, 1
	mtc0	\r0, C0_PERFCNT, 3
	lui	\r0, %hi(_TMR_overflow1)
	lw	\r1, %lo(_TMR_overflow1)(\r0)
	addi	\r1, 1
	sw	\r1, %lo(_TMR_overflow1)(\r0)
#ifdef CONFIG_ARCH_MIPS_QPC
91:	mfc0	\r0, C0_PERFCNT, 5
	ext	\r1, \r0, 31, 1
	beqz	\r1, 91f
	ins	\r0, zero, 31, 1
	mtc0	\r0, C0_PERFCNT, 5
	lui	\r0, %hi(_TMR_overflow2)
	lw	\r1, %lo(_TMR_overflow2)(\r0)
	addi	\r1, 1
	sw	\r1, %lo(_TMR_overflow2)(\r0)
91:	mfc0	\r0, C0_PERFCNT, 7
	ext	\r1, \r0, 31, 1
	beqz	\r1, 92f
	ins	\r0, zero, 31, 1
	mtc0	\r0, C0_PERFCNT, 7
	lui	\r0, %hi(_TMR_overflow3)
	lw	\r1, %lo(_TMR_overflow3)(\r0)
	addi	\r1, 1
	sw	\r1, %lo(_TMR_overflow3)(\r0)
#endif
91:
92:
.endm
#endif

#
# FUNCTION:	_head
#
# DESCRIPTION:	First level interrupt handler.
#
NESTED(_head, 0, zero)
	.set	push
	.set	noat
	# Stash user context
	sw	$1, CTX_AT(k0)		#1
#ifdef CONFIG_ARCH_MIPS_PCINT
	mfc0	$1, C0_CR
	dopci	$1, k1
#endif
	sw	v0, CTX_V0(k0)		#2
	sw	v1, CTX_V1(k0)		#3
	sw	a0, CTX_A0(k0)		#4
	sw	a1, CTX_A1(k0)		#5
	sw	a2, CTX_A2(k0)		#6
	sw	a3, CTX_A3(k0)		#7
	sw	t0, CTX_T0(k0)		#8
	sw	t1, CTX_T1(k0)		#9
	sw	t2, CTX_T2(k0)		#10
	sw	t3, CTX_T3(k0)		#11
	sw	t4, CTX_T4(k0)		#12
	sw	t5, CTX_T5(k0)		#13
	sw	t6, CTX_T6(k0)		#14
	sw	t7, CTX_T7(k0)		#15
	sw	s0, CTX_S0(k0)		#16
	sw	s1, CTX_S1(k0)		#17
	sw	s2, CTX_S2(k0)		#18
	sw	s3, CTX_S3(k0)		#19
	sw	s4, CTX_S4(k0)		#20
	sw	s5, CTX_S5(k0)		#21
	sw	s6, CTX_S6(k0)		#22
	sw	s7, CTX_S7(k0)		#23
	sw	t8, CTX_T8(k0)		#24
	sw	t9, CTX_T9(k0)		#25
	sw	gp, CTX_GP(k0)		#28
	sw	sp, CTX_SP(k0)		#29
	sw	fp, CTX_FP(k0)		#30
	sw	ra, CTX_RA(k0)		#31
	lw	t0, CTX_NEST(k0)
	addiu	t0, t0, 1
	sw	t0, CTX_NEST(k0)
#ifdef CONFIG_ARCH_MIPS_VZ
	mfc0	t0, C0_GUESTCTL0
	sw	t0, CTX_GCTL0(k0)
	ins	t0, zero, GUESTCTL0_GM_SHIFT, 1
	mtc0	t0, C0_GUESTCTL0
	la 	gp, _gp
#endif
#ifdef CONFIG_ARCH_MIPS_DSP
	# Stash DSP context
	move	t7, k0
	addiu	t7, t7, CTX_SIZE + DOFFSET
	li 	t6, LINKCTX_TYPE_DSP
	sw	t7, CTX_LINK(k0)
	sw	t6, LINKCTX_ID + CTX_SIZE + DOFFSET(k0)
	sw	zero, LINKCTX_NEXT + CTX_SIZE + DOFFSET(k0)
	rddsp	t1
	mfhi	t2, $ac1
	mflo	t3, $ac1
	mfhi	t4, $ac2
	mflo	t5, $ac2
	mfhi	t6, $ac3
	mflo	t7, $ac3
	sw	t1, DSPCTX_DSPC + CTX_SIZE + DOFFSET(k0)
	sw	t2, DSPCTX_HI1 + CTX_SIZE + DOFFSET(k0)
	sw	t3, DSPCTX_LO1 + CTX_SIZE + DOFFSET(k0)
	sw	t4, DSPCTX_HI2 + CTX_SIZE + DOFFSET(k0)
	sw	t5, DSPCTX_LO2 + CTX_SIZE + DOFFSET(k0)
	sw	t6, DSPCTX_HI3 + CTX_SIZE + DOFFSET(k0)
	sw	t7, DSPCTX_LO3 + CTX_SIZE + DOFFSET(k0)
#else
	sw	zero, CTX_LINK(k0)
#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	mfhi	t1
	mflo	t2
#endif
	mfc0	e_SR, C0_SR
	mfc0	e_EPC, C0_EPC
	mfc0	e_CR, C0_CR
#ifdef CONFIG_ARCH_MIPS_BADINSTR
	mfc0	t3, C0_BADINSTR
	mfc0	t4, C0_BADPINSTR
#endif
	mfc0	t5, C0_BADVADDR
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	sw	t1, CTX_HI0(k0)
	sw	t2, CTX_LO0(k0)
#endif
	sw	e_SR, CTX_STATUS(k0)
	sw	e_EPC, CTX_EPC(k0)
	sw	e_CR, CTX_CAUSE(k0)
#ifdef CONFIG_ARCH_MIPS_BADINSTR
	sw	t3, CTX_BADINSTR(k0)
	sw	t4, CTX_BADPINSTR(k0)
#endif
	sw	t5, CTX_BADVADDR(k0)
	move	a1, k0
	# Store interrupt state
	ext	e_IST, e_SR, SR_IE_SHIFT, 1
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	# Disable FPU so interrupts lazily save FPU context
	ext 	t1, e_SR, SR_CU1_SHIFT, 1
	ins	e_IST, t1, IST_FP, 1
	ins	e_SR, zero, SR_CU1_SHIFT, 1
	mtc0	e_SR, C0_SR
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
	# Disable MSA so interrupts lazily save MSA context
	mfc0	t1, $16 ,5 # CONFIG5
	ext	t2, t1, CFG5_MSAEN_SHIFT, 1
	ins	t1, zero, CFG5_MSAEN_SHIFT, 1
	mtc0	t1, $16 ,5 # CONFIG5
	ins	e_IST, t2, IST_MSA, 1
#endif
	# Move to interrupt stack
	lui	sp, %hi(_IRQ_intStack)
	lw	sp, %lo(_IRQ_intStack)(sp)
#ifdef CONFIG_ARCH_MIPS_REENT
	# Use _impure_data for _interrupts
	lui	t0, %hi(_CTX_impure_data)
	addiu	t0, t0, %lo(_CTX_impure_data)
	lw	t0, 0(t0)
	lui	t1, %hi(_impure_ptr)
	addiu	t1, t1, %lo(_impure_ptr)
	sw	t0, 0(t1)
#endif
	# Call root
	bal	_root
	# Has the service routine caused a reschedule?
	lw	t0, _KRN_schedNeeded
	# Prepare to reschedule
	li	t1, 0x8<<2
	beqz	t0, head_complete
	# Enable exception nesting
	move	t0, e_SR
	ins	t0, zero, SR_IE_SHIFT, 2
	mtc0	t0, C0_SR
	# Get vector
	lw	e_ISR, _IRQ_excTable(t1)
						#ifdef CONFIG_DEBUG_TRACE_ISR_HARD
							li	t2, DBG_TRACE_SCHED_ISR
							mtc0	t2, $23, 3
							mtc0	e_EPC, $24, 3
						#endif
						#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
							la	t3, DBG_interruptSched
							jalr	ra, t3
							move	a1, k0
						#endif
	li	a0, 0x8008
	ehb
	jalr	ra, e_ISR
head_complete:
	# Return from head
	# Get back into exception mode
	mfc0	t0, C0_SR
	ori	t0, SR_EXL
	ins	t0, e_IST, IST_IE, 1
	mtc0	t0, C0_SR
#if (defined(CONFIG_ARCH_MIPS_MSA) || defined (CONFIG_ARCH_MIPS_HARD_FLOAT))

	# Get the exception
	ext	t7, e_CR, 2, 5

#ifdef CONFIG_ARCH_MIPS_MSA
	mfc0	t1, $16 ,5 # CONFIG5
	# MSA flag = exception == MSA ? hardware status : previous status
	addiu	t6, t7, -0x15
	ext	t2, t1, CFG5_MSAEN_SHIFT, 1
	ext	t3, e_IST, IST_MSA, 1
	SELEQZ	($1, t2, t6)
	SELNEZ	(t4, t3, t6)
	or	t4, t4, $1
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	# float flag = exception == CU1 ? hardware status : previous status
	addiu	t6, t7, -0xb
	ext	t2, t0, SR_CU1_SHIFT, 1
	ext	t3, e_IST, IST_FP, 1
	SELEQZ	($1, t2, t6)
	SELNEZ	(t5, t3, t6)
	or	t5, t5, $1
#endif

	# flags = _CTX_fpuUser == NULL ? 0 : flags
	lw	t2, _CTX_fpuUser
#ifdef CONFIG_ARCH_MIPS_MSA
	SELNEZ	(t4, t4, t2)
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	SELNEZ	(t5, t5, t2)
#endif

	# Enable hardware as per flags
#ifdef CONFIG_ARCH_MIPS_MSA
	ins	t1, t4, CFG5_MSAEN_SHIFT, 1
	mtc0	t1, $16 ,5 # CONFIG5
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	ins	t0, t5, SR_CU1_SHIFT, 1
	mtc0	t0, C0_SR
#endif

#endif

#ifdef CONFIG_ARCH_MIPS_REENT
	# Restore _impure_ptr
	addu	t0, k0, CTX_SIZE + ROFFSET
	lui	t1, %hi(_impure_ptr)
	addiu	t1, t1, %lo(_impure_ptr)
	sw	t0, 0(t1)
#endif
	# Restore remaining context
#ifdef CONFIG_ARCH_MIPS_DSP
	lw	t1, DSPCTX_DSPC + CTX_SIZE + DOFFSET(k0)
	lw	t2, DSPCTX_HI1 + CTX_SIZE + DOFFSET(k0)
	lw	t3, DSPCTX_LO1 + CTX_SIZE + DOFFSET(k0)
	lw	t4, DSPCTX_HI2 + CTX_SIZE + DOFFSET(k0)
	lw	t5, DSPCTX_LO2 + CTX_SIZE + DOFFSET(k0)
	lw	t6, DSPCTX_HI3 + CTX_SIZE + DOFFSET(k0)
	lw	t7, DSPCTX_LO3 + CTX_SIZE + DOFFSET(k0)
	wrdsp	t1
	mthi	t2, $ac1
	mtlo	t3, $ac1
	mthi	t4, $ac2
	mtlo	t5, $ac2
	mthi	t6, $ac3
	mtlo	t7, $ac3
#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	lw	t1, CTX_HI0(k0)
	lw	t2, CTX_LO0(k0)
#endif
	lw	e_EPC, CTX_EPC(k0)
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	mthi	t1
	mtlo	t2
#endif
	mtc0	e_EPC, C0_EPC
	lw	t0, CTX_NEST(k0)
	addiu	t0, t0, -1
	sw	t0, CTX_NEST(k0)
#ifdef CONFIG_ARCH_MIPS_VZ
	lw	t0, CTX_GCTL0(k0)
	mtc0	t0, C0_GUESTCTL0
	lw	t0, CTX_K0(k0)
	lw	k1, CTX_K1(k0)
	mtc0	t0, C0_KSCRATCH2
#endif
	lw	$1, CTX_AT(k0)
	lw	v0, CTX_V0(k0)
	lw	v1, CTX_V1(k0)
	lw	a0, CTX_A0(k0)
	lw	a1, CTX_A1(k0)
	lw	a2, CTX_A2(k0)
	lw	a3, CTX_A3(k0)
	lw	t0, CTX_T0(k0)
	lw	t1, CTX_T1(k0)
	lw	t2, CTX_T2(k0)
	lw	t3, CTX_T3(k0)
	lw	t4, CTX_T4(k0)
	lw	t5, CTX_T5(k0)
	lw	t6, CTX_T6(k0)
	lw	t7, CTX_T7(k0)
	lw	s0, CTX_S0(k0)
	lw	s1, CTX_S1(k0)
	lw	s2, CTX_S2(k0)
	lw	s3, CTX_S3(k0)
	lw	s4, CTX_S4(k0)
	lw	s5, CTX_S5(k0)
	lw	s6, CTX_S6(k0)
	lw	s7, CTX_S7(k0)
	lw	t8, CTX_T8(k0)
	lw	t9, CTX_T9(k0)
	lw	gp, CTX_GP(k0)
	lw	sp, CTX_SP(k0)
	lw	fp, CTX_FP(k0)
	lw	ra, CTX_RA(k0)
#ifdef	CONFIG_ARCH_MIPS_VZ
	ehb
	mfc0	k0, C0_KSCRATCH2
#endif
	# Return from exception
	eret
.globl _head_end
_head_end:
	.set pop
END(_head)

#
# FUNCTION:	_nest
#
# DESCRIPTION:	Nested interrupt handler.
#
NESTED(_nest, CTX_SIZE, zero)
	.set	push
	.set	noat
	# Save SP
	sw		sp, (-CTX_SIZEADJ + ADJ + CTX_SP)(sp)#29
	# Fix SP
	addiu	sp, -CTX_SIZEADJ
	# Stash user context
	sw	$1, CTX_AT + ADJ(sp)		#1
	sw	v0, CTX_V0 + ADJ(sp)		#2
	sw	v1, CTX_V1 + ADJ(sp)		#3
	sw	a0, CTX_A0 + ADJ(sp)		#4
	sw	a1, CTX_A1 + ADJ(sp)		#5
	sw	a2, CTX_A2 + ADJ(sp)		#6
	sw	a3, CTX_A3 + ADJ(sp)		#7
	sw	t0, CTX_T0 + ADJ(sp)		#8
	sw	t1, CTX_T1 + ADJ(sp)		#9
	sw	t2, CTX_T2 + ADJ(sp)		#10
	sw	t3, CTX_T3 + ADJ(sp)		#11
	sw	t4, CTX_T4 + ADJ(sp)		#12
	sw	t5, CTX_T5 + ADJ(sp)		#13
	sw	t6, CTX_T6 + ADJ(sp)		#14
	sw	t7, CTX_T7 + ADJ(sp)		#15
	sw	s0, CTX_S0 + ADJ(sp)		#16
	sw	s1, CTX_S1 + ADJ(sp)		#17
	sw	s2, CTX_S2 + ADJ(sp)		#18
	sw	s3, CTX_S3 + ADJ(sp)		#19
	sw	s4, CTX_S4 + ADJ(sp)		#20
	sw	s5, CTX_S5 + ADJ(sp)		#21
	sw	s6, CTX_S6 + ADJ(sp)		#22
	sw	s7, CTX_S7 + ADJ(sp)		#23
	sw	t8, CTX_T8 + ADJ(sp)		#24
	sw	t9, CTX_T9 + ADJ(sp)		#25
	sw	gp, CTX_GP + ADJ(sp)		#28
	#sw	sp, CTX_SP + ADJ(sp)		#29 # Done by _toupee
	sw	fp, CTX_FP + ADJ(sp)		#30
	sw	ra, CTX_RA + ADJ(sp)		#31
	lw	t0, CTX_NEST(k0)
	addiu	t0, t0, 1
	sw	t0, CTX_NEST(k0)
#ifdef CONFIG_ARCH_MIPS_DSP
	# Stash DSP context
	move	t7, sp
	addiu	t7, t7, CTX_SIZE + DOFFSET
	li	t6, LINKCTX_TYPE_DSP
	sw	t7, CTX_LINK(sp)
	sw	t6, LINKCTX_ID + CTX_SIZE + DOFFSET(k0)
	sw	zero, LINKCTX_NEXT + CTX_SIZE + DOFFSET(sp)
	rddsp	t1
	mfhi	t2, $ac1
	mflo	t3, $ac1
	mfhi	t4, $ac2
	mflo	t5, $ac2
	mfhi	t6, $ac3
	mflo	t7, $ac3
	sw	t1, DSPCTX_DSPC + CTX_SIZE + DOFFSET(sp)
	sw	t2, DSPCTX_HI1 + CTX_SIZE + DOFFSET(sp)
	sw	t3, DSPCTX_LO1 + CTX_SIZE + DOFFSET(sp)
	sw	t4, DSPCTX_HI2 + CTX_SIZE + DOFFSET(sp)
	sw	t5, DSPCTX_LO2 + CTX_SIZE + DOFFSET(sp)
	sw	t6, DSPCTX_HI3 + CTX_SIZE + DOFFSET(sp)
	sw	t7, DSPCTX_LO3 + CTX_SIZE + DOFFSET(sp)
#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	mfhi	t1
	mflo	t2
#endif
	mfc0	e_SR, C0_SR
	mfc0	e_EPC, C0_EPC
	mfc0	e_CR, C0_CR
#ifdef CONFIG_ARCH_MIPS_BADINSTR
	mfc0	t3, C0_BADINSTR
	mfc0	t4, C0_BADPINSTR
#endif
	mfc0	t5, C0_BADVADDR
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	sw	t1, CTX_HI0 + ADJ(sp)
	sw	t2, CTX_LO0 + ADJ(sp)
#endif
	sw	zero, CTX_LINK + ADJ(sp)
	sw	e_SR, CTX_STATUS + ADJ(sp)
	sw	e_EPC, CTX_EPC + ADJ(sp)
	sw	e_CR, CTX_CAUSE + ADJ(sp)
#ifdef CONFIG_ARCH_MIPS_BADINSTR
	sw	t3, CTX_BADINSTR + ADJ(sp)
	sw	t4, CTX_BADPINSTR + ADJ(sp)
#endif
	sw	t5, CTX_BADVADDR + ADJ(sp)
	move	a1, sp
	addiu	a1, 16
	# Store interrupt state
	ext	e_IST, e_SR, SR_IE_SHIFT, 1
	# Call root
	bal	_root
	# Return from root
#ifdef CONFIG_ARCH_MIPS_DSP
	lw	t1, DSPCTX_DSPC + CTX_SIZE + ADJ(sp)
	lw	t2, DSPCTX_HI1 + CTX_SIZE + ADJ(sp)
	lw	t3, DSPCTX_LO1 + CTX_SIZE + ADJ(sp)
	lw	t4, DSPCTX_HI2 + CTX_SIZE + ADJ(sp)
	lw	t5, DSPCTX_LO2 + CTX_SIZE + ADJ(sp)
	lw	t6, DSPCTX_HI3 + CTX_SIZE + ADJ(sp)
	lw	t7, DSPCTX_LO3 + CTX_SIZE + ADJ(sp)
	wrdsp	t1
	mthi	t2, $ac1
	mtlo	t3, $ac1
	mthi	t4, $ac2
	mtlo	t5, $ac2
	mthi	t6, $ac3
	mtlo	t7, $ac3
#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	lw	t1, CTX_HI0 + ADJ(sp)
	lw	t2, CTX_LO0 + ADJ(sp)
#endif
	lw	e_EPC, CTX_EPC + ADJ(sp)
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	mthi	t1
	mtlo	t2
#endif
	mtc0	e_EPC, C0_EPC
	lw	t0, CTX_NEST(k0)
	addiu	t0, t0, -1
	sw	t0, CTX_NEST(k0)
#ifdef CONFIG_ARCH_MIPS_VZ
	lw	t0, CTX_K0 + ADJ(sp)
	lw	k1, CTX_K1 + ADJ(sp)
	mtc0	t0, C0_KSCRATCH2
#endif
	lw	$1, CTX_AT + ADJ(sp)
	lw	v0, CTX_V0 + ADJ(sp)
	lw	v1, CTX_V1 + ADJ(sp)
	lw	a0, CTX_A0 + ADJ(sp)
	lw	a1, CTX_A1 + ADJ(sp)
	lw	a2, CTX_A2 + ADJ(sp)
	lw	a3, CTX_A3 + ADJ(sp)
	lw	t0, CTX_T0 + ADJ(sp)
	lw	t1, CTX_T1 + ADJ(sp)
	lw	t2, CTX_T2 + ADJ(sp)
	lw	t3, CTX_T3 + ADJ(sp)
	lw	t4, CTX_T4 + ADJ(sp)
	lw	t5, CTX_T5 + ADJ(sp)
	lw	t6, CTX_T6 + ADJ(sp)
	lw	t7, CTX_T7 + ADJ(sp)
	lw	s0, CTX_S0 + ADJ(sp)
	lw	s1, CTX_S1 + ADJ(sp)
	lw	s2, CTX_S2 + ADJ(sp)
	lw	s3, CTX_S3 + ADJ(sp)
	lw	s4, CTX_S4 + ADJ(sp)
	lw	s5, CTX_S5 + ADJ(sp)
	lw	s6, CTX_S6 + ADJ(sp)
	lw	s7, CTX_S7 + ADJ(sp)
	lw	t8, CTX_T8 + ADJ(sp)
	lw	t9, CTX_T9 + ADJ(sp)
	lw	gp, CTX_GP + ADJ(sp)
	lw	fp, CTX_FP + ADJ(sp)
	lw	ra, CTX_RA + ADJ(sp)
	lw	sp, CTX_SP + ADJ(sp)
#ifdef CONFIG_ARCH_MIPS_VZ
	ehb
	mfc0	k0, C0_KSCRATCH2
#endif
	# Return from exception
	eret
.globl _nest_end
_nest_end:
	.set pop
END(_nest)

#
# FUNCTION:	_root
#
# DESCRIPTION: Decode the cause, find the registered handler, and invoke it.
#
NESTED(_root, 24, ra)
	.set	push
	.set	noat
	# Stow context pointer
	move	e_CTX, a1
	# Stow return address
	addiu	sp, sp, -24
	sw	ra, 16(sp)
	move	e_RA, ra
	# Refresh cause and try exceptions
	mfc0	e_CR, C0_CR
#ifdef CONFIG_ARCH_MIPS_PCINT
	move	t0, e_CR
	dopci	t0, t1
#endif
	andi	t0, e_CR, CR_XMASK
	lw	e_ISR, _IRQ_excTable(t0)
	# Exception or interrupt?
	beqz	e_ISR, interrupt
	# Exception
	srl	a0, t0, 2
	# Is it a syscall?
	li	t1, 0x8<<2
	ori	a0, a0, 0x8000
	bne	t0, t1, invoke
	# Is it ours ?
	beq	v0, zero, my_syscall
	# No - invoke _CTX_UHI instead
	la	e_ISR, _CTX_UHI
	b	invoke
	# Yes - fix up syscall EPC
my_syscall:
	addiu	e_EPC, e_EPC, 4
	sw	e_EPC, CTX_EPC(e_CTX)

invoke:
	move 	e_SIG, a0
	move	e_CTX, a1
	# Enable exception nesting
	move	t0, e_SR
	ins	t0, zero, SR_IE_SHIFT, 2
	mtc0	t0, C0_SR
	# Invoke it
						#ifdef CONFIG_DEBUG_TRACE_ISR_HARD
							mtc0	zero, $23, 3 # DBG_TRACE_ENTER_ISR
							mtc0	a0, $24, 3
							mtc0	e_EPC, $24, 3
						#endif
						#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
							la	t0, DBG_interruptIn
							jalr	ra, t0
							move	a0, e_SIG
							move	a1, e_CTX
							jalr	ra, e_ISR
						#else
	jalr	ra, e_ISR
						#endif
						#ifdef CONFIG_DEBUG_TRACE_ISR_HARD
							li	t0, DBG_TRACE_EXIT_ISR
							mtc0	t0, $23, 3
							mtc0	e_SIG, $24, 3
							mtc0	e_EPC, $24, 3
						#endif
						#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
							la	t0, DBG_interruptOut
							move	a0, e_SIG
							jalr	ra, t0
						#endif
	# Back to _head/_nest
	mfc0	t1, C0_SR
	andi	t2, e_SR, SR_EXL
	ins	t2, e_IST, SR_IE, 1
	ext	t3, t1, 2, 30
    sll t3, t3, 2
	or	e_SR, t2, t3
	mtc0	e_SR, C0_SR
	beqz	t2, complete
	# Try interrupts
	mfc0	e_CR, C0_CR
#ifdef CONFIG_ARCH_MIPS_PCINT
	move	t0, e_CR
	dopci	t0, t1
#endif

	# Interrupt
interrupt:
	# Handle timer
	ext	t0, e_CR, CR_TI_SHIFT, 1
	bnez	t0, timer_int
	# Handle FDC
	ext	t0, e_CR, CR_FDCI_SHIFT, 1
	bnez	t0, fdc_int
numints:
	# Invoke highest numbered interrupt
	# Mask out everything but the cause bits
	andi	t0, e_CR, 0xff00
	and	t0, t0, e_SR
	li	t1, 23
	# Bail out if nothing set - level sensitive interrupt probably de-asserted
	# before being serviced
	beqz	t0, complete
	# Find the highest cause bit, convert it to an array index
	clz	t0, t0
	sub	a0, t1, t0
	sll	t2, a0, 2
	# Get the ISR for that index and invoke it
	lui	e_ISR, %hi(_IRQ_intTable)
	addu	e_ISR, e_ISR, t2
	lw	e_ISR, %lo(_IRQ_intTable)(e_ISR)
	b invoke

timer_int:
	li	a0, IRQ_TIMER
	lui	e_ISR, %hi(_IRQ_intTable)
	addu	e_ISR, e_ISR, IRQ_TIMER * 4
	lw	e_ISR, %lo(_IRQ_intTable)(e_ISR)
	bnez	e_ISR, invoke
	b	numints

fdc_int:
	li	a0, IRQ_FDC
	lui	e_ISR, %hi(_IRQ_intTable)
	addu	e_ISR, e_ISR, IRQ_FDC * 4
	lw	e_ISR, %lo(_IRQ_intTable)(e_ISR)
	bnez	e_ISR, invoke
	b	numints

complete:
	addiu	sp, sp, 24
	jr	e_RA
.globl _root_end
_root_end:
	.set pop
END(_root)

#
# FUNCTION:	_hotwire
#
# DESCRIPTION: Exit an interrupt, but activate another context while doing so.
#
LEAF(_hotwire)
	.set	push
	.set	noat
						#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
							move	s0, a0
							la	t0, DBG_hotwire
							jalr	ra, t0
								move	a0, s0
						#endif

# Get back into exception mode
	mfc0	t0, C0_SR
	ori	t0, SR_IE | SR_EXL
	mtc0	t0, C0_SR
	lw	t0, CTX_NEST(k0)
	addiu	t0, t0, -1
	move	k0, a0
#ifdef CONFIG_ARCH_MIPS_VZ
	mtc0 	k0, C0_KSCRATCH1
#endif
#ifdef CONFIG_ARCH_MIPS_REENT
	# Restore _impure_ptr
	addu	t1, k0, CTX_SIZE + ROFFSET
	lui	t2, %hi(_impure_ptr)
	addiu	t2, t2, %lo(_impure_ptr)
	sw	t1, 0(t2)
#endif
	# Restore context
	sw	t0, CTX_NEST(k0)
#ifdef CONFIG_ARCH_MIPS_DSP
	lw	t1, DSPCTX_DSPC + CTX_SIZE + DOFFSET(k0)
	lw	t2, DSPCTX_HI1 + CTX_SIZE + DOFFSET(k0)
	lw	t3, DSPCTX_LO1 + CTX_SIZE + DOFFSET(k0)
	lw	t4, DSPCTX_HI2 + CTX_SIZE + DOFFSET(k0)
	lw	t5, DSPCTX_LO2 + CTX_SIZE + DOFFSET(k0)
	lw	t6, DSPCTX_HI3 + CTX_SIZE + DOFFSET(k0)
	lw	t7, DSPCTX_LO3 + CTX_SIZE + DOFFSET(k0)
	wrdsp	t1
	mthi	t2, $ac1
	mtlo	t3, $ac1
	mthi	t4, $ac2
	mtlo	t5, $ac2
	mthi	t6, $ac3
	mtlo	t7, $ac3
#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	lw	t1, CTX_HI0(k0)
	lw	t2, CTX_LO0(k0)
#endif
	lw	e_EPC, CTX_EPC(k0)
						#ifdef CONFIG_DEBUG_TRACE_ISR_HARD
							li	t0, DBG_TRACE_HOTWIRE_ISR
							mtc0	t0, $23, 3
							mtc0	a0, $24, 3
							mtc0	e_EPC, $24, 3
						#endif
#if defined(CONFIG_ARCH_MIPS_DSP) || (!defined(CONFIG_ARCH_MIPS_R6) && !defined(CONFIG_ARCH_MIPS_MICRO_R6))
	mthi	t1
	mtlo	t2
#endif
	mtc0	e_EPC, C0_EPC
#ifdef CONFIG_ARCH_MIPS_VZ
	lw	t0, CTX_GCTL0(k0)
	mtc0	t0, C0_GUESTCTL0
	lw	t0, CTX_K0(k0)
	lw	k1, CTX_K1(k0)
	mtc0	t0, C0_KSCRATCH2
#endif
	lw	$1, CTX_AT(k0)
	lw	v0, CTX_V0(k0)
	lw	v1, CTX_V1(k0)
	lw	a0, CTX_A0(k0)
	lw	a1, CTX_A1(k0)
	lw	a2, CTX_A2(k0)
	lw	a3, CTX_A3(k0)
	lw	t0, CTX_T0(k0)
	lw	t1, CTX_T1(k0)
	lw	t2, CTX_T2(k0)
	lw	t3, CTX_T3(k0)
	lw	t4, CTX_T4(k0)
	lw	t5, CTX_T5(k0)
	lw	t6, CTX_T6(k0)
	lw	t7, CTX_T7(k0)
	lw	s0, CTX_S0(k0)
	lw	s1, CTX_S1(k0)
	lw	s2, CTX_S2(k0)
	lw	s3, CTX_S3(k0)
	lw	s4, CTX_S4(k0)
	lw	s5, CTX_S5(k0)
	lw	s6, CTX_S6(k0)
	lw	s7, CTX_S7(k0)
	lw	t8, CTX_T8(k0)
	lw	t9, CTX_T9(k0)
	lw	gp, CTX_GP(k0)
	lw	sp, CTX_SP(k0)
	lw	fp, CTX_FP(k0)
	lw	ra, CTX_RA(k0)
#ifdef CONFIG_ARCH_MIPS_VZ
	ehb
	mfc0	k0, C0_KSCRATCH2
#endif
	eret
	.set	pop
END(_hotwire)

#
# FUNCTION:	_terminate
#
# DESCRIPTION: Invoke a task function, then terminate the task.
#
NESTED(_terminate, 0, ra)
	.set	push
	.set	noat
	# Let the backtracer bail
	lui	gp, 0
.globl _task_entry
_task_entry:
	# Call task_func
	jalr	ra, a0
	# Remove task
	la	t0, KRN_removeTask
	li	a0, 0
	jalr	ra, t0
	# No return
	.set 	pop
END(_terminate)
