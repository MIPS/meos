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
*   Description:	QIO framework and ISR
*
*************************************************************************/

#include "MEOS.h"

PARATYPE(QIOI, QIO_IOCB_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);
PARATYPE(QIOD, QIO_DEVICE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

#define WRITE(A,V) (*((volatile uint32_t *)(A))=((uint32_t)V))
#define READ(A) (*(volatile uint32_t *)(A))

/* local prototypes */

static void defaultISRFunction(QIO_DEVICE_T * dev);
void _QIO_powerUp(QIO_DEVICE_T * dev);
void _QIO_powerDown(QIO_DEVICE_T * dev);
void _QIO_cancelIO(QIO_IOCB_T * iocb, QIO_STATE_T cancelState);
static void _QIO_timeout(KRN_TIMER_T * timer, void *arg);
static void _QIO_extioISR(int32_t sigNum);

/* module variables */
QIO_SYS_T *_QIO;

/*
** FUNCTION:	QIO_init
**
** DESCRIPTION:	Initialise a device
**
** RETURNS:	void
*/
void QIO_init(QIO_DEVICE_T * device, const char *devName, uint32_t id,
	      QIO_DRIVER_T const *driver)
{
	IRQ_IPL_T oldipl;
	int32_t n;

	PARACHECK();

	DBG_assert(IRQ_bg(),
		   "Can not initialise QIO device from interrupt context");

	PARADEL(QIOD, device);

	device->devName = devName;
	device->id = id;
	DQ_init(&device->pendingQueue);
	DQ_init(&device->activeQueue);
	device->driver = driver;
	device->active = 0;
	device->enabled = 0;
	device->powerState = QIO_POWERUNKNOWN;

	PARAADD(QIOD, device);

	oldipl = IRQ_raiseIPL();
	driver->initFunc(device, &device->powerClass, &device->rank);

	for (n = 0; n < device->numIrqs; n++) {
		device->irqDescs[n].priv = device;
		device->irqDescs[n].isrFunc = _QIO_extioISR;
		IRQ_route(&device->irqDescs[n]);
	}
	IRQ_restoreIPL(oldipl);

	/* if the device is simulated by a software task, then start the
	   simulation task */
	if (driver->simFunc != NULL)
		driver->simFunc(device, 1);

	PARACHECK();
}

/*
** FUNCTION:	QIO_qio
**
** DESCRIPTION:	Queue an operation
**
** RETURNS:	void
*/
void QIO_qio(QIO_DEVICE_T * dev, QIO_IOCB_T * iocb, QIO_IOPARS_T * iopars,
	     KRN_MAILBOX_T * outMbox, QIO_COMPFUNC_T * compFunc,
	     int32_t timeout)
{
	IRQ_IPL_T oldipl;

	PARACHECK();

	DBG_assert(IRQ_bg(),
		   "Can not queue a QIO operation from interrupt context");

	PARADEL(QIOI, iocb);

	/* initial state for IO operation */
	iocb->ioState = QIO_IOCANCELLED;

	/* copy driver parameters */
	iocb->device = dev;
	iocb->ioParameters = *iopars;	/* structure copy ! */

	PARAADD(QIOI, iocb);

	oldipl = IRQ_raiseIPL();

	/* set timer for timeout processing */
	iocb->timerSet = 0;
	if (timeout > 0) {
		KRN_setTimer(&iocb->timer, _QIO_timeout, iocb, timeout);
		iocb->timerSet = 1;

	}

	/* queue the operation */
	if (dev->enabled) {
		iocb->mailbox = outMbox;
		iocb->ioState = QIO_IOPENDING;
		iocb->compfunc = compFunc;
		DQ_addTail(&dev->pendingQueue, iocb);	/* add to queue */
		QIO_start(dev);	/* start device if necessary */
	} else {
		/* synchronous completion (cancelled) */
		if ((compFunc == NULL)
		    || !compFunc(dev, iocb, &iocb->ioParameters,
				 (QIO_STATUS_T) QIO_IOCANCELLED)) {
			DBG_assert(outMbox != NULL, "Expected non-NULL mailbox");	/* check for bad mailbox/completion function combination */
			KRN_putMbox(outMbox, iocb);
		}
	}

	/* cancel the timeout if it wasn't needed */
	if ((timeout > 0) && (iocb->ioState == QIO_IOCANCELLED))
		KRN_cancelTimer(&iocb->timer);

	IRQ_restoreIPL(oldipl);

	PARACHECK();

	return;
}

/*
** FUNCTION:	QIO_result
**
** DESCRIPTION:	Get QIO result from mailbox
**
** RETURNS:	QIO_IOCB_T Pointer to IOCB of completed operation
**						or NULL if fetch timed out.
*/
QIO_IOCB_T *QIO_result(KRN_MAILBOX_T * mbox, QIO_DEVICE_T ** dev,
		       QIO_STATUS_T * status, QIO_IOPARS_T * iopars,
		       int32_t timeout)
{
	QIO_IOCB_T *result;

	PARACHECK();
	result = (QIO_IOCB_T *) KRN_getMbox(mbox, timeout);
	if (result != NULL) {
		*dev = result->device;
		*status = (QIO_STATUS_T) result->ioState;
		*iopars = result->ioParameters;	/* structure copy ! */
	}

	PARACHECK();
	return result;
}

/*
** FUNCTION:	_QIO_timeout
**
** DESCRIPTION:	QIO timeout processing function
**
** RETURNS:	void
*/
static void _QIO_timeout(KRN_TIMER_T * timer, void *arg)
{
	(void)timer;
	/* cancelIO includes protection against cancelling operations that
	   can't be cancelled or are already complete */
	_QIO_cancelIO((QIO_IOCB_T *) arg, (QIO_STATE_T) QIO_IOTIMEOUT);
}

/*
** FUNCTION:	defaultISRFunction
**
** DESCRIPTION:	Default device ISR
**
** RETURNS:	void
*/
static void defaultISRFunction(QIO_DEVICE_T * dev)
{
	QIO_complete(dev, QIO_IOCOMPLETE);
	QIO_start(dev);
}

/*
** FUNCTION:	_QIO_powerUp
**
** DESCRIPTION:	Power up device prior to IO - for use only at device IPL
**
** RETURNS:	void
*/
void _QIO_powerUp(QIO_DEVICE_T * dev)
{
	PARACHECK();

	if (dev->powerClass == QIO_POWERNONE)
		return;		/* no power control features */
	if (dev->powerState != QIO_POWERNORMAL) {
		dev->driver->powerFunc(dev, QIO_POWERNORMAL);
		dev->powerState = QIO_POWERNORMAL;
	}

	PARACHECK();
	return;
}

/*
** FUNCTION:	_QIO_powerDown
**
** DESCRIPTION:	Power down device after IO - for use only at device IPL
**
** RETURNS:	void
*/
void _QIO_powerDown(QIO_DEVICE_T * dev)
{
	PARACHECK();

	if (dev->powerClass == QIO_POWERNONE)
		return;		/* no power control features */
	if (dev->powerState != QIO_POWERSAVE) {
		if ((dev->powerClass == QIO_POWERFINE) || !dev->enabled) {
			dev->driver->powerFunc(dev, QIO_POWERSAVE);
			dev->powerState = QIO_POWERSAVE;
		}
	}

	PARACHECK();
	return;
}

/*
** FUNCTION:	QIO_start
**
** DESCRIPTION:	Called at device IPL for interrupting devices
**				whenever there is a possibility that more operations could be
**				started on a particular device.
**
** RETURNS:	void
*/
void QIO_start(QIO_DEVICE_T * dev)
{
	QIO_IOCB_T *iocb;

	PARACHECK();

	/* assume we are called at device IPL */
	while ((dev->active < dev->rank) &&
	       ((iocb =
		 (QIO_IOCB_T *) DQ_removeHead(&dev->pendingQueue)) != NULL)) {
		iocb->ioState = QIO_IOACTIVE;
		DQ_addTail(&dev->activeQueue, iocb);
		dev->active++;
		_QIO_powerUp(dev);
		dev->driver->startFunc(dev, &iocb->ioParameters);
	}

	PARACHECK();
	return;
}

/*
** FUNCTION:	QIO_complete
**
** DESCRIPTION:	Called at device IPL for interrupting devices
**				to tidy up after an operation has completed.
**
** RETURNS:	void
*/
void QIO_complete(QIO_DEVICE_T * dev, QIO_STATE_T iostate)
{
	QIO_IOCB_T *iocb;

	PARACHECK();

	iocb = (QIO_IOCB_T *) DQ_removeHead(&dev->activeQueue);
	DBG_assert(iocb != NULL, "NULL iocb");	/* check we haven't lost count of interrupts */
	if (--(dev->active) <= 0)	/* decrement active request count... */
		_QIO_powerDown(dev);	/* ...and power down device if necessary */
	/* Deal with possible user-supplied completion function and, if necessary,
	   deliver result to output mailbox */
	if ((iocb->compfunc == NULL)
	    || !iocb->compfunc(dev, iocb, &iocb->ioParameters,
			       (QIO_STATUS_T) iostate)) {
		DBG_assert(iocb->mailbox != NULL, "Expected non-NULL mailbox");	/* check for bad mailbox/completion function combination */
		iocb->ioState = iostate;
		KRN_putMbox(iocb->mailbox, iocb);
	}

	/* cancel the QIO timeout if one was set
	   (KRN_cancelTimer is protected against the timer
	   having already expired) */
	if (iocb->timerSet)
		KRN_cancelTimer(&iocb->timer);

	PARACHECK();

	return;
}

/*
** FUNCTION:	QIO_extioISR
**
** DESCRIPTION:	ISR for external device interrupts
**
** RETURNS:	void
*/
static void _QIO_extioISR(int32_t sigNum)
{
	IRQ_DESC_T *irqDesc;
	QIO_DEVICE_T *dev;
#ifdef CONFIG_DEBUG_PROFILING
	KRN_RAWSTATS_T s1, s2;
#endif

	irqDesc = IRQ_ack(IRQ_cause(sigNum));
	if (!irqDesc)
		return;

	PARACHECK();

#ifdef CONFIG_DEBUG_PROFILING
	if (_ThreadStatsCtrl && _ThreadStatsArray)
		/* performance monitoring installed and switched on */
		_KRN_grabStats(&s1);
#endif
	dev = (QIO_DEVICE_T *) irqDesc->priv;
	if (dev->driver->isrFunc)
		dev->driver->isrFunc(dev);
	else
		defaultISRFunction(dev);

	if (_KRN_current == NULL)
		/* if scheduler was blocked, it may now find a task */
		KRN_scheduleProtected();
#ifdef CONFIG_DEBUG_PROFILING
	if (_ThreadStatsCtrl && _ThreadStatsArray) {
		/* performance monitoring installed and switched on */
		KRN_SCHEDULE_T *s = _KRN_schedule;
		KRN_STATS_T *sp = s->dIntStats;
		_KRN_grabStats(&s2);
		_KRN_deltaStats(&s2, &s2, &s1);
		_KRN_accStats(sp, &s2, 0);
		sp->runCount++;
		/* time attributed to QIO processing is removed from the current task
		 * (although interrupt overheads will remain with the interrupted task
		 */
		sp = _KRN_current ? _KRN_current->statsPtr : s->nullStats;
		_KRN_accStats(sp, &s2, 1);
	}
#endif

	PARACHECK();
}
