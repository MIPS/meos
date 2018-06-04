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
*   Description:	NULL QIO driver example
*
*************************************************************************/

#include <assert.h>

#include "MEOS.h"

#include "nulldev.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define NUM_UNITS 4
#define STACKSIZE 1000

/* local variables */

static struct regSet {
	QIO_NULLDEV_OPCODE_T opcode;
	uint8_t *pointer;
	int32_t counter;
	int32_t cancelled;
	KRN_SEMAPHORE_T trigger;
} registers[NUM_UNITS];
static uint32_t simStacks[NUM_UNITS][STACKSIZE];
static KRN_TASK_T simTasks[NUM_UNITS];

/* local prototypes */

static void init(QIO_DEVICE_T * dev, QIO_DEVPOWER_T * pwrClass,
		 int32_t * devRank);
static void start(QIO_DEVICE_T * dev, QIO_IOPARS_T * ioPars);
static void cancel(QIO_DEVICE_T * dev);
static void power(QIO_DEVICE_T * dev, QIO_POWERCMD_T cmd);
static void simStart(QIO_DEVICE_T * dev, uint32_t start);
static void simDev(void);

/* the driver object - initialised at compile time */

QIO_DRIVER_T QIO_NULLDEVDriver = {
	NULL,			/* Use default ISR        */
	init,			/* init function          */
	start,			/* start function         */
	cancel,			/* cancel function        */
	power,			/* power control function */
	simStart,		/* sim start function     */
	NULL			/* shutdown function      */
};

static IRQ_DESC_T irq[2];

static void init(QIO_DEVICE_T * dev, QIO_DEVPOWER_T * pwrClass,
		 int32_t * devRank)
{
	assert(dev->id >= 0);
	assert(dev->id < NUM_UNITS);
	/* provide information about the device */

	*devRank = 1;
	switch (dev->id) {
	case 0:
		*pwrClass = QIO_POWERNONE;
		break;
	case 1:
		*pwrClass = QIO_POWERCOARSE;
		break;
	default:
		*pwrClass = QIO_POWERFINE;
		break;
	}

	/* initialise the "hardware" */
	KRN_initSemaphore(&registers[dev->id].trigger, 0);
	registers[dev->id].cancelled = 0;
	/* set up interrupt mapping we pretend our NULL devices are on */
	if (dev->id == 2) {
		dev->numIrqs = 0;
	} else {
		dev->numIrqs = 1;
		IRQ_soft(dev->id, &irq[dev->id]);
		dev->irqDescs = &irq[dev->id];
	}
}

static void start(QIO_DEVICE_T * dev, QIO_IOPARS_T * ioPars)
{
	/* load device registers */
	registers[dev->id].counter = ioPars->counter;
	registers[dev->id].pointer = (uint8_t *) ioPars->pointer;
	registers[dev->id].opcode = (QIO_NULLDEV_OPCODE_T) ioPars->opcode;
	/* start device */
	KRN_setSemaphore(&registers[dev->id].trigger, 1);
}

static void cancel(QIO_DEVICE_T * dev)
{
	registers[dev->id].cancelled = 1;
}

static void power(QIO_DEVICE_T * dev, QIO_POWERCMD_T cmd)
{
	DBG_logF("%s: Power %s\n", QIO_devName(dev),
		 cmd == QIO_POWERNORMAL ? "UP" : cmd ==
		 QIO_POWERSAVE ? "DOWN" : "UNKNOWN");
	/* nothing to do */
}

static void simStart(QIO_DEVICE_T * dev, uint32_t start)
{
	int32_t d = dev->id;
	if (start)
		KRN_startTask(simDev, &simTasks[d], &simStacks[d][0], STACKSIZE,
			      KRN_maxPriority() - 1, dev, "null simulator");
	else
		KRN_removeTask(&simTasks[d]);
}

static void simDev()
{
	uint8_t *p;
	int32_t c;
	QIO_NULLDEV_OPCODE_T op;
	QIO_DEVICE_T *dev;
	IRQ_IPL_T oldipl;
	IRQ_DESC_T soft;

	dev = (QIO_DEVICE_T *) KRN_taskParameter(NULL);
	for (;;) {
		KRN_testSemaphore(&registers[dev->id].trigger, 1, KRN_INFWAIT);

		c = registers[dev->id].counter;
		p = registers[dev->id].pointer;
		op = registers[dev->id].opcode;
		if (op == QIO_NULLDEV_READ) {
			DBG_logF("%s: READ %" PRId32 " bytes\n",
				 QIO_devName(dev), c);
			/* zero fill */
			while (c--)
				*p++ = 0;
		} else if (op == QIO_NULLDEV_WRITE) {
			DBG_logF("%s: WRITE %" PRId32 " bytes\n",
				 QIO_devName(dev), c);
		} else
			DBG_logF("%s: BAD opcode\n", QIO_devName(dev));
		/* unit 2 is a simulated broken device which never interrupts -
		   we use it to exercise IO timeouts. */
		if (dev->id == 2) {
			DBG_logF("%s: Simulated device failure\n",
				 QIO_devName(dev));
		} else {
			oldipl = IRQ_raiseIPL();
			if (registers[dev->id].cancelled) {
				registers[dev->id].cancelled = 0;	/* clear cancel flag without interrupting */
			} else {
				if (IRQ_soft(dev->id, &soft))
					IRQ_synthesize(&soft);
			}
			IRQ_restoreIPL(oldipl);
		}
	}
}
