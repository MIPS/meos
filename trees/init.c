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
*   Description:	TRE_initTree
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

#ifdef CONFIG_DEBUG_PARANOIA
void *_TRE_getChild(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_TRE_getChild(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return TRE_firstChild(vitem);
}

void *_TRE_getNext(void *vtype, void *vlist, void *vitem)
    __attribute__ ((no_instrument_function));
void *_TRE_getNext(void *vtype, void *vlist, void *vitem)
{
	(void)vtype;
	(void)vlist;
	return TRE_nextSibling(vitem);
}
#endif

PARATYPE(TreI, TRE_LINKAGE_T, _TRE_getChild, &_paraDesc_TreI,
	 _DBG_paranoidGetChildType, NULL, _TRE_getNext);
PARATYPE(Tree, TRE_T, _TRE_getChild, &_paraDesc_TreI,
	 _DBG_paranoidGetChildType, NULL, _DBG_paranoidGetNext);

/*
** FUNCTION:      TRE_initTree
**
** DESCRIPTION:   Initialise a tree
**
** RETURNS:       void
*/

void TRE_initTree(TRE_T * tree)
{
	PARACHECK();
	PARADEL(Tree, tree);

	tree->root = NULL;
	tree->tip = NULL;

	PARAADD(Tree, tree);
	PARACHECK();
}
