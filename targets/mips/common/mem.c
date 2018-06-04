/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	MIPS TLB specialisation
*
*************************************************************************/

#include "meos/debug/dbg.h"
#include "meos/inttypes.h"
#include "meos/mem/mem.h"
#include <mips/cpu.h>

/*
** FUNCTION:	MEM_v2p
**
** DESCRIPTION:	Convert virtual to physical
**
** RETURNS:	uintptr_t
*/
uintptr_t MEM_v2p(void *vaddr)
{
	DBG_assert(IS_KVA01(vaddr), "%08" PRIx32 " is outside KSEG0/1\n",
		   (uint32_t) vaddr);
	return KVA_TO_PA(vaddr);
}

/*
** FUNCTION:	MEM_p2v
**
** DESCRIPTION:	Convert physical to virtual
**
** RETURNS:	void *
*/
void *MEM_p2v(uintptr_t paddr, MEM_P2V_VIEW_T view)
{
	DBG_assert(paddr <= KSEG0_SIZE,
		   "%08" PRIx32 " is unaddressable via KSEG0/1 (%08" PRIx32
		   ")\n", (uint32_t) paddr, (uint32_t) KSEG0_SIZE);
	if (paddr >= KSEG0_SIZE)
		return NULL;
	switch (view) {
	case MEM_P2V_CACHED:
		return PA_TO_KVA0(paddr);
		break;
	case MEM_P2V_UNCACHED:
		return PA_TO_KVA1(paddr);
		break;
	default:
		return NULL;
	}
}

/*
** FUNCTION:	MEM_map
**
** DESCRIPTION:	Map physical into virtual
**
** RETURNS:	int32_t
*/
int32_t MEM_map(uintptr_t paddr, size_t length, void *vaddr)
{
	DBG_assert(KVA_TO_PA(vaddr) == paddr,
		   "Mapping %08" PRIx32 ":%" PRIu32 " to %08" PRIx32
		   " requires a TLB manager\n", (uint32_t) paddr,
		   (uint32_t) length, (uint32_t) vaddr);
	if (KVA_TO_PA(vaddr) != paddr)
		return 0;
	DBG_assert(paddr + length < KSEG0_SIZE,
		   "Mapping %08" PRIx32 ":%" PRIu32 " to %08" PRIx32
		   " exceeds KSEG0/1 by %" PRIu32 " bytes\n", (uint32_t) paddr,
		   (uint32_t) length, (uint32_t) vaddr,
		   (uint32_t) (paddr + length - (KSEG0_SIZE - 1)));
	if (paddr + length >= KSEG0_SIZE)
		return 0;
	/* If it's in KSEG0/1, pretend it succeeded */
	return 1;
}

/* Work around bad section placement in old tools */
int (*m_tlbprobe2) (tlbhi_t, tlblo_t *, tlblo_t *, unsigned *) =
    (int (*)(tlbhi_t, tlblo_t *, tlblo_t *, unsigned int *))&mips_tlbprobe2;
void (*m_tlbri2) (tlbhi_t *, tlblo_t *, tlblo_t *, unsigned *, int) =
    (void (*)(tlbhi_t *, tlblo_t *, tlblo_t *, unsigned int *, int))
    &mips_tlbri2;

int32_t MEM_revmap(void *vaddr, size_t length, MEM_REVMAPFUNC_T cb, void *cbPar)
{
	/* Loop control */
	int32_t index;
	uintptr_t eaddr = (uintptr_t) vaddr + length;

	/* Input to TLB probe */
	uint32_t asid;
	reg_t dummy;
	tlbhi_t hi;
	tlblo_t lo0, lo1;
	unsigned msk;

	/* Output from TLB probe */
	uintptr_t addr = (uintptr_t) vaddr;
	uintptr_t vpaddr, ppaddr;
	size_t pagesize;

	/* Get the local ASID */
	asid = _m32c0_mfc0(C0_ENTRYHI, 0) & C0_ENTRYHI_ASID_MASK;

	/* While we can successfully probe and haven't finished reverse mapping */
	while (((index = m_tlbprobe2((tlbhi_t)
				     ((addr & C0_ENTRYHI_VPN2_MASK) | asid),
				     (tlblo_t *) & dummy, (tlblo_t *) & dummy,
				     (unsigned *)&dummy)) != -1)
	       && (addr < eaddr)) {

		/* Read the probed mapping */
		m_tlbri2(&hi, &lo0, &lo1, &msk, index);
		/* Decode it's size */
		switch (msk) {
		case 0x00000000:
			pagesize = 0x1000;
			break;
		case 0x00006000:
			pagesize = 0x4000;
			break;
		case 0x0001e000:
			pagesize = 0x10000;
			break;
		case 0x0007e000:
			pagesize = 0x40000;
			break;
		case 0x001fe000:
			pagesize = 0x100000;
			break;
		case 0x007fe000:
			pagesize = 0x400000;
			break;
		case 0x01ffe000:
			pagesize = 0x1000000;
			break;
		case 0x07ffe000:
			pagesize = 0x4000000;
			break;
		case 0x1fffe000:
			pagesize = 0x10000000;
			break;
		default:
			pagesize = 0;
		}
		/* Get the start of the virtual page */
		vpaddr = (hi & C0_ENTRYHI_VPN2_MASK);
		/* Work out which of the two physical pages it lays in */
		if ((addr >= vpaddr) && (addr < vpaddr + pagesize)) {
			ppaddr =
			    ((lo0 & ENTRYLO0_PFN_MASK) >> ENTRYLO0_PFN_SHIFT) <<
			    12;
			/* Continue from the end of the virtual page */
			addr = vpaddr + pagesize;
		} else {
			ppaddr =
			    ((lo1 & ENTRYLO1_PFN_MASK) >> ENTRYLO1_PFN_SHIFT) <<
			    12;
			vpaddr += pagesize;
			/* Continue from the end of the virtual page */
			addr = vpaddr + pagesize;
		}
		/* Emit the map via the callback */
		cb(ppaddr, pagesize, (void *)vpaddr, cbPar);
	}

	/* If we finished, we succeeded */
	return addr >= eaddr;
}
