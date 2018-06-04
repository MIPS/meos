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
*   Description:	TFTP library
*
*************************************************************************/

#include <lwip/sockets.h>
#include "MEOS.h"

enum {
	RRQ = 1,
	ACK = 4,
	ERR = 5
};

enum {
	FILENOTFOUND = 1,
	ACCESSVIOLATION = 2,
	DISKFULL = 3,
	ILLEGALOP = 4,
	BADID = 5,
	EXISTS = 6,
	NOUSER = 7
};

#define HEADER_SIZE	4
#define PAYLOAD_SIZE	512

void TFTP_init(TFTP_T * tftp, const char *host, const char *filename)
{
	tftp->host = host;
	tftp->filename = filename;
	tftp->port = 69;
}

int TFTP_read(void *offset, void *recv_buffer, int size, int n, void *vtftp)
{
	TFTP_T *tftp = (TFTP_T *) vtftp;
	size_t limit = size * n, paylen, delta, written = 0, rxd = 0;
	char *payload;
	struct sockaddr_in from, to;
	int skt, opt, len;
	socklen_t alen;
	char msg[HEADER_SIZE + PAYLOAD_SIZE];
	char *w = (char *)recv_buffer;

	/* Set up socket */
	to.sin_family = AF_INET;
	to.sin_port = lwip_htons(tftp->port);
	if (!inet_aton(tftp->host, &to.sin_addr)) {
		DBG_assert(0, "Bad TFTP address!\n");
		return -EADDRNOTAVAIL;
	}
	skt = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (skt < 0) {
		DBG_assert(0, "Insufficient sockets!\n");
		return -ENOBUFS;
	}
	opt = CONFIG_TFTP_TIMEOUT;
	lwip_setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (void *)&opt,
			sizeof(int));

	/* Read ReQuest */
	msg[0] = 0;
	msg[1] = RRQ;
	/* File name */
	len = strlen(tftp->filename);
	memcpy(&msg[2], tftp->filename, len);
	/* Format */
	memcpy(&msg[len + 2], "\x00octet\x00", 7);
	len += 9;
	/* Tx */
	alen = sizeof(struct sockaddr_in);
	lwip_sendto(skt, msg, len, 0, (struct sockaddr *)&to, alen);

	/* Rx */
	do {
		len = lwip_recvfrom(skt,
				    msg,
				    sizeof(msg),
				    0, (struct sockaddr *)&from, &alen);
		if (len <= 0)
			break;

		if (msg[1] == ERR) {
			lwip_close(skt);
			switch (msg[3]) {
			case FILENOTFOUND:
				return -ENOENT;
			case ACCESSVIOLATION:
				return -EACCES;
			case DISKFULL:
				return -ENOMEM;
			case ILLEGALOP:
				return -EOPNOTSUPP;
			case BADID:
				return -EBADF;
			case EXISTS:
				return -EEXIST;
			case NOUSER:
				return -EINVAL;
			case 0:	/* Not defined */
			default:
				return -EFAULT;
			}
		}

		/* If there's payload data */
		if (len > HEADER_SIZE) {
			payload = msg + HEADER_SIZE;
			paylen = len - HEADER_SIZE;
			if (rxd < (size_t) offset) {
				delta =
				    (((size_t) offset - rxd) <
				     paylen) ? ((size_t) offset - rxd) : paylen;
				rxd += delta;
				payload += delta;
				paylen -= delta;
			}
			if ((paylen > 0) && (written < limit)) {
				delta =
				    (paylen <
				     (limit - written)) ? paylen : (limit -
								    written);
				memcpy(w, payload, delta);
				w += delta;
				written += delta;
				rxd += delta;
			}

			/* ACK */
			msg[0] = 0;
			msg[1] = ACK;
			alen = sizeof(struct sockaddr_in);
			lwip_sendto(skt,
				    msg,
				    HEADER_SIZE,
				    0, (struct sockaddr *)&from, alen);
		}
		/* Short packet indicate EOF */
	} while (len == (HEADER_SIZE + PAYLOAD_SIZE));

	lwip_close(skt);

	if (len <= 0)
		return -ETIMEDOUT;
	return written;
}
