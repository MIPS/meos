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
*   Description:	Paranoia tracking
*
*************************************************************************/

#include <stdint.h>
#include "meos/config.h"
#include "meos/kernel/krn.h"
#include "meos/ctx/ctx.h"
#include "meos/debug/dbg.h"
#include "meos/lists/lst.h"
#include "meos/irq/irq.h"

#ifdef CONFIG_DEBUG_PARANOIA

int32_t _DBG_disableParanoia = 0;

#define S(X) (((X) > 31) ? (X) : '?')

extern uintptr_t _DBG_intStackStart, _DBG_intStackEnd;

void *_DBG_paranoidGetNext(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_DBG_paranoidGetNext(void *vtype, void *vlist, void *vitem)
{
	(void)vlist;
	DBG_PARATYPE_T *type = (DBG_PARATYPE_T *) vtype;
	uintptr_t *item = (uintptr_t *) vitem;
	if (DBG_badPtr((uintptr_t) & item[type->nextOffset]))
		return NULL;
	else
		return (void *)item[type->nextOffset];
}

void *_DBG_paranoidGetTopLevelChildType(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));

void *_DBG_paranoidGetTopLevelChild(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_DBG_paranoidGetTopLevelChild(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	DBG_PARATYPE_T *item = (DBG_PARATYPE_T *) vitem;
	return item->child;
}

DBG_PARATYPE_T _paraItem_Para;
#if !defined(__cplusplus)
DBG_PARATYPE_T _paraDesc_Para = {
	._frontSentinel = 0x7e117a1e,
	._fourcc = 0x50617261,
	.frontOffset =
	    offsetof(DBG_PARATYPE_T, _frontSentinel) / sizeof(uintptr_t),
	.fourccOffset = offsetof(DBG_PARATYPE_T, _fourcc) / sizeof(uintptr_t),
	.fourcc = 0x50617261,
	.nextOffset = offsetof(DBG_PARATYPE_T, _nextPar) / sizeof(uintptr_t),
	.backOffset =
	    offsetof(DBG_PARATYPE_T, _backSentinel) / sizeof(uintptr_t),
	.child = &_paraItem_Para,
	.getChild = _DBG_paranoidGetTopLevelChild,
	.childType = NULL,
	.getChildType = _DBG_paranoidGetTopLevelChildType,
	.getNext = _DBG_paranoidGetNext,
	._nextPar = NULL,
	._backSentinel = 0x5e1f1e55
};
#else
void _DBG_paraParaInit(void)
{
	if (_paraDesc_Para._frontSentinel != 0x7e117a1e) {
		_paraDesc_Para._frontSentinel = 0x7e117a1e;
		_paraDesc_Para._fourcc = 0x50617261;
		_paraDesc_Para.frontOffset =
		    offsetof(DBG_PARATYPE_T,
			     _frontSentinel) / sizeof(uintptr_t);
		_paraDesc_Para.fourccOffset =
		    offsetof(DBG_PARATYPE_T, _fourcc) / sizeof(uintptr_t);
		_paraDesc_Para.fourcc = 0x50617261;
		_paraDesc_Para.nextOffset =
		    offsetof(DBG_PARATYPE_T, _nextPar) / sizeof(uintptr_t);
		_paraDesc_Para.backOffset =
		    offsetof(DBG_PARATYPE_T, _backSentinel) / sizeof(uintptr_t);
		_paraDesc_Para.child = &_paraItem_Para;
		_paraDesc_Para.getChild = _DBG_paranoidGetTopLevelChild;
		_paraDesc_Para.childType = NULL;
		_paraDesc_Para.getChildType = _DBG_paranoidGetTopLevelChildType;
		_paraDesc_Para.getNext = _DBG_paranoidGetNext;
		_paraDesc_Para._nextPar = NULL;
		_paraDesc_Para._backSentinel = 0x5e1f1e55;
	}
	DBG_goodPtr((uintptr_t) &
		    ((uintptr_t *) &
		     _paraItem_Para)[_paraDesc_Para.nextOffset]);
}

#endif

void _paraConstructor_Para(void)
    __attribute__ ((constructor, no_instrument_function));
void _paraConstructor_Para(void)
{
#if defined(__cplusplus)
	_DBG_paraParaInit();
#endif
	DBG_goodPtr((uintptr_t) &
		    ((uintptr_t *) &
		     _paraItem_Para)[_paraDesc_Para.nextOffset]);
}

void *_DBG_paranoidGetTopLevelChildType(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return vitem;
}

/* We can't use the data types, or we'll recurse */
void _DBG_paranoidRemoveList(DBG_PARATYPE_T * type, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void _DBG_paranoidRemoveList(DBG_PARATYPE_T * type, void *vlist, void *vitem)
{
	uintptr_t *list = (uintptr_t *) vlist;
	uintptr_t *current, *compensated =
	    &((uintptr_t *) vitem)[-type->frontOffset];
	/*  uintptr_t *next, *nextcomp; */
	uintptr_t *last = list;
	void *vchild;
	if ((!vitem)
	    || ((!DBG_badPtr((uintptr_t) & compensated[type->nextOffset])) &&
		(compensated[type->nextOffset] == (uintptr_t) - 1)))
		return;
	/*  printf("|\n"); */
	for (current = (uintptr_t *) type->getNext(type, list, list); current;
	     current = (uintptr_t *) type->getNext(type, list, compensated)) {
		/*  printf("%p\n", current); */

		compensated = &current[-type->frontOffset];
		/*  next = type->getNext(type, list, compensated); */
		/*  nextcomp = &next[-type->frontOffset]; */
		/*  printf("%c%c%c%c:current=%p vitem=%p (%p/%p/%p/%p/%p/%p)\n", */
		/*         (int8_t)((type->fourcc & 0xff000000) >> 24), */
		/*         (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
		/*         (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
		/*         (int8_t)((type->fourcc & 0x000000ff)), current, vitem, */
		/*         vlist, vitem, current, compensated, next, nextcomp); */
		/*  if ((next != NULL) && ((uintptr_t) next < 0x80000000)) */
		/*       for (;;) ; */
		if (current == vitem) {
			/*  printf */
			/*      ("%c%c%c%c:%p on %p removing current[%lu]=0, last(%p)=%08lx.\n", */
			/*       (int8_t)((type->fourcc & 0xff000000) >> 24), */
			/*       (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
			/*       (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
			/*       (int8_t)((type->fourcc & 0x000000ff)), compensated, */
			/*       list, type->nextOffset, last, */
			/*       compensated[type->nextOffset]); */
			last[type->nextOffset] = compensated[type->nextOffset];
			compensated[type->nextOffset] = 0;
			return;
		}
		vchild =
		    type->getChild ? type->getChild(type, list,
						    compensated) : NULL;
		if (vchild) {
			/*  printf(">\n"); */
			_DBG_paranoidRemoveList((DBG_PARATYPE_T *)
						type->getChildType(type, list,
								   compensated),
						vchild, vitem);
			/*  printf("<\n"); */
		}
		last = compensated;
	}
	/*  printf("|\n"); */
}

void _DBG_paranoidRemove(DBG_PARATYPE_T * type, void *vitem)
    __attribute__ ((no_instrument_function));
void _DBG_paranoidRemove(DBG_PARATYPE_T * type, void *vitem)
{
	uintptr_t *item = (uintptr_t *) vitem;
	/*  printf */
	/*      ("removing %c%c%c%c:%p\nremove>\n", */
	/*       (int8_t)((type->fourcc & 0xff000000) >> 24), */
	/*       (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
	/*       (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
	/*       (int8_t)((type->fourcc & 0x000000ff)), vitem); */
	PARACHECK();
	IRQ_IPL_T oldIPL = IRQ_raiseIPL();
	_DBG_paranoidRemoveList(&_paraDesc_Para, &_paraItem_Para,
				&item[type->frontOffset]);
	/*  printf("<remove\n"); */
	IRQ_restoreIPL(oldIPL);
}

void _DBG_paranoidAddItem(DBG_PARATYPE_T * type, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void _DBG_paranoidAddItem(DBG_PARATYPE_T * type, void *vlist, void *vitem)
{
	(void)vlist;
	IRQ_IPL_T oldIPL = IRQ_raiseIPL();

	uintptr_t *item = (uintptr_t *) vitem;

	/*  printf */
	/*      ("adding %c%c%c%c:%p to %p\n", */
	/*       (int8_t)((type->fourcc & 0xff000000) >> 24), */
	/*       (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
	/*       (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
	/*       (int8_t)((type->fourcc & 0x000000ff)), vitem, vlist); */

	/* Protect */
	item[type->frontOffset] = 0x7e117a1e;
	item[type->fourccOffset] = type->fourcc;
	item[type->backOffset] = 0x5e1f1e55;
	DBG_goodPtr((uintptr_t) & item[type->nextOffset]);
	item[type->nextOffset] = 0;

	IRQ_restoreIPL(oldIPL);
}

void _DBG_paranoidAdd(DBG_PARATYPE_T * type, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void _DBG_paranoidAdd(DBG_PARATYPE_T * type, void *vlist, void *vitem)
{
	if (_DBG_disableParanoia)
		return;

	IRQ_init(NULL, 0);

	IRQ_IPL_T oldIPL = IRQ_raiseIPL();

	uintptr_t *list = (uintptr_t *) vlist;
	uintptr_t *item = (uintptr_t *) vitem;
	uintptr_t *current = (uintptr_t *) type->getNext(type, list,
							 list), *compensated =
	    &current[-type->frontOffset];
	/* uintptr_t last; */

	/* Check whether this list is already added */
	while (current) {
		if (current == item) {
			IRQ_restoreIPL(oldIPL);
			return;
		}
		current = (uintptr_t *) type->getNext(type, list, compensated);
		compensated = &current[-type->frontOffset];
	}

	_DBG_paranoidAddItem(type, vlist, vitem);

	/* Add to head */
	/* last = list[type->nextOffset]; */
	DBG_goodPtr((uintptr_t) & item[type->nextOffset]);
	item[type->nextOffset] = list[type->nextOffset];
	DBG_goodPtr((uintptr_t) & list[type->nextOffset]);
	list[type->nextOffset] = (uintptr_t) & item[type->frontOffset];
	/*  printf("%c%c%c%c:%p added on %p before %08lx\n", */
	/*         (int8_t)((type->fourcc & 0xff000000) >> 24), */
	/*         (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
	/*         (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
	/*         (int8_t)((type->fourcc & 0x000000ff)), vitem, vlist, last); */

	IRQ_restoreIPL(oldIPL);
}

int32_t _DBG_paranoidVerifyItem(DBG_PARATYPE_T * type, void *vitem)
    __attribute__ ((no_instrument_function));
int32_t _DBG_paranoidVerifyItem(DBG_PARATYPE_T * type, void *vitem)
{
	/*  printf("%c%c%c%c:%p verified\n", */
	/*         (int8_t)((type->fourcc & 0xff000000) >> 24), */
	/*         (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
	/*         (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
	/*         (int8_t)((type->fourcc & 0x000000ff)), vitem); */
	uintptr_t *item = (uintptr_t *) vitem;
	return ((item[type->frontOffset] == 0x7e117a1e)
		&& (item[type->fourccOffset] == type->fourcc)
		&& (item[type->backOffset] == 0x5e1f1e55));
}

void _DBG_paranoidVerifyList(DBG_PARATYPE_T * type, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void _DBG_paranoidVerifyList(DBG_PARATYPE_T * type, void *vlist, void *vitem)
{
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(3);
	DBG_enter(&wp, (void *)_DBG_paranoidVerifyList, 3, type, vlist, vitem);
	DBG_walkExtra(&wp, 1);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, (void *)_DBG_paranoidVerifyList, 3, type, vlist, vitem);
#endif
#endif
	uintptr_t *list = (uintptr_t *) vitem;
	uintptr_t *current =
	    (uintptr_t *) type->getNext(type, list, list), *compensated =
	    &current[-type->frontOffset], *last = list;
	/* uintptr_t *next, *nextcomp; */
	void *vchild;
	while (current) {
		/* next = type->getNext(type, list, compensated); */
		/* nextcomp = &next[-type->frontOffset]; */
		/*  printf("%c%c%c%c:%p/%p/%p/%p/%p/%p\n", */
		/*         (int8_t)((type->fourcc & 0xff000000) >> 24), */
		/*         (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
		/*         (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
		/*         (int8_t)((type->fourcc & 0x000000ff)), vlist, vitem, */
		/*         current, compensated, next, nextcomp); */
		if (!_DBG_paranoidVerifyItem(type, compensated))
			/*  printf */
			/*      ("%c%c%c%c:%p on %p failed verification. Possibly a %c%c%c%c?\n", */
			/*       (int8_t)((type->fourcc & 0xff000000) >> 24), */
			/*       (int8_t)((type->fourcc & 0x00ff0000) >> 16), */
			/*       (int8_t)((type->fourcc & 0x0000ff00) >> 8), */
			/*       (int8_t)((type->fourcc & 0x000000ff)), compensated, */
			/*       vlist, */
			/*       (int8_t)((compensated[type->fourccOffset] & */
			/*            0xff000000) >> 24), */
			/*       (int8_t)((compensated[type->fourccOffset] & */
			/*            0x00ff0000) >> 16), */
			/*       (int8_t)((compensated[type->fourccOffset] & */
			/*            0x0000ff00) >> 8), */
			/*       (int8_t)((compensated[type->fourccOffset] & */
			/*            0x000000ff))); */
			DBG_assert(_DBG_paranoidVerifyItem(type, compensated),
				   "%c%c%c%c:%p on %p failed verification. Possibly a %c%c%c%c? Got here via %c%c%c%c:%p\n",
				   S((int8_t)
				     ((type->fourcc & 0xff000000) >> 24)),
				   S((int8_t)
				     ((type->fourcc & 0x00ff0000) >> 16)),
				   S((int8_t)
				     ((type->fourcc & 0x0000ff00) >> 8)),
				   S((int8_t) ((type->fourcc & 0x000000ff))),
				   compensated, vlist, S((int8_t)
							 ((compensated
							   [type->fourccOffset]
							   & 0xff000000)
							  >> 24)), S((int8_t)
								     ((compensated[type->fourccOffset] & 0x00ff0000)
								      >> 16)),
				   S((int8_t)
				     ((compensated[type->fourccOffset] &
				       0x0000ff00)
				      >> 8)), S((int8_t)
						((compensated
						  [type->fourccOffset] &
						  0x000000ff))), S((int8_t)
								   ((last
								     [type->
								      fourccOffset]
								     &
								     0xff000000)
								    >> 24)),
				   S((int8_t)
				     ((last[type->fourccOffset] & 0x00ff0000)
				      >> 16)), S((int8_t)
						 ((last[type->fourccOffset] &
						   0x0000ff00)
						  >> 8)), S((int8_t)
							    ((last
							      [type->fourccOffset]
							      & 0x000000ff))),
				   last);
		vchild =
		    type->getChild ? type->getChild(type, list,
						    compensated) : NULL;
		if (vchild)
			_DBG_paranoidVerifyList((DBG_PARATYPE_T *)
						type->getChildType(type, list,
								   compensated),
						compensated, vchild);
		current = (uintptr_t *) type->getNext(type, list, compensated);
		last = compensated;
		compensated = &current[-type->frontOffset];
	}
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	wp = DBG_openTrace(2);
	DBG_exit(&wp, (void *)_DBG_paranoidVerifyList, 0);
	DBG_walkExtra(&wp, 0);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	wp = DBG_openTrace(1);
	DBG_exit(&wp, (void *)_DBG_paranoidVerifyList, 0);
#endif
#endif
}

void DBG_paranoidSweepList(DBG_PARATYPE_T * type, void *vlist, void *vitem,
			   uintptr_t start, uintptr_t end)
    __attribute__ ((no_instrument_function));
void DBG_paranoidSweepList(DBG_PARATYPE_T * type, void *vlist, void *vitem,
			   uintptr_t start, uintptr_t end)
{
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(3);
	DBG_enter(&wp, (void *)DBG_paranoidSweepList, 5, type, vlist, vitem,
		  start, end);
	DBG_walkExtra(&wp, 1);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, (void *)DBG_paranoidSweepList, 5, type, vlist, vitem,
		  start, end);
#endif
#endif
	(void)vlist;
	uintptr_t *list = (uintptr_t *) vitem;
	uintptr_t *current =
	    (uintptr_t *) type->getNext(type, list, list), *compensated =
	    &current[-type->frontOffset];
	uintptr_t *last, *lastcur;
	void *vchild;
	while (current) {
		vchild =
		    type->getChild ? type->getChild(type, list,
						    compensated) : NULL;
		if (vchild)
			DBG_paranoidSweepList((DBG_PARATYPE_T *)
					      type->getChildType(type, list,
								 compensated),
					      list, vchild, start, end);
		lastcur = current;
		last = compensated;
		current = (uintptr_t *) type->getNext(type, list, compensated);
		compensated = &current[-type->frontOffset];
		if (((uintptr_t) last >= start) && ((uintptr_t) last <= end)) {
			/*  printf("Sweeping %p\n", last); */
			_DBG_paranoidRemoveList(type, list, lastcur);
		}
	}
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	wp = DBG_openTrace(2);
	DBG_exit(&wp, (void *)DBG_paranoidSweepList, 0);
	DBG_walkExtra(&wp, 0);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	wp = DBG_openTrace(1);
	DBG_exit(&wp, (void *)DBG_paranoidSweepList, 0);
#endif
#endif
}

void *_DBG_paranoidGetChildType(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_DBG_paranoidGetChildType(void *vtype, void *vlist, void *vitem)
{
	(void)vlist;
	(void)vitem;
	DBG_PARATYPE_T *type = (DBG_PARATYPE_T *) vtype;
	return type->childType;
}

void _DBG_paranoidVerify()
    __attribute__ ((no_instrument_function));
void _DBG_paranoidVerify()
{
	if (!DBG_paranoiaAllowed())
		return;
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, (void *)_DBG_paranoidVerify, 0);
	DBG_walkExtra(&wp, 1);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(1);
	DBG_enter(&wp, (void *)_DBG_paranoidVerify, 0);
#endif
#endif
	IRQ_IPL_T oldIPL = IRQ_raiseIPL();
	DBG_PPL_T oldpar;
	oldpar = DBG_raisePPL();
	/*  printf("verify>\n"); */
	_DBG_paranoidVerifyList(&_paraDesc_Para, NULL, &_paraItem_Para);
	/*  printf("<verify\n"); */
	DBG_restorePPL(oldpar);
	IRQ_restoreIPL(oldIPL);
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	wp = DBG_openTrace(2);
	DBG_exit(&wp, (void *)_DBG_paranoidVerify, 0);
	DBG_walkExtra(&wp, 0);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	wp = DBG_openTrace(1);
	DBG_exit(&wp, (void *)_DBG_paranoidVerify, 0);
#endif
#endif
}

void DBG_paranoidSweep(uintptr_t start, uintptr_t end)
    __attribute__ ((no_instrument_function));
void DBG_paranoidSweep(uintptr_t start, uintptr_t end)
{
	/* printf("SWEEP %p-%p\n", (void *)start, (void *)end); */
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, (void *)DBG_paranoidSweep, 2, start, end);
	DBG_walkExtra(&wp, 1);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(1);
	DBG_enter(&wp, (void *)DBG_paranoidSweep, 2, start, end);
#endif
#endif
	IRQ_IPL_T oldIPL = IRQ_raiseIPL();
	DBG_PPL_T oldpar;
	oldpar = DBG_raisePPL();
	DBG_paranoidSweepList(&_paraDesc_Para, NULL, &_paraItem_Para,
			      start, end);
	/*  printf("<sweep\n"); */
	DBG_restorePPL(oldpar);
	IRQ_restoreIPL(oldIPL);
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	wp = DBG_openTrace(2);
	DBG_exit(&wp, (void *)DBG_paranoidSweep, 0);
	DBG_walkExtra(&wp, 0);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	wp = DBG_openTrace(1);
	DBG_exit(&wp, (void *)DBG_paranoidSweep, 0);
#endif
#endif
}

void *__lfn = NULL;

extern __thread KRN_TASK_T *_CTX_current;
void __cyg_profile_func_enter(void *this_fn, void *call_site)
    __attribute__ ((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	(void)this_fn;
	(void)call_site;
	IRQ_IPL_T oldipl;
	DBG_PPL_T oldpar;
	uintptr_t wm = 0;
	uintptr_t msp;

	__lfn = this_fn;
	if ((!DBG_paranoiaAllowed()) || (_DBG_disableParanoia))
		return;
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_enter(&wp, this_fn, 0);
	DBG_walkExtra(&wp, 2);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(1);
	DBG_enter(&wp, this_fn, 0);
#endif
#endif

	CTX_verifySelf(_KRN_current);
	oldipl = IRQ_raiseIPL();
	oldpar = DBG_raisePPL();

	DBG_walk(2, NULL, &msp);

#ifdef STACK_GROWS_UP
	if ((msp >= _DBG_intStackStart) && (msp <= _DBG_intStackEnd))
		wm = _DBG_intStackEnd;
	else if (_KRN_current
		 && !(wm = (uintptr_t) (_KRN_current->stackEnd - 1))) {
		if (msp > (wm = _KRN_current->wm)) {
			_KRN_current->wm = msp;
		}
	}
#else
	if ((msp >= _DBG_intStackStart) && (msp <= _DBG_intStackEnd))
		wm = _DBG_intStackStart;
	else if (_KRN_current && !(wm = (uintptr_t) _KRN_current->stackStart)) {
		if (msp < (wm = _KRN_current->wm)) {
			_KRN_current->wm = msp;
		}
	}
#endif
	DBG_restorePPL(oldpar);
	IRQ_restoreIPL(oldipl);
}

void __cyg_profile_func_exit(void *this_fn, void *call_site)
    __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
	(void)this_fn;
	(void)call_site;
	IRQ_IPL_T oldipl;
	DBG_PPL_T oldpar;
	uintptr_t wm = 0;
	uintptr_t msp;

	__lfn = this_fn;
	if ((!DBG_paranoiaAllowed()) || (_DBG_disableParanoia))
		return;
#ifdef CONFIG_DEBUG_TRACE_API
#ifdef CONFIG_DEBUG_TRACE_EXTRA
	KRN_TRACE_T *wp = DBG_openTrace(2);
	DBG_exit(&wp, this_fn, 0);
	DBG_walkExtra(&wp, 1);
#elif defined(CONFIG_DEBUG_TRACE_SOFT)
	KRN_TRACE_T *wp = DBG_openTrace(1);
	DBG_exit(&wp, this_fn, 0);
#endif
#endif
	oldipl = IRQ_raiseIPL();
	oldpar = DBG_raisePPL();

	DBG_walk(2, NULL, &msp);

#ifdef STACK_GROWS_UP
	if ((msp >= _DBG_intStackStart) && (msp <= _DBG_intStackEnd))
		wm = _DBG_intStackEnd;
	else if (_KRN_current
		 && !(wm = (uintptr_t) (_KRN_current->stackEnd - 1))) {
		if (msp > (wm = _KRN_current->wm)) {
			_KRN_current->wm = msp;
			/* No point, nothing to sweep */
			DBG_restorePPL(oldpar);
			IRQ_restoreIPL(oldipl);
			return;
		}
	}
	if (wm) {
		DBG_paranoidSweep(msp, wm);
	}
#else
	if ((msp >= _DBG_intStackStart) && (msp <= _DBG_intStackEnd))
		wm = _DBG_intStackStart;
	else if (_KRN_current && !(wm = (uintptr_t) _KRN_current->stackStart)) {
		if (msp < (wm = _KRN_current->wm)) {
			_KRN_current->wm = msp;
			/* No point, nothing to sweep */
			DBG_restorePPL(oldpar);
			IRQ_restoreIPL(oldipl);
			return;
		}
	}
	if (wm) {
		DBG_paranoidSweep(wm, msp);
	}
#endif
	DBG_restorePPL(oldpar);
	IRQ_restoreIPL(oldipl);
}
#endif
