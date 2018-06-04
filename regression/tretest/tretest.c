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
*   Description:	Tree test
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
void parseTree(TRE_T * tree, const char *treeName);

char hook[] = "+";
char noHook[] = " ";

int main()
{
	TRE_COPY_T copy;	/* Scratch object for tree-copy */
	TRE_T tree1, tree2;
	TRE_TEST_T testObjs[10], *t, *u;
	int32_t errCount = 0;
	POOL_T pool;
	uint8_t poolSpace[POOLITEMS * sizeof(TRE_TEST_T)];

	TRE_initTree(&tree1);
	TRE_initTree(&tree2);
	initPool(&pool, poolSpace, POOLITEMS, sizeof(TRE_TEST_T));

	/* Add root */
	strcpy(&testObjs[0].data[0], "root");
	TRE_addChild(&testObjs[0], &tree1);

	/* Add child to root. */
	strcpy(&testObjs[1].data[0], "rootChild1");
	TRE_addChild(&testObjs[1], &testObjs[0]);

	/* Add new sibling to existing child. */
	strcpy(&testObjs[2].data[0], "rootChild2");
	TRE_addSibling(&testObjs[2], &testObjs[1]);

	/* Check nextSib gives the next sib */
	if (&testObjs[2] != (TRE_nextSibling(&testObjs[1])))
		errCount++;

	/* Check nextSib gives NULL if none */
	if (NULL != (TRE_nextSibling(&testObjs[0])))
		errCount++;

	/* test the pool stubs */
	t = (TRE_TEST_T *) takePool(&pool);
	strcpy(&t->data[0], "poolchild1");

	/* add as grandchild */
	TRE_addChild(t, TRE_firstChild(&testObjs[0]));

	/* Test utility func for finding  the next member of a given subtree,
	   also contiguity of siblings */
	if (t != (TRE_subTreeNext(TRE_firstChild(&testObjs[0]),
				  &testObjs[1], &tree1)))
		errCount++;

	if (&testObjs[1] !=
	    (TRE_subTreeNext(&testObjs[0], &testObjs[0], &tree1)))
		errCount++;

	if (&testObjs[0] != (TRE_subTreeNext(&tree1, &tree1, &tree1)))
		errCount++;

	if (&testObjs[1] != (TRE_firstChild(&testObjs[0])))
		errCount++;

	if (&testObjs[0] != (TRE_firstChild(&tree1)))
		errCount++;

	/* Test siblingHood predicate */
	if (TRE_areSiblings(TRE_firstChild(&testObjs[0]),
			    TRE_firstChild(TRE_firstChild(&testObjs[0]))))
		errCount++;

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
		t = (TRE_TEST_T *) takePool(&pool);
		if (!t)
			break;

		/* duplicate the data contents */
		memcpy(t, u, sizeof(TRE_TEST_T));

	}
	while ((u = (TRE_TEST_T *) TRE_copyNextInSubTree(&copy, t)));

	/* Print it out */
	parseTree(&tree1, "Tree 1");
	parseTree(&tree2, "Tree 2");

	/* Trim the copied tree */
	TRE_removeSubTree(TRE_nextSibling
			  (TRE_firstChild(TRE_firstChild(&tree2))), &tree2);

	/* print it out again */
	parseTree(&tree2, "Trimmed Tree 2");

	if (!TRE_firstChild(TRE_firstChild(&tree1)))
		errCount++;

	if (!TRE_firstChild(TRE_firstChild(&tree2)))
		errCount++;

	DBG_logF("Completed: %" PRId32 " errors \n", errCount);

	exit(errCount);
}

void parseSubtree(TRE_TEST_T * item, int32_t level)
{
	TRE_TEST_T *child;
	int32_t n;

	/* indent */
	if (level > 0) {
		for (n = 0; n < (level - 1); n++)
			DBG_logF("|   ");
		DBG_logF("+---");
	}
	DBG_logF("%s [add=%04" PRIx32 ", prev=%04" PRIx32 ", par=%04" PRIx32
		 ", next=%04" PRIx32 "]\n", item->data,
		 (uint32_t) ((uintptr_t) item) & 0xffff,
		 (uint32_t) ((uintptr_t) TRE_prevSibling(item)) & 0xffff,
		 (uint32_t) ((uintptr_t) TRE_parent(item)) & 0xffff,
		 (uint32_t) ((uintptr_t) TRE_nextSibling(item)) & 0xffff);
	child = (TRE_TEST_T *) TRE_firstChild(item);
	while (child) {
		parseSubtree(child, level + 1);
		child = (TRE_TEST_T *) TRE_nextSibling(child);
	}

}

void parseTree(TRE_T * tree, const char *treeName)
{
	TRE_TEST_T *item;

	DBG_logF("....................%s....................\n", treeName);
	item = (TRE_TEST_T *) TRE_firstChild(tree);
	while (item) {
		parseSubtree(item, 0);
		item = (TRE_TEST_T *) TRE_nextSibling(item);
	}
	DBG_logF("...........................................\n");
}

static void *takePool(POOL_T * pool)
{
	uint8_t *outP;

	if (pool->nOut >= pool->nItems) {
		DBG_logF("POOL TOO SMALL FOR TEST - RESIZE AND TRY AGAIN\n");
		exit(1);
	} else {
		outP = &pool->data[pool->nOut * pool->itemSize];
		pool->nOut++;
	}

	return (outP);
}

static void
initPool(POOL_T * pool, uint8_t * datBase, uint32_t nItems, uint32_t itemSize)
{
	pool->data = datBase;
	pool->nItems = nItems;
	pool->nOut = 0;
	pool->itemSize = itemSize;
	return;
}
