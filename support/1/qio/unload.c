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
*   Description:	QIO unload
*
*************************************************************************/

#include "MEOS.h"

#define WRITE(A,V) (*((volatile uint32_t *)(A))=((uint32_t)V))
#define READ(A) (*(volatile uint32_t *)(A))

extern QIO_SYS_T *_QIO;

void QIO_unload(QIO_DEVICE_T * dev)
{
	QIO_DRIVER_T const *driver = dev->driver;
	IRQ_IPL_T oldipl;
	int32_t n;

	DBG_assert(IRQ_bg(),
		   "Can not unload QIO device from interrupt context");
	DBG_assert(dev->enabled == 0, "Device not loaded");
	DBG_assert(driver != NULL, "Driver NULL");

	/* if the device is simulated by a software task, then stop the
	   simulation task */
	if (driver->simFunc != NULL)
		driver->simFunc(dev, 0);
	oldipl = IRQ_raiseIPL();
	for (n = 0; n < dev->numIrqs; n++) {
		dev->irqDescs[n].isrFunc = NULL;
		IRQ_route(&dev->irqDescs[n]);
	}
	IRQ_restoreIPL(oldipl);

	/* "disable" the driver object */
	dev->enabled = 0;
	dev->devName = 0;
	dev->driver = NULL;
}
