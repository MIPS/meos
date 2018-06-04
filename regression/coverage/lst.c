/***(C)2014***************************************************************
*
* Copyright (C) 2014 MIPS Tech, LLC
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
****(C)2014**************************************************************/

/*************************************************************************
*
*   Description:	List module coverage test
*
*************************************************************************/

#undef NDEBUG

#include "MEOS.h"

#define NUM_ITEMS 10

typedef struct {
	LST_LINK;
	int32_t seqnum;
} ITEM_T;

static LST_T l;
static ITEM_T items[NUM_ITEMS];

#ifdef LST_INLINE
void lst_inline(void)
#else
void lst(void)
#endif
{
	int32_t n;
	ITEM_T *i;

	LST_init(&l);

	DBG_assert(LST_empty(&l), "List not empty");

	for (n = 0; n < NUM_ITEMS; n++) {
		/* check addressing assumptions */
		DBG_assert((void *)&items[n] == (void *)&items[n].LST_link,
			   "Addressing assumption wrong");
		items[n].seqnum = n;
		LST_add(&l, &items[n]);
		DBG_assert(LST_last(&l) == &items[n], "Last wrong");
		DBG_assert(LST_first(&l) == &items[0], "First wrong");
		DBG_assert(!LST_empty(&l), "List empty");
	}

/*     DBG_logF("Items base apparently at %08x\n", int32_t items); */
/*     for (n= 0; n< (NUM_ITEMS-1); n++) */
/*     { */
/*         DBG_logF("Item %" PRId32 " at %08x  points to %08x. Next item at %08x\n", */
/*                n, (int32_t)&items[n], (int32_t)LST_next(&items[n]), (int32_t)&items[n+1]); */
/*     } */
	for (n = 0; n < (NUM_ITEMS - 1); n++)
		DBG_assert(LST_next(&items[n]) == &items[n + 1], "Next wrong");

	n = 0;
	while ((i = (ITEM_T *) LST_removeHead(&l)) != NULL) {
		DBG_assert(i == &items[n], "Sequence wrong");
		n++;
		if (!LST_empty(&l)) {
			DBG_assert(LST_first(&l) == &items[n], "First wrong");
			DBG_assert(LST_last(&l) == &items[NUM_ITEMS - 1],
				   "Last wrong");
		} else {
			DBG_assert(LST_first(&l) == NULL, "First wrong");
			DBG_assert(LST_last(&l) == NULL, "Last wrong");
		}
	}
	DBG_assert(n == NUM_ITEMS, "Insufficient items");
	DBG_assert(LST_empty(&l), "List not empty");

	/* rebuild the list */
	for (n = 0; n < NUM_ITEMS; n++) {
		LST_addHead(&l, &items[NUM_ITEMS - (n + 1)]);
	}
	/* Remove from the end */
	for (n = 0; n < NUM_ITEMS; n++) {
		LST_remove(&l, LST_last(&l));
	}
	DBG_assert(LST_empty(&l), "List not empty");

	/* rebuild the list */
	for (n = 0; n < NUM_ITEMS; n++) {
		LST_addHead(&l, &items[NUM_ITEMS - (n + 1)]);
	}
	/* remove odd items... */
	for (n = 1; n < NUM_ITEMS; n += 2) {

		LST_remove(&l, &items[n]);
/*         DBG_logF("Counter=%" PRId32 "\n",n); */
	}
	/* ...and confirm even items left behind */
	for (n = 0; n < NUM_ITEMS; n += 2) {
		i = (ITEM_T *) LST_removeHead(&l);
		DBG_assert(i != NULL, "i NULL");
		DBG_logF
		    ("Item address = %p, Sequence number = %" PRId32
		     ", expected %" PRId32 "\n", i, i->seqnum, n);
		DBG_assert(i->seqnum == n, "Sequence wrong");
	}
	DBG_assert(LST_empty(&l), "List not empty");

	/* repeat test removing even items first */
	for (n = 0; n < NUM_ITEMS; n++)
		LST_add(&l, &items[n]);
	for (n = 0; n < NUM_ITEMS; n += 2) {
		LST_remove(&l, &items[n]);
	}
	for (n = 1; n < NUM_ITEMS; n += 2) {
		i = (ITEM_T *) LST_removeHead(&l);
		DBG_assert(i != NULL, "i NULL");
		DBG_assert(i->seqnum == n,
			   "Sequence wrong (%" PRIu32 " != %" PRIu32 ")",
			   i->seqnum, n);
	}
	DBG_assert(LST_empty(&l), "List not empty");
}
