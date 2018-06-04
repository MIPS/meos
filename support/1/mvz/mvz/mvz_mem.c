/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Guest memory operations
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/inttypes.h"
#include <string.h>

#ifdef CONFIG_DEBUG_PARANOIA
#include "meos/debug/dbg.h"
PARATYPE(PTET, MVZ_PTE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
#endif

static inline reg_t isr6()
{
	return (mips32_getconfig0() & CFG0_AR_MASK) == (2 << CFG0_AR_SHIFT);
}

#ifdef CONFIG_MVZ_TLB_REFILL
void MVZ_addMapping(MVZ_PTE_T * pte, uintptr_t pa, size_t len,
		    MVZ_GUEST_T * guest, uintptr_t gpa, MVZ_MEM_FLAG_T flags)
{
	PARACHECK();
	PARADEL(PTET, pte);

	const uintptr_t pm = ((CONFIG_MVZ_PAGESIZE * 2) - 0x2000) | 0x1fff;

	DBG_assert(_MVZ->maxpm, "MVZ_addMapping() requires a TLB!\n");
	DBG_assert((pa & pm) == 0,
		   "MVZ_addMapping(paddr) is not at a double page boundary!\n");
	DBG_assert((gpa & pm) == 0,
		   "MVZ_addMapping(gpaddr) is not at a double page boundary!\n");
	DBG_assert((len & pm) == 0,
		   "MVZ_addMapping(length) is not a multiple of a double page size!\n");
	pte->start = gpa;
	pte->stop = gpa + len - 1;
	pte->paddr = pa;
	pte->flags = flags;
	LST_add(&guest->mappings, pte);

	PARAADD(PTET, pte);
	PARACHECK();
}
#else
static inline reg_t largest_possible_pm(reg_t pa)
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
		mask = 0xffffffff << shift;
	} else
		mask = 0;
	return _MVZ->maxpm & ~mask;
}

/* Compute the next largest page mask for a given page mask */
static inline reg_t next_pm(reg_t pm)
{
	if (pm != 0)		/* 4K page */
		return ((pm << 2) | pm) & _MVZ->maxpm;
	else
		return 0x6000;	/* 16K page */
}

void MVZ_fixMapping(uint32_t * index, uintptr_t pa, size_t len,
		    MVZ_GUEST_T * guest, uintptr_t gpa, MVZ_MEM_FLAG_T flags)
{
	PARACHECK();

	reg_t ihi;
	reg_t ctl1;
	uint8_t newgid = 0;

	reg_t p_bigmask, gp_bigmask;
	reg_t p_nextmask, gp_nextmask;
	reg_t pagemask;
	reg_t pagesize;
	reg_t distance, p_distance, gp_distance;
	reg_t target;

	reg_t pa2;
	reg_t hi;
	reg_t lf;
	reg_t l0;
	reg_t l1;

	DBG_assert(_MVZ->maxpm, "MVZ_fixMapping() requires a TLB!\n");
	DBG_assert((pa & 0x1fff) == 0,
		   "MVZ_fixMapping(paddr) is not at a double page boundary!\n");
	DBG_assert((gpa & 0x1fff) == 0,
		   "MVZ_fixMapping(gpaddr) is not at a double page boundary!\n");
	DBG_assert((len & 0x1fff) == 0,
		   "MVZ_fixMapping(length) is not a multiple of a double page size!\n");

	/* Sanitise GID */
	if (guest)
		newgid = guest->gid;
	/* Stash ASID/GID */
	ihi = mips32_get_c0(C0_ENTRYHI);
	ctl1 = mips32_get_c0(C0_GUESTCTL1);
	/* Set GID */
	mips32_bcs_c0(C0_GUESTCTL1, GUESTCTL1_RID,
		      guest->gid << GUESTCTL1_RID_SHIFT);
	if (!(mips32_get_c0(C0_GUESTCTL0) & GUESTCTL0_RAD))
		newgid = 0;

	do {
		/* Compute the current largest page mask */
		p_bigmask = largest_possible_pm(pa);
		gp_bigmask = largest_possible_pm(gpa);
		/* Compute the next largest pagesize */
		p_nextmask = next_pm(p_bigmask);
		gp_nextmask = next_pm(gp_bigmask);
		/*
		 * Compute the distance from our current physical address to the next page
		 * boundary.
		 */
		p_distance = (p_nextmask + 0x2000) - (pa & p_nextmask);
		gp_distance = (gp_nextmask + 0x2000) - (gpa & gp_nextmask);
		distance =
		    (p_distance < gp_distance) ? p_distance : gp_distance;
		/*
		 * Decide between searching to get to the next highest page boundary or
		 * finishing.
		 */
		target = distance < len ? distance : len;
		/* Fit */
		while (target) {
			/* Find the largest supported page size that will fit */
			for (pagesize = _MVZ->maxpm + 0x2000;
			     (pagesize > 0x2000) && (pagesize > target);
			     pagesize /= 4) ;
			/* Convert it to a page mask */
			pagemask = pagesize - 0x2000;
			/* Write it to the TLB */
			pa2 = pa + (pagesize / 2);
			hi = (gpa & C0_ENTRYHI_VPN2_MASK) | newgid;
			lf = (flags &
			      (ENTRYLO0_RI_MASK | ENTRYLO0_D_MASK |
			       ENTRYLO0_XI_MASK)) ^ ENTRYLO0_D_MASK;
			l0 = ((pa >> 6) & ENTRYLO0_PFN_MASK) | (5 <<
								ENTRYLO0_C_SHIFT)
			    | lf | ENTRYLO0_V_MASK;
			l1 = ((pa2 >> 6) & ENTRYLO1_PFN_MASK) | (5 <<
								 ENTRYLO1_C_SHIFT)
			    | lf | ENTRYLO1_V_MASK;
			/* Generate TLB entry, setting ASID in the process */
			mips_tlbwi2(hi, l0, l1, pagemask, *index);
			/* Move to next step */
			(*index)++;
			len -= pagesize;
			pa += pagesize;
			gpa += pagesize;
			target -= pagesize;
		}
	} while (len);

	/* Reset ASID/GID */
	mips32_set_c0(C0_ENTRYHI, ihi);
	mips32_set_c0(C0_GUESTCTL1, ctl1);

	PARACHECK();
}
#endif

#if 0
static inline void invalidate(uint32_t index)
{
	reg_t hi = 0x80000000;
	reg_t l0;
	reg_t l1;
	reg_t pagemask;
	while (mips_tlbprobe2(hi, &l0, &l1, &pagemask) != -1)
		hi += 0x2000;
	mips_tlbwi2(hi, 0, 0, 0, index);
}

void MVZ_unmap(uint32_t * index, MVZ_GUEST_T * guest)
{
	reg_t ihi;
	reg_t ctl1;
	reg_t gid;
	reg_t hi;
	reg_t l0;
	reg_t l1;
	reg_t pagemask;
	reg_t i, j;

	DBG_assert(_MVZ->maxpm, "MVZ_unmap() requires a TLB!\n");

	/* Stash ASID/GID */
	ihi = mips32_get_c0(C0_ENTRYHI);
	ctl1 = mips32_get_c0(C0_GUESTCTL1);

	/* Scan table */
	for (i = 0; i < *index; i++) {
		/* Find entries that match the specified guest */
		mips_tlbri2(hi, l0, l1, pagemask, i);
		if (mips32_get_c0(C0_GUESTCTL0) & GUESTCTL0_RAD)
			gid =
			    (hi & C0_ENTRYHI_ASID_MASK) >>
			    C0_ENTRYHI_ASID_SHIFT;
		else
			gid =
			    (mips32_get_c0(C0_GUESTCTL1) &
			     GUESTCTL1_RID) >> GUESTCTL1_RID_SHIFT;
		/* On finding one */
		if (gid == guest->gid) {
			/* Shunt the rest of the table into the hole */
			for (j = i; j < *index; j++) {
				/* Doing a special dance to ensure the TLB entries stay unique */
				mips_tlbri2(&hi, &l0, &l1, &pagemask, j + 1);
				invalidate(j + 1);
				mips_tlbwi2(hi, l0, l1, pagemask, j);
			}
			*index--;
		}
	}

	/* Reset ASID/GID */
	mips32_set_c0(C0_ENTRYHI, ihi);
	mips32_set_c0(C0_GUESTCTL1, ctl1);
}
#endif

enum {
	MVZ_CACHE_OP = 0,
	MVZ_CACHEE_OP = 0x20,
	MVZ_READ_OP = 0x40,
	MVZ_WRITE_OP,
	MVZ_STRLEN_OP,
	MVZ_ZERO_OP
};

#define	KSEG3_BASE	0xe0000000
#define	KSEG3_SIZE	0x20000000

extern int _mips_gtlbprobeint(tlbhi_t * hi, tlblo_t * lo0,
			      tlblo_t * lo1, unsigned int *mask);

static inline int mips_gtlbprobe2(MVZ_GUEST_T * guest, tlbhi_t hi,
				  tlbhi_t * hio, tlblo_t * pl0,
				  tlblo_t * pl1, unsigned *pmsk)
{
	/* Is the guest TLB resident? */
	if ((MVZ_GUEST_T *) _KRN_current == guest) {
		/* Yes - query via hardware */
		*hio = hi;
		return _mips_gtlbprobeint(hio, pl0, pl1, pmsk);
	} else {
		/* No - search context */
		reg_t i;
		for (i = 0; i < _MVZ->ngtlbs; i++) {
			/* Does EntryHi match? */
			if ((guest->tlbs.entries[i].
			     EntryHi & C0_ENTRYHI_VPN2_MASK)
			    == (hi &
				~(guest->tlbs.entries[i].PageMask | 0x1fff))
			    ) {
				*hio = guest->tlbs.entries[i].EntryHi;
				*pl0 = guest->tlbs.entries[i].EntryLo0;
				*pl1 = guest->tlbs.entries[i].EntryLo1;
				*pmsk = guest->tlbs.entries[i].PageMask;
				return i;
			}
		}
	}
	return -1;
}

inline reg_t MVZ_C3()
{
	if (!(mips32_get_gc0(C0_CONFIG) & CFG0_M))
		return 0;
	if (!(mips32_get_gc0(C0_CONFIG1) & CFG1_M))
		return 0;
	if (!(mips32_get_gc0(C0_CONFIG2) & CFG2_M))
		return 0;
	return mips32_get_gc0(C0_CONFIG3);
}

inline uintptr_t MVZ_gv2gp(MVZ_GUEST_T * guest, uintptr_t gva,
			   uintptr_t * glen, uint32_t * cca)
{
	reg_t current = (MVZ_GUEST_T *) _KRN_current == guest;
	reg_t erl =
	    (current ? mips32_get_gc0(C0_STATUS) : guest->guest.
	     Status) & SR_ERL;
	uint8_t asid;
	int32_t gindex;
	tlbhi_t ghi;
	tlblo_t gl0, gl1;
	unsigned gmsk, pmsk, hbit;
	uintptr_t gpa;

	/* GVA -> GPA */
	if ((current ? mips32_get_gc0(C0_CONFIG3) : guest->guest.
	     Config3) & CFG3_SC) {
		/* EVA */
		reg_t seg = 0;
		/* Get segment configuration */
		if (gva & 0x80000000) {
			gpa = gva & 0x1fffffff;
			if (glen)
				*glen = 0x20000000 - gpa;
			/* Top */
			switch (gva & 0x60000000) {
			case 0x00000000:
				seg =
				    (current ? mips32_get_gc0(C0_SEGCTL1) :
				     guest->guest.SegCtl1) >> 16;
				break;
			case 0x20000000:
				seg =
				    (current ? mips32_get_gc0(C0_SEGCTL1) :
				     guest->guest.SegCtl1) & 0xffff;
				break;
			case 0x40000000:
				seg =
				    (current ?
				     mips32_get_gc0(C0_SEGCTL0) :
				     guest->guest.SegCtl0) >> 16;
				if ((current ?
				     mips32_get_gc0(C0_CONFIG5) :
				     guest->guest.Config5) & CFG5_K)
					seg =
					    ((seg & ~7) | (current ?
							   mips32_get_gc0
							   (C0_CONFIG) :
							   guest->guest.Config)
					    ) & CFG0_K0MASK;
				break;
			case 0x60000000:
				seg =
				    (current ?
				     mips32_get_gc0(C0_SEGCTL0) :
				     guest->guest.SegCtl1) & 0xffff;
				break;
			}
		} else {
			gpa = gva & 0x3fffffff;
			if (glen)
				*glen = 0x40000000 - gpa;
			/* Bottom */
			if (gva & 0x40000000)
				(current ? mips32_get_gc0(C0_SEGCTL2) :
				 guest->guest.SegCtl2) & 0xffff;
			else
				(current ? mips32_get_gc0(C0_SEGCTL2) :
				 guest->guest.SegCtl2) >> 16;
		}
		/* Check if mapped */
		if ((!(((seg & 0x70) >= 1) && ((seg & 0x70) <= 3)))
		    || ((seg & 0x8) && erl)) {
			/* Unmapped, for whatever reason */
			gpa += (seg & 0xfe00) << 16;
			if (cca) {
				if (erl)
					*cca = 2;
				else
					*cca = seg & 7;
			}
			/* Static map */
			return gpa;
		}
	}

	else {
		/* Traditional */
		if ((gva & 0xc0000000) == 0x80000000) {
			/* KSEG0/1 - static mapping */
			gpa = gva & 0x1fffffff;
			if (glen)
				*glen = KSEG0_SIZE - gpa;
			if (cca) {
				if ((gva & 0xe0000000) == 0x80000000)
					*cca =
					    (current ?
					     mips32_get_gc0(C0_CONFIG) :
					     guest->guest.Config) & CFG0_K0MASK;
				else
					*cca = 2;
			}
			return gpa;
		} else if (erl && gva < 0x80000000) {
			/* ERL mapping */
			gpa = gva;
			*glen = 0x800000000 - gva;
			if (cca)
				*cca = 2;
			return gpa;
		}
	}

	/* TLB mapped */
	/* Get the current guest ASID */
	if (current)
		asid =
		    (uint8_t) (mips32_get_gc0
			       (C0_ENTRYHI) & C0_ENTRYHI_ASID_BITS);
	else
		asid = (uint8_t) (guest->guest.EntryHi & C0_ENTRYHI_ASID_BITS);
	/* Probe the GTLB */
	gindex =
	    mips_gtlbprobe2(guest,
			    (gva &
			     C0_ENTRYHI_VPN2_MASK) |
			    asid, &ghi, &gl0, &gl1, &gmsk);
	if (gindex < 0)
		return -1;
	pmsk = gmsk | 0x1fff;
	hbit = ((gmsk << 1) | 0x3fff) ^ pmsk;
	gpa = (((hbit ? gl1 : gl0) & ENTRYLO0_PFN_MASK) << 12) | (gva & pmsk);
	if (glen)
		*glen = (gmsk + 0x2000) - (gva - (ghi & C0_ENTRYHI_VPN2_MASK));
	if (cca)
		*cca =
		    ((hbit ? gl1 : gl0) & ENTRYLO0_C_MASK) >> ENTRYLO0_C_SHIFT;
	return gpa;
}

typedef enum {
	VIRTUAL = 0,
	UNCACHED = 2,
	CACHED = 5
} MVZ_VCCA_T;

static inline reg_t _MVZ_opGuest(MVZ_GUEST_T * guest,
				 MVZ_VCCA_T vcca,
				 uintptr_t addr, void *buf,
				 size_t size, int32_t op)
{
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	size_t rem = size, window, rlen, glen;
	uintptr_t coff = 0, roff, gpa;
	int result = 0;
	reg_t cca = 2;
	while (rem) {
		/* GVA -> GPA */
		if (vcca == VIRTUAL) {
			coff = addr & _MVZ->dealias;
			gpa = MVZ_gv2gp(guest, addr, &glen, (uint32_t *) & cca);
			if (gpa == -1)
				break;
		} else {
			gpa = addr;
			glen = (size_t) - 1;
			cca = vcca;
		}
		/* GPA->RPA - find the RTLB entry for the guest */
		mips32_bcs_c0(C0_GUESTCTL1, GUESTCTL1_RID,
			      guest->gid << GUESTCTL1_RID_SHIFT);
#ifndef CONFIG_MVZ_TLB_REFILL
		uint8_t qgid = guest->gid;
		reg_t dummy;
		int32_t rindex;
		tlbhi_t rhi;
		tlblo_t rl0, rl1;
		unsigned rmsk;
		if (!(mips32_get_c0(C0_GUESTCTL0) & GUESTCTL0_RAD))
			qgid = 0;
		rindex =
		    mips_tlbprobe2(gpa | qgid, (tlblo_t *) & dummy,
				   (tlblo_t *) & dummy, (unsigned *)&dummy);
		if (rindex >= 0) {
			/* In TLB */
			mips_tlbri2(&rhi, &rl0, &rl1, &rmsk, rindex);
			coff &= ~rmsk;
			roff = (gpa - (rhi & C0_ENTRYHI_VPN2_MASK));
			rlen = (rmsk + 0x2000) - roff;
			mips32_bc_c0(C0_GUESTCTL1, GUESTCTL1_RID);
			/* Replace with RTLB mapping for root from the start of KUSEG */
			mips_tlbwi2(KSEG3_BASE + coff,
				    (rl0 & ~ENTRYLO0_C_MASK) | (cca <<
								ENTRYLO0_C_SHIFT),
				    (rl1 & ~ENTRYLO1_C_MASK) | (cca <<
								ENTRYLO1_C_SHIFT),
				    rmsk, rindex);
		}
#else
		/* Check if in PTEs */
		MVZ_PTE_T *pte;
		for (pte = (MVZ_PTE_T *) LST_first(&guest->mappings);
		     pte && (((pte->start & 0x1) == 1)
			     || (gpa < pte->start)
			     || (gpa > pte->stop));
		     pte = (MVZ_PTE_T *) LST_next((LST_T *) pte)) ;
		if (!pte)
			break;
		/* In PTEs */
		const uintptr_t pm = ((CONFIG_MVZ_PAGESIZE * 2) - 0x2000);
		roff = gpa & ((~C0_ENTRYHI_VPN2_MASK) | pm);
		rlen = (2 * CONFIG_MVZ_PAGESIZE) - roff;
		uintptr_t goff = gpa - pte->start;
		reg_t pa = pte->paddr + goff - roff;
		reg_t pa2 = pa + CONFIG_MVZ_PAGESIZE;
		reg_t lo0 =
		    ((pa >> 6) & ENTRYLO0_PFN_MASK) | (5 << ENTRYLO0_C_SHIFT)
		    | ENTRYLO0_D_MASK | ENTRYLO0_V_MASK;
		reg_t lo1 =
		    ((pa2 >> 6) & ENTRYLO0_PFN_MASK) | (5 << ENTRYLO0_C_SHIFT)
		    | ENTRYLO0_D_MASK | ENTRYLO0_V_MASK;
		mips32_bc_c0(C0_GUESTCTL1, GUESTCTL1_RID);
		mips_tlbwr2(KSEG3_BASE + coff, lo0, lo1, pm);
#endif
		/* Compute overlapping window */
		if (glen < rlen)
			window = glen;
		else
			window = rlen;
		if (window > rem)
			window = rem;
		/* Do the op */
		switch (op) {
		case MVZ_READ_OP:
			memcpy((void *)buf, (void *)(KSEG3_BASE + coff + roff),
			       window);
			result = size - (rem - window);
			break;
		case MVZ_WRITE_OP:
			memcpy((void *)(KSEG3_BASE + coff + roff), (void *)buf,
			       window);
			KRN_syncCache(KSEG3_BASE + coff + roff, window);
			result = size - (rem - window);
			break;
		case MVZ_STRLEN_OP:
			glen =
			    strnlen((void *)(KSEG3_BASE + coff + roff), window);
			if (glen < window) {
				result = (size - rem) + (window - glen);
				rem = window;
			}
			break;
		case MVZ_ZERO_OP:
			memset((void *)(KSEG3_BASE + coff + roff), 0, window);
			KRN_syncCache(KSEG3_BASE + coff + roff, window);
			result = size - rem;
			break;
#if 0
#define CACHEOP(N) case (MVZ_CACHE_OP+N): asm ("cache " #N ", 0(%0)"::"r"(KSEG3_BASE + coff + roff):"memory"); rem = 0; break
			CACHEOP(0);
			CACHEOP(1);
			CACHEOP(2);
			CACHEOP(3);
			CACHEOP(4);
			CACHEOP(5);
			CACHEOP(6);
			CACHEOP(7);
			CACHEOP(8);
			CACHEOP(9);
			CACHEOP(10);
			CACHEOP(11);
			CACHEOP(12);
			CACHEOP(13);
			CACHEOP(14);
			CACHEOP(15);
			CACHEOP(16);
			CACHEOP(17);
			CACHEOP(18);
			CACHEOP(19);
			CACHEOP(20);
			CACHEOP(21);
			CACHEOP(22);
			CACHEOP(23);
			CACHEOP(24);
			CACHEOP(25);
			CACHEOP(26);
			CACHEOP(27);
			CACHEOP(28);
			CACHEOP(29);
			CACHEOP(30);
			CACHEOP(31);
#undef CACHEOP
#ifdef CONFIG_EVA
#define CACHEEOP(N) case (MVZ_CACHEE_OP+N): asm ("cachee " #N ", 0(%0)"::"r"(KSEG3_BASE + coff + roff):"memory"); rem = 0; break
			CACHEEOP(0);
			CACHEEOP(1);
			CACHEEOP(2);
			CACHEEOP(3);
			CACHEEOP(4);
			CACHEEOP(5);
			CACHEEOP(6);
			CACHEEOP(7);
			CACHEEOP(8);
			CACHEEOP(9);
			CACHEEOP(10);
			CACHEEOP(11);
			CACHEEOP(12);
			CACHEEOP(13);
			CACHEEOP(14);
			CACHEEOP(15);
			CACHEEOP(16);
			CACHEEOP(17);
			CACHEEOP(18);
			CACHEEOP(19);
			CACHEEOP(20);
			CACHEEOP(21);
			CACHEEOP(22);
			CACHEEOP(23);
			CACHEEOP(24);
			CACHEEOP(25);
			CACHEEOP(26);
			CACHEEOP(27);
			CACHEEOP(28);
			CACHEEOP(29);
			CACHEEOP(30);
			CACHEEOP(31);
#undef CACHEEOP
#endif
#endif
		default:
			DBG_assert(0, "Illegal guest op %" PRId32 "!\n", op);
		}
#ifndef CONFIG_MVZ_TLB_REFILL
		/* Return RTLB mapping to guest */
		mips32_bcs_c0(C0_GUESTCTL1, GUESTCTL1_RID,
			      guest->gid << GUESTCTL1_RID_SHIFT);
		mips_tlbwi2(rhi, rl0, rl1, rmsk, rindex);
		mips32_bc_c0(C0_GUESTCTL1, GUESTCTL1_RID);
#else
		mips_tlbinval(KSEG3_BASE + coff);
#endif
		/* Move on to the next page */
		rem -= window;
		addr += window;
	}
	IRQ_restoreIPL(ipl);
	return result;
}

#if 0
void MVZ_cache(MVZ_GUEST_T * gt, reg_t eva, reg_t op, uintptr_t gvaddr)
{
	PARACHECK();
	_MVZ_opGuest(gt, VIRTUAL, gvaddr, NULL, 1,
		     (eva ? MVZ_CACHEE_OP : MVZ_CACHE_OP) + op);
	PARACHECK();
}
#endif

int MVZ_readGV(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, VIRTUAL,
			    (uintptr_t) paddr, buf, size * n, MVZ_READ_OP);
}

int MVZ_writeGV(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, VIRTUAL,
			    (uintptr_t) paddr, buf, size * n, MVZ_WRITE_OP);
}

size_t MVZ_zeroGV(MVZ_GUEST_T * guest, uintptr_t gvaddr, size_t len)
{
	return (size_t) _MVZ_opGuest(guest, VIRTUAL, gvaddr, NULL,
				     len, MVZ_ZERO_OP);
}

int MVZ_readGP(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, UNCACHED,
			    (uintptr_t) paddr, buf, size * n, MVZ_READ_OP);
}

int MVZ_writeGP(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, UNCACHED,
			    (uintptr_t) paddr, buf, size * n, MVZ_WRITE_OP);
}

size_t MVZ_zeroGP(MVZ_GUEST_T * guest, uintptr_t gvaddr, size_t len)
{
	size_t r;
	PARACHECK();
	r = (size_t) _MVZ_opGuest(guest, UNCACHED, gvaddr, NULL, len,
				  MVZ_ZERO_OP);
	PARACHECK();
	return r;
}

int MVZ_readGPC(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, CACHED,
			    (uintptr_t) paddr, buf, size * n, MVZ_READ_OP);
}

int MVZ_writeGPC(void *paddr, void *buf, int size, int n, void *priv)
{
	return _MVZ_opGuest((MVZ_GUEST_T *) priv, CACHED,
			    (uintptr_t) paddr, buf, size * n, MVZ_WRITE_OP);
}

size_t MVZ_zeroGPC(MVZ_GUEST_T * guest, uintptr_t gvaddr, size_t len)
{
	return (size_t) _MVZ_opGuest(guest, CACHED, gvaddr, NULL,
				     len, MVZ_ZERO_OP);
}

size_t MVZ_gstrlen(MVZ_GUEST_T * guest, uintptr_t gvaddr)
{
	return (size_t) _MVZ_opGuest(guest, 1, gvaddr, NULL,
				     (int)-1, MVZ_STRLEN_OP);
}

/* Transfer from a reader to a writer */
void MVZ_xfer(MVZ_XFERFUNC_T * r, void *rp, uintptr_t ro,
	      size_t size, MVZ_XFERFUNC_T * w, void *wp, uintptr_t wo)
{
	static uint8_t buf[4096];
	size_t rlen, wlen;
	/* While there's still data to transfer */
	while (size) {
		/* Work out the largest read block */
		rlen = (size < sizeof(buf)) ? size : sizeof(buf);
		/* Read the data, updating the amount, in case of short read */
		rlen = r((void *)ro, buf, rlen, 1, rp);
		/* While there's still data */
		while (rlen) {
			/* Try writing it out */
			wlen = w((void *)wo, buf, rlen, 1, wp);
			/* Update offsets and sizes to follow write */
			ro += wlen;
			wo += wlen;
			rlen -= wlen;
			size -= wlen;
}}}
