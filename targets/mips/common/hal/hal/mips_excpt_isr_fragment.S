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

#
# Keep each function in a separate named section
#define _FUNCTION_SECTIONS_

#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/cpu.h>

.set	noat
.extern	__isr_vec_fallback

#ifdef ZERO
LEAF(__isr_vec)
#else
_TEXT_SECTION_NAMED(DEF)
#endif

#define	__isr_vec_0 __isr_vec_sw0
#define	__isr_vec_1 __isr_vec_sw1
#define	__isr_vec_2 __isr_vec_hw0
#define	__isr_vec_3 __isr_vec_hw1
#define	__isr_vec_4 __isr_vec_hw2
#define	__isr_vec_5 __isr_vec_hw3
#define	__isr_vec_6 __isr_vec_hw4
#define	__isr_vec_7 __isr_vec_hw5

#define	_mips_isr_0 _mips_isr_sw0
#define	_mips_isr_1 _mips_isr_sw1
#define	_mips_isr_2 _mips_isr_hw0
#define	_mips_isr_3 _mips_isr_hw1
#define	_mips_isr_4 _mips_isr_hw2
#define	_mips_isr_5 _mips_isr_hw3
#define	_mips_isr_6 _mips_isr_hw4
#define	_mips_isr_7 _mips_isr_hw5

.weak	ISR
.extern	REF

AENT(DEF)
	mtc0	k1, C0_KSCRATCH1
	LA	k1, ISR
	beqz	k1, __isr_vec_fallback
	jr	k1
	# Padding required for successful link

	nop
#if SZPTR==8
	nop
	nop
	nop
	nop
#endif

#ifdef __mips_micromips
	nop
	nop
	nop

#if SZPTR==8
	nop
	nop
	nop
	nop
#endif
#endif

