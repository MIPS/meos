/***(C)2011***************************************************************
*
* Copyright (C) 2011 MIPS Tech, LLC
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
****(C)2011**************************************************************/

/*************************************************************************
*
*   Description:	Test TLBs
*
*************************************************************************/

/*
 * This test exercises the TLBs
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mips/cpu.h>
#include <mips/m32c0.h>

reg_t maxpm()
{
	mips32_set_c0(C0_PAGEMASK, 0x1fffe000);	/* Max out at 256M */
	return mips32_get_c0(C0_PAGEMASK);
}

static reg_t largest_possible_pm(reg_t pa)
{
	reg_t mask;
	/* Find address bits limiting alignment */
	reg_t shift = __builtin_ffs(pa);
	/* Obey MIPS restrictions on page sizes */
	if (pa) {
		if (shift & 1)
			shift -= 2;
		else
			shift--;
	}
	mask = 0xffffffff << shift;
	return maxpm() & ~mask;
}

/* Compute the next largest page mask for a given page mask */
static inline reg_t next_pm(reg_t pm)
{
	if (pm != 0)		/* 4K page */
		return ((pm << 2) | pm) & maxpm();
	else
		return 0x6000;	/* 16K page */
}

void map(reg_t * index, paddr_t pa, size_t len, vaddr_t va)
{
	reg_t ihi;

	reg_t bigmask;
	reg_t nextmask;
	reg_t pagemask;
	reg_t pagesize;
	reg_t distance;
	reg_t target;

	reg_t pa2;
	reg_t hi;
	reg_t l0;
	reg_t l1;

	/* Stash ASID/GID */
	ihi = mips32_get_c0(C0_ENTRYHI);
	do {
		/* Compute the current largest page mask */
		bigmask = largest_possible_pm(pa);
		/* Compute the next largest pagesize */
		nextmask = next_pm(bigmask);
		/*
		 * Compute the distance from our current physical address to the next page
		 * boundary.
		 */
		distance = (nextmask + 0x2000) - (pa & nextmask);
		/*
		 * Decide between searching to get to the next highest page boundary or
		 * finishing.
		 */
		target = distance < len ? distance : len;
		/* Fit */
		while (target) {
			/* Find the largest supported page size that will fit */
			for (pagesize = maxpm() + 0x2000;
			     (pagesize > 0x2000) && (pagesize > target);
			     pagesize /= 4) ;
			/* Convert it to a page mask */
			pagemask = pagesize - 0x2000;
			/* Write it to the TLB */
			pa2 = pa + (pagesize / 2);
			hi = va & C0_ENTRYHI_VPN2_MASK;
			l0 = ((pa >> 6) & ENTRYLO0_PFN_MASK) | (5 <<
								ENTRYLO0_C_SHIFT)
			    | ENTRYLO0_D_MASK | ENTRYLO0_V_MASK;
			l1 = ((pa2 >> 6) & ENTRYLO1_PFN_MASK) | (5 <<
								 ENTRYLO1_C_SHIFT)
			    | ENTRYLO1_D_MASK | ENTRYLO1_V_MASK;
			/* Generate TLB entry, setting ASID in the process */
			mips_tlbwi2(hi, l0, l1, pagemask, *index);
			/* Move to next step */
			(*index)++;
			len -= pagesize;
			pa += pagesize;
			va += pagesize;
			target -= pagesize;
		}
	} while (len);

	/* Reset ASID */
	mips32_set_c0(C0_ENTRYHI, ihi);
}

void writemapped(size_t l)
{
	reg_t *p = (reg_t *) 0x10000000;
	reg_t a = 1, b = 1, i;
	l /= sizeof(reg_t);

	for (i = 0; i < l; i++) {
		p[i] = a;
		a = b;
		b = p[i] + a;
	}
}

void verifymapped(size_t l)
{
	reg_t *p = (reg_t *) 0x10000000;
	reg_t a = 1, b = 1, i;
	l /= sizeof(reg_t);

	for (i = 0; i < l; i++) {
		assert(p[i] == a);
		a = b;
		b = p[i] + a;
	}
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main()
{
	reg_t index = 0;
	map(&index, 0x1000000, 0x100000, 0x10000000);
	writemapped(0x100000);
	verifymapped(0x100000);

	return 0;
}
