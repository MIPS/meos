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
*   Description:	MIPS context specialisation
*
*************************************************************************/

#include "meos/config.h"
#include "meos/debug/dbg.h"
#include "meos/kernel/krn.h"
#include "meos/irq/irq.h"
#include "meos/ctx/ctx.h"
#include "meos/target/krn.h"
#include <mips/m32c0.h>
#include <mips/m32c1.h>
#include <string.h>

#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
#define FP_ENABLED() (mips_getsr() & SR_CU1)
#define FP_ENABLE() mips_bissr(SR_CU1)
#define FP_DISABLE() mips_bicsr(SR_CU1)
#ifndef fp_getsr
#define fp_getsr() \
__extension__({ \
  register unsigned __r; \
  __asm__ __volatile (".set push\n" \
                      ".set fp=64\n" \
                      "cfc1 %0,$31\n" \
                      ".set pop":"=d"(__r)); \
  __r; \
})
#endif
#ifndef fp_setsr
#define fp_setsr(X) \
__extension__({ \
  __asm__ __volatile (".set push\n" \
                      ".set fp=64\n" \
                      "ctc1 %0,$31\n" \
                      ".set pop"::"d"(X)); \
})
#endif
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
#define MSA_ENABLED() (mips32_getconfig5() & CFG5_MSAEN)
#define MSA_ENABLE() _m32c0_mtc0(C0_CONFIG, 5, mips32_getconfig5() | CFG5_MSAEN)
#define MSA_DISABLE() _m32c0_mtc0(C0_CONFIG, 5, mips32_getconfig5() & ~CFG5_MSAEN)
#ifndef msa_getsr
#define msa_getsr() \
__extension__ ({ \
  register unsigned __r; \
  __asm__ __volatile (".set push\n" \
		      ".set fp=64\n" \
		      ".set msa\n" \
		      "cfcmsa %0,$1\n" \
		      ".set pop": "=d" (__r)); \
  __r; \
})
#endif
#ifndef msa_setsr
#define msa_setsr(X) \
__extension__ ({ \
  __asm__ __volatile (".set push\n" \
		      ".set fp=64\n" \
		      ".set msa\n" \
		      "ctcmsa %0,$1\n" \
		      ".set pop":: "d" (X)); \
})
#endif
#endif
void _KRN_prepareTask(KRN_TASKFUNC_T * taskfunc, KRN_TASK_T * task,
		      uint32_t * uworkspace, size_t wssize,
		      KRN_PRIORITY_T * priority, void *parameter,
		      const char *taskname);

#ifdef __cplusplus
extern "C" {
#endif

	extern void _sleep(void);
	extern void _hotwire(KRN_CTX_T * ctx) __attribute__ ((noreturn));

#ifdef __cplusplus
}
#endif
#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
KRN_TASK_T *_CTX_fpuUser;
reg_t _CTX_ifsr, _CTX_imsr;

/*
** FUNCTION:	_CTX_activateFP
**
** DESCRIPTION:	Set CU1/MSA enable bits to match context from task if current
**
** RETURNS:	void
*/
void _CTX_activateFP(KRN_TASK_T * task)
{
	uint32_t type = LINKCTX_TYPE((struct fpctx *)&task->savedContext.fpa);
#ifdef CONFIG_ARCH_MIPS_MSA
	/* If we're the current MSA user, enable it. Otherwise disable it so lazy
	 * works.
	 */
	if ((_CTX_fpuUser == task)
	    && (type == LINKCTX_TYPE_MSA || type == LINKCTX_TYPE_FMSA))
		MSA_ENABLE();
	else
		MSA_DISABLE();
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	/* If we're the current FPU user, enable it. Otherwise disable it so lazy
	 * works.
	 */
	if ((_CTX_fpuUser == task)
	    && (type == LINKCTX_TYPE_FP32 || type == LINKCTX_TYPE_FP64
		|| type == LINKCTX_TYPE_FMSA))
		FP_ENABLE();
	else
		FP_DISABLE();
#endif
}
#endif

/*
** FUNCTION:	_CTX_scheduleOut
**
** DESCRIPTION:	Handle switching out a task
**
** RETURNS:	void
*/
void _CTX_scheduleOut(void)
{
	/* Interrupt handler has already handled GP context */
#ifdef CONFIG_DEBUG_STACK_CHECKING
	/* Check for stack over/underflow */
	DBG_assert(!_KRN_current->stackStart
		   || (_KRN_current->savedContext.gp.sp >
		       (uint32_t) _KRN_current->stackStart),
		   "%s overflowed stack", _KRN_current->name);
	DBG_assert(!_KRN_current->stackEnd
		   || (_KRN_current->savedContext.gp.sp <
		       (uint32_t) _KRN_current->stackEnd),
		   "%s underflowed stack", _KRN_current->name);
#endif
}

/*
** FUNCTION:	_CTX_scheduleIn
**
** DESCRIPTION:	Handle switching in a task
**
** RETURNS:	void
*/
void _CTX_scheduleIn()
{
#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
	/* Reactivate FPUs if possible */
	_CTX_activateFP(_KRN_current);
#endif
	/* Activate the new task context to resume execution */
	_hotwire(&(_KRN_current->savedContext));
}

static KRN_TASK_T _CTX_waitTask;

/*
** FUNCTION:	CTX_sleep
**
** DESCRIPTION:	Go to sleep
**
** RETURNS:	void
*/
void CTX_sleep(void)
{
	/*
	 * 'wait' only works properly in user mode. Bodge things so we execute it
	 * there but don't think we're in a task context.
	 */
	KRN_CTX_T *awakeCtx;
	awakeCtx =
	    _KRN_current ? &_KRN_current->savedContext : &_KRN_schedule->
	    deferred->savedContext;
	_CTX_waitTask.savedContext.gp.epc = (uint32_t) _sleep;

	_CTX_waitTask.savedContext.gp.sp = awakeCtx->gp.sp;
	_CTX_waitTask.savedContext.gp.fp = awakeCtx->gp.fp;
	_CTX_waitTask.savedContext.gp.ra = awakeCtx->gp.ra;

	_KRN_current = NULL;
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	FP_DISABLE();
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
	MSA_DISABLE();
#endif
	_hotwire(&_CTX_waitTask.savedContext);
}

#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
/*
** FUNCTION:	_CTX_CpUISR
**
** DESCRIPTION:	CpU exception handler. Handles laziness.
**
** RETURNS:	void
*/
void _CTX_CpUISR(int32_t sig, KRN_CTX_T * ctx)
    __attribute__ ((no_instrument_function));
void _CTX_CpUISR(int32_t sig, KRN_CTX_T * ctx)
{
	uint32_t msa = 1, fpu = 1;
	/* Have we deferred save? */
	if ((_CTX_fpuUser) && (_CTX_fpuUser != _KRN_current)) {
		/* Yes */
		/* Restore _CTX_fpuUser's */
		_CTX_activateFP(_CTX_fpuUser);
		/* Save */
		if (
#ifdef CONFIG_ARCH_MIPS_MSA
			   _msactx_save((struct msactx *)
					&_CTX_fpuUser->savedContext.fpa) ||
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
			   _fpctx_save((struct fpctx *)
				       &_CTX_fpuUser->savedContext.fpa) ||
#endif
			   0)
			_linkctx_append(&_CTX_fpuUser->savedContext.gp,
					(struct linkctx *)
					&_CTX_fpuUser->savedContext.fpa);
		/* Load */
		/* Disable FP/MSA, let load sort it out */
#ifdef CONFIG_ARCH_MIPS_MSA
		MSA_DISABLE();
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
		FP_DISABLE();
#endif
		if (_KRN_current) {
#ifdef CONFIG_ARCH_MIPS_MSA
			if (_msactx_load
			    ((struct msactx *)&_KRN_current->savedContext.fpa))
				msa = 0;
#endif
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
			if (_fpctx_load
			    ((struct fpctx *)&_KRN_current->savedContext.fpa))
				fpu = 0;
#endif
		}
	}
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	if ((fpu) && (sig == 0x800b)) {
/* Enable CU1 */
		FP_ENABLE();
		if (_KRN_current) {
			fp_setsr(((struct fpctx *)
				  &_KRN_current->savedContext.fpa)->fcsr);
#ifdef CONFIG_ARCH_MIPS_MSA
			if (LINKCTX_TYPE((struct fpctx *)
					 &_KRN_current->savedContext.fpa) ==
			    LINKCTX_TYPE_MSA)
				LINKCTX_TYPE((struct fpctx *)
					     &_KRN_current->savedContext.fpa) =
				    LINKCTX_TYPE_FMSA;
			else
#endif
				LINKCTX_TYPE((struct fpctx *)&_KRN_current->savedContext.fpa) = LINKCTX_TYPE_FP32;	/* The correct value will get written in on save */
		} else {
			fp_setsr(_CTX_ifsr);
		}
	}
#endif

#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
	else
#endif
	if (msa) {
#ifdef CONFIG_ARCH_MIPS_MSA
		/* Enable MSA */
		MSA_ENABLE();
		if (_KRN_current) {
			msa_setsr(((struct msactx *)
				   &_KRN_current->savedContext.fpa)->msacsr);
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
			if ((LINKCTX_TYPE((struct fpctx *)
					  &_KRN_current->savedContext.fpa) ==
			     LINKCTX_TYPE_FP32)
			    || (LINKCTX_TYPE((struct fpctx *)
					     &_KRN_current->savedContext.fpa) ==
				LINKCTX_TYPE_FP64))
				LINKCTX_TYPE((struct fpctx *)
					     &_KRN_current->savedContext.fpa) =
				    LINKCTX_TYPE_FMSA;
			else
#endif
				LINKCTX_TYPE((struct fpctx *)
					     &_KRN_current->savedContext.fpa) =
				    LINKCTX_TYPE_MSA;
		} else {
			msa_setsr(_CTX_imsr);
		}
#endif
	}

	/* Null on interrupt, so user code gets restored */
	if ((!_KRN_current) || (ctx != &_KRN_current->savedContext))
		_CTX_fpuUser = NULL;
	else
		_CTX_fpuUser = _KRN_current;
}
#endif

/* CTX related interrupt descriptors */
void _KRN_schedulerISR(void);
IRQ_DESC_T _CTX_syscall
#ifndef __cplusplus
    = {
	.intNum = 0x8008,.isrFunc = (void (*)(int32_t))_KRN_schedulerISR
}
#endif
;

#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
IRQ_DESC_T _CTX_CpU
#ifndef __cplusplus
    = {
	.intNum = 0x800b,.isrFunc = (IRQ_ISRFUNC_T *) _CTX_CpUISR
}
#endif
;
#endif

#ifdef CONFIG_ARCH_MIPS_MSA
IRQ_DESC_T _CTX_MSAX
#ifndef __cplusplus
    = {
	.intNum = 0x8015,.isrFunc = (IRQ_ISRFUNC_T *) _CTX_CpUISR
}
#endif
;
#endif

#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
void _CTX_FPEISR(int32_t exception, KRN_CTX_T * ctx)
{
	FP_ENABLE();
	reg_t csr = fp_getsr();
	if (csr & FPA_CSR_FS) {
		DBG_logF("FPE error\n");
		_DBG_stop(__FILE__, __LINE__);
	}
	fp_setsr((csr & ~FPA_CSR_ALL_X) | FPA_CSR_FS);
	FP_DISABLE();
}

IRQ_DESC_T _CTX_FPE
#ifndef __cplusplus
    = {
	.intNum = 0x800f,.isrFunc = (IRQ_ISRFUNC_T *) _CTX_FPEISR
}
#endif
;
#endif

#ifdef CONFIG_ARCH_MIPS_MALLOC_BFL
KRN_LOCK_T _CTX_bfl;
KRN_TASK_T *_CTX_bflOwner;
reg_t _CTX_bflCount;

void __malloc_lock()
{
	if (_KRN_current && (_KRN_current != _CTX_bflOwner))
		KRN_lock(&_CTX_bfl, KRN_INFWAIT);
	_CTX_bflOwner = _KRN_current;
	_CTX_bflCount++;
}

void __malloc_unlock()
{
	if (_KRN_current && (!--_CTX_bflCount)) {
		KRN_unlock(&_CTX_bfl);
		_CTX_bflOwner = NULL;
	}
}
#endif

/*
** FUNCTION:	CTX_init
**
** DESCRIPTION:	Initialise CTX
**
** RETURNS:	void
*/
void CTX_init(void)
{
	KRN_PRIORITY_T priority = KRN_LOWEST_PRIORITY;

#ifdef CONFIG_ARCH_MIPS_MALLOC_BFL
	KRN_initLock(&_CTX_bfl);
	_CTX_bflOwner = NULL;
	_CTX_bflCount = 0;
#endif

#ifdef __cplusplus
	_CTX_syscall.intNum = 0x8008;
	_CTX_syscall.isrFunc = (void (*)(int32_t))_KRN_schedulerISR;
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	_CTX_CpU.intNum = 0x800b;
	_CTX_CpU.isrFunc = (IRQ_ISRFUNC_T *) _CTX_CpUISR;
	_CTX_FPE.intNum = 0x800f;
	_CTX_FPE.isrFunc = (IRQ_ISRFUNC_T *) _CTX_FPEISR;
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
	_CTX_MSA.intNum = 0x8015;
	_CTX_MSA.isrFunc = (IRQ_ISRFUNC_T *) _CTX_CpUISR;
#endif
#endif

	/* Make a task for sleeping, but don't let the scheduler know about it */
	_KRN_prepareTask(_sleep, &_CTX_waitTask,
			 NULL, 0, &priority, NULL, "<sleep>");
	/* Hook syscalls for kernel use */
	IRQ_route(&_CTX_syscall);
	/* Check for an FPU */
	int32_t reg = mips32_getconfig0();
	if (reg & CFG0_M)
		reg = mips32_getconfig1();
	else
		reg = 0;
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	if (reg & CFG1_FP) {
		/* Route CU1 usage exception to laziness handler */
		IRQ_route(&_CTX_CpU);
		_CTX_fpuUser = NULL;
		FP_DISABLE();
		IRQ_route(&_CTX_FPE);
	}
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
	/* Check for MSA */
	if ((reg & CFG1_M) && (mips32_getconfig2() & CFG2_M)
	    && (mips32_getconfig3() & CFG3_MSAP)) {
		/* Route MSA usage exception to laziness handler */
		IRQ_route(&_CTX_MSAX);
		_CTX_fpuUser = NULL;
		MSA_DISABLE();
	}
#endif

}

extern void _DBG_fdcInit();

#ifdef CONFIG_ARCH_MIPS_REENT
struct _reent *_CTX_impure_data;
#endif

/*
** FUNCTION:	CTX_attachBGTask
**
** DESCRIPTION:	Assimilate the initial task.
**
** RETURNS:	void *
*/
void CTX_attachBGTask(KRN_TASK_T * task)
{
#ifndef CONFIG_DEBUG_DIAGNOSTICS
	task->schedOut = NULL;
#else
	task->schedOut = _CTX_scheduleOut;
#endif
	task->schedIn = _CTX_scheduleIn;
#ifdef CONFIG_DEBUG_STACK_CHECKING
	task->stackStart = NULL;
	task->stackEnd = NULL;
#endif
#ifdef CONFIG_ARCH_MIPS_VZ
	_m32c0_mtc0(31, 2, &(task->savedContext));	/* Stick context in KSCRATCH1 */
#endif
	asm volatile ("move $k0, %0"::"r" (&(task->savedContext)));
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
	FP_ENABLE();
	_CTX_ifsr = fp_getsr();
	((struct fpctx *)&task->savedContext.fpa)->fcsr = _CTX_ifsr;
	FP_DISABLE();
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
	MSA_ENABLE();
	_CTX_imsr = msa_getsr();
	((struct msactx *)&task->savedContext.fpa)->msacsr = _CTX_imsr;
	MSA_DISABLE();
#endif
#if (defined(CONFIG_ARCH_MIPS_HARD_FLOAT) || defined(CONFIG_ARCH_MIPS_MSA))
	_CTX_fpuUser = task;
#endif
#ifdef CONFIG_ARCH_MIPS_CHANNEL_FAST
	_DBG_fdcInit();
#endif
#ifdef CONFIG_ARCH_MIPS_REENT
	_REENT_INIT_PTR(&task->savedContext.reent);
	_CTX_impure_data = _impure_ptr;
	_impure_ptr = &task->savedContext.reent;
#endif
}

/*
** FUNCTION:	_terminate/_task_entry
**
** DESCRIPTION:	Entry point for a thread - invokes user function, then removes
**              itself if it ever returns.
**
** RETURNS:	void
*/
#ifdef __cplusplus
extern "C" {
#endif
	extern void _terminate(KRN_TASKFUNC_T * task_func);
	extern void _task_entry(void);
#ifdef __cplusplus
}
#endif
/*
** FUNCTION:	CTX_startTask
**
** DESCRIPTION:	Create a new thread of execution for the supplied task.
**
** RETURNS:	void
*/ void CTX_startTask(KRN_TASK_T * task, uintptr_t workspace,
				       size_t wssize,
				       KRN_TASKFUNC_T * task_func)
{
#ifndef CONFIG_DEBUG_DIAGNOSTICS
	task->schedOut = NULL;
#else
	task->schedOut = _CTX_scheduleOut;
#endif
	task->schedIn = _CTX_scheduleIn;
	memset(&task->savedContext, 0, sizeof(KRN_CTX_T));
	if (task_func != NULL) {
		task->savedContext.gp.epc = (uint32_t) _task_entry;
		task->savedContext.gp.a[0] = (uint32_t) task_func;
		task->savedContext.gp.k[0] = (uint32_t) & (task->savedContext);
		task->savedContext.nest = 0;
		asm volatile ("move %0, $gp":"=r" (task->savedContext.gp.gp));
		task->savedContext.gp.sp =
		    (((uintptr_t) workspace) + ((wssize - 5) * 4)) & ~7;
		task->savedContext.gp.ra = 0;
#ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
		FP_ENABLE();
		((struct fpctx *)&task->savedContext.fpa)->fcsr = fp_getsr();
		FP_DISABLE();
#endif
#ifdef CONFIG_ARCH_MIPS_MSA
		MSA_ENABLE();
		((struct msactx *)&task->savedContext.fpa)->msacsr =
		    msa_getsr();
		MSA_DISABLE();
#endif
	}
#ifdef CONFIG_ARCH_MIPS_REENT
	_REENT_INIT_PTR(&task->savedContext.reent);
#endif
}

/*
** FUNCTION:	CTX_verifyTask
**
** DESCRIPTION:	Context verification can't be done on Linux.
**
** RETURNS:	void
*/
void CTX_verifyTask(KRN_TASK_T * current)
{
	(void)current;
#ifdef CONFIG_DEBUG_STACK_CHECKING
	DBG_assert(!current->stackStart
		   || (current->savedContext.gp.sp >
		       (uint32_t) current->stackStart),
		   "%s overflowed stack", current->name);
	DBG_assert(!current->stackEnd
		   || (current->savedContext.gp.sp <
		       (uint32_t) current->stackEnd),
		   "%s underflowed stack", current->name);
#endif
}

extern uintptr_t _DBG_intStackStart, _DBG_intStackEnd;
/*
** FUNCTION:	CTX_verifySelf
**
** DESCRIPTION:	Context verification can't be done on Linux.
**
** RETURNS:	void
*/
void CTX_verifySelf(KRN_TASK_T * current)
    __attribute__ ((no_instrument_function));
void CTX_verifySelf(KRN_TASK_T * current)
{
	(void)current;
	register uintptr_t sp asm("$sp");
	if ((sp >= _DBG_intStackStart) && (sp <= _DBG_intStackEnd))
		return;
#ifdef CONFIG_DEBUG_STACK_CHECKING
	if (current) {
		DBG_assert(!current->stackStart
			   || (sp >
			       (uint32_t) current->stackStart),
			   "%s overflowed stack", current->name);
		DBG_assert(!current->stackEnd
			   || (sp <
			       (uint32_t) current->stackEnd),
			   "%s underflowed stack", current->name);
	}
#endif
}

/*
** FUNCTION:	_CTX_UHI
**
** DESCRIPTION:	Forward UHI via sdbbp
**
** RETURNS:	void
*/
void _CTX_UHI(int sigNum, struct gpctx *ctx)
    __attribute__ ((no_instrument_function));
void _CTX_UHI(int sigNum, struct gpctx *ctx)
{
	register reg_t arg1 asm("$4") = ctx->a[0];
	register reg_t arg2 asm("$5") = ctx->a[1];
	register reg_t arg3 asm("$6") = ctx->a[2];
	register reg_t arg4 asm("$7") = ctx->a[3];
	register reg_t op asm("$25") = ctx->t2[1];
	register reg_t ret1 asm("$2") = 1;
	register reg_t ret2 asm("$3");

	__asm__ __volatile__(" # UHI indirect\n"
			     "\tsdbbp 1":"+r"(ret1), "=r"(ret2), "+r"(arg1),
			     "+r"(arg2)
			     :"r"(arg3), "r"(arg4), "r"(op));

	ctx->v[0] = ret1;
	ctx->v[1] = ret2;
	ctx->a[0] = arg1;
	ctx->a[1] = arg2;
	/* Handled, move on.  SYSCALL is 4-bytes in all ISAs.  */
	ctx->epc += 4;
}
