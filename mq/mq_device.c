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
*   Description:	VRing-alike implementation
*
*************************************************************************/

#include "meos/debug/dbg.h"
#include "meos/irq/irq.h"
#include "meos/mq/mq.h"
#include "meos/mem/mem.h"
#include <stdlib.h>

PARATYPE(MsQD, MQDEVICE_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

/* Used by a driver to represent a device */

#define MQ_CLASS	MQDEVICE
#define MQ_HOST		1
#define	MQ_EASY		1
#define MQ_KICK(MQ)	MEM_ww((MQ)->qNotify,(MQ)->qNum)
#define MQ_NOKICKSTART	1
#define MQ_GETDESC(MQ, DEST, INDEX)	do {(DEST) = (MQ)->descriptors[(INDEX)];} while (0)
#define MQ_SETDESC(MQ, SRC, INDEX)	do {(MQ)->descriptors[(INDEX)] = (SRC);} while (0)
#define MQ_GETAVAIL(MQ, DEST)	do {(DEST) = *(MQ)->available;} while (0)
#define MQ_SETAVAIL(MQ, SRC)	do {*(MQ)->available = (SRC);} while (0)
#define MQ_GETAVAILRING(MQ, DEST, INDEX)	do {(DEST) = (MQ)->available->ring[(INDEX)];} while (0)
#define MQ_SETAVAILRING(MQ, SRC, INDEX)	do {(MQ)->available->ring[(INDEX)] = (SRC);} while (0)
#define MQ_GETMQUSED(MQ, DEST)	do {(DEST) = *(MQ)->used;} while (0)
#define MQ_SETMQUSED(MQ, SRC)	do {*(MQ)->used = (SRC);} while (0)
#define MQ_GETUSEDRING(MQ, DEST, INDEX)	do {(DEST) = (MQ)->used->ring[(INDEX)];} while (0)
#define MQ_SETUSEDRING(MQ, SRC, INDEX)	do {(MQ)->used->ring[(INDEX)] = (SRC);} while (0)

#include "meos/mq/mq_template.h"

void MQDEVICE_init(MQDEVICE_T * mq, KRN_POOL_T * hpool,
		   void *buffer, size_t pagesize, IRQ_DESC_T * shin,
		   uint32_t * vAddr, uint32_t queueNum,
		   KRN_POOL_T * mpool, size_t msize)
{
	mq->qNotify = vAddr + (VIRTIO_REG_QUEUENOTIFY / sizeof(uint32_t));
#ifdef CONFIG_ARCH_MIPS_BIG_ENDIAN
	mq->qNum = ((queueNum >> 24) & 0xff) |
	    ((queueNum << 8) & 0xff0000) |
	    ((queueNum >> 8) & 0xff00) | ((queueNum << 24) & 0xff000000);
#else
	mq->qNum = queueNum;
#endif
	MQDEVICE_initAll(mq, hpool, (uintptr_t) buffer, 0, 0, pagesize,
			 shin, mpool, msize);
	MEM_leww(vAddr + (VIRTIO_REG_QUEUESEL / sizeof(uint32_t)), queueNum);
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEDESCLOW / sizeof(uint32_t)),
		 MEM_v2p((void *)mq->mq.descriptors));
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEDESCHIGH / sizeof(uint32_t)), 0);
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEAVAILLOW / sizeof(uint32_t)),
		 MEM_v2p((void *)mq->mq.available));
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEAVAILHIGH / sizeof(uint32_t)), 0);
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEUSEDLOW / sizeof(uint32_t)),
		 MEM_v2p((void *)mq->mq.used));
	MEM_leww(vAddr + (VIRTIO_REG_QUEUEUSEDHIGH / sizeof(uint32_t)), 0);
	MEM_leww(vAddr + (VIRTIO_REG_QUEUENUM / sizeof(uint32_t)),
		 _LST_len(&hpool->freeList));
}
