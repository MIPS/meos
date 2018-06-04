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
*   Description:	Tree module coverage test
*
*************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEOS.h"

#define MAX_DISP_TREE_DEPTH  10
#define MAX_DISP_TREE_WIDTH 80
#define POOLITEMS           100
#define TRE_TEST_DATSIZE     32

typedef struct {
	TRE_LINK;
	char data[TRE_TEST_DATSIZE];
} TRE_TEST_T;

typedef struct {
	uint8_t *data;
	int32_t nItems;
	int32_t nOut;
	int32_t itemSize;
} POOL_T;

/* local prototypes */
static void initPool(POOL_T * pool,
		     uint8_t * datBase, uint32_t nItems, uint32_t itemSize);
static void *takePool(POOL_T * pool);

/* Array for writing tree into as WIDTH separate strings */
#define STR_LEN 25
char toSpace[MAX_DISP_TREE_WIDTH * MAX_DISP_TREE_DEPTH * STR_LEN];

void tre()
{
	TRE_COPY_T copy;	/* Scratch object for tree-copy */
	TRE_T tree1, tree2;
	TRE_TEST_T testObjs[10], *t, *u;
	POOL_T pool;
	uint8_t poolSpace[POOLITEMS * sizeof(TRE_TEST_T)];

	TRE_initTree(&tree1);
	TRE_initTree(&tree2);
	initPool(&pool, poolSpace, POOLITEMS, sizeof(TRE_TEST_T));

	/* Add root */
	strcpy(&testObjs[0].data[0], "root");
	TRE_addChild(&testObjs[0], &tree1);
	TRE_removeLeaf(&testObjs[0], &tree1);
	TRE_addChild(&testObjs[0], &tree1);

	/* Add child to root. */
	strcpy(&testObjs[1].data[0], "rootChild1");
	TRE_addChild(&testObjs[1], &testObjs[0]);
	TRE_removeLeaf(&testObjs[1], &tree1);
	TRE_addChild(&testObjs[1], &testObjs[0]);

	/* Add new sibling to existing child. */
	strcpy(&testObjs[2].data[0], "rootChild2");
	TRE_addSibling(&testObjs[2], &testObjs[1]);
	TRE_removeLeaf(&testObjs[2], &tree1);
	TRE_addSibling(&testObjs[2], &testObjs[1]);

	/* Check nextSib gives the next sib */
	DBG_assert(&testObjs[2] == (TRE_nextSibling(&testObjs[1])),
		   "Failed to provide next sibling");

	/* Check nextSib gives NULL if none */
	DBG_assert(NULL == (TRE_nextSibling(&testObjs[0])),
		   "Provided unexpected next sibling");

	/* test the pool stubs */
	t = (TRE_TEST_T *) takePool(&pool);
	strcpy(&t->data[0], "poolchild1");

	/* add as grandchild */
	TRE_addChild(t, TRE_firstChild(&testObjs[0]));

	/* Test utility func for finding  the next member of a given subtree,
	   also contiguity of siblings */
	DBG_assert(t == (TRE_subTreeNext(TRE_firstChild(&testObjs[0]),
					 &testObjs[1], &tree1)),
		   "Failed to find in subtree");

	DBG_assert(&testObjs[1] ==
		   (TRE_subTreeNext(&testObjs[0], &testObjs[0], &tree1)),
		   "Failed to find next in subtree");
	DBG_assert(&testObjs[0] == (TRE_subTreeNext(&tree1, &tree1, &tree1)),
		   "Failed to find next in subtree (3)");
	DBG_assert(&testObjs[1] == (TRE_firstChild(&testObjs[0])),
		   "Failed to get first in subtree");
	DBG_assert(&testObjs[0] == (TRE_firstChild(&tree1)),
		   "Failed to get first in subtree (2)");
	/* Test siblingHood predicate */
	DBG_assert(!TRE_areSiblings(TRE_firstChild(&testObjs[0]),
				    TRE_firstChild(TRE_firstChild
						   (&testObjs[0]))),
		   "Failed to obtain siblings");

	/* Duplicate the tree */
	u = (TRE_TEST_T *) TRE_initCopySubTree(TRE_root(&tree1), &tree1, &tree2,
					       &tree2, &copy);
	do {
		t = (TRE_TEST_T *) takePool(&pool);
		if (!t)
			break;

		/* duplicate the data contents */
		memcpy(t, u, sizeof(TRE_TEST_T));

	}
	while ((u = (TRE_TEST_T *) TRE_copyNextInSubTree(&copy, t)));

	/* Duplicate the tree again */
	u = (TRE_TEST_T *) TRE_initCopySubTree(TRE_root(&tree1), &tree1,
					       TRE_firstChild(TRE_firstChild
							      (&tree2)), &tree2,
					       &copy);
	do {
		t = (TRE_TEST_T *) (TRE_TEST_T *) takePool(&pool);
		if (!t)
			break;

		/* duplicate the data contents */
		memcpy(t, u, sizeof(TRE_TEST_T));

	}
	while ((u = (TRE_TEST_T *) TRE_copyNextInSubTree(&copy, t)));

	/* Trim the copied tree */
	TRE_removeSubTree(TRE_nextSibling
			  (TRE_firstChild(TRE_firstChild(&tree2))), &tree2);

	DBG_assert(TRE_firstChild(TRE_firstChild(&tree1)), "First child wrong");

	DBG_assert(TRE_firstChild(TRE_firstChild(&tree2)),
		   "First child wrong (2)");

	memset(testObjs, 0, sizeof(testObjs));

	TRE_initTree(&tree1);
	TRE_addChild(&testObjs[0], &tree1);
	TRE_addChild(&testObjs[1], &tree1);
	TRE_addChild(&testObjs[2], &testObjs[0]);
	TRE_addChild(&testObjs[3], &testObjs[0]);
	TRE_addChild(&testObjs[4], &testObjs[2]);
	TRE_addChild(&testObjs[5], &testObjs[1]);
	TRE_addChild(&testObjs[6], &testObjs[1]);

	DBG_assert(NULL ==
		   (TRE_subTreeNext(&testObjs[0], &testObjs[4], &tree1)),
		   "Failed to find next in subtree (4)");

	TRE_addChild(&testObjs[7], &testObjs[3]);
	TRE_addChild(&testObjs[8], &testObjs[3]);
	TRE_addChild(&testObjs[9], &testObjs[3]);

	DBG_assert(&testObjs[7] ==
		   (TRE_subTreeNext(&testObjs[0], &testObjs[4], &tree1)),
		   "Failed to find next in subtree (5)");

	DBG_assert(TRE_finalSibling(&testObjs[2]) == &testObjs[3],
		   "Final sibling wrong");
	DBG_assert(TRE_finalSibling(&testObjs[5]) == &testObjs[6],
		   "Final sibling wrong (2)");

	DBG_assert(NULL ==
		   TRE_prevSibling(&testObjs[5]), "Previous sibling wrong");
	DBG_assert(&testObjs[8] ==
		   TRE_prevSibling(&testObjs[9]), "Previous sibling wrong (2)");

	DBG_assert(NULL ==
		   TRE_initCopySubTree(&testObjs[0], &tree1, &testObjs[3],
				       &tree1, &copy),
		   "Copy shouldn't succeed");

	DBG_assert(&testObjs[9] == TRE_removeSubTree(&testObjs[0], &tree1),
		   "Sub tree removal wrong");

	DBG_assert(&testObjs[6] ==
		   TRE_removeLeafFromSubTree(&testObjs[6], &tree1),
		   "Removed wrong leaf");
	DBG_assert(&testObjs[5] ==
		   TRE_removeLeafFromSubTree(&testObjs[1], &tree1),
		   "Removed wrong leaf (2)");
}

static void *takePool(POOL_T * pool)
{
	uint8_t *outP;

	DBG_assert(pool->nOut < pool->nItems,
		   "POOL TOO SMALL FOR TEST - RESIZE AND TRY AGAIN");
	outP = &pool->data[pool->nOut * pool->itemSize];
	pool->nOut++;

	return (outP);
}

static void initPool(POOL_T * pool, uint8_t * datBase, uint32_t nItems,
		     uint32_t itemSize)
{
	pool->data = datBase;
	pool->nItems = nItems;
	pool->nOut = 0;
	pool->itemSize = itemSize;
	return;
}
