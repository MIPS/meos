/***(C)2004***************************************************************
*
* Copyright (C) 2004 MIPS Tech, LLC
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
****(C)2004**************************************************************/

/*************************************************************************
*
*   Description:	DQ test
*
*************************************************************************/

/* included header files */

#undef NDEBUG

#include "MEOS.h"

#include <stdlib.h>

#define NUM_ITEMS 10

#ifdef DQ_INLINE
#define ILS "(Inlined DQ_ functions)"
#else
#define ILS "(Normally compiled DQ_ functions)"
#endif

typedef struct {
	DQ_LINK;
	int32_t seqnum;
} ITEM_T;

DQ_T q;
ITEM_T items[NUM_ITEMS];

int main(int argc, char **argv)
{
	int32_t n;
	ITEM_T *i;

	(void)argc;
	(void)argv;

	DBG_logF("Testing module DQ_ %s\n", ILS);

	/* check addressing assumptions */

	DBG_assert((void *)&q.DQ_link == (void *)&q,
		   "Addressing assumption wrong");

	/* initialise queue */
	DQ_init(&q);
	DBG_assert(DQ_empty(&q), "DQ not empty");

	/* test addTail/removeHead */
	for (n = 0; n < NUM_ITEMS; n++) {
		items[n].seqnum = n;
		DQ_addTail(&q, &items[n]);
		DBG_assert(!DQ_empty(&q), "DQ empty");
	}

	n = 0;

	while ((i = (ITEM_T *) DQ_removeHead(&q)) != NULL) {
		DBG_assert(i->seqnum == n, "Sequence wrong");
		n++;
	}
	DBG_assert(n == NUM_ITEMS, "Insufficient items");
	DBG_assert(DQ_empty(&q), "DQ not empty");

	/* test addHead/removeTail */
	for (n = 0; n < NUM_ITEMS; n++) {
		items[n].seqnum = n;
		DQ_addHead(&q, &items[n]);
		DBG_assert(!DQ_empty(&q), "DQ empty");
	}

	n = 0;

	while ((i = (ITEM_T *) DQ_removeTail(&q)) != NULL) {
		DBG_assert(i->seqnum == n, "Sequence wrong");
		n++;
	}
	DBG_assert(n == NUM_ITEMS, "Insufficient items");
	DBG_assert(DQ_empty(&q), "DQ not empty");

	/* test addHead/removeHead */
	for (n = 0; n < NUM_ITEMS; n++) {
		items[n].seqnum = n;
		DQ_addHead(&q, &items[n]);
		DBG_assert(!DQ_empty(&q), "DQ empty");
	}

	n = NUM_ITEMS;

	while ((i = (ITEM_T *) DQ_removeHead(&q)) != NULL) {
		n--;
		DBG_assert(i->seqnum == n, "Sequence wrong");
	}
	DBG_assert(n == 0, "Too many items");
	DBG_assert(DQ_empty(&q), "DQ not empty");

	/* test addTail/removeTail */
	for (n = 0; n < NUM_ITEMS; n++) {
		items[n].seqnum = n;
		DQ_addTail(&q, &items[n]);
		DBG_assert(!DQ_empty(&q), "DQ empty");
	}

	n = NUM_ITEMS;

	while ((i = (ITEM_T *) DQ_removeTail(&q)) != NULL) {
		n--;
		DBG_assert(i->seqnum == n, "Sequence wrong");
	}
	DBG_assert(n == 0, "Too many items");
	DBG_assert(DQ_empty(&q), "DQ not empty");

	/* test addAfter/addBefore */
	DQ_addHead(&q, &items[1]);
	DQ_addBefore(&items[1], &items[0]);
	DQ_addAfter(&items[1], &items[2]);

	n = 0;
	while ((i = (ITEM_T *) DQ_removeHead(&q)) != NULL) {
		DBG_assert(i->seqnum == n, "Sequence wrong");
		n++;
	}
	DBG_assert(n == 3, "Insufficient items");

	/* test DQ_first DQ_last DQ_next and DQ_previous and DQ_remove */
	DQ_addHead(&q, &items[1]);
	DBG_assert(DQ_first(&q) == &items[1], "First wrong");
	DBG_assert(DQ_last(&q) == &items[1], "Last wrong");

	DQ_addBefore(&items[1], &items[0]);
	DBG_assert(DQ_first(&q) == &items[0], "First wrong");
	DBG_assert(DQ_last(&q) == &items[1], "Last wrong");

	DQ_addAfter(&items[1], &items[2]);
	DBG_assert(DQ_last(&q) == &items[2], "Last wrong");

	DBG_assert(DQ_next(&items[0]) == &items[1], "Next wrong");
	DBG_assert(DQ_next(&items[1]) == &items[2], "Next wrong");
	DBG_assert(DQ_next(&items[2]) == &q, "Next wrong");

	DBG_assert(DQ_previous(&items[2]) == &items[1], "Previous wrong");
	DBG_assert(DQ_previous(&items[1]) == &items[0], "Previous wrong");
	DBG_assert(DQ_previous(&items[0]) == &q, "Previous wrong");

	DQ_remove(&items[1]);
	DBG_assert(DQ_first(&q) == &items[0], "First wrong");
	DQ_remove(&items[0]);
	DBG_assert(DQ_first(&q) == &items[2], "First wrong");
	DQ_remove(&items[2]);
	DBG_assert(DQ_empty(&q), "Not empty");

	DBG_logF("TEST PASSED\n");

	return 0;
}
