/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Virtio console
*
*************************************************************************/

#include <stdio.h>
#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/mem/mem.h"
#include "meos/inttypes.h"

int VIO_readDummy(void *address, void *buffer, int size, int n, void *priv)
{
	DBG_assert(0,
		   "Guest '%s' attempted read from unsupported Virtio dummy register %03"
		   PRIx32 "\n", KRN_taskName(NULL), (uintptr_t) address);
	MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	return 0;
}

int VIO_writeDummy(void *address, void *buffer, int size, int n, void *priv)
{
	DBG_assert(0,
		   "Guest '%s' attempted read from unsupported Virtio dummy register %03"
		   PRIx32 "\n", KRN_taskName(NULL), (uintptr_t) address);
	MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	return 0;
}

void VIO_budDummy(VIO_T * vio, reg_t down)
{
	if (!down) {
		DBG_assert(0,
			   "Guest '%s' attempted bring up of Virtio dummy device\n",
			   KRN_taskName(NULL));
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
}

void VIO_initDummy(VIO_DUMMY_T * dummy, uintptr_t base)
{
	VIO_init(&dummy->vio, 0, 0);
	dummy->vio.queues = NULL;
	dummy->vio.nQueues = 0;
	dummy->vio.read = VIO_readDummy;
	dummy->vio.write = VIO_writeDummy;
	dummy->vio.bringUpDown = VIO_budDummy;
	dummy->vio.gInt = 0;
	dummy->vio.regs.start = base;
	dummy->vio.regs.stop = base + 0xff;
}
