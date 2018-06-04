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
*   Description:	SNTP library
*
*************************************************************************/

#include <time.h>
#include "lwip/sockets.h"
#include "lwip/opt.h"
#include "MEOS.h"

typedef struct __attribute__ ((__packed__)) {
	uint8_t liVnMode;
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	uint32_t rootDelay;
	uint32_t rootDispersion;
	uint32_t refId;
	uint32_t refSeconds;
	uint32_t refSplitSeconds;
	uint32_t orgSeconds;
	uint32_t orgSplitSeconds;
	uint32_t rxSeconds;
	uint32_t rxSplitSeconds;
	uint32_t txSeconds;
	uint32_t txSplitSeconds;
} SNTP_MSG_T;

int32_t SNTP_getDetailedTime(char *host, uint64_t * t)
{
	struct sockaddr_in from, to;
	int skt, opt, len, r = 0;
	SNTP_MSG_T msg;
	socklen_t alen;

	/* Handle default */
	if (!host)
		host = CONFIG_SNTP_DEFAULT;

	/* Set up socket */
	to.sin_family = AF_INET;
	to.sin_port = lwip_htons(123);
	if (!inet_aton(host, &to.sin_addr)) {
		DBG_assert(0, "Bad SNTP address!\n");
		return -EADDRNOTAVAIL;
	}
	skt = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (skt < 0) {
		DBG_assert(0, "Insufficient sockets!\n");
		return -ENOBUFS;
	}
	opt = CONFIG_SNTP_TIMEOUT;
	lwip_setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (void *)&opt,
			sizeof(int));

	/* Tx */
	memset(&msg, 0, sizeof(SNTP_MSG_T));
	msg.liVnMode = 0x23;
	alen = sizeof(struct sockaddr_in);
	lwip_sendto(skt, (void *)&msg, sizeof(SNTP_MSG_T), 0,
		    (struct sockaddr *)&to, alen);

	/* Rx */
	len =
	    lwip_recvfrom(skt, (void *)&msg, sizeof(SNTP_MSG_T), 0,
			  (struct sockaddr *)&from, &alen);
	if (len == sizeof(SNTP_MSG_T)) {
		if ((msg.liVnMode & 0x06) == 0x04)
			*t = ((uint64_t)
			      (lwip_ntohl(msg.txSeconds) -
			       2208988800UL) << 32) |
			    lwip_ntohl(msg.txSplitSeconds);
		else
			r = -ENOMSG;
	} else {
		r = -ETIMEDOUT;
	}

	lwip_close(skt);
	return r;
}

int32_t SNTP_getTime(char *host, time_t * t)
{
	uint64_t split = 0;
	int32_t r = SNTP_getDetailedTime(host, &split);
	*t = (split + 0x80000000) >> 32;
	return r;
}
