#ifndef LWIPOPTS_H
#define LWIPOPTS_H
/* These need fleshing out and making configurable */

#define LWIP_TCPIP_CORE_LOCKING  0
#define LWIP_NETIF_API 1

#define SYS_LIGHTWEIGHT_PROT	1
#ifdef CONFIG_NEWTRON
/* Newtron stats macros clash */
#define LWIP_STATS	0
#else
#define LWIP_STATS	0
#endif
#define LWIP_DHCP	1
#define LWIP_AUTOIP	1
#define LWIP_DNS	1
#define PPP_SUPPORT	0
#define TCPIP_THREAD_STACKSIZE CONFIG_LWIP_MAX_STACK
#define MEM_ALIGNMENT	4
#define MEM_SIZE	8192

#define LWIP_NETIF_HOSTNAME	1
#define LWIP_PROVIDE_ERRNO	1
#define LWIP_NETIF_LINK_CALLBACK	1
#define LWIP_ARP_FILTER_NETIF	1
#define LWIP_ARP_FILTER_NETIF_FN(P,N,T)	__extension__ ({ \
extern struct netif *LWIP_filter(struct pbuf *pbuf, struct netif *netif, u16_t type); \
LWIP_filter((P),(N),(T)); \
})

#define LWIP_TCP 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_RCVBUF 1
#define LWIP_CHECKSUM_ON_COPY 1
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_GEN_ICMP 1
#define CHECKSUM_GEN_ICMP6 1

#endif				/* LWIPOPTS_H */
