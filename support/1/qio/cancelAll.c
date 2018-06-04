/***(C)2003***************************************************************
*
* Copyright (C) 2003 MIPS Tech, LLC
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
****(C)2003**************************************************************/

/*************************************************************************
*
*   Description:	QIO cancel all
*
*************************************************************************/

#include "MEOS.h"

void _QIO_cancelIO(QIO_IOCB_T * iocb, QIO_STATE_T cancelState);

/*
** FUNCTION:      QIO_cancelAll
**
** DESCRIPTION:   Cancel all operations on a device
**
** RETURNS:       void
*/
void QIO_cancelAll(QIO_DEVICE_T * dev)
{
	IRQ_IPL_T oldipl;
	QIO_IOCB_T *iocb;
	DQ_T tempq;
	QIO_CANCELFUNC_T *cFunc;

	PARACHECK();

	DBG_assert(IRQ_bg(),
		   "Can not cancel all QIO operations from interrupt context");

	DQ_init(&tempq);
	cFunc = dev->driver->cancelFunc;

	/* two stage IPL management anticipates a future stack based optimisation */
	oldipl = IRQ_raiseIPL();	/* stop other tasks queueing new operations until we're done */
	DQ_move(&dev->pendingQueue, &tempq);	/* move all pending operations to a temporary queue */

	/* no pending operations, so now deal with the active operations */
	if ((cFunc != NULL) && (dev->active > 0)) {
		cFunc(dev);
		while (dev->active > 0)
			QIO_complete(dev, QIO_IOCANCELLED);
	}
	IRQ_restoreIPL(oldipl);

	KRN_release();		/* avoids latency problems with restoreIPL, which could
				   result in lengthy interrupt lockout */

	oldipl = IRQ_raiseIPL();
	while ((iocb = (QIO_IOCB_T *) DQ_first(&tempq)) != NULL) {
		_QIO_cancelIO(iocb, QIO_IOCANCELLED);
		IRQ_restoreIPL(oldipl);
		KRN_release();	/* ensure other active tasks are scheduled */
		oldipl = IRQ_raiseIPL();
	}
	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:      _QIO_cancelAllProtected
**
** DESCRIPTION:   A version of QIO_cancelAll for use with interrupts disabled
**                This is for use only by QIO_disable in circumstances where we
**				  expect there to be few or no pernding operations
**
** RETURNS:       void
*/
void _QIO_cancelAllProtected(QIO_DEVICE_T * dev)
{
	QIO_IOCB_T *iocb;
	DQ_T tempq;
	QIO_CANCELFUNC_T *cFunc;

	PARACHECK();

	DQ_init(&tempq);
	cFunc = dev->driver->cancelFunc;

	DQ_move(&dev->pendingQueue, &tempq);	/* move all pending operations to a temporary queue */

	/* no pending operations, so now deal with the active operations */
	if ((cFunc != NULL) && (dev->active > 0)) {
		cFunc(dev);
		while (dev->active > 0)
			QIO_complete(dev, QIO_IOCANCELLED);
	}
	while ((iocb = (QIO_IOCB_T *) DQ_first(&tempq)) != NULL)
		_QIO_cancelIO(iocb, QIO_IOCANCELLED);

	PARACHECK();
}
