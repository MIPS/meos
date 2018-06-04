/***(C)2016***************************************************************
*
* Copyright (C) 2016 MIPS Tech, LLC
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
****(C)2016**************************************************************/

/*************************************************************************
*
*   Description:	virtio network driver
*
*************************************************************************/

#include "MEOS.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/etharp.h"

static err_t VIRTIONET_txFrame(struct netif *netif, struct pbuf *p)
{
	VIRTIONET_T *net = (VIRTIONET_T *) netif->state;
	struct pbuf *q;
	uint8_t *pdata, *mdata;

	MQ_MSG_T *first, *msg;
	size_t len, hlen, mlen, plen;

	if ((msg = first = MQDEVICE_take(&net->wq, KRN_INFWAIT)) == NULL)
		return ERR_MEM;
	mdata = MQ_MSG_data(msg);
	mlen = MQ_MSG_length(msg);

	/* Form header */
	hlen = sizeof(VIRTIO_NET_HDR_T);
	while (hlen) {
		len = hlen < mlen ? hlen : mlen;
		memset(mdata, 0, len);
		hlen -= len;
		mlen -= len;
		mdata += len;
		if (hlen)
			msg = MQDEVICE_takeMore(&net->wq, msg, KRN_INFWAIT);
	}

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE);
#endif

	/* Form packet */
	for (q = p; q != NULL; q = q->next) {
		plen = q->len;
		pdata = q->payload;
		while (plen) {
			len = plen < mlen ? plen : mlen;
			memcpy(mdata, pdata, len);
			plen -= len;
			mlen -= len;
			mdata += len;
			if (plen) {
				msg =
				    MQDEVICE_takeMore(&net->wq, msg,
						      KRN_INFWAIT);
				mdata = MQ_MSG_data(msg);
				mlen = MQ_MSG_length(msg);
			}
		}
	}

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE);
#endif

	/* Send it */
	msg->length = p->tot_len + sizeof(VIRTIO_NET_HDR_T);
	MQDEVICE_send(&net->wq, first);
	LINK_STATS_INC(link.xmit);
	return ERR_OK;
}

static struct pbuf *VIRTIONET_rxFrame(struct netif *netif)
{
	VIRTIONET_T *net = (VIRTIONET_T *) netif->state;
	struct pbuf *p, *q;
	uint16_t len;
	uint8_t *data;
	uint32_t i;
	MQ_MSG_T *msg;
	VIRTIO_NET_HDR_T *hdr;
	if ((msg = MQDEVICE_recv(&net->rq, 0)) == NULL)
		return NULL;
	len = MQ_MSG_length(msg) - sizeof(VIRTIO_NET_HDR_T);
	hdr = (VIRTIO_NET_HDR_T *) MQ_MSG_data(msg);
	data = (uint8_t *) & hdr[1];
#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE;
#endif
	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
	if (p != NULL) {

#if ETH_PAD_SIZE
		pbuf_header(p, -ETH_PAD_SIZE);
#endif
		for (q = p; q != NULL; q = q->next) {
			i = len < q->len ? len : q->len;
			memcpy(q->payload, data, i);
			data += i;
		}

#if ETH_PAD_SIZE
		pbuf_header(p, ETH_PAD_SIZE);
#endif
		LINK_STATS_INC(link.recv);
	} else {
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
	}
	MQDEVICE_return(&net->rq, msg);
	return p;
}

static void VIRTIONET_input(struct netif *netif)
{
	struct pbuf *p;
	struct eth_hdr *ethhdr;
	for (;;) {
		p = VIRTIONET_rxFrame(netif);
		if (p == NULL)
			return;
		ethhdr = (struct eth_hdr *)p->payload;
		switch (htons(ethhdr->type)) {
		case ETHTYPE_ARP:
		case ETHTYPE_IP:
#if PPPOE_SUPPORT
		case ETHTYPE_PPPOE:
		case ETHTYPE_PPPOEDISC:

#endif				/* PPPOE_SUPPORT */
			if (netif->input(p, netif) == ERR_OK)
				continue;
			LWIP_DEBUGF(NETIF_DEBUG,
				    ("VIRTIONET_input: packet input error\n"));
		default:
			break;
		}
		pbuf_free(p);
	}
}

err_t VIRTIONET_init(struct netif *netif)
{
	VIRTIONET_T *net = (VIRTIONET_T *) netif->state;
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	void *vAddr = MEM_p2v(net->pAddr, MEM_P2V_UNCACHED);
#if LWIP_NETIF_HOSTNAME
	netif->hostname = "lwip";
#endif				/* LWIP_NETIF_HOSTNAME */
	/* Brazenly assume 100baseT */
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100 * 1024 * 1024);
	netif->name[0] = 'e';
	netif->name[1] = 'n';
	netif->output = etharp_output;
	netif->linkoutput = VIRTIONET_txFrame;
	netif->mtu = 1500;
	netif->flags =
	    NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	/* Read MAC */
	netif->hwaddr[0] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 0);
	netif->hwaddr[1] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 1);
	netif->hwaddr[2] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 2);
	netif->hwaddr[3] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 3);
	netif->hwaddr[4] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 4);
	netif->hwaddr[5] = MEM_rb(vAddr + VIRTIO_REG_NET_MAC + 5);
	return ERR_OK;
}

static void VIRTIONET_isr(int32_t sigNum)
{
	IRQ_DESC_T *desc = IRQ_cause(sigNum);
	VIRTIONET_T *net = (VIRTIONET_T *) desc->priv;
	VIRTIONET_input(&net->netif);
	MQDEVICE_notify(&net->wq);
	IRQ_ack(desc);
	MEM_leww(net->ack, VIRTIO_INT_USED_RING_UPDATE | VIRTIO_INT_CONFIGURATION_CHANGE);
}

void VIRTIONET_dtInit(VIRTIONET_T * net, IRQ_DESC_T * irq)
{
	ip_addr_t zero;
	void *vAddr = MEM_p2v(net->pAddr, MEM_P2V_UNCACHED);
	net->ack = vAddr + VIRTIO_REG_INTERRUPTACK;
	/* Verify */
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_MAGICVALUE) == 0x74726976UL,
		   "Virtio magic mismatch! %08" PRIx32 "@%p != %08"
		   PRIx32 "\n", MEM_lerw(vAddr + VIRTIO_REG_MAGICVALUE),
		   vAddr + VIRTIO_REG_MAGICVALUE, 0x74726976UL);
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_VERSION) == 2,
		   "Virtio version mismatch!\n");
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_DEVICEID) == VIRTIO_DEV_NET,
		   "Not a Virtio net!\n");
	/* Reset */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, 0);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Set ACKNOWLEDGE (1) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Set DRIVER (2) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Deal with feature bits */
	/* FIXME: require MAC feature */
	MEM_leww(vAddr + VIRTIO_REG_DRIVERFEATURESSEL, 0);
	MEM_leww(vAddr + VIRTIO_REG_DRIVERFEATURES, 0);	/* We don't use any special features */
	/* Set FEATURES_OK (8) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS,
		 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Check FEATURES_OK (8) still set */
	DBG_assert((MEM_lerw(vAddr + VIRTIO_REG_STATUS) & VIRTIO_STATUS_FEATURES_OK) ==
		   VIRTIO_STATUS_FEATURES_OK,
		   "Failed to negotiated Virtio net device! STATUS=%08"
		   PRIx32 ".\n", MEM_lerw(vAddr + VIRTIO_REG_STATUS));
	/* Init MQs */
	KRN_initPool(&net->rhpool, &net->rheaders, 64, sizeof(MQ_MSG_T));
	KRN_initPool(&net->rmpool, &net->rmsgs, 64, sizeof(VIRTIONETMSG_T));
	KRN_initPool(&net->whpool, &net->wheaders, 64, sizeof(MQ_MSG_T));
	KRN_initPool(&net->wmpool, &net->wmsgs, 64, sizeof(VIRTIONETMSG_T));
	MQDEVICE_init(&net->rq, &net->rhpool, &net->rring, 4096,
		      NULL, (uint32_t *) vAddr, 0, &net->rmpool,
		      sizeof(VIRTIONETMSG_T));
	MQDEVICE_init(&net->wq, &net->whpool, &net->wring, 4096, NULL,
		      (uint32_t *) vAddr, 1, &net->wmpool,
		      sizeof(VIRTIONETMSG_T));
	/* Init driver */
	IP4_ADDR(&zero, 0, 0, 0, 0);
	netif_add(&net->netif, &zero,
		  &zero, &zero, (void *)net, VIRTIONET_init, tcpip_input);
	irq->isrFunc = VIRTIONET_isr;
	irq->priv = (void *)net;
	IRQ_route(irq);
	/* Set MQs */
	MEM_leww(vAddr + VIRTIO_REG_QUEUESEL, 0);
	MEM_leww(vAddr + VIRTIO_REG_QUEUEREADY, 1);
	MEM_leww(vAddr + VIRTIO_REG_QUEUESEL, 1);
	MEM_leww(vAddr + VIRTIO_REG_QUEUEREADY, 1);
	/* Set DRIVER_OK (4) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS,
		 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK |
		 VIRTIO_STATUS_DRIVER_OK);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
}
