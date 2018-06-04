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
*   Description:    Virtio net
*
*************************************************************************/

#include <stdio.h>
#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/mem/mem.h"
#include "meos/inttypes.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"

int VIO_readNet(void *address, void *buffer, int size, int n, void *priv)
{
	VIO_NET_T *net = (VIO_NET_T *) priv;
	uintptr_t offset = (uintptr_t) address;
	size *= n;
	if ((offset >= VIRTIO_REG_NET_MAC)
	    && (offset < VIRTIO_REG_NET_STATUS + 2)
	    && (size <= ((VIRTIO_REG_NET_STATUS + 2) - offset))) {
		memcpy(buffer, net->mac + (offset - VIRTIO_REG_NET_MAC), size);
	} else {
		DBG_assert(0,
			   "Guest '%s' attempted read from unsupported Virtio net register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}
	return size;
}

int VIO_writeNet(void *address, void *buffer, int size, int n, void *priv)
{
	uintptr_t offset = (uintptr_t) offset;
	DBG_assert(0,
		   "Guest '%s' attempted write to unsupported Virtio net register %03"
		   PRIx32 "\n", KRN_taskName(NULL), offset);
	MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	return 0;
}

void VIO_txNet(MQ_T * mq, void *cbPar)
{
	VIO_NET_T *net = (VIO_NET_T *) cbPar;
	VIO_Q_T *vq = &net->queues[1];
	MVZ_GUEST_T *gt = vq->guest;
	struct netif *netif = (struct netif *)net->netif;
	uint16_t start, token, flags;

	size_t fsize, size, qlen, total = 0;
	uintptr_t faddr, addr;
	struct pbuf *p, *q;
	uint8_t *qaddr;

	/* While there are packets to transmit */
	for (;;) {
		/* Get the start of the packet */
		start = token =
		    MQDRIVER_getFirstAvailable(&vq->mq, &faddr, &fsize, &flags);
		if (token == 0xffff)
			break;
		size = fsize;
		/* Get the size of the packet */
		while (token != 0xffff) {
			total += size;
			token =
			    MQDRIVER_getMoreAvailable(&vq->mq, token, &addr,
						      &size, &flags);
		}
		token = start;
		/* Allocate pbufs for packet */
		p = q =
		    pbuf_alloc(PBUF_RAW, total - sizeof(VIRTIO_NET_HDR_T),
			       PBUF_POOL);
		/* Transfer from guest descriptors to pbuf chain */
		size = fsize;
		addr = faddr;
		/* Skip header */
		qlen = sizeof(VIRTIO_NET_HDR_T);
		for (;;) {
			if (qlen > size) {
				qlen -= size;
				token =
				    MQDRIVER_getMoreAvailable(&vq->mq, token,
							      &addr, &size,
							      &flags);
			} else {
				addr += qlen;
				size -= qlen;
				break;
			}
		}
		/* Transfer data */
		qaddr = (uint8_t *) q->payload;
		qlen = q->len;
		for (;;) {
			if (size > qlen) {
				/** Guest descriptor is bigger **/
				/* Copy data */
				MVZ_readGPC((void *)addr, qaddr, qlen, 1, gt);
				/* Step pbuf */
				addr += qlen;
				size -= qlen;
				q = q->next;
				if (!q)
					break;
				qaddr = (uint8_t *) q->payload;
				qlen = q->len;
			} else {
				/** Guest descriptor is smaller **/
				/* Copy data */
				MVZ_readGPC((void *)addr, qaddr, size, 1, gt);
				/* Step guest descriptor */
				qaddr += size;
				qlen -= size;
				token =
				    MQDRIVER_getMoreAvailable(&vq->mq, token,
							      &addr, &size,
							      &flags);
				if (token == 0xffff)
					break;
			}
		}
		/* Transmit the packet */
		netif->linkoutput(netif, p);
		/* Delete the pbuf */
		pbuf_free(p);
		/* Step descriptor chains */
		MQDRIVER_getNextAvailable(&vq->mq);
		MQDRIVER_addUsed(&vq->mq, start, total);
		VIO_synthInt(&net->vio, VIRTIO_INT_USED_RING_UPDATE);
		total = 0;
	}
}

int VIO_rxNet(VIO_NET_T * net, struct pbuf *p)
{
	struct pbuf *q;
	VIO_Q_T *vq = &net->queues[0];
	MVZ_GUEST_T *gt = vq->guest;
	uint16_t start, token, flags;
	VIRTIO_NET_HDR_T header = { 0, 0, 0, 0, 0, 0, 1 };
	size_t total = p->tot_len, size, qlen;
	uintptr_t addr;
	uint8_t *qaddr;

	/* Try and get a virtio descriptor */
	start = token =
	    MQDRIVER_getFirstAvailable(&vq->mq, &addr, &size, &flags);
	if (token == 0xffff)
		return 0;
	/* Write header */
	qaddr = (uint8_t *) & header;
	qlen = sizeof(header);
	for (;;) {
		if (size > qlen) {
		/** Guest descriptor is bigger **/
			/* Copy data */
			MVZ_writeGPC((void *)addr, qaddr, qlen, 1, gt);
			/* Step pbuf */
			addr += qlen;
			size -= qlen;
			break;
		} else {
		/** Guest descriptor is smaller **/
			/* Copy data */
			MVZ_writeGPC((void *)addr, qaddr, size, 1, gt);
			/* Step guest descriptor */
			qaddr += size;
			qlen -= size;
			token =
			    MQDRIVER_getMoreAvailable(&vq->mq, token,
						      &addr, &size, &flags);
			if (token == 0xffff) {
				VIO_synthInt(&net->vio,
					     VIRTIO_INT_USED_RING_UPDATE);
				DBG_logF("Dropped due to resources\n");
				return 0;
			}
		}
	}
	/* Coalesce the packet into the guest buffer */
	q = p;
	qaddr = (uint8_t *) p->payload;
	qlen = p->len;
	for (;;) {
		if (size > qlen) {
		/** Guest descriptor is bigger **/
			/* Copy data */
			MVZ_writeGPC((void *)addr, qaddr, qlen, 1, gt);
			/* Step pbuf */
			addr += qlen;
			size -= qlen;
			q = q->next;
			if (!q)
				break;
			qaddr = (uint8_t *) q->payload;
			qlen = q->len;
		} else {
		/** Guest descriptor is smaller **/
			/* Copy data */
			MVZ_writeGPC((void *)addr, qaddr, size, 1, gt);
			/* Step guest descriptor */
			qaddr += size;
			qlen -= size;
			token =
			    MQDRIVER_getMoreAvailable(&vq->mq, token,
						      &addr, &size, &flags);
			if (token == 0xffff) {
				VIO_synthInt(&net->vio,
					     VIRTIO_INT_USED_RING_UPDATE);
				DBG_logF("Dropped due to resources\n");
				return 0;
			}
		}
	}
	/* Inject the packet */
	MQDRIVER_getNextAvailable(&vq->mq);
	MQDRIVER_addUsed(&vq->mq, start, total + sizeof(header));
	VIO_synthInt(&net->vio, VIRTIO_INT_USED_RING_UPDATE);
	return total;
}

/* Override the LWIP filter to intercept incoming frames */
struct netif *LWIP_filter(struct pbuf *p, struct netif *netif, u16_t type)
{
	struct eth_hdr *ethhdr;
	uint8_t *nmac, *pmac;
	VIO_NET_T *net;

	(void)type;
	(void)nmac;
	(void)pmac;

	if (!_MVZ || p->len <= SIZEOF_ETH_HDR)
		goto escape;

	ethhdr = (struct eth_hdr *)p->payload;
	pmac = ethhdr->dest.addr;

	/* Walk all VIO_net devices */
	net = LST_first(&_MVZ->nets);
	while (net) {
		nmac = net->mac;
		/* If bound to this netif */
		if (net->netif == netif) {
#ifdef CONFIG_MVZ_CLEAN_NET
			/* Skip if not for this VIO_net */
			if (!
			    ((pmac[0] == pmac[1] == pmac[2] ==
			      pmac[3] == pmac[4] == pmac[5] == 0xff)
			     || ((pmac[0] == nmac[0])
				 && (pmac[1] == nmac[1])
				 && (pmac[2] == nmac[2])
				 && (pmac[3] == nmac[3])
				 && (pmac[4] == nmac[4])
				 && (pmac[5] == nmac[5]))))
				continue;
#else
			/* Allow */
#endif
			VIO_rxNet(net, p);
		}
		net = LST_next(net);
	}

      escape:
	return netif;
}

void VIO_budNet(VIO_T * vio, reg_t down)
{
	MVZ_GUEST_T *gt = vio->regs.guest;
	if (down) {
		/* Don't remove the link state callback, that will break any
		 * other users of the same interface.
		 */
		LST_remove(&_MVZ->nets, vio);
	} else {
		/* 0: rxq */
		vio->queues[0].guest = gt;
		KRN_initPool(&vio->queues[0].hPool,
			     vio->queues[0].headers,
			     VIO_QUEUE_NUM_MAX,
			     sizeof(vio->queues[0].headers[0]));
		MQDRIVER_init(&vio->queues[0].mq, &vio->queues[0].hPool,
			      vio->queues[0].desc, vio->queues[0].avail,
			      vio->queues[0].used, 4096, gt, vio);
		/* 1: txq */
		vio->queues[1].guest = gt;
		KRN_initPool(&vio->queues[1].hPool,
			     vio->queues[1].headers,
			     VIO_QUEUE_NUM_MAX,
			     sizeof(vio->queues[1].headers[0]));
		MQDRIVER_init(&vio->queues[1].mq, &vio->queues[1].hPool,
			      vio->queues[1].desc, vio->queues[1].avail,
			      vio->queues[1].used, 4096, gt, vio);
		MQDRIVER_setCallback(&vio->queues[1].mq, VIO_txNet, vio);
		/* Add to list of nets */
		LST_remove(&_MVZ->nets, vio);
		LST_add(&_MVZ->nets, vio);
	}
}

void VIO_linkChange(struct netif *netif)
{
	VIO_NET_T *net;
	/* Walk all VIO_net devices */
	net = LST_first(&_MVZ->nets);
	while (net) {
		/* If bound to this netif */
		if (net->netif == netif) {
			net->status =
			    (netif->flags & NETIF_FLAG_LINK_UP) ? 1 : 0;
			VIO_reconfigure(&net->vio);
		}
		net = LST_next(net);
	}
}

void VIO_initNet(VIO_NET_T * net, uintptr_t base, uint32_t gInt,
		 struct netif *netif)
{
	VIO_init(&net->vio, VIRTIO_DEV_NET, VIRTIO_DEV_MIPS);
	memset(net->queues, 0, sizeof(net->queues));
	net->vio.queues = net->queues;
	net->vio.nQueues = 2;
	net->vio.queues[0].numMax = VIO_QUEUE_NUM_MAX;
	net->vio.queues[1].numMax = VIO_QUEUE_NUM_MAX;
	net->vio.deviceFeatures[0] =
	    VIRTIO_FLAG_NET_MAC | VIRTIO_FLAG_NET_STATUS;
	net->mac[0] = netif->hwaddr[0];
	net->mac[1] = netif->hwaddr[1];
	net->mac[2] = netif->hwaddr[2];
	net->mac[3] = netif->hwaddr[3];
	net->mac[4] = netif->hwaddr[4];
	net->mac[5] = netif->hwaddr[5];
	net->status = (netif->flags & NETIF_FLAG_LINK_UP) ? 1 : 0;
	netif_set_link_callback(netif, &VIO_linkChange);
	net->vio.read = VIO_readNet;
	net->vio.write = VIO_writeNet;
	net->vio.bringUpDown = VIO_budNet;
	net->vio.gInt = gInt;
	net->vio.regs.start = base;
	net->vio.regs.stop = base + 0x107;
	net->netif = netif;
}

void VIO_setMAC(VIO_NET_T * net, uint8_t m1, uint8_t m2, uint8_t m3,
		uint8_t m4, uint8_t m5, uint8_t m6)
{
	net->mac[0] = m1;
	net->mac[1] = m2;
	net->mac[2] = m3;
	net->mac[3] = m4;
	net->mac[4] = m5;
	net->mac[5] = m6;
}

void VIO_linkStatus(VIO_NET_T * net, int32_t up)
{
	net->status = up ? 1 : 0;
}
