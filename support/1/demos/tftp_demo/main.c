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
*   Description:        TFTP demo
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
static KRN_TASK_T *bgtask;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static uint32_t istack[STACKSIZE];
static KRN_TASK_T *meos_timer_task;
static uint32_t meos_timer_task_stack[2048];

int32_t zoom_level = 100;

static char tftpserver[256], tftpfile[256];
static uint32_t tftpoffset, tftpsize;

#define DL_BUFF_SIZE	0x1000
static uint8_t dl_buff[DL_BUFF_SIZE];

static TFTP_T tftp;

void tftpdemo()
{
	int result;
	const char *err_txt[] = {
		"ERR_TFTP_ALL_OK",
		"ERR_TFTP_NO_SOCKET",
		"ERR_TFTP_TIMEOUT",
		"ERR_TFTP_FILE_TOO_BIG",
		"ERR_TFTP_NOT_DEFINED",
		"ERR_TFTP_FILE_NOT_FOUND",
		"ERR_TFTP_ACCESS_VIOLATION",
		"ERR_TFTP_DISK_FULL",
		"ERR_TFTP_ILLEGAL_OPERATION",
		"ERR_TFTP_UNKNOWN_TRANSFER_ID",
		"ERR_TFTP_FILE_ALREADY_EXISTS",
		"ERR_TFTP_NO_SUCH_USER"
	};

	TFTP_init(&tftp, tftpserver, tftpfile);
	result =
	    TFTP_read((void *)tftpoffset, (void *)dl_buff,
		      (int)tftpsize < 0x1000 ? tftpsize : 0x1000, 1,
		      (void *)&tftp);
	DBG_logF("result:[%d]", result);
	if (result <= 0) {
		DBG_logF(" %s", err_txt[-result]);
	}
	DBG_logF("\n");
	if (result >= 0) {
		DBG_logF("recvd:\n%*s\n", (int)tftpsize, dl_buff);
	}
}

static inline char hexval(char v)
{
	if ((v >= '0') && (v <= '9'))
		return v - '0';
	else if ((v >= 'a') && (v <= 'f'))
		return (v - 'a') + 0xa;
	else if ((v >= 'A') && (v <= 'F'))
		return (v - 'A') + 0xa;
	else
		return 0;
}

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
	char *token;

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
	} while (!strstr((const char *)RxData, "\r\n\r\n"));

	DBG_logF("[READ]:-\r\n");
	DBG_logF("%.24s", RxData);
	DBG_logF("...\r\n\r\n");

	/* Handle form */
	if ((token = strstr((const char *)RxData, "Server="))) {
		uint32_t i, l;
		token += 7;
		strncpy(tftpserver, token, 255);
		l = strlen(tftpserver);
		for (i = 0; i < l; i++)
			switch (tftpserver[i]) {
			case '+':
				tftpserver[i] = ' ';
				break;
			case '&':
				tftpserver[i] = 0;
				l = i;
				break;
			case '%':
				tftpserver[i] =
				    hexval(tftpserver[i + 1]) << 4 |
				    hexval(tftpserver[i + 2]);
				memmove(&tftpserver[i + 1], &tftpserver[i + 3],
					strlen(&tftpserver[i + 3]) + 1);
			default:
				break;
			}
	}
	if ((token = strstr((const char *)RxData, "File="))) {
		uint32_t i, l;
		token += 5;
		strncpy(tftpfile, token, 255);
		l = strlen(tftpfile);
		for (i = 0; i < l; i++)
			switch (tftpfile[i]) {
			case '+':
				tftpfile[i] = ' ';
				break;
			case '&':
				tftpfile[i] = 0;
				l = i;
				break;
			case '%':
				tftpfile[i] =
				    hexval(tftpfile[i + 1]) << 4 |
				    hexval(tftpfile[i + 2]);
				memmove(&tftpfile[i + 1], &tftpfile[i + 3],
					strlen(&tftpfile[i + 3]) + 1);
			default:
				break;
			}
	}
	if ((token = strstr((const char *)RxData, "Offset="))) {
		token += 7;
		sscanf(token, "%u&", (unsigned int *)&tftpoffset);
	}
	if ((token = strstr((const char *)RxData, "Size="))) {
		token += 5;
		sscanf(token, "%u&", (unsigned int *)&tftpsize);
	}
	if ((token = strstr((const char *)RxData, "Fetch")))
		tftpdemo();

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
		       "Please provide details of TFTP server. Beware that this demo supports minimal form decoding, so may be limited to only fetching from simple paths.<br><br>");

		strcat((char *)TxData, "TFTP server IP:");
		strcat((char *)TxData,
		       "<input type=\"text\" name=\"Server\" value=\"Server\"><br><br>");
		strcat((char *)TxData, "File name to fetch:");
		strcat((char *)TxData,
		       "<input type=\"text\" name=\"File\" value=\"File\"><br><br>");
		strcat((char *)TxData, "Offset to begin fetch from:");
		strcat((char *)TxData,
		       "<input type=\"text\" name=\"Offset\" value=\"Offset\"><br><br>");
		strcat((char *)TxData, "Number of bytes to fetch:");
		strcat((char *)TxData,
		       "<input type=\"text\" name=\"Size\" value=\"Size\"><br><br>");
		strcat((char *)TxData,
		       "<input type=\"submit\" name=\"Fetch\" value=\"Fetch\"><br><br>");
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

int main(void)
{
	KRN_TASKQ_T queue;
	struct netconn *HTTPListener, *NewConnection;
	err_t error;
	struct netif *nif;

	/* Initialise MeOS */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	DQ_init(&queue);

	meos_timer_task =
	    KRN_startTimerTask("timerTask", meos_timer_task_stack,
			       sizeof(meos_timer_task_stack));

	BSP_init();
	nif = netif_find("en0");
	netifapi_netif_set_default(nif);

	/* Specify host name, and start DHCP */
	netif_set_hostname(nif, "web_page");

	DBG_logF("Using DHCP to acquire IP address\r\n");
	netifapi_netif_set_up(nif);
	netifapi_dhcp_start(nif);

	while (nif->ip_addr.addr == 0L) {
		KRN_hibernate(&queue, DHCP_FINE_TIMER_MSECS);
		KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	}

	/* Create a new tcp connection handle */
	HTTPListener = netconn_new(NETCONN_TCP);
	netconn_bind(HTTPListener, NULL, WEB_HTTP_PORT);
	netconn_listen(HTTPListener);

	/* Display aquired IP address */
	{
		static uint8_t ip[4], outbuff[81];
		ip[0] = nif->ip_addr.addr & 0xFF;
		ip[1] = (nif->ip_addr.addr >> 8) & 0xFF;
		ip[2] = (nif->ip_addr.addr >> 16) & 0xFF;
		ip[3] = (nif->ip_addr.addr >> 24) & 0xFF;
		sprintf((char *)outbuff,
			"Connect to %d.%d.%d.%d port 80 with a web browser for control interface!\r\n",
			ip[0], ip[1], ip[2], ip[3]);
		DBG_logF("%s", outbuff);
	}

	DBG_logF("Demo up and running!\r\n");
	/* Loop forever */
	for (;;) {
		/* Wait for connection. */
		error = netconn_accept(HTTPListener, &NewConnection);
		mips_flush_dcache();

		if (error == ERR_OK) {
			/* Service connection. */
			ProcessConnection(NewConnection);
			while (netconn_delete(NewConnection) != ERR_OK) {
				KRN_hibernate(&queue, DHCP_FINE_TIMER_MSECS);
			}
		}
	}
	return 0;
}
