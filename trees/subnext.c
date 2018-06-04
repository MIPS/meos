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
*   Description:	TRE_subTreeNext
*
*************************************************************************/

#include <stddef.h>
#include "meos/trees/tre.h"

/*
** FUNCTION:      TRE_subTreeNext
**
** DESCRIPTION:   Remove leaf from subtree
**
** RETURNS:       void *
*/

void *TRE_subTreeNext(void *subTreeRootNode, void *node, TRE_T * tree)
{
	TRE_LINKAGE_T *next, *parent, *jtem;

	/* if root, then can only return child: not siblings. */
	if (node == subTreeRootNode)
		return (TRE_firstChild(subTreeRootNode));

	/* Else scan the tree like a list and check ancestry. */
	next = (TRE_LINKAGE_T *) ((TRE_LINKAGE_T *) node)->next;
	if (!next)
		return (NULL);

	parent = (TRE_LINKAGE_T *) next->parent;

	while (parent != subTreeRootNode) {
		if (parent == (TRE_LINKAGE_T *) tree) {	/* start checking ancestry again with the next in the tree
							   that is not of this family  */
			while (NULL !=
			       (jtem = (TRE_LINKAGE_T *) TRE_nextSibling(next)))
				next = jtem;

			next = (TRE_LINKAGE_T *) ((TRE_LINKAGE_T *) next)->next;
			/* Break if list end */
			if (next == NULL)
				return (NULL);
			parent =
			    (TRE_LINKAGE_T *) ((TRE_LINKAGE_T *) next)->parent;
		} else {
			parent = (TRE_LINKAGE_T *) ((TRE_LINKAGE_T *)
						    parent)->parent;
		}
	}

	/* Have found next item in subtree */
	return (next);
}
