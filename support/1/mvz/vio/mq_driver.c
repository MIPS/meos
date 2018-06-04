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
*          File:	$File: //meta/fw/meos2/DEV/LISA.PARRATT/ipm/c $
* Revision date:	$Date: 2016/05/12 $
*   Description:	VRing-alike implementation
*
*************************************************************************/

#include "meos/debug/dbg.h"
#include "meos/irq/irq.h"
#include "meos/mq/mq.h"
#include "meos/mem/mem.h"
#include <stdlib.h>
#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"

PARATYPE(MsQV, MQDRIVER_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/* Used by a driver to represent a driver */

#define MQ_CLASS	MQDRIVER
#define MQ_KICK(MQ)	do {VIO_synthInt((MQ)->vio, VIRTIO_INT_USED_RING_UPDATE);} while (0)
#define MQ_GETDESC(MQ, DEST, INDEX)	do {MVZ_readGPC((void*)&(MQ)->descriptors[(INDEX)], &(DEST), sizeof(MQ_DESC_T), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_SETDESC(MQ, SRC, INDEX)	do {MVZ_writeGPC((void*)&(MQ)->descriptors[(INDEX)], &(SRC), sizeof(MQ_DESC_T), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_GETAVAIL(MQ, DEST)	do {MVZ_readGPC((void*)((MQ)->available), &(DEST), offsetof(MQ_AVAIL_T, ring), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_SETAVAIL(MQ, SRC)	do {MVZ_writeGPC((void*)((MQ)->available), &(SRC), offsetof(MQ_AVAIL_T, ring), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_GETAVAILRING(MQ, DEST, INDEX)	do {MVZ_readGPC((void*)&(MQ)->available->ring[(INDEX)], &(DEST), sizeof(uint16_t), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_SETAVAILRING(MQ, SRC, INDEX)	do {MVZ_writeGPC((void*)&(MQ)->available->ring[(INDEX)], &(SRC), sizeof(uint16_t), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_GETMQUSED(MQ, DEST)	do {MVZ_readGPC((void*)((MQ)->used), &(DEST), offsetof(MQ_USED_T, ring), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_SETMQUSED(MQ, SRC)	do {MVZ_writeGPC((void*)((MQ)->used), &(SRC), offsetof(MQ_USED_T, ring), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_GETUSEDRING(MQ, DEST, INDEX)	do {MVZ_readGPC((void*)&(MQ)->used->ring[(INDEX)], &(DEST), sizeof(MQ_USED_ELEM_T), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)
#define MQ_SETUSEDRING(MQ, SRC, INDEX)	do {MVZ_writeGPC((void*)&(MQ)->used->ring[(INDEX)], &(SRC), sizeof(MQ_USED_ELEM_T), 1, ((MQDRIVER_T*)(MQ))->guest);} while (0)

#include "meos/mq/mq_template.h"

void MQDRIVER_init(MQDRIVER_T * mq, KRN_POOL_T * hpool,
		   uintptr_t desc, uintptr_t avail, uintptr_t used,
		   size_t pagesize, MVZ_GUEST_T * gt, VIO_T * vio)
{
	mq->guest = gt;
	mq->vio = vio;
	MQDRIVER_initAll(mq, hpool, desc, avail, used, pagesize, NULL);
}
