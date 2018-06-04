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
*   Description:	Single linked lists
*
*************************************************************************/

#include "meos/config.h"

/* controls for inline compilation */
#define LST_COMPILE
#ifndef LST_BUILDC
#ifdef LST_INLINE
#ifndef LST_CINCLUDE
#undef LST_COMPILE		/* only compile through the H file when inlining */
#endif
#endif
#else
#undef LST_INLINE
#undef LST_FQUALS
#define LST_FQUALS
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef LST_COMPILE

#ifndef LST_INLINE
#include "meos/lists/lst.h"	/* don't re-include H file if inlining */
#endif

#include "meos/debug/dbg.h"

#ifdef CONFIG_DEBUG_PARANOIA
#ifndef LST_INLINE
void *_LST_getChild(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_LST_getChild(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return LST_first((LST_T *) vitem);
}

void *_LST_getNext(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_LST_getNext(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return LST_next(vitem);
}

PARATYPE(LstI, LST_LINKAGE_T, NULL, NULL, NULL, NULL, _LST_getNext);
PARATYPE(List, LST_T, _LST_getChild, &_paraDesc_LstI,
	 _DBG_paranoidGetChildType, NULL, _DBG_paranoidGetNext);
#else
PARAEXTERN(LstI, LST_LINKAGE_T);
PARAEXTERN(List, LST_T);
#endif
#endif

LST_FQUALS void LST_init(LST_T * list)
{
	PARACHECK();
	PARADEL(List, list);

	list->first = (void **)NULL;
	list->last = (void **)NULL;

	PARAADD(List, list);
	PARACHECK();
}

LST_FQUALS void LST_add(LST_T * list, void *vitem)
{
	PARACHECK();
	PARADEL(LstI, vitem);

	LST_T *item = (LST_T *) vitem;
	if (list->first == NULL) {
		list->first = (void **)vitem;
		list->last = (void **)vitem;
	} else {
		((LST_T *) list->last)->first = (void **)vitem;	/* link to last item   */
		list->last = (void **)vitem;	/* update tail pointer */
	}
	item->first = (void **)NULL;	/* terminate list      */

	PARAADDI(LstI, vitem);
	PARACHECK();
}

LST_FQUALS void LST_addHead(LST_T * list, void *vitem)
{
	PARACHECK();
	PARADEL(LstI, vitem);

	LST_T *item = (LST_T *) vitem;
	if (list->first == NULL) {
		list->first = (void **)vitem;
		list->last = (void **)vitem;
		item->first = (void **)NULL;	/* terminate list      */
	} else {
		item->first = list->first;
		list->first = (void **)vitem;	/* link to first item   */
	}

	PARAADDI(LstI, vitem);
	PARACHECK();
}

LST_FQUALS int32_t LST_empty(LST_T * list)
{
	return list->first == NULL;
}

LST_FQUALS void *LST_first(LST_T * list)
    __attribute__ ((no_instrument_function));
LST_FQUALS void *LST_first(LST_T * list)
{
	return list->first;
}

LST_FQUALS void *LST_last(LST_T * list)
{
	return list->last;
}

LST_FQUALS void *LST_next(void *item)
    __attribute__ ((no_instrument_function));
LST_FQUALS void *LST_next(void *item)
{
	return ((LST_T *) item)->first;
}

LST_FQUALS void *LST_removeHead(LST_T * list)
{
	PARACHECK();

	LST_T *current = (LST_T *) list->first;

	if (current != NULL) {
		if ((list->first = current->first) == NULL)
			list->last = (void **)NULL;
	}

	PARADEL(LstI, current);
	PARACHECK();
	return current;
}

LST_FQUALS void LST_remove(LST_T * list, void *vitem)
{
	PARACHECK();

	LST_T *current = list;

	while (current->first) {
		if (current->first == vitem) {
			current->first = ((LST_T *) current->first)->first;	/* unlink item */
			if (list->last == vitem)
				list->last = (void **)current;	/* update tail pointer when last item removed */

			PARADEL(LstI, vitem);
			PARACHECK();
			return;	/* premature return if item located and removed */
		}
		current = (LST_T *) current->first;
	}

	PARACHECK();
}

#endif
