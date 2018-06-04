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
*   Description:	QIO wait
*
*************************************************************************/

#include "MEOS.h"

/*
** FUNCTION:      QIO_qioWait
**
** DESCRIPTION:   Simplified synchronous interface to QIO_qio
**
** RETURNS:       QIO_STATUS_T completion status
*/
QIO_STATUS_T QIO_qioWait(QIO_DEVICE_T * dev, QIO_IOPARS_T * iopars,
			 int32_t timeout)
{
	KRN_MAILBOX_T outBox;
	QIO_IOCB_T iocb;
	QIO_DEVICE_T *dummyDev;
	QIO_STATUS_T status;

	PARACHECK();

	KRN_initMbox(&outBox);
	QIO_qio(dev, &iocb, iopars, &outBox, NULL, timeout);
	QIO_result(&outBox, &dummyDev, &status, iopars, KRN_INFWAIT);
	DBG_assert(dummyDev == dev, "Result from wrong device");

	PARACHECK();
	return status;
}
