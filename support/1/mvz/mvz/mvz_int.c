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

void _KRN_scheduleTask(KRN_TASK_T * task);

static void MVZ_passInt(int32_t sigNum)
{
	IRQ_DESC_T *irq = IRQ_cause(sigNum);
	if (irq) {
		KRN_TASK_T *task = (KRN_TASK_T *) irq->priv;
		//MVZ_GUEST_T *guest = (MVZ_GUEST_T *) task;
		/* Inject virtual interrupt and force schedule */
		/*guest->root.GuestCtl2 =
		   (1 << GUESTCTL2_VIP_SHIFT) << (irq->intNum - 2); */
		if ((task->reason != KRN_DEAD) && (task != _KRN_current))
			_KRN_scheduleTask(task);	/* FIXME: prioritise? */
		else
			mips32_bicsr(SR_IM0 << irq->intNum);
	}
}

#ifdef CONFIG_ARCH_MIPS_GIC
static void MVZ_passIntGIC(int32_t sigNum)
{
	IRQ_DESC_T *irq = IRQ_cause(sigNum);
	if (irq) {
		KRN_TASK_T *task = (KRN_TASK_T *) irq->priv;
		/* HW will inject interrupt, force schedule */
		if (task != _KRN_current)
			_KRN_scheduleTask(task);
	}
}
#endif

void MVZ_intMap(IRQ_DESC_T * irq, MVZ_GUEST_T * guest)
{
	PARACHECK();

	irq->priv = guest;
#ifdef CONFIG_ARCH_MIPS_GIC
	if ((mips32_getconfig() & CFG0_M)
	    && (mips32_getconfig1() & CFG1_M)
	    && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_VEIC)) {
		/* Register root handler to forward */
		irq->impSpec.guest = guest->gid;
		irq->isrFunc = MVZ_passIntGIC;
		IRQ_route(irq);
	} else
#endif
	{
		/* Configure PIP */
		guest->root.GuestCtl0 |=
		    (1 << GUESTCTL0_PIP_SHIFT) << (irq->intNum - 2);
		guest->root.GuestCtl2 |=
		    (1 << GUESTCTL2_HC_SHIFT) << (irq->intNum - 2);
		guest->intMask |= SR_IM0 << irq->intNum;
		/* Register root handler to forward */
		irq->isrFunc = MVZ_passInt;
		IRQ_route(irq);
	}

	PARACHECK();
}

void MVZ_upInt(MVZ_GUEST_T * gt, uint32_t i)
{
	PARACHECK();

	if ((mips32_getconfig() & CFG0_M)
	    && (mips32_getconfig1() & CFG1_M)
	    && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_VEIC)) {
		if (gt == (MVZ_GUEST_T *) _KRN_current)
			mips32_set_c0(C0_GUESTCTL2,
				      (mips32_get_c0(C0_GUESTCTL2) &
				       ~GUESTCTL2_GRIPL) | ((i -
							     2) <<
							    GUESTCTL2_GRIPL_SHIFT));
		else
			gt->root.GuestCtl2 =
			    (gt->root.GuestCtl2 & ~GUESTCTL2_GRIPL) | ((i -
									2) <<
								       GUESTCTL2_GRIPL_SHIFT);
	} else {
		if (gt == (MVZ_GUEST_T *) _KRN_current)
			mips32_set_c0(C0_GUESTCTL2,
				      mips32_get_c0(C0_GUESTCTL2) |
				      (1 << GUESTCTL2_VIP_SHIFT) << (i - 2));
		else
			gt->root.GuestCtl2 |=
			    (1 << GUESTCTL2_VIP_SHIFT) << (i - 2);
	}

	PARACHECK();
}

void MVZ_downInt(MVZ_GUEST_T * gt, uint32_t i)
{
	PARACHECK();

	if ((mips32_getconfig() & CFG0_M)
	    && (mips32_getconfig1() & CFG1_M)
	    && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_VEIC)) {
		/* FIXME: nop? */
	} else {
		if (gt == (MVZ_GUEST_T *) _KRN_current)
			mips32_set_c0(C0_GUESTCTL2,
				      mips32_get_c0(C0_GUESTCTL2) &
				      ~((1 << GUESTCTL2_VIP_SHIFT) << (i - 2)));
		else
			gt->root.GuestCtl2 &=
			    ~((1 << GUESTCTL2_VIP_SHIFT) << (i - 2));
	}

	PARACHECK();
}

reg_t MVZ_intSet(MVZ_GUEST_T * gt, uint32_t i)
{
	PARACHECK();

	reg_t r = 0;

	if ((mips32_getconfig() & CFG0_M)
	    && (mips32_getconfig1() & CFG1_M)
	    && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_VEIC)) {
		if (gt == (MVZ_GUEST_T *) _KRN_current)
			r = i ==
			    ((mips32_get_c0(C0_GUESTCTL2) & GUESTCTL2_GRIPL) >>
			     GUESTCTL2_GRIPL_SHIFT);
		else
			r = i ==
			    ((gt->root.GuestCtl2 & GUESTCTL2_GRIPL) >>
			     GUESTCTL2_GRIPL_SHIFT);
	} else {
		if (gt == (MVZ_GUEST_T *) _KRN_current)
			r = !!(mips32_get_c0(C0_GUESTCTL2) &
			       ((1 << GUESTCTL2_VIP_SHIFT) << (i - 2)));
		else
			r = !!(gt->root.GuestCtl2 & ((1 << GUESTCTL2_VIP_SHIFT)
						     << (i - 2)));
	}

	PARACHECK();
	return r;
}
