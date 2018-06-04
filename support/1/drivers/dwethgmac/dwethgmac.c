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
*   Description:	DesignWare Ethernet GMAC LWIP driver
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

#define TDES0_OWN			0x80000000
#define ETDES0_SECOND_ADDRESS_CHAINED	0x00100000
#define ETDES0_FIRST_SEGMENT		0x10000000
#define ETDES0_LAST_SEGMENT		0x20000000

#define TDES1_BUFFER1_SIZE		0x000007ff
#define TDES1_SECOND_ADDRESS_CHAINED	0x01000000
#define TDES1_FIRST_SEGMENT		0x20000000
#define TDES1_LAST_SEGMENT		0x40000000

#define RDES0_LAST_DESCRIPTOR		0x00000100
#define RDES0_FIRST_DESCRIPTOR		0x00000200
#define RDES0_FRAME_LENGTH		0x3fff0000
#define RDES0_FRAME_LENGTH_SHIFT	16
#define RDES0_OWN			0x80000000

#define RDES1_BUFFER1_SIZE		0x000007ff
#define RDES1_SECOND_ADDRESS_CHAINED	0x01000000
#define ERDES1_SECOND_ADDRESS_CHAINED	0x00004000

#define DMA_BUS_MODE			0x00001000
#define DMA_BUS_MODE_SFT_RESET		0x00000001
#define DMA_BUS_MODE_4PBL		0x01000000
#define DMA_BUS_MODE_DSL_SHIFT		2
#define DMA_BUS_MODE_PBL_MASK		0x00003f00
#define DMA_BUS_MODE_PBL_SHIFT		8
#define DMA_BUS_MODE_FB			0x00010000
#define DMA_XMT_POLL_DEMAND		0x00001004
#define DMA_RCV_POLL_DEMAND		0x00001008
#define DMA_RCV_BASE_ADDR		0x0000100c
#define DMA_TX_BASE_ADDR		0x00001010
#define DMA_STATUS			0x00001014
#define DMA_STATUS_TI			0x00000001
#define DMA_STATUS_RI			0x00000040
#define DMA_STATUS_TU			0x00000004
#define DMA_STATUS_NIS			0x00010000
#define DMA_STATUS_TS_MASK		0x00700000
#define DMA_STATUS_TS_SHIFT		20
#define DMA_STATUS_RS_MASK		0x000e0000
#define DMA_STATUS_RS_SHIFT		17
#define DMA_CONTROL			0x00001018
#define DMA_CONTROL_ST			0x00002000
#define DMA_CONTROL_SR			0x00000002
#define DMA_CONTROL_FUF			0x00000040
#define DMA_CONTROL_DT			0x04000000
#define DMA_CONTROL_RSF			0x02000000

#define DMA_CONTROL_RTC_32		0x00000008
#define DMA_CONTROL_OSF			0x00000004
#define DMA_CONTROL_TSF			0x00200000

#define DMA_INTR_ENA			0x0000101c
#define DMA_INTR_ENA_NIE		0x00010000
#define DMA_INTR_ENA_TIE		0x00000001
#define DMA_INTR_ENA_RIE		0x00000040

#define DMA_INTR_NORMAL			(DMA_INTR_ENA_NIE | DMA_INTR_ENA_RIE | DMA_INTR_ENA_TIE)
#define DMA_INTR_ENA_AIE		 0x00008000
#define DMA_INTR_ENA_FBE		 0x00002000
#define DMA_INTR_ENA_UNE		 0x00000020

#define DMA_INTR_ABNORMAL		(DMA_INTR_ENA_AIE | DMA_INTR_ENA_FBE | DMA_INTR_ENA_UNE)

#define DMA_INTR_DEFAULT_MASK		(DMA_INTR_NORMAL | DMA_INTR_ABNORMAL)
#define INTERRUPT_MASK			0x1FFFF

#define GMAC_VERSION			0x00000020
#define GMAC_ADDR_HIGH(reg)		(0x00000040 + (reg * 8))
#define GMAC_ADDR_LOW(reg)		(0x00000044 + (reg * 8))
#define GMAC_MII_ADDR			0x00000010
#define GMAC_MII_DATA			0x00000014
#define GMAC_CONTROL			0x00000000
#define GMAC_CONTROL_DCRS		0x00010000
#define GMAC_CONTROL_PS			0x00008000
#define GMAC_CONTROL_JD			0x00400000
#define GMAC_CONTROL_ACS		0x00000080
#define GMAC_CONTROL_JE			0x00100000
#define GMAC_CONTROL_BE			0x00200000
#define GMAC_CONTROL_FES		0x00004000
#define GMAC_CONTROL_LUD		0x00000100
#define GMAC_CONTROL_TE			0x00000008
#define GMAC_CONTROL_RE			0x00000004
#define GMAC_CONTROL_FD			0x00000800
#define GMAC_HASH_HIGH			0x00000008
#define GMAC_HASH_LOW			0x0000000c
#define GMAC_DEBUG			0x00000024
#define GMAC_DEBUG_TXSTSFSTS		0x02000000
#define GMAC_DEBUG_TXFSTS		0x01000000
#define GMAC_DEBUG_TWCSTS		0x00400000
#define GMAC_DEBUG_TRCSTS_MASK		0x00300000
#define GMAC_DEBUG_TRCSTS_SHIFT		20
#define GMAC_DEBUG_TRCSTS_IDLE		0
#define GMAC_DEBUG_TRCSTS_READ		1
#define GMAC_DEBUG_TRCSTS_TXW		2
#define GMAC_DEBUG_TRCSTS_WRITE		3
#define GMAC_DEBUG_TXPAUSED		0x00080000
#define GMAC_DEBUG_TFCSTS_MASK		0x00060000
#define GMAC_DEBUG_TFCSTS_SHIFT		17
#define GMAC_DEBUG_TFCSTS_IDLE		0
#define GMAC_DEBUG_TFCSTS_WAIT		1
#define GMAC_DEBUG_TFCSTS_GEN_PAUSE	2
#define GMAC_DEBUG_TFCSTS_XFER		3
#define GMAC_DEBUG_TPESTS		0x00010000
#define GMAC_DEBUG_RXFSTS_MASK		0x00000300
#define GMAC_DEBUG_RXFSTS_SHIFT		8
#define GMAC_DEBUG_RXFSTS_EMPTY		0
#define GMAC_DEBUG_RXFSTS_BT		1
#define GMAC_DEBUG_RXFSTS_AT		2
#define GMAC_DEBUG_RXFSTS_FULL		3
#define GMAC_DEBUG_RRCSTS_MASK		0x00000060
#define GMAC_DEBUG_RRCSTS_SHIFT		5
#define GMAC_DEBUG_RRCSTS_IDLE		0
#define GMAC_DEBUG_RRCSTS_RDATA		1
#define GMAC_DEBUG_RRCSTS_RSTAT		2
#define GMAC_DEBUG_RRCSTS_FLUSH		3
#define GMAC_DEBUG_RWCSTS		0x00000010
#define GMAC_DEBUG_RFCFCSTS_MASK	0x00000006
#define GMAC_DEBUG_RFCFCSTS_SHIFT	1
#define GMAC_DEBUG_RPESTS		0x00000001
#define GMAC_RGSMIIIS			0x000000d8
#define GMAC_RGSMIIIS_LNKMODE		0x00000001
#define GMAC_RGSMIIIS_SPEED		0x00000006
#define GMAC_RGSMIIIS_SPEED_SHIFT	1
#define GMAC_RGSMIIIS_LNKSTS		0x00000008
#define GMAC_RGSMIIIS_JABTO		0x00000010
#define GMAC_RGSMIIIS_FALSECARDET	0x00000020
#define GMAC_RGSMIIIS_SMIDRXS		0x00010000
#define GMAC_RGSMIIIS_SPEED_125		0x00000002
#define GMAC_RGSMIIIS_SPEED_25		0x00000001
#define GMAC_RGSMIIIS_SPEED_2_5		0x00000000
#define GMAC_FRAME_FILTER		0x00000004
#define GMAC_FRAME_FILTER_PR		0x00000001
#define GMAC_FRAME_FILTER_HUC		0x00000002
#define GMAC_FRAME_FILTER_HMC		0x00000004
#define GMAC_FRAME_FILTER_DAIF		0x00000008
#define GMAC_FRAME_FILTER_PM		0x00000010
#define GMAC_FRAME_FILTER_DBF		0x00000020
#define GMAC_FRAME_FILTER_SAIF		0x00000100
#define GMAC_FRAME_FILTER_SAF		0x00000200
#define GMAC_FRAME_FILTER_HPF		0x00000400
#define GMAC_FRAME_FILTER_RA		0x80000000
#define	GMAC_INT_STATUS			0x00000038
#define GMAC_INT_RGMII			0x00000001
#define	GMAC_INT_MASK			0x0000003c
#define	GMAC_INT_DISABLE_RGMII		0x00000001
#define	GMAC_INT_DISABLE_PCSLINK	0x00000002
#define	GMAC_INT_DISABLE_PCSAN		0x00000004
#define	GMAC_INT_DISABLE_PMT		0x00000008
#define	GMAC_INT_DISABLE_TIMESTAMP	0x00000200
#define	GMAC_INT_DISABLE_PCS		(GMAC_INT_DISABLE_RGMII | \
				 	GMAC_INT_DISABLE_PCSLINK | \
				 	GMAC_INT_DISABLE_PCSAN)
#define	GMAC_INT_DEFAULT_MASK		(GMAC_INT_DISABLE_TIMESTAMP | \
				 	GMAC_INT_DISABLE_PCS)

static err_t DWETHGMAC_txFrame(struct netif *netif, struct pbuf *p)
{
	DWETHGMAC_T *dwethgmac = netif->state;
	uint8_t *regs = dwethgmac->regs;
	struct pbuf *q;
	uint8_t *data;

	/* Drop packet if there isn't space to queue it */
	KRN_flushCache(dwethgmac->txDesc[dwethgmac->txi],
		       sizeof(dwethgmac->txDesc[0]), KRN_FLUSH_FLAG_D);
	if ((dwethgmac->txDesc[dwethgmac->txi][0] & TDES0_OWN) == TDES0_OWN) {
		MEM_ww(regs + DMA_XMT_POLL_DEMAND, 0xffffffff);
		return ERR_IF;
	}
#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE);
#endif
	data = dwethgmac->txBuf[dwethgmac->txi];

	for (q = p; q != NULL; q = q->next) {
		memcpy(data, q->payload, q->len);
		data += q->len;
	}
	KRN_flushCache(dwethgmac->txBuf[dwethgmac->txi], p->tot_len,
		       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D);

	dwethgmac->txDesc[dwethgmac->txi][1] = dwethgmac->txReset1 |
	    (p->tot_len & TDES1_BUFFER1_SIZE);
	dwethgmac->txDesc[dwethgmac->txi][0] = dwethgmac->txReset0 | TDES0_OWN;
	KRN_flushCache(dwethgmac->txDesc[dwethgmac->txi],
		       sizeof(dwethgmac->txDesc[dwethgmac->txi]),
		       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D);
	MEM_ww(regs + DMA_XMT_POLL_DEMAND, 0xffffffff);
	dwethgmac->txi++;
	dwethgmac->txi %= CONFIG_DRIVER_DWETHGMAC_NUM_DESCS;

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE);
#endif

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

static struct pbuf *DWETHGMAC_rxFrame(struct netif *netif)
{
	DWETHGMAC_T *dwethgmac = netif->state;
	uint32_t *desc, *prev;
	struct pbuf *p, *q;
	uint8_t *daddr, *qaddr;
	uint32_t dlen, qlen, i, stat;

	KRN_flushCache(dwethgmac->rxDesc,
		       sizeof(dwethgmac->rxDesc), KRN_FLUSH_FLAG_D);
	for (i = 0; i < CONFIG_DRIVER_DWETHGMAC_NUM_DESCS; i++) {
		desc = (uint32_t *) & dwethgmac->rxDesc[i];
		/* If filled */
		if (((desc[0] & RDES0_OWN) == 0)
		    && ((desc[0] & RDES0_FIRST_DESCRIPTOR) ==
			RDES0_FIRST_DESCRIPTOR)) {
			qlen =
			    (desc[0] & RDES0_FRAME_LENGTH) >>
			    RDES0_FRAME_LENGTH_SHIFT;
			/* Traverse all the frames for this packet */
			while ((desc[0] & RDES0_LAST_DESCRIPTOR) == 0) {
				desc = (uint32_t *) (DWETHGMAC_DMADESC_T *)
				    MEM_p2v(desc[3], MEM_P2V_CACHED);
				qlen +=
				    (desc[0] & RDES0_FRAME_LENGTH) >>
				    RDES0_FRAME_LENGTH_SHIFT;
			}
			goto rx;
		}
	}
	MEM_ww(dwethgmac->regs + DMA_RCV_POLL_DEMAND, 0xffffffff);
	return NULL;
      rx:

#if ETH_PAD_SIZE
	qlen += ETH_PAD_SIZE;
#endif
	p = pbuf_alloc(PBUF_RAW, qlen, PBUF_POOL);
	if (p != NULL) {

#if ETH_PAD_SIZE
		pbuf_header(p, -ETH_PAD_SIZE);
#endif

		desc = (uint32_t *) & dwethgmac->rxDesc[i];
		daddr = (uint8_t *) dwethgmac->rxBuf[i];
		dlen = (desc[0] & RDES0_FRAME_LENGTH) >>
		    RDES0_FRAME_LENGTH_SHIFT;
		for (q = p; q != NULL; q = q->next) {
			qaddr = q->payload;
			qlen = q->len;
			for (;;)
				if (dlen > qlen) {
					/* DMA descriptor is bigger */
					/* Flush and copy data */
					KRN_flushCache(daddr,
						       qlen, KRN_FLUSH_FLAG_D);
					memcpy(qaddr, daddr, qlen);
					/* Step pointers */
					daddr += qlen;
					dlen -= qlen;
					/* Move to next pbuf */
					q = q->next;
					if (q) {
						/* There's another pbuf - try it */
						qaddr = (uint8_t *) q->payload;
						qlen = q->len;
						continue;
					}
					/* Ran out of pbufs - liberate remaining DMA descriptors */
					for (;;) {
						prev = desc;
						desc = (uint32_t *)
						    MEM_p2v(desc[3],
							    MEM_P2V_CACHED);
						stat = prev[0];
						prev[0] = RDES0_OWN;
						KRN_flushCache(&prev[0],
							       sizeof(prev),
							       KRN_FLUSH_FLAG_D
							       |
							       KRN_FLUSH_FLAG_WRITEBACK_D);
						if ((stat &
						     RDES0_LAST_DESCRIPTOR)
						    == RDES0_LAST_DESCRIPTOR)
							break;
					}
					break;
				} else {
					/* DMA descriptor is smaller */
					/* Flush and copy data */
					KRN_flushCache(daddr,
						       dlen, KRN_FLUSH_FLAG_D);
					memcpy(qaddr, daddr, dlen);
					/* Step pointers */
					qaddr += dlen;
					qlen -= dlen;
					/* Move to next DMA descriptor */
					prev = desc;
					desc = (uint32_t *)
					    MEM_p2v(desc[3], MEM_P2V_CACHED);
					daddr = (uint8_t *)
					    MEM_p2v(desc[2], MEM_P2V_CACHED);
					dlen =
					    (desc[0] & RDES0_FRAME_LENGTH) >>
					    RDES0_FRAME_LENGTH_SHIFT;
					/* Liberate spent DMA descriptor */
					stat = prev[0];
					prev[0] = RDES0_OWN;
					KRN_flushCache(&prev[0], sizeof(prev),
						       KRN_FLUSH_FLAG_D |
						       KRN_FLUSH_FLAG_WRITEBACK_D);
					if ((stat & RDES0_LAST_DESCRIPTOR) ==
					    RDES0_LAST_DESCRIPTOR) {
						/* Ran out of DMA descriptors - done */
						break;
					}

				}
		}

#if ETH_PAD_SIZE
		pbuf_header(p, ETH_PAD_SIZE);
#endif
		LINK_STATS_INC(link.recv);
	} else {
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
	}

	MEM_ww(dwethgmac->regs + DMA_RCV_POLL_DEMAND, 0xffffffff);
	return p;
}

static void DWETHGMAC_input(struct netif *netif)
{
	struct pbuf *p;
	struct eth_hdr *ethhdr;
	for (;;) {
		p = DWETHGMAC_rxFrame(netif);
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
				    ("DWETHGMAC_input: packet input error\n"));
		default:
			break;
		}
		pbuf_free(p);
	}
}

err_t DWETHGMAC_init(struct netif *netif)
{
	DWETHGMAC_T *dwethgmac = (DWETHGMAC_T *) netif->state;
	uint32_t i, rchained;
	uint8_t *regs;
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	regs = (uint8_t *) MEM_p2v(dwethgmac->pAddr, MEM_P2V_UNCACHED);
	dwethgmac->regs = regs;
#if LWIP_NETIF_HOSTNAME
	netif->hostname = "lwip";
#endif				/* LWIP_NETIF_HOSTNAME */
	/* Brazenly assume 100baseT */
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100 * 1024 * 1024);
	netif->name[0] = 'e';
	netif->name[1] = 'n';
	netif->output = etharp_output;
	netif->linkoutput = DWETHGMAC_txFrame;
	netif->mtu = 1500;
	netif->flags =
	    NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	/* Initialise GMAC DMA */
	/* Initiate SW reset */
	MEM_ww(regs + DMA_BUS_MODE,
	       MEM_rw(regs + DMA_BUS_MODE) | DMA_BUS_MODE_SFT_RESET);
	while (MEM_rw(regs + DMA_BUS_MODE) & DMA_BUS_MODE_SFT_RESET) ;
	/* Disable interrupts */
	/* Disable GMAC interrupts */
	MEM_ww(regs + GMAC_INT_MASK, GMAC_INT_DEFAULT_MASK);
	MEM_ww(regs + DMA_INTR_ENA, DMA_INTR_DEFAULT_MASK);
	MEM_ww(regs + DMA_CONTROL,
	       MEM_rw(regs + DMA_CONTROL) & ~(DMA_CONTROL_SR | DMA_CONTROL_ST));
	/* Version specific descriptor ugliness */
	if ((MEM_rw(regs + GMAC_VERSION) & 0xff)
	    >= 0x35) {
		dwethgmac->txReset0 =
		    ETDES0_SECOND_ADDRESS_CHAINED | ETDES0_FIRST_SEGMENT |
		    ETDES0_LAST_SEGMENT;
		dwethgmac->txReset1 = 0;
		rchained = ERDES1_SECOND_ADDRESS_CHAINED;
	} else {
		dwethgmac->txReset0 = 0;
		dwethgmac->txReset1 =
		    TDES1_SECOND_ADDRESS_CHAINED | TDES1_FIRST_SEGMENT |
		    TDES1_LAST_SEGMENT;
		rchained = RDES1_SECOND_ADDRESS_CHAINED;
	}

	/* Initialise DMA descriptors */
	for (i = 0; i < CONFIG_DRIVER_DWETHGMAC_NUM_DESCS; i++) {

		dwethgmac->txDesc[i][3] =
		    MEM_v2p(&dwethgmac->txDesc
			    [(i + 1) % CONFIG_DRIVER_DWETHGMAC_NUM_DESCS]);
		dwethgmac->txDesc[i][2] = MEM_v2p(&dwethgmac->txBuf[i]);
		dwethgmac->txDesc[i][1] = dwethgmac->txReset1;
		dwethgmac->txDesc[i][0] = dwethgmac->txReset0;
		dwethgmac->rxDesc[i][3] =
		    MEM_v2p(&dwethgmac->rxDesc
			    [(i + 1) % CONFIG_DRIVER_DWETHGMAC_NUM_DESCS]);
		dwethgmac->rxDesc[i][2]
		    = MEM_v2p(&dwethgmac->rxBuf[i]);
		dwethgmac->rxDesc[i][1] =
		    (2047 & RDES1_BUFFER1_SIZE) | rchained;
		dwethgmac->rxDesc[i][0] = RDES0_OWN;
	}
	dwethgmac->txi = 0;
	KRN_flushCache(dwethgmac->rxDesc,
		       sizeof(dwethgmac->rxDesc),
		       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D);
	KRN_flushCache(dwethgmac->txDesc, sizeof(dwethgmac->txDesc),
		       KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D);
	/* Configure bus */
	MEM_ww(regs + DMA_BUS_MODE,
	       DMA_BUS_MODE_FB | (8 << DMA_BUS_MODE_PBL_SHIFT));
	/* Write descriptor list addresses into controller */
	MEM_ww(regs + DMA_TX_BASE_ADDR, MEM_v2p(dwethgmac->txDesc));
	MEM_ww(regs + DMA_RCV_BASE_ADDR, MEM_v2p(dwethgmac->rxDesc));
	MEM_ww(regs + DMA_CONTROL, MEM_rw(regs + DMA_CONTROL) | DMA_CONTROL_SR | DMA_CONTROL_ST	/*| DMA_CONTROL_DT */
	       | DMA_CONTROL_TSF | DMA_CONTROL_RSF |
	       /* DMA_CONTROL_FUF | */ DMA_CONTROL_RTC_32);
	/* Clear interrupts */
	MEM_ww(dwethgmac->regs + DMA_STATUS,
	       MEM_rw(dwethgmac->regs + DMA_STATUS) | INTERRUPT_MASK);

	/* Set MAC address */
	if (!((dwethgmac->mac[5] == 0) && (dwethgmac->mac[4] == 0)
	      && (dwethgmac->mac[3] == 0) && (dwethgmac->mac[2] == 0)
	      && (dwethgmac->mac[1] == 0) && (dwethgmac->mac[0] == 0))) {
		MEM_ww(regs + GMAC_ADDR_HIGH(0),
		       (dwethgmac->mac[5] << 8) | dwethgmac->mac[4]);
		MEM_ww(regs + GMAC_ADDR_LOW(0),
		       (dwethgmac->mac[3] << 24) | (dwethgmac->mac[2] << 16) |
		       (dwethgmac->mac[1]
			<< 8) | dwethgmac->mac[0]);
		netif->hwaddr[0] = dwethgmac->mac[0];
		netif->hwaddr[1] = dwethgmac->mac[1];
		netif->hwaddr[2] = dwethgmac->mac[2];
		netif->hwaddr[3] = dwethgmac->mac[3];
		netif->hwaddr[4] = dwethgmac->mac[4];
		netif->hwaddr[5] = dwethgmac->mac[5];
	} else {
		uint32_t hi = MEM_rw(regs + GMAC_ADDR_HIGH(0)), lo =
		    MEM_rw(regs + GMAC_ADDR_LOW(0));
		netif->hwaddr[0] = (lo & 0x000000ff) >> 0;
		netif->hwaddr[1] = (lo & 0x0000ff00) >> 8;
		netif->hwaddr[2] = (lo & 0x00ff0000) >> 16;
		netif->hwaddr[3] = (lo & 0xff000000) >> 24;
		netif->hwaddr[4] = (hi & 0x000000ff) >> 0;
		netif->hwaddr[5] = (hi & 0x0000ff00) >> 8;
	}

	/* Set DMA operation mode */
	MEM_ww(regs + DMA_CONTROL,
	       MEM_rw(regs +
		      DMA_CONTROL) | DMA_CONTROL_TSF | DMA_CONTROL_OSF |
	       DMA_CONTROL_RSF);
	/* Initialise the core */
	MEM_ww(regs + GMAC_CONTROL, MEM_rw(regs + GMAC_CONTROL)
	       | GMAC_CONTROL_JD	/* Jabber disable */
	       //| GMAC_CONTROL_PS      /* Port select 0:GMI 1:MII */
	       | GMAC_CONTROL_ACS	/* Automatic Pad Stripping */
	       | GMAC_CONTROL_BE	/* Frame Burst Enable */
	       | GMAC_CONTROL_TE	/* Transmitter Enable */
	       | GMAC_CONTROL_FES	/* Speed 0:10 1:100 */
	       | GMAC_CONTROL_LUD	/* Link up/down */
	       | GMAC_CONTROL_RE	/* Receiver Enable */
	    );
	MEM_ww(regs + GMAC_HASH_HIGH, 0xffffffff);
	MEM_ww(regs + GMAC_HASH_LOW, 0xffffffff);
	/* FIXME: Promiscuous mode */
	MEM_ww(regs + GMAC_FRAME_FILTER, 0 /*GMAC_FRAME_FILTER_PR */ );
	return ERR_OK;
}

static void DWETHGMAC_rgsmiis(DWETHGMAC_T * dwethgmac)
{
	uint32_t status, mbps;
	status = MEM_rw(dwethgmac->regs + GMAC_RGSMIIIS);
	if (status & GMAC_RGSMIIIS_LNKSTS) {
		uint32_t ps, fes, fd;
		uint32_t speed =
		    (status & GMAC_RGSMIIIS_SPEED) >> GMAC_RGSMIIIS_SPEED_SHIFT;
		switch (speed) {
		case GMAC_RGSMIIIS_SPEED_125:	/* 1000 */
			mbps = 1000;
			break;
		case GMAC_RGSMIIIS_SPEED_25:	/* 100 */
			mbps = 100;
			break;
		default:	/* 10 */
			mbps = 10;
			break;
		}
		(void)(status & GMAC_RGSMIIIS_LNKMODE);	/* Full duplex if true */
		ps = (mbps != 1000) ? GMAC_CONTROL_PS : 0;
		fes = (speed == 100) ? GMAC_CONTROL_FES : 0;
		fd = (status & GMAC_RGSMIIIS_LNKMODE)
		    ? GMAC_CONTROL_FD : 0;
		MEM_ww(dwethgmac->regs + GMAC_CONTROL,
		       (MEM_rw(dwethgmac->regs + GMAC_CONTROL) &
			~(GMAC_CONTROL_PS | GMAC_CONTROL_FES | GMAC_CONTROL_FD))
		       | ps | fes | fd);
		/* FIXME: reconfigure GMAC_CONTROL based on speed, etc. */
		netif_set_link_up(&dwethgmac->netif);
		MEM_ww(dwethgmac->regs + DMA_RCV_POLL_DEMAND, 0xffffffff);
		DBG_logF("dwethgmac: %.2s%u link up (%" PRIu32
			 "/%s duplex)\n", dwethgmac->netif.name,
			 dwethgmac->netif.num, mbps,
			 (status & GMAC_RGSMIIIS_LNKMODE) ? "Full" : "Half");
	} else {
		netif_set_link_down(&dwethgmac->netif);
		DBG_logF("dwethgmac: %s%u link down\n",
			 dwethgmac->netif.name, dwethgmac->netif.num);
	}
}

static void DWETHGMAC_isr(int32_t sigNum)
{
	IRQ_DESC_T *cause = IRQ_cause(sigNum);
	DWETHGMAC_T *dwethgmac = (DWETHGMAC_T *) cause->priv;
	uint32_t status, spurious = 1;
#if 0
	status = MEM_rw(dwethgmac->regs + DMA_STATUS);
	switch ((status & DMA_STATUS_TS_MASK) >> DMA_STATUS_TS_SHIFT) {
	case 0:
		DBG_logF("- TX (Stopped): Reset or Stop command\n");
		break;
	case 1:
		DBG_logF("- TX (Running): Fetching the Tx desc\n");
		break;
	case 2:
		DBG_logF("- TX (Running): Waiting for end of tx\n");
		break;
	case 3:
		DBG_logF("- TX (Running): Reading the data "
			 "and queuing the data into the Tx buf\n");
		break;
	case 6:
		DBG_logF("- TX (Suspended): Tx Buff Underflow "
			 "or an unavailable Transmit descriptor\n");
		break;
	case 7:
		DBG_logF("- TX (Running): Closing Tx descriptor\n");
		break;
	default:
		break;
	}

	switch ((status & DMA_STATUS_RS_MASK) >> DMA_STATUS_RS_SHIFT) {
	case 0:
		DBG_logF("- RX (Stopped): Reset or Stop command\n");
		break;
	case 1:
		DBG_logF("- RX (Running): Fetching the Rx desc\n");
		break;
	case 2:
		DBG_logF("- RX (Running): Checking for end of pkt\n");
		break;
	case 3:
		DBG_logF("- RX (Running): Waiting for Rx pkt\n");
		break;
	case 4:
		DBG_logF("- RX (Suspended): Unavailable Rx buf\n");
		break;
	case 5:
		DBG_logF("- RX (Running): Closing Rx descriptor\n");
		break;
	case 6:
		DBG_logF("- RX(Running): Flushing the current frame"
			 " from the Rx buf\n");
		break;
	case 7:
		DBG_logF("- RX (Running): Queuing the Rx frame"
			 " from the Rx buf into memory\n");
		break;
	default:
		break;
	}

	status = MEM_rw(dwethgmac->regs + GMAC_DEBUG);
	if (status & GMAC_DEBUG_TXSTSFSTS)
		DBG_logF("MTL TX status FIFO full\n");
	if (status & GMAC_DEBUG_TXFSTS)
		DBG_logF("MTL TX FIFO not empty\n");
	if (status & GMAC_DEBUG_TWCSTS)
		DBG_logF("MTL FIFO control?\n");
	if (status & GMAC_DEBUG_TRCSTS_MASK) {
		switch ((status & GMAC_DEBUG_TRCSTS_MASK)
			>> GMAC_DEBUG_TRCSTS_SHIFT) {
		case GMAC_DEBUG_TRCSTS_WRITE:
			DBG_logF("MTL TX FIFO read control writing\n");
			break;
		case GMAC_DEBUG_TRCSTS_TXW:
			DBG_logF("MTL TX FIFO read control waiting\n");
			break;
		case GMAC_DEBUG_TRCSTS_READ:
			DBG_logF("MTL TX FIFO read control reading\n");
			break;
		default:
			DBG_logF("MTL TX FIFO read control idle\n");
			break;
		}
	}
	if (status & GMAC_DEBUG_TXPAUSED)
		DBG_logF("MAC TX paused\n");
	if (status & GMAC_DEBUG_TFCSTS_MASK) {
		switch ((status & GMAC_DEBUG_TFCSTS_MASK)
			>> GMAC_DEBUG_TFCSTS_SHIFT) {
		case GMAC_DEBUG_TFCSTS_XFER:
			DBG_logF("MAC TX frame control transfering\n");
			break;
		case GMAC_DEBUG_TFCSTS_GEN_PAUSE:
			DBG_logF("MAC TX frame control paused\n");
			break;
		case GMAC_DEBUG_TFCSTS_WAIT:
			DBG_logF("MAC TX frame control waiting\n");
			break;
		default:
			DBG_logF("MAC TX frame control idle\n");
			break;
		}
	}
	if (status & GMAC_DEBUG_TPESTS)
		DBG_logF("GMII TX proto engine?\n");
	if (status & GMAC_DEBUG_RXFSTS_MASK) {
		switch ((status & GMAC_DEBUG_RXFSTS_MASK)
			>> GMAC_DEBUG_RRCSTS_SHIFT) {

		case GMAC_DEBUG_RXFSTS_FULL:
			DBG_logF("MTL RX FIFO full\n");
			break;
		case GMAC_DEBUG_RXFSTS_AT:
			DBG_logF("MTL RX FIFO above threshold\n");
			break;
		case GMAC_DEBUG_RXFSTS_BT:
			DBG_logF("MTL RX FIFO below threshold\n");
			break;
		default:
			DBG_logF("MTL RX FIFO empty\n");
			break;
		}
	}
	if (status & GMAC_DEBUG_RRCSTS_MASK) {
		switch ((status & GMAC_DEBUG_RRCSTS_MASK) >>
			GMAC_DEBUG_RRCSTS_SHIFT) {
		case GMAC_DEBUG_RRCSTS_FLUSH:
			DBG_logF("MTL RX FIFO read control flushing\n");
			break;
		case GMAC_DEBUG_RRCSTS_RSTAT:
			DBG_logF("MTL RX FIFO read control reading\n");
		case GMAC_DEBUG_RRCSTS_RDATA:
			DBG_logF("MTL RX FIFO read control statusing\n");
		default:
			DBG_logF("MTL RX FIFO read control idle\n");
		}
	}
	if (status & GMAC_DEBUG_RWCSTS)
		DBG_logF("MTL RX FIFO control active\n");
	if (status & GMAC_DEBUG_RFCFCSTS_MASK)
		DBG_logF("MAC RX frame control fifo: %"
			 PRIu32 "\n", (status & GMAC_DEBUG_RFCFCSTS_MASK)
			 >> GMAC_DEBUG_RFCFCSTS_SHIFT);
	if (status & GMAC_DEBUG_RPESTS)
		DBG_logF("GMII RX proto engine?\n");
#endif
	if (MEM_rw(dwethgmac->regs + GMAC_INT_STATUS) & GMAC_INT_RGMII) {
		spurious = 0;
		DWETHGMAC_rgsmiis(dwethgmac);
	}
	status = MEM_rw(dwethgmac->regs + DMA_STATUS);
	if ((status & INTERRUPT_MASK) == 0 && spurious) {
		DBG_logF
		    ("Spurious dwethgmac (@%p) DMA interrupt!\n",
		     (void *)dwethgmac->pAddr);
	} else {
		if (status & DMA_STATUS_RI) {
			DWETHGMAC_input(&dwethgmac->netif);
		}
		if (status &
		    (DMA_INTR_DEFAULT_MASK &
		     ~(DMA_STATUS_RI | DMA_STATUS_TI |
		       DMA_STATUS_TU | DMA_STATUS_NIS))) {
			DBG_logF
			    ("Unhandled dwethgmac (@%p) DMA interrupt %08"
			     PRIx32 "!\n",
			     (void *)dwethgmac->pAddr, status & INTERRUPT_MASK);
		}
	}
	/* Clear all DMA interrupts */
	MEM_ww(dwethgmac->regs + DMA_STATUS,
	       MEM_rw(dwethgmac->regs + DMA_STATUS) | INTERRUPT_MASK);
	IRQ_ack(cause);
}

void DWETHGMAC_dtInit(DWETHGMAC_T * dwethgmac, IRQ_DESC_T * irq)
{
	irq->isrFunc = DWETHGMAC_isr;
	irq->priv = (void *)dwethgmac;
	ip_addr_t zero;
	IP4_ADDR(&zero, 0, 0, 0, 0);
	netif_add(&dwethgmac->netif, &zero, &zero, &zero,
		  (void *)dwethgmac, DWETHGMAC_init, tcpip_input);
	DWETHGMAC_rgsmiis(dwethgmac);
	IRQ_route(irq);
}
