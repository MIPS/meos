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
*   Description:	Subtree copy functions
*
*************************************************************************/

#include "meos/config.h"
#include "meos/trees/tre.h"
#include "meos/debug/dbg.h"

#ifdef CONFIG_DEBUG_PARANOIA
extern TRE_T _paraItem_Tree;
extern DBG_PARATYPE_T _paraDesc_TreI, _paraDesc_Tree;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
** FUNCTION:      TRE_initCopySubTree
**
** DESCRIPTION:   Prepare for subtree copy
**
** RETURNS:       void *
*/
void *TRE_initCopySubTree(void *subTree, TRE_T * tree, void *destSubTree,
			  TRE_T * destTree, TRE_COPY_T * copy)
{
	PARACHECK();

	copy->sourceRoot = subTree;
	copy->destRoot = destSubTree;
	copy->destCurrent = destSubTree;
	copy->sourceCurrent = subTree;
	copy->sourcePrev = NULL;
	copy->sourceTree = tree;

	/* ascertain that the destination is not a subtree of the source
	 */

	if ((TRE_isInSubTree(subTree, destSubTree, destTree)))
		return (NULL);

	return (copy->sourceRoot);
}

/*
** FUNCTION:      TRE_copyNextInSubTree
**
** DESCRIPTION:   Copy one item of a sub-tree
**
** RETURNS:       void *
*/
void *TRE_copyNextInSubTree(TRE_COPY_T * copy, void *looseItem)
{
	PARACHECK();
	PARADEL(TreI, looseItem);

	if (!copy->sourcePrev) {
		TRE_addChild(looseItem, copy->destRoot);
		copy->subRoot = looseItem;	/* Log this item for later */
	} else if (TRE_areSiblings(copy->sourceCurrent, copy->sourcePrev)) {
		TRE_addSibling(looseItem, copy->destCurrent);
	} else {
		/* must link to correct parent in new subtree:
		   do this by exploiting isomorphism in those parts of the
		   source and destination subtrees already copied so far. */

		/* Counts the number of steps within the underlying list
		   structure from a to b counting only those objects that lie
		   within the subtree. */
		void *next = copy->sourceRoot;
		void *end = TRE_parent(copy->sourceCurrent);
		int32_t steps = -1;

		do {
			steps++;

			if (next == end)
				break;

		} while (NULL !=
			 (next =
			  TRE_subTreeNext(copy->sourceRoot, next,
					  (TRE_T *) copy->sourceTree)));

		{
			void *a;
			int32_t i;

			a = copy->subRoot;
			for (i = 0; i < steps; i++)
				a = TRE_subTreeNext(copy->subRoot, a,
						    (TRE_T *) copy->sourceTree);
			TRE_addChild(looseItem, a);
		}
	}

	/* Increment the source and destination item pointers */
	copy->sourcePrev = copy->sourceCurrent;
	copy->sourceCurrent = TRE_subTreeNext(copy->sourceRoot,
					      copy->sourceCurrent,
					      (TRE_T *) copy->sourceTree);
	copy->destCurrent = looseItem;

	PARAADDI(TreI, looseItem);
	PARACHECK();

	/* return the next source item to copy
	   or NULL if it is time to stop. */
	return (copy->sourceCurrent);
}
