####(C)2015###############################################################
#
# Copyright (C) 2015 MIPS Tech, LLC
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
####(C)2015###############################################################
#
#   Description:	UHI chaining
#
##########################################################################

#include "meos/config.h"
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include "meos/target/ctx.h"

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

#define DBG_TRACE_CHAIN_UHI 0x8000000d

# Create space to store k0, epc, and sr & ie
	.data
	.global	_chain_chn
_chain_chn:
#ifdef __mips64
#error fixme
#else
#define chain_chn_size	16
	.space	4
#define chain_chn_chain 0
#define	chain_chn_k0	4
#define	chain_chn_epc	8
#define	chain_chn_srie	12
#endif

# This is a *naughty* function, and relies on the saved registers of the
# interrupt core.
NESTED(_chain_exception, (40 + chain_chn_size), ra)
	.set	push
	.set	noat

	# Prologue
	addiu	sp, sp, -(40 + chain_chn_size)
	sw	ra, 32(sp)

	# Prepare arguments
	move 	a1, e_SIG
	andi	a1, a1, 0x7fff
	move	a0, e_CTX

	# Link to chain
	lui	t0, %hi(_chain_chn)
	addiu	t0, t0, %lo(_chain_chn)
	lw	t1, 0(t0)
	addiu	t2, sp, 40
	sw	t1, (40 + chain_chn_chain)(sp)
	sw	t2, 0(t0)

	# Stash chained context
	sw	k0, (40 + chain_chn_k0)(sp)
	ext     t1, e_IST, IST_IE, 1
	addiu	t2, e_EPC, 4
	sw	t2, (40 + chain_chn_epc)(sp)
	sw	t1, (40 + chain_chn_srie)(sp)

	# Fix context to return to context restore
	lui	t0, %hi(_chain_trampoline)
	addiu	t0, t0, %lo(_chain_trampoline)
	sw	t0, CTX_EPC(e_CTX)

	# Prepare for call to _mips_handle_exception
	lui	t0, %hi(_mips_handle_exception)
	addiu	t0, t0, %lo(_mips_handle_exception)

	# Fix up nest
	lw	t1, CTX_NEST(k0)
	addiu	t1, -1
	sw	t1, CTX_NEST(k0)

	# Call
	di
	jalr	ra, t0

	# We got back - that means we didn't eret: unlink chain
	lui	t0, %hi(_chain_chn)
	addiu	t0, t0, %lo(_chain_chn)
	lw	t1, (40 + chain_chn_chain)(sp)
	sw	t1, 0(t0)
	# Restore chaining context
	lw	k0, (40 + chain_chn_k0)(sp)
	lw	e_EPC, (40 + chain_chn_epc)(sp)
	# Restore nest
	lw	t1, CTX_NEST(k0)
	addiu	t1, 1
	sw	t1, CTX_NEST(k0)
	# Reform SR using stashed IE
	lw	t1, (40 + chain_chn_srie)(sp)
	mfc0	t2, C0_SR
	ori	t2, t2, SR_IE
	xori	t2, t2, SR_IE
	mtc0	t2, C0_SR
	# Work out EPC to use - is it pointing near the trampoline?
	lui	t0, %hi(_chain_trampoline)
	addiu	t0, t0, %lo(_chain_trampoline)
	lw	t1, CTX_EPC(e_CTX)
	addiu	t0, t0, 4
	beq	t0, t1, 1f
	# No - use the value provided in the context
	b	2f
	# Yes - restore one instruction after the original
1:	sw	e_EPC, CTX_EPC(e_CTX)

	# Epilogue
2:	lw	ra, 32(sp)
	addiu	sp, sp, (40 + chain_chn_size)
	jr	ra
	.set pop
END(_chain_exception)

# This should only ever be invoked when a boot ROM erets. Assume UHI ABI,
# giving us k0, k1, a2 and a3 to play with. This takes some juggling...
LEAF(_chain_trampoline)
	syscall 1 # This should never be executed, the ROM should increment.
	# UHI gives us
	lui	k0, %hi(_chain_chn)
	addiu	k0, k1, %lo(_chain_chn)
	lw	a3, 0(k0)
	# Unlink chain
	lw	k1, chain_chn_chain(a3)
	sw	k1, 0(k0)
	# Reform SR using stashed IE but don't load it
	lw	k1, chain_chn_srie(a3)
	mfc0	a2, C0_SR
	ori	a2, a2, SR_IE
	xori	a2, a2, SR_IE
	or	a2, a2, k1
	# Restore remaining context
	lw	k0, chain_chn_k0(a3)
	lw	a3, chain_chn_epc(a3)
	# Return
	mtc0	a2, C0_SR # Restoring the interrupt state
	jr	a3
END(_chain_trampoline)

#ifdef CONFIG_FEATURE_INTERCEPT_EXCEPTIONS
#
# FUNCTION:	_do_uhi
#
# DESCRIPTION:	Exit an interrupt, and invoke the debugger to service UHI
#
LEAF(_do_uhi)
	.weak	_do_uhi
	.set	push
	.set	noat
						#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
							move		s0, a0
							la		t0, DBG_chain
							move		a0, s0
							jalr	ra, t0
						#endif
# Get back into exception mode
	mfc0	t0, C0_SR
	ori		t0, SR_EXL
	mtc0	t0, C0_SR
#ifdef CONFIG_ARCH_MIPS_DSP
	lw		t1, DSPCTX_DSPC(a0)
	lw		t2, DSPCTX_HI1(a0)
	lw		t3, DSPCTX_LO1(a0)
	lw		t4, DSPCTX_HI2(a0)
	lw		t5, DSPCTX_LO2(a0)
	lw		t6, DSPCTX_HI3(a0)
	lw		t7, DSPCTX_LO3(a0)
	wrdsp	t1
	mthi	t2, $ac1
	mtlo	t3, $ac1
	mthi	t4, $ac2
	mtlo	t5, $ac2
	mthi	t6, $ac3
	mtlo	t7, $ac3
#endif
#ifndef CONFIG_ARCH_MIPS_R6
	lw		t1, CTX_HI0(a0)
	lw		t2, CTX_LO0(a0)
#endif
	lw		e_EPC, CTX_EPC(a0)
						#ifdef CONFIG_DEBUG_TRACE_ISR_HARD
							li		t0, DBG_TRACE_CHAIN_UHI
							mtc0	t0, $23, 3
							mtc0	a0, $24, 3
							mtc0	e_EPC, $24, 3
						#endif
#ifndef CONFIG_ARCH_MIPS_R6
	mthi	t1
	mtlo	t2
#endif
	mtc0	e_EPC, C0_EPC
	lw		$1, CTX_AT(a0)
	lw		v0, CTX_V0(a0)
	lw		v1, CTX_V1(a0)
	lw		a1, CTX_A1(a0)
	lw		a2, CTX_A2(a0)
	lw		a3, CTX_A3(a0)
	lw		t0, CTX_T0(a0)
	lw		t1, CTX_T1(a0)
	lw		t2, CTX_T2(a0)
	lw		t3, CTX_T3(a0)
	lw		t4, CTX_T4(a0)
	lw		t5, CTX_T5(a0)
	lw		t6, CTX_T6(a0)
	lw		t7, CTX_T7(a0)
	lw		s0, CTX_S0(a0)
	lw		s1, CTX_S1(a0)
	lw		s2, CTX_S2(a0)
	lw		s3, CTX_S3(a0)
	lw		s4, CTX_S4(a0)
	lw		s5, CTX_S5(a0)
	lw		s6, CTX_S6(a0)
	lw		s7, CTX_S7(a0)
	lw		t8, CTX_T8(a0)
	lw		t9, CTX_T9(a0)
	lw		gp, CTX_GP(a0)
	lw		sp, CTX_SP(a0)
	lw		fp, CTX_FP(a0)
	lw		ra, CTX_RA(a0)
	lw		a0, CTX_A0(a0)
	sdbbp	1
	eret
	.set	pop
END(_do_uhi)
#endif
