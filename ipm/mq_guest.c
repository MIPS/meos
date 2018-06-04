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

PARATYPE(MsQG, MQGUEST_T, NULL, NULL, NULL, NULL, _DBG_paranoidGetNext);

#define MQ_CLASS	MQGUEST
#define	MQ_EASY		1
#define MQ_KICK(MQ)	IRQ_synthesize(((MQGUEST_T*)(MQ))->kick)
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

void MQGUEST_init(MQGUEST_T * mq, KRN_POOL_T * hpool, void *buffer,
		  size_t pagesize, IRQ_DESC_T * shin, IRQ_DESC_T * kick)
{
	mq->kick = kick;
	MQGUEST_initAll(mq, hpool, (uintptr_t) buffer, 0, 0, pagesize, shin);
}
