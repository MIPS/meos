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
*   Description:	Trace support
*
*************************************************************************/

#include <stdarg.h>
#include "meos/debug/dbg.h"
#include "meos/target/dbg.h"
#include "meos/kernel/krn.h"
#include "meos/tmr/tmr.h"
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "meos/inttypes.h"

#if UINTPTR_MAX == UINT32_MAX
#define EXIT_BIT	0x40000000
#else
#define EXIT_BIT	0x4000000000000000
#endif

#ifdef __cplusplus
extern "C" {
#endif
	KRN_TRACE_T *DBG_openTrace(uint32_t n)
	    __attribute__ ((no_instrument_function));
	KRN_TRACE_T *DBG_stepTrace(KRN_TRACE_T ** p, uint32_t n)
	    __attribute__ ((no_instrument_function));
	inline static KRN_TRACE_T *DBG_openTraceCore(uint32_t n)
	    __attribute__ ((no_instrument_function));
	inline static KRN_TRACE_T *DBG_stepTraceCore(KRN_TRACE_T ** p,
						     uint32_t n)
	    __attribute__ ((no_instrument_function));
	void DBG_atrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i,
			uintptr_t * ap)
	    __attribute__ ((no_instrument_function));
	void DBG_trace(KRN_TRACE_T ** wp, uintptr_t event, uint32_t i, ...)
	    __attribute__ ((no_instrument_function));
	void DBG_enter(KRN_TRACE_T ** wp, void *fn, uint32_t i, ...)
	    __attribute__ ((no_instrument_function));
	void DBG_exit(KRN_TRACE_T ** wp, void *fn, uint32_t i, ...)
	    __attribute__ ((no_instrument_function));
	void DBG_ctxSw(int32_t reason, void *vfrom, void *vto)
	    __attribute__ ((no_instrument_function));
	void DBG_interruptIn(int32_t i)
	    __attribute__ ((no_instrument_function));
	void DBG_interruptSched(void)
	    __attribute__ ((no_instrument_function));
	void DBG_interruptOut(int32_t i)
	    __attribute__ ((no_instrument_function));
	void DBG_hotwire(KRN_CTX_T * ctx)
	    __attribute__ ((no_instrument_function));
	void DBG_chain(KRN_CTX_T * ctx)
	    __attribute__ ((no_instrument_function));
	void DBG_walkExtra(KRN_TRACE_T ** wpp, uint32_t depth)
	    __attribute__ ((no_instrument_function));
#ifdef __cplusplus
}
#endif
#ifdef CONFIG_DEBUG_PER_OBJECT_TRACE
typedef struct {
	KRN_POOLLINK_T KRN_plink;
	void *obj;
} _DBG_PEROBJ_T;

KRN_POOL_T _DBG_perObjPool;
LST_T _DBG_perObjReg;
KRN_LOCK_T _DBG_perObjLock;

void _DBG_perObjInit(void *buf, size_t size)
{
	LST_init(&_DBG_perObjReg);
	KRN_initPool(&_DBG_perObjPool, buf,
		     size / sizeof(_DBG_PEROBJ_T), sizeof(_DBG_PEROBJ_T));
	KRN_initLock(&_DBG_perObjLock);
}

int32_t DBG_traceObjects(void *i, ...)
{
	/* This is in the BSS, can rely on being NULL if not initialised */
	if (&_DBG_perObjPool.sem.waitq.DQ_link.fwd == NULL) {
		va_list ap;
		_DBG_PEROBJ_T *obj;
		KRN_lock(&_DBG_perObjLock, KRN_INFWAIT);
		va_start(ap, i);
		while (i) {
			obj = LST_first(&_DBG_perObjReg);
			while (obj) {
				if (obj->obj == i)
					return 1;
				obj = LST_next(obj);
			}
			i = va_arg(ap, void *);
		}
		va_end(ap);
		KRN_unlock(&_DBG_perObjLock);
	}
	return 0;
}

void DBG_tronObject(void *obj)
{
	_DBG_PEROBJ_T *pobj;

	KRN_lock(&_DBG_perObjLock, KRN_INFWAIT);
	pobj = KRN_takePool(&_DBG_perObjPool, 0);
	DBG_insist(pobj, "Per-object tracing pool exhausted.\n");
	LST_add(&_DBG_perObjReg, pobj);
	KRN_unlock(&_DBG_perObjLock);
}

void DBG_troffObject(void *obj)
{
	_DBG_PEROBJ_T *pobj = (_DBG_PEROBJ_T *) LST_first(&_DBG_perObjReg);

	KRN_lock(&_DBG_perObjLock, KRN_INFWAIT);
	while (pobj) {
		if (pobj->obj == obj) {
			LST_remove(&_DBG_perObjReg, pobj);
			KRN_returnPool(pobj);
		}
	}
	KRN_unlock(&_DBG_perObjLock);
}
#endif

#ifdef CONFIG_DEBUG_TRACE_SOFT
#ifdef CONFIG_DEBUG_WRAPPER
extern const char *_DBG_symnames[];

void DBG_printName(void *vp)
    __attribute__ ((no_instrument_function));
void DBG_printName(void *vp)
{
	const char *p = (const char *)vp;
	const char **e;
	for (e = _DBG_symnames; p > e[2]; e += 2) ;
	if (p == e[0])
		DBG_logF("%p (%s)", p, e[1]);
	else
		DBG_logF("%p (%s+0x%" PRIxPTR ")", p, e[1],
			 (uintptr_t) p - (uintptr_t) e[0]);
}
#else
void DBG_printName(void *vp)
    __attribute__ ((no_instrument_function));
void DBG_printName(void *vp)
{
	DBG_logF("%p", vp);
}
#endif

inline static KRN_TRACE_T *DBG_openTraceCore(uint32_t n)
{
	uintptr_t o;
	KRN_TRACE_T *b = NULL, *a;
	if (KRN_tracePtr)
		do {
			b = (KRN_TRACE_T *) KRN_tracePtr;
			o = ((uintptr_t) b -
			     (uintptr_t) KRN_traceMin) / sizeof(KRN_TRACE_T);
			o = o + n;
			if (o >= KRN_traceSize)
				o -= KRN_traceSize;
			a = (KRN_TRACE_T *) (((uintptr_t) KRN_traceMin)
					     + o * sizeof(KRN_TRACE_T));
		} while (!__sync_bool_compare_and_swap(&KRN_tracePtr, b, a));
	return b;
}

KRN_TRACE_T *DBG_openTrace(uint32_t n)
{
	return DBG_openTraceCore(n);
}

inline static KRN_TRACE_T *DBG_stepTraceCore(KRN_TRACE_T ** p, uint32_t n)
{
	*p += n;
	if (*p > KRN_traceMax)
		*p -= KRN_traceSize;
	return *p;
}

KRN_TRACE_T *DBG_stepTrace(KRN_TRACE_T ** p, uint32_t n)
{
	return DBG_stepTraceCore(p, n);
}

#ifdef CONFIG_DEBUG_TRACE_SOFT_DUMP
void _DBG_dumpTrace()
    __attribute__ ((no_instrument_function));
void _DBG_dumpTrace()
{
	KRN_TRACE_T *rp;
	volatile KRN_TRACE_T *start = KRN_tracePtr;
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	KRN_tracePtr = NULL;
	if (start) {
		int32_t size = KRN_traceSize;
		rp = (KRN_TRACE_T *) start;
		DBG_logF("Trace dump\n");
		do {
			if (!((rp->event == 0)
			      && (rp->initial.absTime == 0)
			      && (rp->initial.p1 == 0)
			      && (rp->initial.p2 == 0))) {
				DBG_logF("%02" PRIu32 "/%10" PRIuPTR ": ",
					 KRN_proc(), rp->initial.absTime);
				switch (rp->event) {
#ifdef CONFIG_DEBUG_TRACE_ISR_SOFT
				case DBG_TRACE_ENTER_ISR:
					DBG_logF("Enter ISR %" PRIxPTR
						 " @ %p\n",
						 rp->initial.p1,
						 (void *)rp->initial.p2);
					break;
				case DBG_TRACE_EXIT_ISR:
					DBG_logF("Exit ISR %" PRIxPTR
						 " @ %p\n",
						 rp->initial.p1,
						 (void *)rp->initial.p2);
					break;
				case DBG_TRACE_SCHED_ISR:
					DBG_logF
					    ("Enter scheduler @ %p\n",
					     (void *)rp->initial.p1);
					break;
				case DBG_TRACE_HOTWIRE_ISR:
					DBG_logF
					    ("Activating context %08"
					     PRIxPTR " @ %p\n",
					     rp->initial.p1,
					     (void *)rp->initial.p1);
					break;
				case DBG_TRACE_CHAIN_UHI:
					DBG_logF
					    ("UHI chaining context %08"
					     PRIxPTR " @ %p\n",
					     rp->initial.p1,
					     (void *)rp->initial.p1);
					break;
#endif
				case DBG_TRACE_LOG:
					{
						DBG_logF("Log %1.8s",
							 rp->text + 4);
						while ((rp[1].event ==
							DBG_TRACE_CONTINUATION1)
						       || (rp[1].event ==
							   DBG_TRACE_CONTINUATION2)
						       || (rp[1].event ==
							   DBG_TRACE_CONTINUATION3))
						{
							DBG_logF("%1.12s",
								 rp[1].text);
							DBG_stepTraceCore(&rp,
									  1);
							size--;
						}
					}
					break;
				case DBG_TRACE_CONTINUATION1:
				case DBG_TRACE_CONTINUATION2:
				case DBG_TRACE_CONTINUATION3:
					DBG_logF
					    ("Unexpected continuation %"
					     PRIuPTR " @ %p! %08"
					     PRIxPTR " / %08" PRIxPTR
					     " / %08" PRIxPTR "\n",
					     1 + rp->event -
					     DBG_TRACE_CONTINUATION1,
					     rp, rp->supplemental.p1,
					     rp->supplemental.p2,
					     rp->supplemental.p3);
					break;
				case DBG_TRACE_CTX_SW:
					DBG_logF
					    ("Context switch from %08"
					     PRIxPTR " to %08" PRIxPTR,
					     rp->initial.p2,
					     rp[1].supplemental.p1);
					switch (rp->initial.p1) {
					case KRN_RELEASE:
						DBG_logF(" due to yield ");
						break;
					case KRN_TIMESLICE:
						DBG_logF
						    (" due to expired timeslice ");
						break;
					default:
						break;
					}
					DBG_stepTraceCore(&rp, 1);
					size--;
					DBG_logF(" @ %p->%p \n", (void *)
						 rp->supplemental.p2, (void *)
						 rp->supplemental.p3);
					break;
#ifdef CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT
				case DBG_TRACE_CTX_BT:
					DBG_logF("Backtrace:\n");
					DBG_logF("%08" PRIxPTR "\n%08"
						 PRIxPTR "\n",
						 rp->initial.p1,
						 rp->initial.p2);
					do {
						DBG_stepTraceCore(&rp, 1);
						switch (rp->event) {
						case DBG_TRACE_CONTINUATION1:
							DBG_logF("%08"
								 PRIxPTR
								 "\n",
								 rp->supplemental.p1);
							break;
						case DBG_TRACE_CONTINUATION2:
							DBG_logF("%08"
								 PRIxPTR
								 "\n%08"
								 PRIxPTR
								 "\n",
								 rp->supplemental.p1,
								 rp->supplemental.p2);
							break;
						case DBG_TRACE_CONTINUATION3:
							DBG_logF("%08"
								 PRIxPTR
								 "\n%08"
								 PRIxPTR
								 "\n%08"
								 PRIxPTR
								 "\n",
								 rp->supplemental.p1,
								 rp->supplemental.p2,
								 rp->supplemental.p3);
							break;
						default:
							break;
						}
					}
					while ((rp->event ==
						DBG_TRACE_CONTINUATION1)
					       || (rp->event ==
						   DBG_TRACE_CONTINUATION2)
					       || (rp->event ==
						   DBG_TRACE_CONTINUATION3));
					continue;
#endif
#ifdef CONFIG_DEBUG_TRACE_IPL_SOFT
				case DBG_TRACE_RAISE:
					DBG_logF
					    ("IPL raise from %" PRIuPTR
					     " to %" PRIuPTR " ",
					     rp->initial.p1,
					     rp->initial.p1 + 1);
#ifdef CONFIG_DEBUG_TRACE_EXTRA
					if (rp[1].event == DBG_TRACE_EXTRA2) {
						DBG_stepTraceCore(&rp, 1);
						size--;
						DBG_logF
						    (" @ %p (D = %p) ", (void *)
						     rp->supplemental.p2,
						     (void *)
						     rp->supplemental.p1);
					}
#endif
					DBG_logF("\n");
					break;
				case DBG_TRACE_LOWER:
					DBG_logF
					    ("IPL lower from %" PRIuPTR
					     " to %" PRIuPTR " ",
					     rp->initial.p1, rp->initial.p2);
#ifdef CONFIG_DEBUG_TRACE_EXTRA
					if (rp[1].event == DBG_TRACE_EXTRA2) {
						DBG_stepTraceCore(&rp, 1);
						size--;
						DBG_logF
						    (" @ %p (D = %p) ", (void *)
						     rp->supplemental.p2,
						     (void *)
						     rp->supplemental.p1);
					}
#endif
					DBG_logF("\n");
					break;
#endif
				case DBG_TRACE_EXTRA1:
				case DBG_TRACE_EXTRA2:
				case DBG_TRACE_EXTRA3:
					DBG_logF
					    ("Unexpected extra %"
					     PRIuPTR " @ %p! %08"
					     PRIxPTR " / %08" PRIxPTR
					     " / %08" PRIxPTR "\n",
					     1 + rp->event -
					     DBG_TRACE_EXTRA1, rp,
					     rp->supplemental.p1,
					     rp->supplemental.p2,
					     rp->supplemental.p3);
					break;
				default:
					if (rp->event & EXIT_BIT) {
						DBG_logF("Exit ");
						DBG_printName((void
							       *)(uintptr_t)
							      ((rp->event &
								~EXIT_BIT)
							       << 2));
						DBG_logF(" = %08"
							 PRIxPTR,
							 rp->initial.p1);
#ifdef CONFIG_DEBUG_TRACE_EXTRA
						if (rp[1].event ==
						    DBG_TRACE_EXTRA2) {
							DBG_stepTraceCore(&rp,
									  1);
							size--;
							DBG_logF
							    (" @ %p(D = %p) ",
							     (void *)
							     rp->supplemental.
							     p2, (void *)
							     rp->supplemental.
							     p1);
						}
#endif
						DBG_logF("\n");
					} else {
						printf("%016" PRIxPTR
						       "\n", rp->event);
						DBG_logF("Enter ");
						DBG_printName((void *)(rp->event
								       << 2));
						DBG_logF("(%08" PRIxPTR
							 ", %08" PRIxPTR,
							 rp->initial.p1,
							 rp->initial.p2);
						DBG_stepTraceCore(&rp, 1);
						size--;
						while ((rp->event >=
							DBG_TRACE_CONTINUATION1)
						       && (rp->event
							   <=
							   DBG_TRACE_CONTINUATION3))
						{
							uintptr_t *v =
							    &rp->
							    supplemental.p1;
							switch (rp->event) {
							case DBG_TRACE_CONTINUATION3:
								DBG_logF
								    (", %08"
								     PRIxPTR,
								     *v++);
							case DBG_TRACE_CONTINUATION2:
								DBG_logF
								    (", %08"
								     PRIxPTR,
								     *v++);
							case DBG_TRACE_CONTINUATION1:
								DBG_logF
								    (", %08"
								     PRIxPTR,
								     *v++);
								break;
							default:
								break;
							}
							DBG_stepTraceCore(&rp,
									  1);
							size--;
						}

						DBG_logF(")");
#ifdef CONFIG_DEBUG_TRACE_EXTRA
						if (rp->event ==
						    DBG_TRACE_EXTRA2) {
							DBG_logF
							    (" @ %p(D = %p) ",
							     (void *)
							     rp->supplemental.
							     p2, (void *)
							     rp->supplemental.
							     p1);
							DBG_stepTraceCore(&rp,
									  1);
							size--;
						}
#endif
						DBG_logF("\n");
						continue;
					}
				}
			}
			DBG_stepTraceCore(&rp, 1);
			size--;
		} while (size > 0);
	}
	KRN_tracePtr = start;
	IRQ_restoreIPL(ipl);
}

#endif

inline static void
DBG_vtrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i, va_list ap)
    __attribute__ ((no_instrument_function));
inline static void
DBG_vtrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i, va_list ap)
{
	if (KRN_tracePtr) {
		KRN_TRACE_T *wp = *wpp;
		wp->event = event;
		wp->initial.absTime = (uintptr_t)
		    TMR_getMonotonic();

		if (i) {
			i--;
			wp->initial.p1 = va_arg(ap, uintptr_t);
		} else
			wp->initial.p1 = 0;
		if (i) {
			i--;
			wp->initial.p2 = va_arg(ap, uintptr_t);
		} else
			wp->initial.p2 = 0;
		wp = DBG_stepTraceCore(wpp, 1);
		while (i) {
			wp->event =
			    (i == 1) ? DBG_TRACE_CONTINUATION1 : (i == 2)
			    ? DBG_TRACE_CONTINUATION2 : DBG_TRACE_CONTINUATION3;
			if (i) {
				i--;
				wp->supplemental.p1 = va_arg(ap, uintptr_t);
			}
			if (i) {
				i--;
				wp->supplemental.p2 = va_arg(ap, uintptr_t);
			} else
				wp->supplemental.p2 = 0;
			if (i) {
				i--;
				wp->supplemental.p3 = va_arg(ap, uintptr_t);
			} else
				wp->supplemental.p3 = 0;
			wp = DBG_stepTraceCore(wpp, 1);
		}
	}
}

void DBG_atrace(KRN_TRACE_T ** wpp, uintptr_t event, int32_t i, uintptr_t * ap)
{
	if (KRN_tracePtr) {
		KRN_TRACE_T *wp = *wpp;

		wp->event = event;
		wp->initial.absTime = (uintptr_t)
		    TMR_getMonotonic();
		if (i) {
			i--;
			wp->initial.p1 = *ap++;
		} else
			wp->initial.p1 = 0;
		if (i) {
			i--;
			wp->initial.p2 = *ap++;
		} else
			wp->initial.p2 = 0;
		wp = DBG_stepTraceCore(wpp, 1);
		while (i) {
			wp->event =
			    (i ==
			     1) ?
			    DBG_TRACE_CONTINUATION1
			    : (i ==
			       2) ?
			    DBG_TRACE_CONTINUATION2 : DBG_TRACE_CONTINUATION3;
			i--;
			wp->supplemental.p1 = *ap++;

			if (i) {
				i--;
				wp->supplemental.p2 = *ap++;
			} else
				wp->supplemental.p2 = 0;
			if (i) {
				i--;
				wp->supplemental.p3 = *ap++;
			} else
				wp->supplemental.p3 = 0;
			wp = DBG_stepTraceCore(wpp, 1);
		}
	}
}

void DBG_trace(KRN_TRACE_T ** wp, uintptr_t event, uint32_t i, ...)
{
	va_list ap;
	if (!*wp)
		return;
	va_start(ap, i);
	DBG_vtrace(wp, event, i, ap);
	va_end(ap);
}

void DBG_enter(KRN_TRACE_T ** wp, void *fn, uint32_t i, ...)
{
	va_list ap;
	va_start(ap, i);
	DBG_vtrace(wp, ((uintptr_t) fn) >> 2, i, ap);
	va_end(ap);
}

void DBG_exit(KRN_TRACE_T ** wp, void *fn, uint32_t i, ...)
{
	va_list ap;
	va_start(ap, i);
	DBG_vtrace(wp, (((uintptr_t) fn) >> 2) | EXIT_BIT, i, ap);
	va_end(ap);
}

void DBG_ctxSw(int32_t reason, void *vfrom, void *vto)
{
	KRN_TASK_T *from = (KRN_TASK_T *) vfrom, *to = (KRN_TASK_T *) vto;
	KRN_TRACE_T *wp = DBG_openTraceCore(2);
	DBG_trace(&wp, (uintptr_t) DBG_TRACE_CTX_SW,
		  5, reason, from, to, DBG_PC(from), DBG_PC(to));

#if defined(CONFIG_DEBUG_TRACE_CTXSW_BT_HARD) || defined(CONFIG_DEBUG_TRACE_CTXSW_BT_SOFT)
	if (from)
		DBG_backtraceCtx(&from->savedContext);
	if (to)
		DBG_backtraceCtx(&to->savedContext);
#endif
}

void DBG_interruptIn(int32_t i)
{
	KRN_TRACE_T *wp = DBG_openTraceCore(1);
	DBG_trace(&wp, DBG_TRACE_ENTER_ISR, 2, i, DBG_PC(_KRN_current));
}

void DBG_interruptSched(void)
{
	KRN_TRACE_T *wp = DBG_openTraceCore(1);
	DBG_trace(&wp, DBG_TRACE_SCHED_ISR, 1, DBG_PC(_KRN_current));
}

void DBG_interruptOut(int32_t i)
{
	KRN_TRACE_T *wp = DBG_openTraceCore(1);
	DBG_trace(&wp, DBG_TRACE_EXIT_ISR, 2, i, DBG_PC(_KRN_current));
}

void DBG_hotwire(KRN_CTX_T * ctx)
{
	KRN_TRACE_T *wp = DBG_openTraceCore(1);
	DBG_trace(&wp, DBG_TRACE_HOTWIRE_ISR, 2, ctx, DBG_PC(_KRN_current));
}

void DBG_chain(KRN_CTX_T * ctx)
{
	KRN_TRACE_T *wp = DBG_openTraceCore(1);
	DBG_trace(&wp, DBG_TRACE_CHAIN_UHI, 2, ctx, DBG_PC(_KRN_current));
}

#if defined(CONFIG_DEBUG_TRACE_LOG_SOFT) || defined(CONFIG_DEBUG_TRACE_LOG_HARD)
void DBG_logT(const char *message, ...)
{
	size_t l;
	va_list ap;
	char buffer[1024];
	uintptr_t *s = (uintptr_t *) buffer;

	va_start(ap, message);
	vsnprintf(buffer, 1023, message, ap);
	va_end(ap);

	l = strlen(buffer);
	KRN_TRACE_T *wp = DBG_openTraceCore(1 + ((l + 3) / 12));
	DBG_atrace(&wp, DBG_TRACE_LOG, (l + 3) / 4, s);
}

void DBG_putT(const char *message)
{
	size_t l;
	uintptr_t *s = (uintptr_t *) message;
	l = strlen(message);
	KRN_TRACE_T *wp = DBG_openTraceCore(1 + ((l + 3) / 12));
	DBG_atrace(&wp, DBG_TRACE_LOG, (l + 3) / 4, s);
}
#endif

#ifdef CONFIG_DEBUG_TRACE_EXTRA
void DBG_walkExtra(KRN_TRACE_T ** wpp, uint32_t depth)
{
	if (*wpp) {
		uintptr_t ra;
		KRN_TRACE_T *wp = *wpp;

		DBG_walk(depth + 1, &ra, NULL);

		wp->event = DBG_TRACE_EXTRA2;
		wp->supplemental.p1 = DBG_extra();
		wp->supplemental.p2 = ra;
		wp->supplemental.p3 = 0;
		DBG_stepTraceCore(wpp, 1);
	}
}
#else
void DBG_walkExtra(KRN_TRACE_T ** wpp, uint32_t depth)
{
}
#endif

#endif
