####(C)2011###############################################################
#
# Copyright (C) 2011 MIPS Tech, LLC
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
####(C)2011###############################################################
#
#   Description:        Interrupt total latency test
#
##########################################################################

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include "cache.h"

#define start0  	t0
#define	modstatus	t2
#define modcause	t3
#define end0		s0
#define	diff0		s3
#define savedstatus	s5
#define savedcause	s6
#define isrbase		s2
#define	epc			s4

NESTED(testtoupeetot, 0, ra)
	.set    push
	.set    noat
	#include "inttot_core.S"
	eret
	.set    pop
END(testtoupeetot)


NESTED(testTotalLatency, 64, ra)
testTotalLatency:
	# Prologue
	addiu   sp,sp,-64
	sw      ra,56(sp)
	sw      fp,52(sp)
	sw      s6,48(sp)
	sw      s5,44(sp)
	sw      s4,40(sp)
	sw      s3,36(sp)
	sw      s2,32(sp)
	sw      s1,28(sp)
	sw      s0,24(sp)
	move	fp,sp
	# Prepare interupts for test
	la	epc, 3f
	mfc0	savedstatus, C0_STATUS
	lui		t4, %hi(~SR_IMASK)
	addiu	t4, t4, %lo(~SR_IMASK)
	and		modstatus, savedstatus, t4
	ori	modstatus, SR_SINT0
	mtc0	modstatus, C0_STATUS
	# Set scene
	li	t0, -4
	lw      isrbase, _EBASE
	addiu		isrbase, isrbase, 0x300
	move	a0, isrbase
	la	a1, testtoupeetot
	and	a1, a1, t0
	li	a2, 0x80
	jal 	memcpy
	li	t0, -4
	lw      isrbase, _EBASE
	addiu		isrbase, isrbase, 0x200
	move	a0, isrbase
	la	a1, testtoupeetot
	and	a1, a1, t0
	li	a2, 0x80
	jal 	memcpy
	lw      isrbase, _EBASE
	li	t0, -4
	addiu	isrbase, isrbase, 0x180
	move	a0, isrbase
	la	a1, testtoupeetot
	and	a1, a1, t0
	li	a2, 0x80
	jal 	memcpy
	# Prepare caches
	sync
	ehb
	# ISR
	move	a0, isrbase
	li	a1, 0x200
	lw 	a2, iline
	lw 	a3, dline
	_KRN_cache(a0, a1, a3, Hit_Writeback_D)
	_KRN_cache(a0, a1, a2, Hit_Invalidate_I)
	_KRN_cache(a0, a1, a2, Fill_I)
	# test
	la	a0, 1f
	la	a1, 2f
	subu	a1, a1, a0
	_KRN_cache(a0, a1, a2, Hit_Invalidate_I)
	_KRN_cache(a0, a1, a2, Fill_I)
	# Prepare to trigger
	mfc0	savedcause, C0_CAUSE
	li	a0, ~CR_IMASK
	and	modcause, savedcause, a0
	ori	modcause, modcause, CR_IP0
	la	t1, 1f
	jr.hb	t1
1:
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	mfc0 	start0, C0_PERFCNT, 1 # Cycles
	# Trigger interrupt
	mtc0	modcause, C0_CAUSE
	# Wait for return from interrupt
3:	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	mfc0 	end0, C0_PERFCNT, 1	# Cycles
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
2:
	# Compute elapsed time
	subu	diff0, end0, start0
	# Reset scene
	di
	lw      isrbase, _EBASE
	addiu	isrbase, isrbase, 0x180
	move	a0, isrbase
	la	a1, TIMING_before
	li	a2, 0x200
	jal 	memcpy
	move	a0, isrbase
	li	a1, 0x200
	lw 	a2, iline
	lw 	a3, iline
	_KRN_cache(a0, a1, a3, Hit_Writeback_D)
	_KRN_cache(a0, a1, a2, Hit_Invalidate_I)
	# Restore interrupts
	mtc0	savedcause, C0_CAUSE
	mtc0	savedstatus, C0_STATUS
	move	v0, diff0
	# Epilogue
	move	sp,fp
	lw      ra,56(sp)
	lw      fp,52(sp)
	lw      s6,48(sp)
	lw      s5,44(sp)
	lw      s4,40(sp)
	lw      s3,36(sp)
	lw      s2,32(sp)
	lw      s1,28(sp)
	lw      s0,24(sp)
	addiu   sp,sp,64
	jr 		ra
END(testTotalLatency)
