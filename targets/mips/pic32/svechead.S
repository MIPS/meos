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
#   Description:	Root interrupt code
#
##########################################################################

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include "meos/config.h"
#include "meos/target/ctx.h"

#define DBG_TRACE_SCHED_ISR 0x80000001
#define DBG_TRACE_HOTWIRE_ISR 0x80000002

# Context size, adjusted for parameter area
#define ADJ 16
#define CTX_SIZEADJ (CTX_SIZE + ADJ)

#define	e_CTX	s0
#define	e_SIG	s1
#define e_ISR	s2
#define e_CR	s3
#define e_SRO	s4
#define e_SR	s5
#define e_EPC	s6
#define e_RA	s7

#
# FUNCTION:	_svechead
#
# DESCRIPTION:	Interrupt entry vector - modified at runtime to chain forwards
#				efficiently to _savenjump.
#
NESTED(_svechead, 0, ra)
	.set	push
	.set	noat
#ifdef CONFIG_ARCH_MIPS_VZ
	mtc0	k0, C0_KSCRATCH2
	mfc0	k0, C0_KSCRATCH1
	sw	k1, CTX_K1(k0)
	ehb
	mfc0	k1, C0_KSCRATCH2
	sw	k1, CTX_K0(k0)
#endif
	sw		a0, CTX_A0(k0)		#4
	sw		e_ISR, CTX_S2(k0)	#17
.globl _svechead1h
_svechead1h:
	lui		a0, 42
.globl _svechead1l
_svechead1l:
	lbu		a0, 42(a0)
.globl _svechead2h
_svechead2h:
	lui		e_ISR, 42
.globl _svechead2l
_svechead2l:
	ori		e_ISR, 42
	sll		k1, a0, 2
	add		e_ISR, e_ISR, k1
	la		k1, _savenjump
	lw		e_ISR, 0(e_ISR)
	jr		k1
	.set	pop
END(_svechead)
