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
*   Description:	QIO cancel
*
*************************************************************************/

#include "MEOS.h"

void _QIO_cancelIO(QIO_IOCB_T * iocb, QIO_STATE_T cancelState);

/*
** FUNCTION:      _QIO_cancelIO
**
** DESCRIPTION:   Cancel a single operation - used both by QIO_cancel and timeout processing
**
** RETURNS:       void
*/
void _QIO_cancelIO(QIO_IOCB_T * iocb, QIO_STATE_T cancelState)
{
	IRQ_IPL_T oldipl;
	QIO_CANCELFUNC_T *cFunc;
	QIO_DEVICE_T *dev;

	PARACHECK();

	oldipl = IRQ_raiseIPL();
	dev = iocb->device;
	if (iocb->ioState == QIO_IOPENDING) {
		/* can always cancel a pending operation */
		DQ_remove(iocb);
		if (iocb->timerSet)
			KRN_cancelTimer(&iocb->timer);
		iocb->ioState = cancelState;
		if ((iocb->compfunc == NULL)
		    || !iocb->compfunc(dev, iocb, &iocb->ioParameters,
				       (QIO_STATUS_T) cancelState)) {
			DBG_assert(iocb->mailbox != NULL, "Expected non-NULL mailbox");	/* check for bad completion function/mailbox combination */
			KRN_putMbox(iocb->mailbox, iocb);
		}
	} else if (iocb->ioState == QIO_IOACTIVE) {
		cFunc = dev->driver->cancelFunc;
		/* can only cancel an active operation on a single rank device with a cancel function */
		if ((dev->rank == 1) && (cFunc != NULL)) {
			cFunc(dev);
			QIO_complete(dev, cancelState);	/* QIO_complete looks after any pending timers */
			QIO_start(dev);
		}
	}
	IRQ_restoreIPL(oldipl);

	PARACHECK();
}

/*
** FUNCTION:      QIO_cancel
**
** DESCRIPTION:   Cancel a single operation
**
** RETURNS:       void
*/
void QIO_cancel(QIO_IOCB_T * iocb)
{
	PARACHECK();

	DBG_assert(IRQ_bg(),
		   "Can not cancel QIO operation from interrupt context");

	_QIO_cancelIO(iocb, QIO_IOCANCELLED);

	PARACHECK();
}
