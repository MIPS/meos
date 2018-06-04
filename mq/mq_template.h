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

PARAEXTERN(HstB, MQ_HOSTBUF_T);
PARAEXTERN(MQMs, MQ_MSG_T);
PARAEXTERN(MsQu, MQ_T);

/*
 * MQ_CLASS 	Class of MQ
 * MQ_PAR	Paranoia fourcc
 * MQ_HOST	Define if this ring should host
 * MQ_EASY	Define if this ring should provide send/recv functions
 * MQ_KICK(mqt)	Define how MQ can kick
 * MQ_NOKICKSTART	Don't kick during startup, required for guesting a virtio device
 * MQ_GETDESC
 * MQ_SETDESC
 * MQ_GETAVAIL
 * MQ_SETAVAIL
 * MQ_GETAVAILRING
 * MQ_SETAVAILRING
 * MQ_GETMQUSED
 * MQ_SETMQUSED
 * MQ_GETUSEDRING
 * MQ_SETUSEDRING
 */

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
#define MQ(NAME) CAT(MQ_CLASS,CAT(_,NAME))

#define TOPOOLABLE(BUF) ((MQ_HOSTBUF_T *) (((uint8_t *) BUF) - offsetof(MQ_HOSTBUF_T, buffer)))
#define FROMPOOLABLE(POOLABLE) ((void *)(((uint8_t *) POOLABLE) + offsetof(MQ_HOSTBUF_T, buffer)))

#ifdef MQ_KICK
inline static void nudge(MQ_T * mq)
{
	PARACHECK();
#ifdef MQ_HOST
	MQ_USED_T used;
	MQ_GETMQUSED(mq, used);
	if (used.flags & MQ_USED_NO_NOTIFY_FLAG)
		return;
#else
	MQ_AVAIL_T avail;
	MQ_GETAVAIL(mq, avail);
	if (avail.flags & MQ_AVAILABLE_NO_INTERRUPT_FLAG)
		return;
#endif
	MQ_KICK((MQ(T) *) mq);
	PARACHECK();
}
#endif

inline static void addFree(MQ_T * mq, uint16_t index)
{
	MQ_DESC_T desc;
	uint16_t lastDesc = index;
	IRQ_IPL_T ipl = IRQ_raiseIPL();

	MQ_GETDESC(mq, desc, lastDesc);
	while ((desc.flags & MQ_DESC_F_NEXT) == MQ_DESC_F_NEXT) {
		lastDesc = desc.next;
		MQ_GETDESC(mq, desc, lastDesc);
	}
	desc.next = mq->freeDescs;
	desc.flags |= MQ_DESC_F_NEXT;
	MQ_SETDESC(mq, desc, lastDesc);
	mq->freeDescs = index;
	IRQ_restoreIPL(ipl);
}

inline static uint16_t getFree(MQ_T * mq)
{
	MQ_DESC_T desc;
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	uint16_t descn = mq->freeDescs;
	MQ_GETDESC(mq, desc, descn);
	mq->freeDescs = desc.next;
	IRQ_restoreIPL(ipl);

	return descn;
}

#ifdef MQ_HOST
inline static int32_t peekUsed(MQ_T * mq, uintptr_t * buf)
{
	MQ_DESC_T desc;
	MQ_USED_T used;
	MQ_USED_ELEM_T elem;
	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETMQUSED(mq, used);
	if (mq->lastUsed == used.index)
		return 0;
	uint32_t i = mq->lastUsed % mq->size;

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETUSEDRING(mq, elem, i);
	uint16_t index = elem.id;
	MQ_GETDESC(mq, desc, index);
	*buf = desc.address;

	PARACHECK();
	return 1;
}
#endif

void MQ(notify) (MQ(T) * mqt);
static void isr(int32_t num)
{
	IRQ_DESC_T *irq = IRQ_ack(IRQ_cause(num));
	if (irq) {
		MQ(T) * mqt = (MQ(T) *) irq->priv;
		do {
			mqt->mq.notify((MQ_T *) mqt);
			mqt = (MQ(T) *) LST_next(mqt);
		} while (mqt);
	}
}

static inline int32_t _LST_len(LST_T * list)
{
	void *item;
	int32_t count = 0;
	for (item = LST_first(list); item; item = LST_next(item))
		count++;
	return count;
}

/* Externally visible and renamed */

size_t MQ(remainingAvailable) (MQ(T) * mqt) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_AVAIL_T avail;
	size_t r;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	r = avail.index - mq->lastAvailable;

	PARACHECK();

	return r;
}

inline void MQ(addAvailable) (MQ(T) * mqt, MQ_MSG_T * msg) {
	MQ_T *mq = (MQ_T *) mqt;
	IRQ_IPL_T ipl;
	MQ_DESC_T availd;
	MQ_AVAIL_T avail;
	uint16_t index, first = 0;

	PARACHECK();

	index = getFree(mq);
	first = index;

	MQ_GETDESC(mq, availd, index);
	availd.address = (uintptr_t) msg->buffer;
	availd.length = msg->length;
	availd.flags = (msg->write) ? 0 : MQ_DESC_F_WRITE;

	msg = msg->next;
	while (msg) {
		availd.next = getFree(mq);
		availd.flags |= MQ_DESC_F_NEXT;
		MQ_SETDESC(mq, availd, index);

		index = availd.next;
		MQ_GETDESC(mq, availd, index);
		availd.address = (uintptr_t) msg->buffer;
		availd.length = msg->length;
		availd.flags = (msg->write) ? 0 : MQ_DESC_F_WRITE;
		msg = msg->next;
	}
	MQ_SETDESC(mq, availd, index);

	ipl = IRQ_raiseIPL();
	MQ_GETAVAIL(mq, avail);
	index = avail.index % mq->size;
	avail.flags = 0;
	MQ_SETAVAIL(mq, avail);
	MQ_SETAVAILRING(mq, first, index);

	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	avail.index++;
	MQ_SETAVAIL(mq, avail);
	IRQ_restoreIPL(ipl);

	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
#ifdef MQ_KICK
	nudge(mq);
#endif
}

inline uint16_t MQ(getFirstAvailable) (MQ(T) * mqt, uintptr_t * buffer,
				       size_t * length, uint16_t * flags) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_DESC_T desc;
	MQ_AVAIL_T avail;
	MQ_USED_T used;
	uint16_t index;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable == avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags &= ~MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
		MQ_GETAVAIL(mq, avail);
		if (mq->lastAvailable == avail.index)
			return 0xffff;
	}

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAILRING(mq, index, mq->lastAvailable % mq->size);
	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETDESC(mq, desc, index);
	*buffer = (uintptr_t) desc.address;
	*length = desc.length;
	*flags = desc.flags;

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable != avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags |= MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
	}

	PARACHECK();
	return index;
}

inline uint16_t MQ(getMoreAvailable) (MQ(T) * mqt, uint16_t token,
				      uintptr_t * buffer, size_t * length,
				      uint16_t * flags) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_DESC_T desc;
	MQ_AVAIL_T avail;
	MQ_USED_T used;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable == avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags &= ~MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
		MQ_GETAVAIL(mq, avail);
		if (mq->lastAvailable == avail.index)
			return 0xffff;
	}

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETDESC(mq, desc, token);
	if (((desc.flags & MQ_DESC_F_NEXT) != MQ_DESC_F_NEXT))
		return 0xffff;
	uint16_t index = desc.next;
	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETDESC(mq, desc, index);
	*buffer = (uintptr_t) desc.address;
	*length = desc.length;
	*flags = desc.flags;

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable != avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags |= MQ_USED_NO_NOTIFY_FLAG;
		MQ_GETMQUSED(mq, used);
	}

	PARACHECK();
	return index;
}

inline void MQ(getNextAvailable) (MQ(T) * mqt) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_AVAIL_T avail;
	MQ_USED_T used;
	PARACHECK();

	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable == avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags &= ~MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
		MQ_GETAVAIL(mq, avail);
		if (mq->lastAvailable == avail.index)
			return;
	}

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	mq->lastAvailable++;
	KRN_barrier(KRN_BARRIER_FLAG_READ);

	PARACHECK();
}

inline uint16_t MQ(getAvailable) (MQ(T) * mqt, uintptr_t * buffer,
				  size_t * length) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_DESC_T desc;
	MQ_AVAIL_T avail;
	MQ_USED_T used;
	uint16_t index;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable == avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags &= ~MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
		MQ_GETAVAIL(mq, avail);
		if (mq->lastAvailable == avail.index)
			return 0xffff;
	}

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAILRING(mq, index, mq->lastAvailable++ % mq->size);
	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETDESC(mq, desc, index);
	*buffer = (uintptr_t) desc.address;
	*length = desc.length;

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETAVAIL(mq, avail);
	if (mq->lastAvailable != avail.index) {
		MQ_GETMQUSED(mq, used);
		used.flags |= MQ_USED_NO_NOTIFY_FLAG;
		MQ_SETMQUSED(mq, used);
	}

	PARACHECK();
	return index;
}

inline uint16_t MQ(chainAvailable) (MQ(T) * mqt, uint16_t last,
				    uintptr_t * buffer, size_t * length) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_DESC_T desc;
	uint16_t result;
	PARACHECK();

	result = MQ(getAvailable) (mqt, buffer, length);
	MQ_GETDESC(mq, desc, last);
	desc.next = result;
	MQ_SETDESC(mq, desc, last);

	PARACHECK();
	return result;
}

size_t MQ(remainingUsed) (MQ(T) * mqt) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_USED_T used;
	size_t r;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETMQUSED(mq, used);
	r = used.index - mq->lastUsed;

	PARACHECK();

	return r;
}

inline void MQ(addUsed) (MQ(T) * mqt, uint16_t id, uint32_t length) {
	MQ_T *mq = (MQ_T *) mqt;
	PARACHECK();

	MQ_USED_T used;
	MQ_USED_ELEM_T elem;

	MQ_GETMQUSED(mq, used);

	uint32_t i = used.index % mq->size;

	elem.id = id;
	elem.length = length;
	MQ_SETUSEDRING(mq, elem, i);

	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	used.index++;
	MQ_SETMQUSED(mq, used);
#ifdef MQ_KICK
	nudge(mq);
#endif
}

inline uint32_t MQ(getUsed) (MQ(T) * mqt, uintptr_t * buffer, size_t * length) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_DESC_T desc;
	MQ_USED_T used;
	MQ_USED_ELEM_T elem;

	PARACHECK();

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETMQUSED(mq, used);
	if (mq->lastUsed == used.index)
		return 0;

	uint32_t i = mq->lastUsed++ % mq->size;

	KRN_barrier(KRN_BARRIER_FLAG_READ);
	MQ_GETUSEDRING(mq, elem, i);
	*length = elem.length;
	uint16_t index = elem.id;
	MQ_GETDESC(mq, desc, index);
	*buffer = (uintptr_t) desc.address;
	addFree(mq, index);
#ifdef MQ_HOST
	MQ(autoRel) (mqt);
#endif

	PARACHECK();
	return 1;
}

#ifdef MQ_HOST
void MQ(autoRel) (MQ(T) * mqt) {
	IRQ_IPL_T ipl;
	MQ_T *mq = (MQ_T *) mqt;
	uintptr_t buf;
	MQ_USED_ELEM_T elem;

	PARACHECK();
	
	ipl = IRQ_raiseIPL();
	while (peekUsed(mq, &buf)) {
		MQ_HOSTBUF_T *p = TOPOOLABLE((void *)
					     MEM_p2v(buf,
						     MEM_P2V_CACHED));
		if (p->header.autoRel) {
			uint32_t i = mq->lastUsed % mq->size;
			MQ_GETUSEDRING(mq, elem, i);
			uint16_t index = elem.id;
			addFree(mq, index);
			mq->lastUsed++;
			KRN_returnPool(p);
		} else
			break;
	}
	IRQ_restoreIPL(ipl);

	PARACHECK();
}
#endif

void MQ(notify) (MQ(T) * mqt) {
	MQ_T *mq = (MQ_T *) mqt;
#ifdef MQ_HOST
	MQ(autoRel) (mqt);
#endif
	KRN_wakeAll(&mq->wq);

	PARACHECK();

	if (mq->callback)
		return mq->callback(mq, mq->cbPar);
}

/* Initialise a message queue */
void MQ(initAll) (MQ(T) * mqt, KRN_POOL_T * hpool, uintptr_t descriptors,
		  uintptr_t available, uintptr_t used, size_t pagesize,
		  IRQ_DESC_T * shin
#ifdef MQ_HOST
		  , KRN_POOL_T * mpool, size_t msize
#endif
    ) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_T *chain;
	IRQ_DESC_T *irq;
	IRQ_IPL_T ipl;
	uint32_t size = 0;
#ifdef MQ_HOST
	uint32_t i;
	MQ_DESC_T desc = {0};
#endif

	PARACHECK();
	PARADEL(MsQu, mq);

	mq->notify = (MQ_NOTIFY_T *) MQ(notify);
	mq->callback = NULL;

	/* Count the number of items in the header pool */
	size = _LST_len(&hpool->freeList);
	/* Limit to 65534 so we can return 0xffff as an error code */
	if (size > 65534)
		size = 65534;
	/* Locate our buffers as per the vring spec */
	mq->descriptors = (MQ_DESC_T *) descriptors;
	if (available) {
		mq->available = (MQ_AVAIL_T *) available;
		mq->used = (MQ_USED_T *) used;
	} else {
		mq->available =
		    (void *)(((uintptr_t) descriptors) +
			     (size * sizeof(MQ_DESC_T)));
		mq->used =
		    (void
		     *)((((uintptr_t) & (mq->available->ring[size])) +
			 (pagesize - 1)) & ~(pagesize - 1));
	}
	/* Initialise all the internal management stuff */
	DQ_init(&mq->wq);
	mq->hpool = hpool;
#ifdef MQ_HOST
	mqt->mpool = mpool;
	mqt->msize = msize - sizeof(MQ_HOSTHEADER_T);
	for (i = 0; i < size; i++) {
		desc.next = i + 1;
		MQ_SETDESC(mq, desc, i);
	}
#endif
	mq->lastAvailable = 0;
	mq->lastUsed = 0;
	mq->freeDescs = 0;
	/* Sort out interrupts */
	mq->shin = shin;

	PARAADD(MsQu, mq);

	ipl = IRQ_raiseIPL();
	if (shin) {
		shin->priv = mq;
		shin->isrFunc = isr;
		irq = IRQ_find(shin);
		/* Initialise the interrupt chain */
		LST_init(&mq->chain);
		if (irq) {
			/* If the interrupt is in use, assume it's by another MQ, and chain */
			chain = (MQ_T *) irq->priv;
		} else {
			/* Otherwise start a chain with ourselves */
			IRQ_route(shin);
			chain = mq;
		}
		LST_add(&chain->chain, mq);
	}
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	mq->size = size;
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	IRQ_restoreIPL(ipl);
#if defined(MQ_KICK) && !defined(MQ_NOKICKSTART)
	MQ_KICK(mqt);
#endif
}

void MQ(setCallback) (MQ(T) * mqt, MQ_CALLBACK_T * cb, void *cbPar) {
	MQ_T *mq = (MQ_T *) mqt;
	PARACHECK();

	mq->callback = cb;
	mq->cbPar = cbPar;
#ifdef MQ_KICK
	MQ_KICK(mqt);
#endif
	MQ(notify) (mqt);
}

#ifdef MQ_EASY
MQ_MSG_T *MQ(take) (MQ(T) * mqt, int32_t timeout) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_MSG_T *msg = NULL;
	PARACHECK();

#ifdef MQ_HOST
	MQ_HOSTBUF_T *buf;
	/* Available buffers live in the pool */
	buf = (MQ_HOSTBUF_T *) KRN_takePool(mqt->mpool, timeout);
	if (buf) {
		PARADEL(HstB, buf);
		PARAADD(HstB, buf);
		buf->header.autoRel = 1;
		/* If there was a buffer, there must be a header available too */
		msg = (MQ_MSG_T *) KRN_takePool(mq->hpool, 0);
		PARADEL(MQMs, msg);
		/* Fill in the header so it points to the right buffer */
		msg->token = 0;
		msg->buffer = FROMPOOLABLE(buf);
		msg->length = mqt->msize;
		PARAADD(MQMs, msg);
	}
#else
	uint16_t token = 0;
	void *buffer = NULL;
	size_t length = 0;
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	/* Prepare for chosen timeout */
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	/* Try taking a buffer */
#ifdef MQ_KICK
	MQ_KICK(mqt);
#endif
	token = MQ(getAvailable) (mqt, (uintptr_t *) & buffer, &length);
	/* While it failed */
	while (token == 0xffff) {
#ifdef MQ_KICK
		MQ_KICK(mqt);
#endif
		if (timeout && _KRN_current && (!_KRN_current->timedOut)) {
			/* Go to sleep until woken by interprocessor interrupt or timeout */
			_KRN_hibernateTask(_KRN_current, &mq->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			/* Try again */
			token =
			    MQ(getAvailable) (mqt, (uintptr_t *) & buffer,
					      &length);
		} else
			break;
	}

	/* Clean up from our timeout */
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	/* If we got a message buffer, then there must be a header available */
	if (token != 0xffff) {
		buffer = MEM_p2v((uintptr_t) buffer, MEM_P2V_CACHED);
		msg = (MQ_MSG_T *) KRN_takePool(mq->hpool, 0);
		PARADEL(MQMs, msg);
		msg->token = token;
		msg->buffer = (void *)buffer;
		msg->length = length;
		PARAADD(MQMs, msg);
	} else
		msg = NULL;
#endif
	PARACHECK();
	return msg;
}

#ifdef MQ_HOST
MQ_MSG_T *MQ(takeMore) (MQ(T) * mqt, MQ_MSG_T * last, int32_t timeout) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_MSG_T *msg = NULL;
	PARACHECK();

	MQ_HOSTBUF_T *buf;
	/* Available buffers live in the pool */
	buf = (MQ_HOSTBUF_T *) KRN_takePool(mqt->mpool, timeout);
	if (buf) {
		PARADEL(HstB, buf);
		PARAADD(HstB, buf);
		buf->header.autoRel = 1;
		/* If there was a buffer, there must be a header available too */
		msg = (MQ_MSG_T *) KRN_takePool(mq->hpool, 0);
		PARADEL(MQMs, msg);
		/* Fill in the header so it points to the right buffer */
		msg->token = 0;
		msg->buffer = FROMPOOLABLE(buf);
		msg->length = mqt->msize;
		msg->next = NULL;
		msg->write = 0;
		last->next = msg;
		PARAADD(MQMs, msg);
	}

	PARADEL(MQMs, msg);
	PARAADD(MQMs, msg);
	PARACHECK();
	return msg;
}
#endif

void MQ(send) (MQ(T) * mqt, MQ_MSG_T * msg) {
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	MQ_MSG_T *walk = msg;
	while (walk) {
		walk->buffer = (void *)MEM_v2p((void *)walk->buffer);
		walk = walk->next;
	}
#ifdef MQ_HOST
	MQ(addAvailable) (mqt, msg);
#else
	MQ(addUsed) (mqt, msg->token, msg->length);
#endif
	IRQ_restoreIPL(ipl);
	KRN_returnPool(msg);
}

MQ_MSG_T *MQ(recv) (MQ(T) * mqt, int32_t timeout) {
	MQ_T *mq = (MQ_T *) mqt;
	MQ_MSG_T *msg = NULL;
	void *buffer = NULL;
	uint16_t token = 0;
	size_t length = 0;
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	uint32_t result;
#ifdef MQ_HOST
	MQ_HOSTBUF_T *pbuf = NULL;
#endif
	/* Prepare for chosen timeout */
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
#ifdef MQ_HOST
	/* Try getting a buffer */
	pbuf = (MQ_HOSTBUF_T *) KRN_takePool(mqt->mpool, 0);
	_KRN_ignoreZeroTimeout();
	/* While it failed */
	while (!pbuf) {
#ifdef MQ_KICK
		MQ_KICK(mqt);
#endif
		if (timeout && _KRN_current && (!_KRN_current->timedOut)) {
			/* Go to sleep until woken by interprocessor interrupt or timeout */
			_KRN_hibernateTask(_KRN_current, &mq->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			/* Try again */
			pbuf = (MQ_HOSTBUF_T *) KRN_takePool(mqt->mpool, 0);
			_KRN_ignoreZeroTimeout();
		} else
			break;
	}
	if (pbuf) {
		PARADEL(HstB, pbuf);
		PARAADD(HstB, pbuf);
		pbuf->header.autoRel = 0;
		MQ_MSG_T empty;
		empty.buffer = (void*)MEM_v2p(FROMPOOLABLE(pbuf));
		empty.length = mqt->msize;
		empty.write = 1;
		empty.next = NULL;
		/* Make the buffer available to the other side */
		MQ(addAvailable) (mqt, &empty);
	}
	/* Try getting a buffer back */
#ifdef MQ_KICK
	MQ_KICK(mqt);
#endif
	result = MQ(getUsed) (mqt, (uintptr_t *) & buffer, &length);
	/* While it failed */
	while ((!result) && timeout && _KRN_current
	       && (!_KRN_current->timedOut)) {
#ifdef MQ_KICK
		MQ_KICK(mqt);
#endif
		/* Go to sleep until woken by interprocessor interrupt or timeout */
		_KRN_hibernateTask(_KRN_current, &mq->wq);
		KRN_scheduleUnprotect(ipl);
		ipl = IRQ_raiseIPL();
		/* Try again */
		result = MQ(getUsed) (mqt, (uintptr_t *) & buffer, &length);
	}
#else
	/* Try getting a buffer */
	token = MQ(getAvailable) (mqt, (uintptr_t *) & buffer, &length);
	/* While it failed */
	while ((token == 0xffff) && timeout && _KRN_current
	       && (!_KRN_current->timedOut)) {
#ifdef MQ_KICK
		MQ_KICK(mqt);
#endif
		/* Go to sleep until woken by interprocessor interrupt or timeout */
		_KRN_hibernateTask(_KRN_current, &mq->wq);
		KRN_scheduleUnprotect(ipl);
		ipl = IRQ_raiseIPL();
		/* Try again */
		token = MQ(getAvailable) (mqt, (uintptr_t *) & buffer, &length);

	}
	result = (token != 0xffff);
#endif
	/* If there was a buffer, there must be a header available too */
	if (result) {
		buffer = MEM_p2v((uintptr_t)
				 buffer, MEM_P2V_CACHED);
		msg = (MQ_MSG_T *) KRN_takePool(mq->hpool, 0);
	}
	if (msg) {
		PARADEL(MQMs, msg);
		/* Fill in the header so it points to the right buffer */
		msg->token = token;
		msg->buffer = buffer;
		msg->length = length;
		msg->next = NULL;
		msg->write = 0;
		PARAADD(MQMs, msg);
	}
	/* Clean up from our timeout */
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	/* Return either the message or NULL on timeout */
	return msg;
}

void MQ(return) (MQ(T) * mqt, MQ_MSG_T * msg) {
#ifdef MQ_HOST
	MQ_HOSTBUF_T *pbuf = TOPOOLABLE(msg->buffer);
	KRN_returnPool(msg);
	KRN_returnPool(pbuf);
#else
	IRQ_IPL_T ipl = IRQ_raiseIPL();
	MQ(addUsed) (mqt, msg->token, msg->length);
	IRQ_restoreIPL(ipl);
	KRN_returnPool(msg);
#endif
}
#endif
