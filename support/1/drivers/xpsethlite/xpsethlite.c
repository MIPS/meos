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
*   Description:	XPS ETH Lite driver
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

#define ETH_TX_DATA	(0x0000 / sizeof(uint32_t))
#define ETH_RX_DATA_PING (0x1000 / sizeof(uint32_t))
#define ETH_RX_DATA_PONG (0x1800 / sizeof(uint32_t))
#define MDIOADDR	(0x07E4 / sizeof(uint32_t))
#define MDIOWR		(0x07E8 / sizeof(uint32_t))
#define MDIORD		(0x07EC / sizeof(uint32_t))
#define MDIOCTRL	(0x07F0 / sizeof(uint32_t))
#define TX_PING_LEN	(0x07F4 / sizeof(uint32_t))
#define	GIE		(0x07F8 / sizeof(uint32_t))
#define TX_PING_CNTL	(0x07FC / sizeof(uint32_t))
#define TX_PONG_LEN	(0x0FF4 / sizeof(uint32_t))
#define TX_PONG_CNTL	(0x0FFC / sizeof(uint32_t))
#define RX_PING_CNTL	(0x17FC / sizeof(uint32_t))
#define RX_PONG_CNTL	(0x1FFC / sizeof(uint32_t))

#define GIE_ENABLE	0x80000000
#define RX_INT_ENABLE	0x00000008
#define TX_ENABLE	0x00000001
#define TX_BUSY		0x00000001
#define RX_READY	0x00000001
#define MAC_SET		0x00000003
#define MDIOADDR_READ	0x00000400
#define MDIOADDR_WRITE	0x00000000
#define MDIOCTRL_E	0x00000008
#define MDIOCTRL_S	0x00000001

#define MII_BCR                 0
#define  BCR_RESET              0x8000
#define  BCR_AUTON              0x1000
#define  BCR_RSTNEG             0x0100
#define MII_BSR		1
#define  BSR_LS		0x0004
#define MII_ANAR	4

/* MII access */

static uint32_t MIIR(XPSETHLITE_T * xpsethlite, uint32_t miir)
{
	volatile uint32_t *eth = (uint32_t *) xpsethlite->regs;

	/* Wait until ready */
	while (eth[MDIOCTRL] & MDIOCTRL_S) ;
	/* Set up direction/device id/address */
	eth[MDIOADDR] = (xpsethlite->phy << 5) | miir | MDIOADDR_READ;
	/* Initiate transfer */
	eth[MDIOCTRL] |= MDIOCTRL_E;
	eth[MDIOCTRL] |= MDIOCTRL_S;
	/* Wait until complete */
	while (eth[MDIOCTRL] & MDIOCTRL_S) ;
	/* Return result */
	return eth[MDIORD];

}

static void MIIW(XPSETHLITE_T * xpsethlite, uint32_t miir, uint32_t v)
{
	volatile uint32_t *eth = (uint32_t *) xpsethlite->regs;

	/* Wait until ready */
	while (eth[MDIOCTRL] & MDIOCTRL_S) ;
	/* Set up direction/device id/address */
	eth[MDIOADDR] = (xpsethlite->phy << 5) | miir | MDIOADDR_WRITE;
	/* Set up value */
	eth[MDIOWR] = v;
	/* Initiate transfer */
	eth[MDIOCTRL] |= MDIOCTRL_E;
	eth[MDIOCTRL] |= MDIOCTRL_S;
	/* Wait until complete */
	while (eth[MDIOCTRL] & MDIOCTRL_S) ;
}

static err_t XPSETHLITE_txFrame(struct netif *netif, struct pbuf *p)
{
	XPSETHLITE_T *xpsethlite = netif->state;
	struct pbuf *q;
	uint8_t *dst;
	volatile uint32_t *eth = (uint32_t *) xpsethlite->regs;

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE);
#endif
	/* Wait to become ready */
	while (eth[TX_PING_CNTL] & TX_BUSY) ;
	/* Set up packet */
	dst = (uint8_t *) & eth[ETH_TX_DATA];
	for (q = p; q != NULL; q = q->next) {
		memcpy(dst, q->payload, q->len);
		dst += q->len;
	}
	eth[TX_PING_LEN] = p->tot_len;
	/* TX */
	eth[TX_PING_CNTL] |= TX_ENABLE;

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE);
#endif

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

static struct pbuf *XPSETHLITE_rxFrame(struct netif *netif, reg_t offset)
{
	XPSETHLITE_T *xpsethlite = netif->state;
	struct pbuf *p, *q;
	uint16_t len = 0;
	uint8_t *ptr;
	uint32_t volatile *eth = (uint32_t *) xpsethlite->regs;
	uint8_t volatile *lenptr = (uint8_t *) & eth[offset];

	/* Read length from hardware */
	if (eth[offset + (RX_PING_CNTL - ETH_RX_DATA_PING)] & RX_READY) {
		len = ((lenptr[12] << 8) + lenptr[13]) & 0xFFFF;
		if (len > 0x600)
			len = 0x500;
	}
	if (len == 0)
		return NULL;

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE;
#endif

	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

	if (p != NULL) {

#if ETH_PAD_SIZE
		pbuf_header(p, -ETH_PAD_SIZE);
#endif
		ptr = (uint8_t *) & eth[offset];
		for (q = p; q != NULL; q = q->next) {
			memcpy(q->payload, ptr, q->len);
			ptr += q->len;
		}
		/* Notify packet read, allow next read */
		eth[offset + (RX_PING_CNTL - ETH_RX_DATA_PING)] &= ~RX_READY;

#if ETH_PAD_SIZE
		pbuf_header(p, ETH_PAD_SIZE);
#endif

		LINK_STATS_INC(link.recv);
	} else {
		/* Drop packet */
		eth[offset + (RX_PING_CNTL - ETH_RX_DATA_PING)] &= ~RX_READY;
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
	}

	return p;
}

static void XPSETHLITE_input(struct netif *netif)
{
	XPSETHLITE_T *xpsethlite = netif->state;
	uint32_t volatile *eth = (uint32_t *) xpsethlite->regs;
	struct pbuf *p;
	struct eth_hdr *ethhdr;

	for (;;) {
		if (!xpsethlite->pong) {
			if ((eth[RX_PING_CNTL] & RX_READY) != RX_READY)
				xpsethlite->pong = !xpsethlite->pong;
			if ((eth[RX_PONG_CNTL] & RX_READY) != RX_READY)
				return;
		} else {
			if ((eth[RX_PONG_CNTL] & RX_READY) != RX_READY)
				xpsethlite->pong = !xpsethlite->pong;
			if ((eth[RX_PING_CNTL] & RX_READY) != RX_READY)
				return;
		}

		p = XPSETHLITE_rxFrame(netif,
				       xpsethlite->pong ? ETH_RX_DATA_PING :
				       ETH_RX_DATA_PONG);

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
				return;
			LWIP_DEBUGF(NETIF_DEBUG,
				    ("XPSETHLITE_input: packet input error\n"));
		default:
			break;
		}
		pbuf_free(p);
	}
}

err_t XPSETHLITE_init(struct netif *netif)
{
	XPSETHLITE_T *xpsethlite = (XPSETHLITE_T *) netif->state;
	uint32_t volatile *eth;
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	eth = xpsethlite->regs = MEM_p2v(xpsethlite->pAddr, MEM_P2V_UNCACHED);
#if LWIP_NETIF_HOSTNAME
	netif->hostname = "lwip";
#endif				/* LWIP_NETIF_HOSTNAME */
	/* Brazenly assume 100baseT */
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100 * 1024 * 1024);
	netif->name[0] = 'e';
	netif->name[1] = 'n';
	netif->output = etharp_output;
	netif->linkoutput = XPSETHLITE_txFrame;
	netif->mtu = 1500;
	netif->flags =
	    NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	/* Reset device */
	eth[TX_PING_CNTL] = 0;
	eth[TX_PONG_CNTL] = 0;
	xpsethlite->pong = 0;
	eth[RX_PING_CNTL] = 0;
	eth[RX_PONG_CNTL] = 0;
	/* Set MAC addresses */
	memcpy(netif->hwaddr, xpsethlite->mac, 6);
	memcpy((void *)&eth[ETH_TX_DATA], xpsethlite->mac, 6);
	eth[TX_PING_LEN] = 6;
	eth[TX_PING_CNTL] |= MAC_SET;
	while ((eth[TX_PING_CNTL] & (MAC_SET | TX_BUSY)) ==
	       (MAC_SET | TX_BUSY)) ;
	/* Interrupt configuration */
	eth[RX_PING_CNTL] &= ~RX_READY;
	eth[RX_PING_CNTL] = RX_INT_ENABLE;
	/* Enable */
	eth[GIE] |= GIE_ENABLE;
	/* Reset PHY */
	MIIW(xpsethlite, MII_BCR, BCR_RESET);
	KRN_delay(1000);
	MIIW(xpsethlite, MII_ANAR, 0x01e1);
	MIIW(xpsethlite, MII_BCR, BCR_AUTON | BCR_RSTNEG);

	return ERR_OK;
}

void XPSETHLITE_phyPoll(KRN_TIMER_T * timer, void *timerPar)
{
	XPSETHLITE_T *xpsethlite = (XPSETHLITE_T *) timerPar;
	uint32_t miistatus;
	/* Check link */
	MIIR(xpsethlite, MII_BSR);	/* Clear sticky */
	miistatus = MIIR(xpsethlite, MII_BSR);
	if (miistatus & BSR_LS)
		netif_set_link_up(&xpsethlite->netif);
	else
		netif_set_link_down(&xpsethlite->netif);
	/* Rearm */
	KRN_setSoftTimer(timer, XPSETHLITE_phyPoll, timerPar, 1000000, 1000);
}

static void XPSETHLITE_isr(int32_t sigNum)
{
	IRQ_DESC_T *cause = IRQ_ack(IRQ_cause(sigNum));
	XPSETHLITE_T *xpsethlite = (XPSETHLITE_T *) cause->priv;
	while (((xpsethlite->regs[RX_PONG_CNTL]) & RX_READY) == RX_READY)
		XPSETHLITE_input(&xpsethlite->netif);
}

void XPSETHLITE_dtInit(XPSETHLITE_T * xpsethlite, IRQ_DESC_T * irq)
{
	irq->isrFunc = XPSETHLITE_isr;
	irq->priv = (void *)xpsethlite;
	ip_addr_t zero;
	IP4_ADDR(&zero, 0, 0, 0, 0);
	netif_add(&xpsethlite->netif, &zero, &zero, &zero,
		  (void *)xpsethlite, XPSETHLITE_init, tcpip_input);
	IRQ_route(irq);
	KRN_setSoftTimer(&xpsethlite->phyTimer, XPSETHLITE_phyPoll,
			 xpsethlite, 1000000, 1000);
}
