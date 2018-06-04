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
*   Description:	Kernel mailboxes
*
*************************************************************************/

#include "meos/lists/lst.h"
#include "meos/kernel/krn.h"

PARATYPE(MBox, KRN_MAILBOX_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/*
** FUNCTION:      KRN_initMbox
**
** DESCRIPTION:   Initialise a mailbox
**
** RETURNS:       void
*/
void KRN_initMbox(KRN_MAILBOX_T * mbox)
{
	PARACHECK();
	PARADEL(MBox, mbox);

	LST_init(&mbox->items);
	KRN_initSemaphore(&mbox->sem, 0);
#ifdef CONFIG_FEATURE_IMPEXP
	mbox->sem.impexp.objtype = KRN_OBJ_MBX;
#endif
	PARAADD(MBox, mbox);
	PARACHECK();
}

/*
** FUNCTION:      KRN_putMbox
**
** DESCRIPTION:   Put item into mailbox
**
** RETURNS:       void
*/
void KRN_putMbox(KRN_MAILBOX_T * mbox, void *item)
{
	PARACHECK();

	IRQ_IPL_T oldipl;

#ifdef CONFIG_FEATURE_IMPEXP
	if (mbox->sem.impexp.thread < 0) {
		/* protected transaction on local mailbox */
		oldipl = IRQ_raiseIPL();
		LST_add(&mbox->items, item);
		KRN_setSemaphore(&mbox->sem, 1);
		IRQ_restoreIPL(oldipl);
	} else {
		/*
		 * imported mailbox - pointer to item is picked up in KRN_setSemaphore
		 * and passed to exporting thread
		 */
		_KRN_current->p1.testPar = (uintptr_t) item;
		KRN_setSemaphore(&mbox->sem, 1);
	}

#else
	oldipl = IRQ_raiseIPL();
	LST_add(&mbox->items, item);
	KRN_setSemaphore(&mbox->sem, 1);
	IRQ_restoreIPL(oldipl);
#endif
	PARACHECK();
}

/*
** FUNCTION:      KRN_getMbox
**
** DESCRIPTION:   Get item from mailbox
**
** RETURNS:       void * - pointer to item
*/
void *KRN_getMbox(KRN_MAILBOX_T * mbox, int32_t timeout)
{
	PARACHECK();

	KRN_TIMER_T timer;
	IRQ_IPL_T oldipl;
	void *item = NULL;
	oldipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (KRN_testSemaphoreProtected(&mbox->sem, 1, &oldipl)) {
#ifdef CONFIG_FEATURE_IMPEXP
		if (mbox->sem.impexp.thread < 0)
			item = LST_removeHead(&mbox->items);	/* local mailbox - remove item from list */
		else
			item = (void *)(_KRN_current->p1.testResult);	/* imported mailbox -
									   item pointer left in TCB by
									   message processing */
#else
		item = LST_removeHead(&mbox->items);	/* local mailbox - remove item from list */
#endif
	}
	_KRN_restoreIPLWithTimeout(oldipl, &timer, timeout);

	PARACHECK();

	return item;
}
