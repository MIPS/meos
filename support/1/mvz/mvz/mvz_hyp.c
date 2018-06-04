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
*   Description:    ELF loader
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/inttypes.h"
#include <string.h>

#ifdef CONFIG_DEBUG_PARANOIA
#include "meos/debug/dbg.h"
PARATYPE(MVZT, MVZ_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
#endif

MVZ_T *_MVZ;

void MVZ_exception(int32_t intNum, struct gpctx *ctx);
void MVZ_tlbx(int32_t intNum, struct gpctx *ctx);
int32_t _IRQ_VZE(void);

void MVZ_hypervise(MVZ_T * mvz, KRN_ISRFUNC_T * hypcall)
{
	PARACHECK();
	PARADEL(MVZT, mvz);

	const reg_t c4 = mips32_getconfig4();
	_MVZ = mvz;
#ifdef CONFIG_LWIP
	/* Initialise list of network devices */
	LST_init(&mvz->nets);
#endif
	/* Initialise hypercall handler */
	mvz->hyper = (void *)hypcall;
	/* Initialise guest configuration */
	mips32_set_c0(C0_GUESTCTL0, GUESTCTL0_CP0 |
		      GUESTCTL0_RI | (3 << GUESTCTL0_AT_SHIFT) |
		      GUESTCTL0_CG | GUESTCTL0_CF |
		      GUESTCTL0_SFC2 | GUESTCTL0_SFC1);
	mips32_set_c0(C0_GUESTCTL0EXT, GUESTCTL0EXT_CGI);
	/* Find the max GuestID */
	mips32_set_c0(C0_GUESTCTL1, 0xff);
	_MVZ->maxgid = mips32_get_c0(C0_GUESTCTL1) & GUESTCTL1_ID;
	mips32_set_c0(C0_GUESTCTL1, 0);
	/* Find the max PageMask */
	mips32_set_c0(C0_PAGEMASK, 0x1fffe000);	/* Max out at 256M */
	_MVZ->maxpm = mips32_get_c0(C0_PAGEMASK);
	/* Decipher the number of root TLBs */
	mvz->ntlbs =
	    ((mips32_getconfig1() & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;
	switch ((c4 & CFG4_MMUED) >> CFG4_MMUED_SHIFT) {
	case 1:
		mvz->ntlbs += ((c4 & CFG4_MMUSE_MASK) >> CFG4_MMUSE_SHIFT)
		    << CFG1_MMUS_BITS;
		break;
	case 3:
		mvz->ntlbs +=
		    ((c4 & CFG4_VTLBSEXT_MASK) >>
		     CFG4_VTLBSEXT_SHIFT) << CFG1_MMUS_BITS;
		break;
	default:
		break;
	}
	/* Assume the guest provides the same until explicitly split */
	_MVZ->ngtlbs = _MVZ->ntlbs;
	/* Register Guest EXCeption handler */
	mvz->gexc.intNum = 0x8000 + EXC_GEXC;
	mvz->gexc.isrFunc = (void (*)(int32_t))MVZ_exception;
	IRQ_route(&mvz->gexc);
	mvz->tlexc.intNum = 0x8000 + EXC_TLBL;
	mvz->tlexc.isrFunc = (void (*)(int32_t))MVZ_tlbx;
	IRQ_route(&mvz->tlexc);
	mvz->tsexc.intNum = 0x8000 + EXC_TLBS;
	mvz->tsexc.isrFunc = (void (*)(int32_t))MVZ_tlbx;
	IRQ_route(&mvz->tsexc);
	/* Initialise root VZ context prototype */
	_vzrootctx_save(&mvz->protoroot);
	mvz->protoroot.GuestCtl0 |= GUESTCTL0_GM;
	mvz->protoroot.GuestCtl1 &= ~GUESTCTL1_ID;
	/* Initialise guest VZ context prototype */
	memset((void *)&mvz->protoguest, 0, sizeof(struct vzguestctx));
	mvz->protoguest.link.id = LINKCTX_TYPE_VZGUEST;
	mvz->protoguest.Status = SR_ERL | SR_BEV;
	mvz->protoguest.EBase = 0x80000000;
	mvz->protoguest.IntCtl =
	    (7 << INTCTL_IPTI_SHIFT) | (7 << INTCTL_IPPCI_SHIFT) | (7 <<
								    INTCTL_IPFDC_SHIFT);
	do {
		mips32_set_gc0(C0_CONFIG, mips32_getconfig() & (CFG0_M |
								CFG0_MT_MASK));
		mvz->protoguest.Config = (mips32_get_gc0(C0_CONFIG) & ~7) | 5;	/* FIXME: KS0 CCA */
		if (!(mvz->protoguest.Config & CFG0_M))
			break;

		mips32_set_gc0(C0_CONFIG1, mips32_getconfig1() & (CFG1_M |
								  CFG1_MMUS_MASK
								  | CFG1_CA |
								  CFG1_FP));
		/* !C2 | !MD | !PC | !WR */
		mvz->protoguest.Config1 = mips32_get_gc0(C0_CONFIG1);
		if (!(mvz->protoguest.Config1 & CFG1_M))
			break;

		mips32_set_gc0(C0_CONFIG2, mips32_getconfig2() & CFG2_M);
		mvz->protoguest.Config2 = mips32_get_gc0(C0_CONFIG2);
		if (!(mvz->protoguest.Config2 & CFG2_M))
			break;

		mips32_set_gc0(C0_CONFIG3, mips32_getconfig3() & (CFG3_M |
								  CFG3_BPG |
								  CFG3_ULRI |
								  CFG3_DSP2P |
								  CFG3_DSPP |
								  CFG3_CTXTC |
								  CFG3_VEIC |
								  CFG3_VI |
								  CFG3_SP));
		/* !CFG3_ITL | !CFG3_LPA | !CFG3_CDMM | !CFG3_MT | CFG3_SM | CFG3_TL */
		mvz->protoguest.Config3 = mips32_get_gc0(C0_CONFIG3);
		if (!(mvz->protoguest.Config3 & CFG1_M))
			break;

		mips32_set_gc0(C0_CONFIG4, mips32_getconfig4() & (CFG4_M |
								  CFG4_VTLBSEXT_MASK
								  |
								  CFG4_MMUSE_MASK));
		mvz->protoguest.Config4 = mips32_get_gc0(C0_CONFIG4);
		if (!(mvz->protoguest.Config4 & CFG1_M))
			break;

		mips32_set_gc0(C0_CONFIG5,
			       mips32_getconfig5() & (CFG5_UFR | CFG5_MSAEN));
		mvz->protoguest.Config5 = mips32_get_gc0(C0_CONFIG5);
	} while (0);
	if (mvz->protoguest.Config3 & CFG3_SC) {
		mvz->protoguest.SegCtl0 = mips32_get_c0(C0_SEGCTL0);
		mvz->protoguest.SegCtl1 = mips32_get_c0(C0_SEGCTL1);
		mvz->protoguest.SegCtl2 = mips32_get_c0(C0_SEGCTL2);
	}
	mips_size_cache();
	mvz->dealias =
	    ((mips_dcache_size / mips_dcache_ways) -
	     1) & ~((CONFIG_MVZ_PAGESIZE * 2) - 1);
	_IRQ_VZE();

	PARAADD(MVZT, mvz);
	PARACHECK();
}

void MVZ_splitTLBs(size_t root)
{
	PARACHECK();

	size_t guest = _MVZ->ntlbs - root;
	struct vzguestctxmax *pg = &_MVZ->protoguest;
	const reg_t c4 = mips32_getconfig4();
	reg_t sz, ext;
	const reg_t mask = (1 << CFG1_MMUS_BITS) - 1;

	/* Sanity check */
	DBG_assert(root <= _MVZ->ntlbs,
		   "Not enough root TLBs! Tried to reserve %" PRIu32
		   ", only %" PRIu32 " available!\n",
		   (uint32_t) root, (uint32_t) _MVZ->ntlbs);
	/* Wire root entries */
	mips32_set_c0(C0_WIRED, root);
	/* Write the remainder into GCP0 */
	_MVZ->ngtlbs = guest;
	sz = (guest - 1) & mask;
	ext = ((guest - 1) & ~mask) >> CFG1_MMUS_BITS;
	pg->Config1 = (pg->Config1 & ~CFG1_MMUS_MASK) | (sz << CFG1_MMUS_SHIFT);
	if (ext)
		switch ((c4 & CFG4_MMUED) >> CFG4_MMUED_SHIFT) {
		case 1:
			DBG_assert((ext & ~((1 << CFG4_MMUSE_BITS) - 1))
				   == 0,
				   "Tried to allocate more guest TLBs than C0_CONFIG4 allows!\n");
			pg->Config4 =
			    (pg->Config4 & ~CFG4_MMUSE_MASK) | (ext <<
								CFG4_MMUSE_SHIFT);
			break;
		case 3:
			DBG_assert((ext &
				    ~((1 << CFG4_VTLBSEXT_BITS) - 1)) ==
				   0,
				   "Tried to allocate more guest TLBs than C0_CONFIG4 allows!\n");
			pg->Config4 =
			    (pg->Config4 & ~CFG4_VTLBSEXT_MASK) | (ext
								   <<
								   CFG4_VTLBSEXT_SHIFT);
			break;
		default:
			break;
		}

	PARACHECK();
}
