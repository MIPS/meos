/***(C)2005***************************************************************
*
* Copyright (C) 2005 MIPS Tech, LLC
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
****(C)2005**************************************************************/

/*************************************************************************
*
*   Description:	TRE_addChild
*
*************************************************************************/

#include <stddef.h>
#include "meos/config.h"
#include "meos/trees/tre.h"
#include "meos/debug/dbg.h"

#ifdef CONFIG_DEBUG_PARANOIA
extern TRE_T _paraItem_Tree;
extern DBG_PARATYPE_T _paraDesc_TreI, _paraDesc_Tree;
#endif

/*
** FUNCTION:      TRE_addChild
**
** DESCRIPTION:   Adds children to the tree such that siblings are contiguous
**                in the list and children are subsequent to their parents.
**                Overloaded to add as root if tree object is passed in.
**
** RETURNS:       void
*/

void TRE_addChild(void *item, void *parent)
{
	PARACHECK();
	PARADEL(TreI, item);
	TRE_LINKAGE_T *addBefore;

	/* root item points to the tree object */
	((TRE_LINKAGE_T *) item)->parent = parent;

	/* Add as final sibling if there are any other siblings,
	   otherwise at end of the whole list. */
	while ((addBefore =
		(TRE_LINKAGE_T *) ((TRE_LINKAGE_T *) parent)->next) != NULL) {
		if (TRE_areSiblings(addBefore, item)) {
			parent = TRE_finalSibling(addBefore);
			break;
		}
		parent = addBefore;
	}

	/* Insert the item */
	((TRE_LINKAGE_T *) item)->next = ((TRE_LINKAGE_T *) parent)->next;	/* root starts with null next */
	((TRE_LINKAGE_T *) parent)->next = item;	/* root is also tree tip */

	PARAADDI(TreI, item);
	PARACHECK();
	return;
}
