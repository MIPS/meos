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
*          File:	$File: //meta/fw/meos2/DEV/LISA.PARRATT/kernel/sched.c $
* Revision date:	$Date: 2015/06/16 $
*   Description:	Guest scheduling utilities
*
*************************************************************************/

#include "MVZ.h"
#include <string.h>

extern void _CTX_activateFP(KRN_TASK_T * task);
extern void _hotwire(KRN_CTX_T * ctx) __attribute__ ((noreturn));
extern void _KRN_prepareTask(KRN_TASKFUNC_T * taskfunc, KRN_TASK_T * task,
			     uint32_t * uworkspace, size_t wssize,
			     KRN_PRIORITY_T * priority, void *parameter,
			     const char *taskname);

#ifdef CONFIG_BUILD_PARANOIA
#include "meos/debug/dbg.h"
PARATYPE(Task, KRN_TASK_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(Schd, KRN_SCHEDULE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
extern DQ_T _paraItem_Dque;
extern DBG_PARATYPE_T _paraDesc_DqIt, _paraDesc_Dque;
#endif

/* Do the exception handler dance! Allows us to catch a breakpoint within an
 * exception. If a breakpoint happens within an exception, the exception handler
 * will be reentered. By replacing the instruction with a branch-to-self
 * (stick), we can detect this situation on timeslice. However, we haven't
 * executed the first instruction when we enter EXL, so we have to do a little
 * shimmy: stick on the second instruction in the handler, then make the first
 * instruction sticky, unstick the second instruction, and continue.
 */

uintptr_t _mips_nextpc(struct gpctx *ctx, uint32_t one, uint32_t two);

static inline reg_t MVZ_broken(MVZ_GUEST_T * gt)
{
	uintptr_t addr = gt->task.savedContext.gp.epc;
	uint32_t brk = 0x0000000d;
	uint32_t mask = 0xfc00003f;
	size_t len = 4;
	uint32_t i;
	if (addr & 1) {
		if (mips32_getconfig1() & CFG1_CA) {
			brk = 0xe805;
			mask = 0xf81f;
			len = 2;
		} else if (mips32_getconfig3() & CFG3_ISA_MASK) {
			brk = 0x441b;
			mask = 0xfc3f;
			len = 2;
		}
	}
	MVZ_readGV((void *)addr, &i, len, 1, gt);
	return (i & mask) == brk;
}

static inline void MVZ_stick(MVZ_GUEST_T * gt, uintptr_t addr, reg_t instr,
			     reg_t offset)
{
	mips_flush_dcache();
	mips_flush_icache();
	/* FIXME: add R6 */
	if (addr & 1) {
		uint16_t stick[] = { 0xcfff, 0x0c00 };	/* b16 0; nop */
		MVZ_readGV((void *)addr, gt->danceInstr + (2 * offset),
			   2 * instr, 1, gt);
		MVZ_writeGV((void *)addr, stick, 2 * instr, 1, gt);
	} else {
		uint32_t stick[] = { 0x1000ffff, 0x00000000 };	/*b 0; nop */
		MVZ_readGV((void *)addr, gt->danceInstr + (4 * offset),
			   4 * instr, 1, gt);
		MVZ_writeGV((void *)addr, stick, 4 * instr, 1, gt);
	}
	DBG_logF("Stick %p/%08x %08x\n", (void *)addr,
		 ((unsigned int *)gt->danceInstr)[0],
		 ((unsigned int *)gt->danceInstr)[1]);
	mips_flush_dcache();
	mips_flush_icache();
}

static inline void MVZ_unstick(MVZ_GUEST_T * gt, uintptr_t addr, reg_t instr,
			       reg_t offset)
{
	/* FIXME: add R6 */
	DBG_logF("Unstick %p/%08x %08x\n", (void *)addr,
		 ((unsigned int *)gt->danceInstr)[0],
		 ((unsigned int *)gt->danceInstr)[1]);
	if (addr & 1)
		MVZ_writeGV((void *)addr, gt->danceInstr + (2 * offset),
			    2 * instr, 1, gt);
	else
		MVZ_writeGV((void *)addr, gt->danceInstr + (4 * offset),
			    4 * instr, 1, gt);
	mips_flush_dcache();
	mips_flush_icache();
}

static inline reg_t MVZ_getEBASE(MVZ_GUEST_T * gt)
{
	if (_KRN_current == &gt->task)
		return mips32_get_gc0(C0_EBASE);
	else
		return gt->guest.EBase;
}

static inline reg_t MVZ_stuck(MVZ_GUEST_T * gt)
{
	switch (gt->danceStep) {
	default:
	case 0:
		return 0;
	case 1:
		return gt->task.savedContext.gp.epc == gt->danceSecond;
	case 2:
		return ((gt->task.savedContext.gp.epc ==
			 (MVZ_getEBASE(gt) & EBASE_BASE) + 0x180)
			|| (gt->task.savedContext.gp.epc == gt->danceThird));
	case 3:
		return gt->task.savedContext.gp.epc ==
		    (MVZ_getEBASE(gt) & EBASE_BASE) + 0x180;
	}
}

void MVZ_EXLProtect(MVZ_GUEST_T * gt)
{
	reg_t ebase = MVZ_getEBASE(gt) & EBASE_BASE;
	switch (gt->danceStep) {
	case 1:
		MVZ_stick(gt, gt->danceSecond, 2, 0);
		break;
	case 2:
		MVZ_stick(gt, gt->danceThird, 2, 0);
		MVZ_stick(gt, ebase + 0x180, 1, 2);
		break;
	case 3:
		MVZ_stick(gt, ebase + 0x180, 2, 0);
		break;
	default:
		break;
	}
}

void MVZ_EXLUnprotect(MVZ_GUEST_T * gt)
{

	reg_t ebase = MVZ_getEBASE(gt) & EBASE_BASE;
	switch (gt->danceStep) {
	case 1:
		MVZ_unstick(gt, gt->danceSecond, 2, 0);
		break;
	case 2:
		MVZ_unstick(gt, gt->danceThird, 2, 0);
		MVZ_unstick(gt, ebase + 0x180, 1, 2);
		break;
	case 3:
		MVZ_unstick(gt, ebase + 0x180, 2, 0);
		break;
	default:
		break;
	}
}

void MVZ_dance(MVZ_GUEST_T * gt, sreg_t exc)
{
	uint32_t i, i2;
	DBG_logF("Dance %p/%" PRIu32 "/%" PRIx32 "\n",
		 (void *)gt->task.savedContext.gp.epc,
		 (uint32_t) gt->danceStep, (int32_t) exc);
	if (exc >= 0)
		gt->danceExc = exc;
	if ((((_KRN_current ==
	       &gt->task) ? mips32_get_gc0(C0_STATUS) : gt->guest.
	      Status) & SR_EXL) == SR_EXL) {
		/* In exception */
		MVZ_EXLUnprotect(gt);
		switch (gt->danceStep) {
		case 1:
			/* Need to do this now, because _mips_nextpc only works for current instruction */
			MVZ_readGV((void *)(gt->task.savedContext.gp.epc),
				   &i, 4, 1, gt);
			MVZ_readGV((void *)(gt->task.savedContext.gp.epc +
					    4), &i2, 4, 1, gt);
			gt->danceThird =
			    _mips_nextpc(&gt->task.savedContext.gp, i, i2);
			gt->danceStep = 2;
			break;
		case 2:
			gt->danceStep = 3;
			break;
		case 0:
			/* Need to do this now, because _mips_nextpc only works for current instruction */
			MVZ_readGV((void *)(gt->task.savedContext.gp.epc),
				   &i, 4, 1, gt);
			MVZ_readGV((void *)(gt->task.savedContext.gp.epc +
					    4), &i2, 4, 1, gt);
			gt->danceSecond =
			    _mips_nextpc(&gt->task.savedContext.gp, i, i2);
			/* Fall through */
		default:
			gt->danceStep = 1;
			break;
		}
		MVZ_EXLProtect(gt);
	} else {
		/* Leaving exception mode */
		MVZ_EXLUnprotect(gt);
		gt->danceStep = 0;
	}
	DBG_logF("Dance out %" PRIu32 "\n", (uint32_t) gt->danceStep);
}

reg_t MVZ_fixDebug(MVZ_GUEST_T * gt)
{
	reg_t r = 0;
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	if (MVZ_broken(gt)) {
		gt->guest.Cause = (gt->guest.Cause & ~CR_X_MASK) | gt->danceExc;
		r = 1;
	} else if (MVZ_stuck(gt)) {
		r = 0;
		DBG_logF("Noo: %p/%p\n", (void *)gt->task.savedContext.gp.epc,
			 (void *)gt->guest.EPC);
		if ((gt->danceStep == 2)
		    && (gt->task.savedContext.gp.epc ==
			(MVZ_getEBASE(gt) & EBASE_BASE) + 0x180)) {
			gt->task.savedContext.gp.epc = gt->danceSecond;
			gt->guest.Cause &= ~CR_BD;
			r = 1;
		}
		MVZ_dance(gt, -1);

	}
	IRQ_restoreIPL(ipl);
	return r;
}

void _KRN_scheduleTask(KRN_TASK_T * newTask);
/*
** FUNCTION:	MVZ_scheduleOut
**
** DESCRIPTION:	Handle switching out a guest
**
** RETURNS:	void
*/
void MVZ_scheduleOut(void)
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	struct gpctx *c = &_KRN_current->savedContext.gp;
	/* Interrupt handler has already handled GP context, laziness handles FP */
	/* Start saving guest context */
	_vzrootctx_save(&gt->root);
	/* Deactivate guest mode */
	mips32_bc_c0(C0_GUESTCTL0, GUESTCTL0_GM);
	/* Continue saving guest context */
	_linkctx_append(c, (struct linkctx *)&gt->root);
	_vzguestctx_save((struct vzguestctx *)&gt->guest);
	_linkctx_append(c, (struct linkctx *)&gt->guest);
	/* Reset GID */
	mips32_bc_c0(C0_GUESTCTL1, GUESTCTL1_ID);
	/* Unmask interrupt switcher */
	if (gt->intMask)
		mips32_bissr(gt->intMask);
}

/*
** FUNCTION:	MVZ_scheduleIn
**
** DESCRIPTION:	Handle switching in a guest
**
** RETURNS:	void
*/
void MVZ_scheduleIn()
{
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	reg_t gcount;
	/* Enter exception mode so guest entry is deferred until eret */
	mips_bissr(SR_EXL);
	/* Mask interrupt switcher */
	if (gt->intMask)
		mips32_bicsr(gt->intMask);
	/* Fix up timer interrupts */
	gcount = mips32_get_gc0(C0_COUNT);
	if (gcount > gt->guest.Count) {
		if ((gt->guest.Compare > gt->guest.Count)
		    && (gt->guest.Compare < gcount))
			gt->guest.Cause |= CR_TI;
		/*mips32_set_gc0(C0_CAUSE,
		   mips32_get_gc0(C0_CAUSE) | CR_TI); */
	} else {
		if ((gt->guest.Compare > gt->guest.Count)
		    || (gt->guest.Compare < gcount))
			gt->guest.Cause |= CR_TI;
		/*mips32_set_gc0(C0_CAUSE,
		   mips32_get_gc0(C0_CAUSE) | CR_TI); */
	}
	/* Load TLBs */
	_vztlbctx_load((struct vztlbctx *)(&gt->tlbs));
	/* Load FP/MSA */
#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
	/* Reactivate FPUs if possible */
	_CTX_activateFP(_KRN_current);
#endif
	/* Load guest context */
	_vzguestctx_load((struct vzguestctx *)(&gt->guest));
	_vzrootctx_load(&gt->root);
	/* Activate the new guest contextStart function, called on cold start of a guest and upon soft restart. If restart is not possible, should handle appropriately to resume execution */
	_hotwire(&(_KRN_current->savedContext));
}

/*
** FUNCTION:	MVZ_restart
**
** DESCRIPTION:	Restart a guest
**
** RETURNS:	void
*/
void MVZ_restart(MVZ_GUEST_T * guest)
{
	uint32_t i;
	/* Clear down */
	memset((void *)&guest->task.savedContext.gp, 0, sizeof(struct gpctx));
	/* Initialise guest TLB */
	guest->tlbs.link.id = LINKCTX_TYPE_VZTLBCTX;
	guest->tlbs.link.next = NULL;
	for (i = 0; i < _MVZ->ngtlbs; i++) {
		/* Invalidate */
		guest->tlbs.entries[i].EntryHi = 0xb0070000 + (i * 0x2000);
		guest->tlbs.entries[i].EntryLo0 = 0;
		guest->tlbs.entries[i].EntryLo1 = 0;
		guest->tlbs.entries[i].PageMask = 0;
	}
	/* Copy prototype GCP0 */
	guest->root.GuestCtl0 &= GUESTCTL0_PIP;
	guest->root.GuestCtl0 |= _MVZ->protoroot.GuestCtl0 & ~GUESTCTL0_PIP;
	guest->task.savedContext.gctl0 = guest->root.GuestCtl0;
	guest->root.GuestCtl1 = _MVZ->protoroot.GuestCtl1 | guest->gid;
	guest->root.GuestCtl2 &= GUESTCTL2_HC;
	guest->root.GuestCtl2 |= _MVZ->protoroot.GuestCtl2 & ~GUESTCTL2_HC;
	guest->root.GuestCtl3 = _MVZ->protoroot.GuestCtl3;
	guest->root.GuestCtl0Ext = _MVZ->protoroot.GuestCtl0Ext;
	guest->root.GTOffset = _MVZ->protoroot.GTOffset;
	memcpy((void *)&guest->guest, (void *)&_MVZ->protoguest,
	       sizeof(struct vzguestctxmax));
	/* Guest specific start up */
	guest->start(guest);
	/* Mark for context switch */
	if (_KRN_current == (KRN_TASK_T *) guest) {
		_KRN_current->reason = KRN_DEAD;
		KRN_scheduleProtected();
	}
}

/*
** FUNCTION:	MVZ_stop
**
** DESCRIPTION:	Default stop handler
**
** RETURNS:	void
*/
int32_t MVZ_stop(MVZ_GUEST_T * guest, int32_t exitcode)
{
	DBG_logF("Guest \"%s\" exited with code %" PRIu32 ".\n",
		 KRN_taskName((KRN_TASK_T *) guest), exitcode);
	return 0;
}

/*
** FUNCTION:	MVZ_initGuest
**
** DESCRIPTION:	Initialise a new guest
**
** RETURNS:	void
*/
void MVZ_initGuest(MVZ_GUEST_T * guest, uint32_t gid, MVZ_STARTFUNC_T * start)
{
	guest->gid = gid;
	guest->start = start;
	guest->stop = &MVZ_stop;
	guest->task.reason = KRN_DEAD;
	LST_init(&guest->vregs);
	memcpy((void *)&guest->root, (void *)&_MVZ->protoroot,
	       sizeof(struct vzrootctx));
}

/*
** FUNCTION:	MVZ_startGuest
**
** DESCRIPTION:	Start a guest
**
** RETURNS:	void
*/
void MVZ_startGuest(MVZ_GUEST_T * guest,
		    KRN_PRIORITY_T priority, const char *guestname)
{
	KRN_TASK_T *task = &guest->task;
	PARACHECK();
	IRQ_IPL_T oldipl;
	DBG_assert(IRQ_bg(), "Can not start guest from interrupt context");
	task->reason = KRN_OTHER;
	task->holdQueue = NULL;
	_KRN_prepareTask(NULL, task, NULL, 0, &priority, NULL, guestname);
	task->schedIn = MVZ_scheduleIn;
	task->schedOut = MVZ_scheduleOut;
	task->holdQueue = _KRN_schedule->priQueues + priority;
#ifdef CONFIG_BUILD_PROFILING
	/* if possible, locate and set up a task performance statistics slot */
	task->statsPtr = NULL;
	if (_ThreadStatsArraySize > 0) {
		int32_t n;
		for (n = 0; n < _ThreadStatsArraySize; n++) {
			oldipl = IRQ_raiseIPL();
			if (_ThreadStatsArray[n].name == NULL) {
				_ThreadStatsArray[n].name =
				    guestname ? guestname : "";
				task->statsPtr = &_ThreadStatsArray[n];
				_KRN_zeroStats(task->statsPtr);
				IRQ_restoreIPL(oldipl);
				break;
			}
			IRQ_restoreIPL(oldipl);
		}
		DBG_insist(n < _ThreadStatsArraySize, "No free perf stats slots!");	/* fails if no slot available */
	}
#endif

	oldipl = IRQ_raiseIPL();
	/* Manually add the task to the all tasks queue to keep it out of the
	 * paranoia system, and set the dirty bit so CodeScape notices.
	 */
	DQ_T *queue = &_KRN_schedule->liveTasks;
	void *item = &task->taskLink; {
		PARACHECK();
		PARADEL(DqIt, item);
		((DQ_LINKAGE_T *) queue)->back =
		    (DQ_LINKAGE_T
		     *) (((uintptr_t) ((DQ_LINKAGE_T *) queue)->back) & ~1);
		((DQ_LINKAGE_T *) item)->fwd = (DQ_LINKAGE_T *) queue;
		((DQ_LINKAGE_T *) item)->back = ((DQ_LINKAGE_T *) queue)->back;
		((DQ_LINKAGE_T *) queue)->back->fwd = (DQ_LINKAGE_T *) item;
		((DQ_LINKAGE_T *) queue)->back =
		    (DQ_LINKAGE_T *) (((uintptr_t) item) | 1);
		PARAADDI(DqIt, item);
		PARACHECK();
	}
	IRQ_restoreIPL(oldipl);
	MVZ_restart(guest);
	/* link the new TCB onto a scheduler queue */
	oldipl = IRQ_raiseIPL();
	DQ_addTail(task->holdQueue, task);	/* activate task */
	*(task->priFlags) |= task->priBit;	/* mark queue as active for optimised search */
	KRN_scheduleUnprotect(oldipl);
	PARACHECK();
	return;
}

/*
** FUNCTION:	MVZ_addRegs
**
** DESCRIPTION:	Add virtual registers to a guest.
**
** RETURNS:	void
*/
void MVZ_addRegs(MVZ_GUEST_T * guest, MVZ_VREGS_T * regs)
{
	regs->guest = guest;
	LST_addHead(&guest->vregs, regs);
}

/*
** FUNCTION:	MVZ_debugGuest
**
** DESCRIPTION:	Attach an SDBBP handler
**
** RETURNS:	void
*/
void MVZ_debugGuest(MVZ_GUEST_T * guest, MVZ_BREAK_T * debugger,
		    void *debugPriv)
{
	guest->debugger = debugger;
	guest->debugPriv = debugPriv;
	if (debugger) {
		if (guest == (MVZ_GUEST_T *) _KRN_current)
			mips32_set_c0(C0_GUESTCTL0,
				      mips32_get_c0(C0_GUESTCTL0) |
				      GUESTCTL0_MC);
		else
			guest->task.savedContext.gctl0 |= GUESTCTL0_MC;
	} else {
		if (guest == (MVZ_GUEST_T *) _KRN_current)
			mips32_set_c0(C0_GUESTCTL0,
				      mips32_get_c0(C0_GUESTCTL0) &
				      ~GUESTCTL0_MC);
		else
			guest->task.savedContext.gctl0 &= ~GUESTCTL0_MC;
	}
}
