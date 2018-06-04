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
*   Description:	QIO disable
*
*************************************************************************/

#include "MEOS.h"

void _QIO_powerDown(QIO_DEVICE_T * dev);
void _QIO_cancelAllProtected(QIO_DEVICE_T * dev);

/*
** FUNCTION:      QIO_disable
**
** DESCRIPTION:   Disable a device
**
** RETURNS:       void
*/
void QIO_disable(QIO_DEVICE_T * dev)
{
	IRQ_IPL_T oldipl;

	/* An initial call to QIO_cancelAll is an attempt to clear out as many operations
	 * as possible before the final atomic cancel and disable operation - this is a crude
	 * way of trying to avoid disabling interrupts for too long
	 */
	QIO_cancelAll(dev);	/* do our best to cancel pending and active ops */

	oldipl = IRQ_raiseIPL();
	_QIO_cancelAllProtected(dev);	/* cancel any new operations that sneaked in... */
	dev->enabled = 0;
	if (dev->active <= 0)
		_QIO_powerDown(dev);
	/* else power down will be deferred until active operations complete */
	IRQ_restoreIPL(oldipl);
}
