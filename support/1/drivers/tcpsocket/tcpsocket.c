/***(C)2001***************************************************************
*
* Copyright (C) 2001 MIPS Tech, LLC
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
****(C)2001**************************************************************/

/*************************************************************************
*
*   Description:	TCPSOCKET driver
*
*************************************************************************/

#include <alloca.h>
#include "MEOS.h"
#include "lwip/api.h"
#include "lwip/netif.h"

int32_t TCPSOCKET_config(UART_T * uuart, const char *config)
{
	DBG_assert(0, "Meaningless to configure a TCP socket UART!\n");
	return 0;
}

inline void TCPSOCKET_tx(TCPSOCKET_T * ts)
{
	uint8_t buffer[128];
	size_t len;
	while (!RING_empty(&ts->uart.tx)) {
		len = RING_readBuffer(&ts->uart.tx, buffer, 128, 0);
		if (ts->conn)
			netconn_write(ts->conn, buffer, len, NETCONN_COPY);
	}
}

void TCPSOCKET_enableTXEmptyInt(UART_T * uart)
{
}

/* Socket read task - only supports one connection */
void TCPSOCKET_read()
{
	TCPSOCKET_T *ts = (TCPSOCKET_T *) KRN_taskParameter(NULL);
	struct netif *nif;
	ip_addr_t *a = IP_ADDR_ANY;
	struct netconn *li;
	struct netbuf *b;
	uint8_t *ch;
	uint16_t le;
	for (;;) {
		/* Wait until LwIP is up */
		while (!LWIP_up())
			KRN_delay(1000);
		/* Convert interface name to address */
		if (ts->interface) {
			while (!(nif = netif_find((char *)ts->interface)))
				KRN_delay(1000);
			while (!netif_is_up(nif)) {
				KRN_delay(1000);
				KRN_barrier(KRN_BARRIER_FLAG_WRITE);
			}
			while (ip_addr_isany(&nif->ip_addr)) {
				KRN_delay(1000);
				KRN_barrier(KRN_BARRIER_FLAG_WRITE);
			}
			a = &nif->ip_addr;
		}
		/* Start listening */
		li = netconn_new(NETCONN_TCP);
		netconn_bind(li, a, ts->port);
		netconn_listen(li);
		for (;;) {
			/* Wait for a connection */
			if (netconn_accept(li, &ts->conn) != ERR_OK)
				break;
			netconn_set_recvtimeout(ts->conn, 10);
			/* Receive packets until it fails */
			for (;;) {

				TCPSOCKET_tx(ts);
				netconn_recv(ts->conn, &b);
				if (ERR_IS_FATAL(netconn_err(ts->conn)))
					break;
				if (b == NULL)
					continue;
				for (;;) {
					netbuf_data(b, (void **)
						    &ch, &le);
					while (le--) {
						if (RING_write
						    (&ts->uart.rx,
						     *ch, 0) == 0) {
							if (ts->uart.rFunc)
								ts->uart.rFunc
								    (ts->
								     uart.rPar);
							RING_write(&ts->uart.rx,
								   *ch,
								   KRN_INFWAIT);
						}
						ch++;
					}
					if (!RING_empty(&ts->uart.rx)
					    && (ts->uart.rFunc))
						ts->uart.rFunc(ts->uart.rPar);
					if (netbuf_next(b) == -1)
						break;
				}
				netbuf_delete(b);
			}
			netconn_delete(ts->conn);
			ts->conn = NULL;
		}
		netconn_delete(li);
	}
}

void TCPSOCKET_init(TCPSOCKET_T * ts, uint8_t * txBuf, size_t txLen,
		    uint8_t * rxBuf, size_t rxLen, KRN_TASK_T * rxTask,
		    uint32_t * rxStack, size_t rxStackSize)
{
	ts->uart.config = TCPSOCKET_config;
	ts->uart.enableTXEmptyInt = TCPSOCKET_enableTXEmptyInt;
	ts->conn = NULL;
	RING_init(&ts->uart.tx, txBuf, txLen);
	RING_init(&ts->uart.rx, rxBuf, rxLen);
	KRN_startTask(TCPSOCKET_read, rxTask, rxStack, rxStackSize,
		      KRN_maxPriority() - 1, ts, ts->name);
}
