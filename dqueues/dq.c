/***(C)2001***************************************************************
*
* Copyright (C) 2001 MIPS Tech, LLC
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
****(C)2001**************************************************************/

/*************************************************************************
*
*   Description:	Double linked queues
*
*************************************************************************/

/* controls for inline compilation */
#define DQ_COMPILE
#ifndef DQ_BUILDC
#ifdef DQ_INLINE
#ifndef DQ_CINCLUDE
#undef DQ_COMPILE		/* only compile through the H file when inlining */
#endif
#endif
#else
#undef DQ_INLINE
#undef DQ_FQUALS
#define DQ_FQUALS
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef DQ_COMPILE

#ifndef DQ_INLINE
#include "meos/dqueues/dq.h"	/* don't re-include H file if inlining */
#endif

#include "meos/debug/dbg.h"

#ifdef CONFIG_DEBUG_PARANOIA
#ifndef DQ_INLINE
void *DQ_first(DQ_T * queue);
void *DQ_next(void *item);

void *_DQ_getChild(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_DQ_getChild(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return vitem;
}

void *_DQ_getNext(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_DQ_getNext(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	void *last = (void *)DQ_last((DQ_T *) vlist);
	return (last) ? ((last == vitem) ? (NULL) : DQ_next((DQ_T *) vitem))
	    : (NULL);
}

PARATYPE(DqIt, DQ_LINKAGE_T, NULL, NULL, NULL, NULL, _DQ_getNext);
PARATYPE(Dque, DQ_LINKAGE_T, _DQ_getChild, &_paraDesc_DqIt,
	 _DBG_paranoidGetChildType, NULL, _DBG_paranoidGetNext);
#else
extern DQ_T _paraItem_Dque;
extern DBG_PARATYPE_T _paraDesc_DqIt, _paraDesc_Dque;
#endif
#endif

#define DQ_EMPTY(Q) ((Q)->DQ_link.fwd == (DQ_LINKAGE_T *)(Q))

DQ_FQUALS void DQ_init(DQ_T * queue)
{
	PARACHECK();
	PARADEL(Dque, queue);

	queue->DQ_link.fwd = (DQ_LINKAGE_T *) queue;
	queue->DQ_link.back = (DQ_LINKAGE_T *) queue;

	PARAADD(Dque, queue);
	PARACHECK();
}

DQ_FQUALS void DQ_addHead(DQ_T * queue, void *item)
{
	PARACHECK();
	PARADEL(DqIt, item);

	((DQ_LINKAGE_T *) item)->back = (DQ_LINKAGE_T *) queue;
	((DQ_LINKAGE_T *) item)->fwd = ((DQ_LINKAGE_T *) queue)->fwd;
	((DQ_LINKAGE_T *) queue)->fwd->back = (DQ_LINKAGE_T *) item;
	((DQ_LINKAGE_T *) queue)->fwd = (DQ_LINKAGE_T *) item;

	PARAADDI(DqIt, item);
	PARACHECK();
}

DQ_FQUALS void DQ_addTail(DQ_T * queue, void *item)
{
	PARACHECK();
	PARADEL(DqIt, item);

	((DQ_LINKAGE_T *) item)->fwd = (DQ_LINKAGE_T *) queue;
	((DQ_LINKAGE_T *) item)->back = ((DQ_LINKAGE_T *) queue)->back;
	((DQ_LINKAGE_T *) queue)->back->fwd = (DQ_LINKAGE_T *) item;
	((DQ_LINKAGE_T *) queue)->back = (DQ_LINKAGE_T *) item;

	PARAADDI(DqIt, item);
	PARACHECK();
}

DQ_FQUALS int32_t DQ_empty(DQ_T * queue)
{
	return DQ_EMPTY(queue);
}

DQ_FQUALS void *DQ_first(DQ_T * queue)
{
	DQ_LINKAGE_T *temp = queue->DQ_link.fwd;

	return temp == (DQ_LINKAGE_T *) queue ? NULL : temp;
}

DQ_FQUALS void *DQ_last(DQ_T * queue)
    __attribute__ ((no_instrument_function));
DQ_FQUALS void *DQ_last(DQ_T * queue)
{
	DQ_LINKAGE_T *temp = queue->DQ_link.back;

	return temp == (DQ_LINKAGE_T *) queue ? NULL : temp;
}

DQ_FQUALS void *DQ_next(void *item)
    __attribute__ ((no_instrument_function));
DQ_FQUALS void *DQ_next(void *item)
{
	return ((DQ_LINKAGE_T *) item)->fwd;
}

DQ_FQUALS void *DQ_previous(void *item)
{
	return ((DQ_LINKAGE_T *) item)->back;
}

DQ_FQUALS void DQ_remove(void *item)
{
	PARACHECK();
	PARADEL(DqIt, item);

	((DQ_LINKAGE_T *) item)->fwd->back = ((DQ_LINKAGE_T *) item)->back;
	((DQ_LINKAGE_T *) item)->back->fwd = ((DQ_LINKAGE_T *) item)->fwd;

	/* make item linkages safe for "orphan" removes */
	((DQ_LINKAGE_T *) item)->fwd = (struct DQ_tag *)item;
	((DQ_LINKAGE_T *) item)->back = (struct DQ_tag *)item;

	PARACHECK();
}

DQ_FQUALS void *DQ_removeHead(DQ_T * queue)
{
	PARACHECK();

	DQ_LINKAGE_T *temp;

	if DQ_EMPTY
		(queue)
		    return NULL;

	temp = ((DQ_LINKAGE_T *) queue)->fwd;
	temp->fwd->back = temp->back;
	temp->back->fwd = temp->fwd;

	/* make item linkages safe for "orphan" removes */
	temp->fwd = temp;
	temp->back = temp;

	PARADEL(DqIt, temp);
	PARACHECK();

	return temp;
}

DQ_FQUALS void *DQ_removeTail(DQ_T * queue)
{
	PARACHECK();

	DQ_LINKAGE_T *temp;

	if DQ_EMPTY
		(queue)
		    return NULL;

	temp = ((DQ_LINKAGE_T *) queue)->back;
	temp->fwd->back = temp->back;
	temp->back->fwd = temp->fwd;

	/* make item linkages safe for "orphan" removes */
	temp->fwd = temp;
	temp->back = temp;

	PARADEL(DqIt, temp);
	PARACHECK();

	return temp;
}

DQ_FQUALS void DQ_addBefore(void *successor, void *item)
{
	PARACHECK();
	PARADEL(DqIt, item);

	((DQ_LINKAGE_T *) item)->fwd = (DQ_LINKAGE_T *) successor;
	((DQ_LINKAGE_T *) item)->back = ((DQ_LINKAGE_T *) successor)->back;
	((DQ_LINKAGE_T *) item)->back->fwd = (DQ_LINKAGE_T *) item;
	((DQ_LINKAGE_T *) successor)->back = (DQ_LINKAGE_T *) item;

	PARAADDI(DqIt, item);
	PARACHECK();
}

DQ_FQUALS void DQ_addAfter(void *predecessor, void *item)
{
	PARACHECK();
	PARADEL(DqIt, item);

	((DQ_LINKAGE_T *) item)->fwd = ((DQ_LINKAGE_T *) predecessor)->fwd;
	((DQ_LINKAGE_T *) item)->back = (DQ_LINKAGE_T *) predecessor;
	((DQ_LINKAGE_T *) item)->fwd->back = (DQ_LINKAGE_T *) item;
	((DQ_LINKAGE_T *) predecessor)->fwd = (DQ_LINKAGE_T *) item;

	PARAADDI(DqIt, item);
	PARACHECK();
}

DQ_FQUALS void DQ_move(DQ_T * from, DQ_T * to)
{
	PARACHECK();

	if DQ_EMPTY
		(from)
		    DQ_init(to);
	else {
		*to = *from;
		to->DQ_link.fwd->back = (DQ_LINKAGE_T *) to;
		to->DQ_link.back->fwd = (DQ_LINKAGE_T *) to;
		DQ_init(from);
	}

	PARACHECK();
}

#endif
