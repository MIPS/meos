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
*   Description:	Kernel locks
*
*************************************************************************/

#include "meos/dqueues/dq.h"
#include "meos/kernel/krn.h"

PARATYPE(Lock, KRN_LOCK_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/*
** FUNCTION:      KRN_initLock
**
** DESCRIPTION:   Initialise a resource lock - a resource lock is
**                just a semaphore with some useage conventions
**
** RETURNS:       void
*/
void KRN_initLock(KRN_LOCK_T * lock)
{
	PARACHECK();
	PARADEL(Lock, lock);

	KRN_initSemaphore(lock, 1);

	PARADEL(Lock, lock);

#ifdef CONFIG_FEATURE_IMPEXP
	lock->impexp.objtype = KRN_OBJ_LOCK;
#endif

	PARAADD(Lock, lock);
	PARACHECK();
}

/*
** FUNCTION:      KRN_lock
**
** DESCRIPTION:   (Try to) Seize a resource lock
**
** RETURNS:       int32_t - 1:  lock seized
**                      0: lock not seized
*/
int32_t KRN_lock(KRN_LOCK_T * lock, int32_t timeout)
{
	PARACHECK();

	int32_t result = KRN_testSemaphore(lock, 1, timeout);

	PARACHECK();

	return result;
}

/*
** FUNCTION:      KRN_unlock
**
** DESCRIPTION:   Release a resource lock
**
** RETURNS:       void
*/
void KRN_unlock(KRN_LOCK_T * lock)
{
	PARACHECK();

	KRN_setSemaphore(lock, 1);

	PARACHECK();
}
