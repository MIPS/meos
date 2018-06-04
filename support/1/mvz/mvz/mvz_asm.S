/***(C)2016***************************************************************
*
* Copyright (C) 2016 MIPS Tech, LLC
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
****(C)2016**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Misc MVZ assembler functions
*
*************************************************************************/

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>
#include <mips/m32tlb.h>
#include <mips/asm.h>

.set virt

/*
 * int _mips_gtlbprobeint(tlbhi_t *hi, tlblo_t *lo0, tlblo_t *lo1,
 *      unsigned int *mask)
 *
 * Probes the GTLB for an entry matching *hi and returns its index, or -1 if
 * not found. If found, then the EntryHi, EntryLo0, EntryLo1 and PageMask parts
 * of the entry are also returned in *hi, *plo0, *plo1 and *pmsk respectively.
 *
 */
LEAF(_mips_gtlbprobeint)

	di	t8
	mfgc0	t0, C0_ENTRYHI		# save all TLB config and prep for probe
	lw	v0, 0(a0)
	mtgc0	v0, C0_ENTRYHI
	mfgc0	t3, C0_ENTRYLO0
	mfgc0	ta0, C0_ENTRYLO1
	mfgc0	ta1, C0_PAGEMASK
	mfgc0	ta2, C0_INDEX
	ehb				# mtc0, hazard on tlbp

	tlbgp
	ehb				# tlpb, hazard on index.

	mfgc0	v0, C0_INDEX
	bltz	v0, 1f                  # Return -1 if not found.

	tlbgr
	ehb                             # tlbr, hazard on entry*, pagemask

	mfgc0	ta3, C0_ENTRYHI
	mfgc0	v1, C0_ENTRYLO0
	mfgc0	t1, C0_ENTRYLO1
	mfgc0	t2, C0_PAGEMASK

	mtgc0	t0, C0_ENTRYHI          # restore TLB config
	mtgc0	t3, C0_ENTRYLO0
	mtgc0	ta0, C0_ENTRYLO1
	mtgc0	ta1, C0_PAGEMASK
	mtgc0	ta2, C0_INDEX

	sw	ta3, 0(a0)
	sw	v1, 0(a1)
	sw	t1, 0(a2)
	sw	t2, 0(a3)
	mtc0	t8, C0_STATUS
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set    pop

1:      mtgc0	t0, C0_ENTRYHI          # restore TLB config
	mtgc0	t3, C0_ENTRYLO0
	mtgc0	ta0, C0_ENTRYLO1
	mtgc0	ta1, C0_PAGEMASK
	mtgc0	ta2, C0_INDEX
	mtc0	t8, C0_STATUS
	li	v0, -1
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop
END(_mips_gtlbprobeint)
