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
*   Description:        mipsFPGA web demo
*
*************************************************************************/

#include <stdio.h>
#include <string.h>
#include "MEOS.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "arch/sys_arch.h"
#include "web_page.h"

/* The size of the buffer in which the dynamic WEB page is created. */
#define WEB_MAX_PAGE_SIZE        ( 4096 )

/* Standard GET response. */
#define WEB_HTTP_OK  "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"

/* The port on which we listen. */
#define WEB_HTTP_PORT            ( 80 )

/* Delay on close error. */
#define WEB_SHORT_DELAY          ( 10 )

#define WEB_TASK_PRIORITY           ( KRN_LOWEST_PRIORITY + 1 )
#define STACKSIZE 2000		/* task stack size */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)
static void ProcessConnection(struct netconn *NetCon);

/*------------------------------------------------------------*/
static int32_t zoom_level = 100;

/*
 * Process an incoming connection on port 80.
 *
 * This simply checks to see if the incoming data contains a GET request, and
 * if so sends back a single dynamically created page.  The connection is then
 * closed.  A more complete implementation could create a task for each
 * connection.
 */
static void ProcessConnection(struct netconn *NetCon)
{
	static uint8_t TxData[WEB_MAX_PAGE_SIZE];
	static uint8_t RxData[WEB_MAX_PAGE_SIZE], *RxDataPtr;
	struct netbuf *RxBuffer;
	int8_t *RxString;
	uint16_t Length, totLen;
	static uint32_t pageHits = 0;
	char num_buff[33];
	uint32_t done = 0;

	/* Where is the data? */
	RxDataPtr = RxData;
	totLen = 0;

	do {
		/* We expect to immediately get data. */
		netconn_recv(NetCon, &RxBuffer);
		if (RxBuffer == NULL)
			/* Can only happen on broken connection */
			goto escape;
		/* Unbuffer and join data */
		do {
			netbuf_data(RxBuffer, (void *)&RxString, &Length);
			memcpy(RxDataPtr, RxString, Length);
			RxDataPtr += Length;
			totLen += Length;
		} while (netbuf_next(RxBuffer) != -1);
		*RxDataPtr = '\0';
		/* Clean up */
		netbuf_delete(RxBuffer);
		/* Continue until we receive the end of request marker */
		if (strstr((const char *)RxData, "GET"))
			done = 1;
		if (strstr((const char *)RxData, "POST")
		    && strstr((const char *)RxData, "endofpage"))
			done = 1;

	} while (!done);

	DBG_logF("[READ{size:%d}]:-\r\n", totLen);
	DBG_logF("%.24s", RxData);
	DBG_logF("%s", RxData);
	DBG_logF("...\r\n\r\n");

	/* Zoom in or out? */
	if (strstr((const char *)RxData, "ZoomIn")) {
		zoom_level += 10;
	}
	if (strstr((const char *)RxData, "ZoomOut")) {
		zoom_level -= 10;
	}

	/* For a GET or a POST we still send the page back */
	if ((strstr((const char *)RxData, "GET")
	     || strstr((const char *)RxData, "POST"))
	    && !strstr((const char *)RxString, "favicon.ico")) {
		/* Write out the HTTP OK header. */
		netconn_write(NetCon, WEB_HTTP_OK,
			      (uint16_t) strlen(WEB_HTTP_OK), NETCONN_COPY);

		/* Generate the dynamic page... */
		strcpy((char *)TxData, "<!DOCTYPE html>");
		strcat((char *)TxData, "<html>");
		strcat((char *)TxData, "<head>");
		strcat((char *)TxData, "</head>");

		sprintf(num_buff, "%u", (unsigned int)zoom_level);
		strcat((char *)TxData, "<body style=\"zoom:");
		strcat((char *)TxData, num_buff);
		strcat((char *)TxData, "%;\">");

		strcat((char *)TxData, "<form method=\"post\" action=\"\">");

		strcat((char *)TxData, "<b>Hello World!</b><br><br>");
		strcat((char *)TxData, "Page hits:");
		sprintf(num_buff, "%u", (unsigned int)++pageHits);
		strcat((char *)TxData, num_buff);
		strcat((char *)TxData, "<br><br>");
		strcat((char *)TxData,
		       "(c) 2016 by MIPS Tech, LLC<br><br>");

		strcat((char *)TxData, "Zoom%:");
		sprintf(num_buff, "%u", (unsigned int)zoom_level);
		strcat((char *)TxData, num_buff);
		strcat((char *)TxData, "<br><br>");

		strcat((char *)TxData,
		       "<input type=\"submit\" name=\"ZoomIn\" value=\"Zoom In\"><br><br>");
		strcat((char *)TxData,
		       "<input type=\"submit\" name=\"ZoomOut\" value=\"Zoom Out\"><br>");
		strcat((char *)TxData,
		       "<input type=\"checkbox\" name=\"endofpage\" value=\"1\" checked hidden>");

		strcat((char *)TxData, "</form></body></html>\r\n");

		/* Write out the dynamically generated page. */
		DBG_logF("[WRITE]:-\r\n");
		DBG_logF("%.24s", TxData);
		DBG_logF("...\r\n\r\n");
		netconn_write(NetCon, TxData,
			      (uint16_t) strlen((char *)TxData), NETCONN_COPY);
	}
      escape:

	netconn_close(NetCon);
}

void web_page(void *param)
{
	struct netconn *HTTPListener, *NewConnection;
	err_t error;
	struct netif *ethernetIf = netif_find("en0");
	extern NS16550A_T ttyS0;

	netifapi_netif_set_default(ethernetIf);

	/* Specify host name, and start DHCP */
	netif_set_hostname(ethernetIf, "fpga_demo");

	DBG_logF("Using DHCP to acquire IP address\n");
	netifapi_netif_set_up(ethernetIf);
	netifapi_dhcp_start(ethernetIf);

	while (ethernetIf->ip_addr.addr == 0L) {
		KRN_delay(DHCP_FINE_TIMER_MSECS * 1000);
		KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	}

	/* Create a new tcp connection handle */
	HTTPListener = netconn_new(NETCONN_TCP);
	netconn_bind(HTTPListener, NULL, WEB_HTTP_PORT);
	netconn_listen(HTTPListener);
	{
		static uint8_t ip[4];
		uint8_t ch = '\n';

		/* Display aquired IP address */
		ip[0] = ethernetIf->ip_addr.addr & 0xFF;
		ip[1] = (ethernetIf->ip_addr.addr >> 8) & 0xFF;
		ip[2] = (ethernetIf->ip_addr.addr >> 16) & 0xFF;
		ip[3] = (ethernetIf->ip_addr.addr >> 24) & 0xFF;
		DBG_logF("IP:%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

		/* write to the serial rx stream, to make the
		   monitor task re-display it's menu */
		UART_inject(NULL, &ch, 1, 1, &ttyS0);
	}

	DBG_logF("Demo up and running!\n");

	/* Loop forever */
	for (;;) {
		/* Wait for connection. */
		error = netconn_accept(HTTPListener, &NewConnection);
		mips_flush_dcache();

		if (error == ERR_OK) {
			/* Service connection. */
			ProcessConnection(NewConnection);
			while (netconn_delete(NewConnection) != ERR_OK) {
				KRN_delay(WEB_SHORT_DELAY);
			}
		}
	}
}
