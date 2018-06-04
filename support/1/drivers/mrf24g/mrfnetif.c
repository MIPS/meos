/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
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
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	MRF24G driver
*
*************************************************************************/

#include "meos/mrf24g/mrfnetif.h"

#include "meos/mrf24g/mrf.h"
#include "mrf_raw.h"
#include "lwip/tcpip.h"

#include <string.h>
#include <assert.h>

#define MTU (1500)

#define IFNAME0 'w'
#define IFNAME1 'i'

#define SNAP_HDR_LENGTH (6)
#define SNAP_VAL        (0xaa)
#define SNAP_CTRL_VAL   (0x03)
#define SNAP_TYPE_VAL   (0x00)

static KRN_SEMAPHORE_T MRF_sem;

static void low_level_init(struct netif *netif, uint8_t hwaddr[6])
{
	/* set MAC hardware address length */
	netif->hwaddr_len = 6;

	/* set MAC hardware address */
	memmove(netif->hwaddr, hwaddr, 6);

	/* maximum transfer unit */
	netif->mtu = MTU;

	/* device capabilities */
	netif->flags =
	    NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q = p;

	if (KRN_testSemaphore(&MRF_sem, 1, 5000) == 0) {
		return ERR_OK;
	}

	if (MRF_raw_alloc_tx_buf(p->tot_len) < 0) {
		KRN_setSemaphore(&MRF_sem, 1);
		return ERR_IF;
	}
	/* preamble */
	uint8_t preamble[] =
	    { MRF_DATA_REQUEST_TYPE, MRF_STD_DATA_MSG_SUBTYPE, 1, 0 };

	uint16_t write_index = 0;

	MRF_tx_write(preamble, write_index, sizeof(preamble));
	write_index += 4;

	/* dest addr */
	MRF_tx_write(p->payload, write_index, 6);
	write_index += 6;

	/* replace source addr */
	uint8_t sar[] = { SNAP_VAL, SNAP_VAL, SNAP_CTRL_VAL, SNAP_TYPE_VAL,
		SNAP_TYPE_VAL, SNAP_TYPE_VAL
	};
	MRF_tx_write(sar, write_index, 6);
	write_index += 6;

	uint16_t packet_len = p->tot_len + 4;	/* 4 is the size of the preamble */
	uint16_t write_len =
	    ((4 + q->len) <
	     packet_len) ? q->len - 12 : packet_len - write_index;
	MRF_tx_write(q->payload + 12, write_index, write_len);
	write_index += q->len - 12;

	for (q = p->next; q != NULL; q = q->next) {
		write_len = (write_index + q->len <
			     packet_len) ? packet_len - write_index : q->len;
		MRF_tx_write(q->payload, write_index, write_len);

		write_index += q->len;
	}

	MRF_raw_send_data_frame(packet_len);
	LINK_STATS_INC(link.xmit);

	KRN_setSemaphore(&MRF_sem, 1);

	return ERR_OK;
}

static void MRFNETIF_input(struct netif *netif)
{
	struct eth_hdr *ethhdr;
	struct pbuf *p, *q;

	uint16_t size = MRF_raw_move_dst(MRF_RAW_DATA_RX_ID, MRF_RAW_MAC, 0);
	if (size <= 0) {
		return;
	}

	size -= 16;
	p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

	if (p != NULL) {
		uint16_t write_index = 16;
		for (q = p; q != NULL; q = q->next) {
			uint16_t toread =
			    (write_index + q->len <
			     size) ? size - write_index : q->len;
			MRF_rx_read(q->payload, write_index, toread);
			write_index += q->len;
		}
		LINK_STATS_INC(link.recv);

		/* points to packet payload, which starts with an Ethernet header */
		ethhdr = p->payload;
		/* TODO: check mac for own broadcast and throw it away */
		switch (htons(ethhdr->type)) {
			/* IP or ARP packet? */
		case ETHTYPE_IP:
			/* FALLS THROUGH */
		case ETHTYPE_ARP:
			/* full packet send to tcpip_thread to process */
			if (netif->input(p, netif) != ERR_OK) {
				LWIP_DEBUGF(NETIF_DEBUG,
					    ("MRFNETIF_input: IP input error\n"));
			}
			break;
		default:
			break;
		}
		pbuf_free(p);
		p = NULL;
	} else {
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
	}
}

int MRFNETIF_idle(void)
{
	int ret = 0;
	if (KRN_testSemaphore(&MRF_sem, 1, 0) != 0) {
		ret = MRF_idle();
		KRN_setSemaphore(&MRF_sem, 1);
	}
	return ret;
}

err_t MRFNETIF_init(struct netif * netif)
{
	uint8_t hwaddr[6];

	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;

	KRN_initSemaphore(&MRF_sem, 1);

	if (MRF_init((void (*)(void *arg))
		     &MRFNETIF_input, netif) < 0)
		return ERR_IF;

	if (MRF_macGet(hwaddr))
		return ERR_IF;

	low_level_init(netif, hwaddr);

	return ERR_OK;
}

void MRFNETIF_dtInit(MRFNETIF_T * mrfnetif, IRQ_DESC_T * irq)
{
	/*irq->isrFunc = MRFNETIF_isr; *//* Disabled for this version */
	irq->priv = (void *)mrfnetif;
	ip_addr_t zero;
	IP4_ADDR(&zero, 0, 0, 0, 0);
	netif_add(&mrfnetif->netif, &zero,
		  &zero, &zero, (void *)mrfnetif, MRFNETIF_init, tcpip_input);
	/*IRQ_route(irq); *//* Disabled for this version */
}
