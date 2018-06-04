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
*   Description:	Kernel pools
*
*************************************************************************/

#include "meos/config.h"
#include "meos/lists/lst.h"
#include "meos/kernel/krn.h"

#define NOWAIT (0)

#ifdef CONFIG_DEBUG_PARANOIA
PARATYPE(Pool, KRN_POOL_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(PlLk, KRN_POOLLINK_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
extern void DBG_paranoidSweep(uintptr_t start, uintptr_t end);
#endif

/*
** FUNCTION:      KRN_initPool
**
** DESCRIPTION:   Initialise a pool
**
** RETURNS:       void
*/
void KRN_initPool(KRN_POOL_T * pool, void *items, int32_t numItems,
		  int32_t itemSize)
{
	PARACHECK();
	PARADEL(Pool, pool);
#ifdef CONFIG_DEBUG_PARANOIA
	DBG_paranoidSweep((uintptr_t) items,
			  (uintptr_t) (items) + (numItems * itemSize));
#endif

	uint8_t *i = (uint8_t *) items;

	DBG_assert(sizeof(uint8_t) == 1, "Unsigned int8_t not 1 byte");
	KRN_initSemaphore(&pool->sem, numItems);
#ifdef CONFIG_FEATURE_IMPEXP
	pool->sem.impexp.objtype = KRN_OBJ_POOL;
#endif
	LST_init(&pool->freeList);
	while (numItems--) {
		((KRN_POOLLINK_T *) i)->owner = pool;
		PARAADD(PlLk, i);
#ifdef CONFIG_FEATURE_IMPEXP
		((KRN_POOLLINK_T *) i)->expinfo = pool->sem.impexp;	/* each item in an exported pool
									   carries a copy of the pool's export
									   info so it can be returned from any
									   thread */
#endif
		LST_add(&pool->freeList, i);
		i += itemSize;
	}

	PARAADD(Pool, pool);
	PARACHECK();
}

/*
** FUNCTION:      KRN_emptyPool
**
** DESCRIPTION:   Test for empty pool
**
** RETURNS:       int32_t 1:  pool empty
**                    0: pool not empty
*/
int32_t KRN_emptyPool(KRN_POOL_T * pool)
{
	PARACHECK();

	/* pool test is done with a take/return pair - only slightly
	   less efficient than just inspecting the semaphore and works with
	   imported pools as well. BEWARE the warning in the manual. The
	   result of this test is likely to be out-of-date before you can
	   exploit it.
	 */
	void *item;

	item = KRN_takePool(pool, NOWAIT);
	if (item != NULL)
		KRN_returnPool(item);	/* restore semaphore */

	PARACHECK();
	return item == NULL ? 1 : 0;
}

/*
** FUNCTION:      KRN_takePool
**
** DESCRIPTION:   Take or allocate item from a pool
**
** RETURNS:       void * Pointer to allocated item
*/
void *KRN_takePool(KRN_POOL_T * pool, int32_t timeout)
{
	PARACHECK();

	void *item = NULL;
	IRQ_IPL_T oldipl;
	KRN_TIMER_T timer;

	oldipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (KRN_testSemaphoreProtected(&pool->sem, 1, &oldipl)) {
#ifdef CONFIG_FEATURE_IMPEXP
		if (pool->sem.impexp.thread < 0)
			item = LST_removeHead(&pool->freeList);
		else {
			item = (void *)(_KRN_current->p1.testResult);	/* imported pool -
									   item pointer left in TCB by
									   message processing */
		}
#else
		item = LST_removeHead(&pool->freeList);
#endif
	}
	_KRN_restoreIPLWithTimeout(oldipl, &timer, timeout);

	PARACHECK();
	return item;
}

/*
** FUNCTION:      KRN_returnPool
**
** DESCRIPTION:   return an item to its pool
**
** RETURNS:       void
*/
void KRN_returnPool(void *item)
{
	PARACHECK();

	IRQ_IPL_T oldipl;
	KRN_POOL_T *pool = NULL;

	/* since a pool item might have arrived by a tortuous route from another
	   thread, via mailbox messages or whatever, there could be invalid cached
	   data for the pool linkage fields on which we are about to rely */
	KRN_flushCache(item, sizeof(KRN_POOLLINK_T), 0);
#ifdef CONFIG_FEATURE_IMPEXP
	if ((((KRN_POOLLINK_T *) item)->expinfo.thread < 0) ||
	    (((KRN_POOLLINK_T *) item)->expinfo.thread ==
	     _KRN_schedule->hwThread))
		pool = ((KRN_POOLLINK_T *) item)->owner;	/* locally defined pool */
	else {
		/* It's from an imported pool so look up the imported "shadow" */
		KRN_IMPORT_T *imp;
		if (_KRN_schedule->importTables
		    [((KRN_POOLLINK_T *) item)->expinfo.thread]) {
			imp = _KRN_schedule->importTables[((KRN_POOLLINK_T *)
							   item)->expinfo.
							  thread] +
			    (((KRN_POOLLINK_T *) item)->expinfo.sId - 1);
			pool = imp->objPtr;
		}
	}
#else
	pool = ((KRN_POOLLINK_T *) item)->owner;	/* locally defined pool */
#endif
	if (pool != NULL) {
#ifdef CONFIG_FEATURE_IMPEXP
		if (pool->sem.impexp.thread < 0) {
			oldipl = IRQ_raiseIPL();
			LST_add(&(pool->freeList), (KRN_POOLLINK_T *) item);
			KRN_setSemaphore(&pool->sem, 1);
			IRQ_restoreIPL(oldipl);
		} else {
			/*
			 * imported pool - pointer to item is picked up in KRN_setSemaphore
			 * and passed to exporting thread
			 */
			_KRN_current->p1.testPar = (uintptr_t) item;
			KRN_setSemaphore(&pool->sem, 1);
		}
#else
		oldipl = IRQ_raiseIPL();
		LST_add(&(pool->freeList), (KRN_POOLLINK_T *) item);
		KRN_setSemaphore(&pool->sem, 1);
		IRQ_restoreIPL(oldipl);
#endif
	} else
		DBG_assert(0, "Pool not imported");	/* user has forgotten to import the pool */

	PARACHECK();
}
