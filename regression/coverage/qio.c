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
*   Description:	QIO module coverage test
*
*************************************************************************/

#undef NDEBUG

#include <stdlib.h>

#include "MEOS.h"

#include "nulldev.h"

#define NUM_IOCBS 4
#define STACKSIZE 2000
#define TIMEOUT 10000

#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES-1)

static KRN_TASK_T ptask;
static KRN_TASK_T ctask;

extern uint32_t stack1[STACKSIZE];
extern uint32_t stack2[STACKSIZE];

/* three devices */
static QIO_DEVICE_T dev0;
static QIO_DEVICE_T dev1;
static QIO_DEVICE_T dev2;
static QIO_DEVICE_T nondev;

static KRN_MAILBOX_T mbox0;
static KRN_MAILBOX_T mbox1;
static KRN_MAILBOX_T mbox2;
static KRN_MAILBOX_T mbox3;

static KRN_SYNC_T sync, done;

void prosumer(void);

static int32_t complete(QIO_DEVICE_T * dev, QIO_IOCB_T * iocb,
			QIO_IOPARS_T * iopars, QIO_STATUS_T status)
{
	(void)iopars;
	(void)iocb;

	DBG_logF("%s: Complete Function, status=%s\n", QIO_devName(dev),
		 status == QIO_NORMAL ? "NORMAL" :
		 status == QIO_CANCEL ? "CANCEL" :
		 status == QIO_TIMEOUT ? "TIMEOUT" : "UNKNOWN");

	return 0;
}

static int32_t completeAndReturn(QIO_DEVICE_T * dev, QIO_IOCB_T * iocb,
				 QIO_IOPARS_T * iopars, QIO_STATUS_T status)
{
	(void)iopars;

	DBG_logF("%s: Complete And Return Function, status=%s\n",
		 QIO_devName(dev),
		 status == QIO_NORMAL ? "NORMAL" : status ==
		 QIO_CANCEL ? "CANCEL" : status ==
		 QIO_TIMEOUT ? "TIMEOUT" : "UNKNOWN");

	KRN_returnPool(iocb);

	return 1;
}

QIO_IOCB_T iocbs[NUM_IOCBS];
KRN_POOL_T iocbPool;
uint8_t buf[20];

void qio(void)
{

	DBG_logF("QIO Test\n");

	KRN_initSync(&sync, 2);
	KRN_initSync(&done, 3);

	/* initialise QIO system and our devices. We may have either vectored or
	   direct devices depending on whether or not the nulldev device driver
	   is compiled with MTXC_ANY
	 */
	QIO_reset(&qioSys);
	QIO_init(&dev0, "NULL device [0]", 0, &QIO_NULLDEVDriver);
	QIO_init(&dev1, "NULL device [1]", 1, &QIO_NULLDEVDriver);
	QIO_init(&dev2, "NULL device [2]", 2, &QIO_NULLDEVDriver);
	QIO_init(&nondev, NULL, 3, &QIO_NULLDEVDriver);

	/* Get the name */
	(void)QIO_devName(&nondev);

	/* unload and re-initialise devices - just to prove we can and to allow
	   a walkthrough of the unload code in a debugger */

	QIO_unload(&dev0);
	QIO_unload(&dev1);
	QIO_unload(&dev2);
	QIO_unload(&nondev);
	QIO_init(&dev0, "NULL device [0]", 0, &QIO_NULLDEVDriver);
	QIO_init(&dev1, "NULL device [1]", 1, &QIO_NULLDEVDriver);
	QIO_init(&dev2, "NULL device [2]", 2, &QIO_NULLDEVDriver);

	/* don't forget to enable our devices! */
	QIO_enable(&dev0);
	QIO_enable(&dev1);
	QIO_enable(&dev2);

	KRN_initMbox(&mbox0);	/* device level mailbox protection !!! */
	KRN_initMbox(&mbox1);	/* device level mailbox protection !!! */
	KRN_initMbox(&mbox2);	/* device level mailbox protection !!! */
	KRN_initMbox(&mbox3);	/* device level mailbox protection !!! */
	KRN_initPool(&iocbPool, iocbs, NUM_IOCBS, sizeof(QIO_IOCB_T));

	KRN_startTask(prosumer, &ptask, stack1, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Producer task");
	KRN_startTask(prosumer, &ctask, stack2, STACKSIZE,
		      KRN_LOWEST_PRIORITY + 1, NULL, "Consumer task");

	KRN_sync(&done, KRN_INFWAIT);
}

void _QIO_cancelAllProtected(QIO_DEVICE_T * dev);

void prosumer()
{
	IRQ_IPL_T ipl;
	QIO_IOPARS_T iopars;
	QIO_IOCB_T *iocb1, *iocbr1;
	QIO_IOCB_T *iocb2, *iocbr2;
	QIO_IOCB_T *iocb3, *iocbr3;
	QIO_DEVICE_T *dev;
	QIO_STATUS_T status;
	uintptr_t n;

	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		/* Bounce test - fire off a string of IOs to the consumer via dev0/mbox0.
		   Consumer will send them back via dev1/mbox1 */
		iopars.pointer = buf;
		iopars.counter = sizeof(buf);
		for (n = 0; n < 5; n++) {
			iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
			iopars.opcode = QIO_NULLDEV_WRITE;
			iopars.spare = (void *)n;
			DBG_logF("%s writing via %s for bounce test\n",
				 KRN_taskName(NULL), QIO_devName(&dev0));
			QIO_qio(&dev0, iocb1, &iopars, &mbox0, NULL,
				KRN_INFWAIT);
			iopars.spare = (void *)-1;
			iocb2 =
			    QIO_result(&mbox1, &dev, &status, &iopars,
				       KRN_INFWAIT);
			DBG_logF("%s read via %s for bounce test\n",
				 KRN_taskName(NULL), QIO_devName(dev));
			DBG_assert(iocb1 == iocb2, "iocb mismatch");
			KRN_returnPool(iocb2);
			DBG_assert(dev == &dev1, "Device wrong");
			DBG_assert(status == QIO_NORMAL, "Status wrong");
			DBG_assert(iopars.spare == (void *)n, "n wrong");
		}
	} else {
		/* consumer assists with bounce test */
		for (n = 0; n < 5; n++) {
			iocb1 =
			    QIO_result(&mbox0, &dev, &status, &iopars,
				       KRN_INFWAIT);
			DBG_logF("%s read via %s for bounce test\n",
				 KRN_taskName(NULL), QIO_devName(dev));
			DBG_assert(dev == &dev0, "Device wrong");
			DBG_assert(status == QIO_NORMAL, "Status wrong");
			DBG_assert(iopars.spare == (void *)n, "n wrong");
			DBG_assert(iopars.opcode == QIO_NULLDEV_WRITE,
				   "opcode wrong");
			iopars.opcode = QIO_NULLDEV_READ;
			DBG_logF("%s writing via %s for bounce test\n",
				 KRN_taskName(NULL), QIO_devName(&dev1));
			QIO_qio(&dev1, iocb1, &iopars, &mbox1, NULL,
				KRN_INFWAIT);
		}
	}
	DBG_logF("%s bounce test done\n", KRN_taskName(NULL));

	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		DBG_logF
		    ("%s bounce test with timeout on write (using \"broken\" device)\n",
		     KRN_taskName(NULL));
		for (n = 0; n < 5; n++) {
			iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
			iopars.opcode = QIO_NULLDEV_WRITE;
			iopars.spare = (void *)n;
			DBG_logF
			    ("%s writing via %s for bounce test with QIO timeout\n",
			     KRN_taskName(NULL), QIO_devName(&dev2));
			QIO_qio(&dev2, iocb1, &iopars, &mbox0, NULL, TIMEOUT);
			iopars.spare = (void *)-1;
			iocb2 =
			    QIO_result(&mbox1, &dev, &status, &iopars,
				       KRN_INFWAIT);
			DBG_logF
			    ("%s read via %s for bounce test with QIO timeout\n",
			     KRN_taskName(NULL), QIO_devName(dev));
			DBG_assert(iocb1 == iocb2, "iocb mismatch");
			KRN_returnPool(iocb2);
			DBG_assert(dev == &dev2, "Device wrong");
			DBG_assert(status == QIO_TIMEOUT, "Status wrong");
			DBG_assert(iopars.spare == (void *)n, "n wrong");
		}
	} else {
		/* consumer assists with bounce test */
		for (n = 0; n < 5; n++) {
			iocb1 =
			    QIO_result(&mbox0, &dev, &status, &iopars,
				       KRN_INFWAIT);
			DBG_logF
			    ("%s read via %s for bounce test with QIO timeout\n",
			     KRN_taskName(NULL), QIO_devName(dev));
			DBG_assert(dev == &dev2, "Device wrong");
			DBG_assert(status == QIO_TIMEOUT, "Status wrong");
			DBG_assert(iopars.spare == (void *)n, "n wrong");
			DBG_assert(iopars.opcode == QIO_NULLDEV_WRITE,
				   "opcode wrong");
			iopars.opcode = QIO_NULLDEV_READ;
			DBG_logF
			    ("%s writing via %s for bounce test with QIO timeout\n",
			     KRN_taskName(NULL), QIO_devName(&dev2));
			QIO_qio(&dev2, iocb1, &iopars, &mbox1, NULL, TIMEOUT);
		}
	}
	DBG_logF("%s bounce test with timeout done\n", KRN_taskName(NULL));

	DBG_logF
	    ("%s bounce test with timeout on write (using disabled \"broken\" device)\n",
	     KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		QIO_disable(&dev2);
		iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iocb2 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iocb3 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iopars.opcode = QIO_NULLDEV_WRITE;
		iopars.spare = (void *)n;
		DBG_logF
		    ("%s writing via %s for bounce test with QIO timeout\n",
		     KRN_taskName(NULL), QIO_devName(&dev2));
		QIO_qio(&dev2, iocb1, &iopars, &mbox3, NULL, TIMEOUT);
		QIO_qio(&dev2, iocb2, &iopars, &mbox3, complete, TIMEOUT);
		QIO_qio(&dev2, iocb3, &iopars, &mbox3, NULL, TIMEOUT);
		QIO_cancel(iocb1);
		QIO_cancel(iocb2);
		QIO_cancel(iocb3);
		QIO_enable(&dev2);
		iocbr1 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		iocbr2 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		iocbr3 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		KRN_returnPool(iocbr1);
		KRN_returnPool(iocbr2);
		KRN_returnPool(iocbr3);
	}
	DBG_logF("%s bounce test with timeout done\n", KRN_taskName(NULL));

	/* Broken device means it never interrupts, thus will always timeout */
	DBG_logF
	    ("%s cancellation test (using \"broken\" device)\n",
	     KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iocb2 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iocb3 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
		iopars.opcode = QIO_NULLDEV_WRITE;
		iopars.spare = (void *)n;
		DBG_logF
		    ("%s writing via %s for cancel test\n",
		     KRN_taskName(NULL), QIO_devName(&dev2));
		QIO_qio(&dev2, iocb1, &iopars, &mbox3, NULL, KRN_INFWAIT);
		QIO_cancel(iocb1);
		QIO_qio(&dev2, iocb2, &iopars, &mbox3, complete, KRN_INFWAIT);
		QIO_qio(&dev2, iocb3, &iopars, &mbox3, complete, KRN_INFWAIT);
		ipl = IRQ_raiseIPL();
		_QIO_cancelAllProtected(&dev2);
		IRQ_restoreIPL(ipl);
		iocbr1 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		iocbr2 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		iocbr3 =
		    QIO_result(&mbox3, &dev, &status, &iopars, KRN_INFWAIT);
		/* If we got here then cancels must have taken, else result would block */
		KRN_returnPool(iocbr1);
		KRN_returnPool(iocbr2);
		KRN_returnPool(iocbr3);
	}
	DBG_logF("%s cancellation test (using \"broken\" device) done \n",
		 KRN_taskName(NULL));

	DBG_logF
	    ("%s cancellation test (using \"broken\" device and using completion)\n",
	     KRN_taskName(NULL));
	for (n = 0; n < NUM_IOCBS; n++) {
		if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
			iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
			iopars.opcode = QIO_NULLDEV_WRITE;
			iopars.spare = (void *)n;
			DBG_logF("%s writing via %s for cancellation test \n",
				 KRN_taskName(NULL), QIO_devName(&dev2));
			QIO_qio(&dev2, iocb1, &iopars, &mbox0, complete,
				3 * TIMEOUT);
		} else {
			while ((iocb1 =
				QIO_result(&mbox0, &dev, &status, &iopars,
					   TIMEOUT)) == NULL) {
				DBG_logF("%s cancelling operations on %s\n",
					 KRN_taskName(NULL),
					 QIO_devName(&dev2));
				QIO_cancelAll(&dev2);
			}
			DBG_logF("%s read via %s for cancel test\n",
				 KRN_taskName(NULL), QIO_devName(dev));
			DBG_assert(dev == &dev2, "Device wrong");
			DBG_assert(status == QIO_CANCEL, "Status wrong");
			DBG_assert(iopars.spare == (void *)n, "n wrong");
			DBG_assert(iopars.opcode == QIO_NULLDEV_WRITE,
				   "opcode wrong");
			DBG_logF
			    ("%s received cancelled operation from %s\n",
			     KRN_taskName(NULL), QIO_devName(&dev2));
			KRN_returnPool(iocb1);
		}
	}
	DBG_logF
	    ("%s cancellation test (using \"broken\" device and using completion) done\n",
	     KRN_taskName(NULL));

	DBG_logF("%s qioWait tests starting\n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		DBG_assert(QIO_qioWait(&dev0, &iopars, TIMEOUT) == QIO_NORMAL,
			   "Wait failure");
		DBG_assert(QIO_qioWait(&dev0, &iopars, KRN_INFWAIT) ==
			   QIO_NORMAL, "Wait failure");
		DBG_assert(QIO_qioWait(&dev2, &iopars, TIMEOUT) == QIO_TIMEOUT,
			   "Wait failure");
	}
	DBG_logF("%s qioWait tests done\n", KRN_taskName(NULL));

	DBG_logF("%s self completion test \n", KRN_taskName(NULL));
	if (KRN_sync(&sync, KRN_INFWAIT) == &ptask) {
		for (n = 0; n < NUM_IOCBS; n++) {
			iocb1 = (QIO_IOCB_T *) KRN_takePool(&iocbPool, KRN_INFWAIT);	/* get an iocb */
			iopars.opcode = QIO_NULLDEV_WRITE;
			iopars.spare = (void *)n;
			DBG_logF
			    ("%s writing via %s for self completion test \n",
			     KRN_taskName(NULL), QIO_devName(&dev0));
			QIO_qio(&dev0, iocb1, &iopars, &mbox0,
				completeAndReturn, KRN_INFWAIT);
		}
	}
	DBG_logF("%s self completion test done\n", KRN_taskName(NULL));
	/* tests complete - kill everything off to exit */
	DBG_logF("Tests complete\n");

	KRN_sync(&done, KRN_INFWAIT);
}
