/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	Linux IPM specialisation
*
*************************************************************************/

#define _GNU_SOURCE
#include "meos/config.h"

#ifdef CONFIG_FEATURE_IMPEXP

#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "meos/debug/dbg.h"
#include "meos/mem/mem.h"
#include "meos/kernel/krn.h"
#include "meos/ipm/ipm.h"

pthread_t _IPM_wtask, _IPM_ltask;

typedef struct {
	uint32_t seq;
	KRN_CMD_T cmd;
	KRN_SUBCMD_T subCmd;
	uint32_t cID;
	uint32_t sID;
	uint32_t p64a;
	uint32_t p64b;
} _IPM_PAYLOAD_T;

typedef struct IPM_tag {
	void *ringAddr[CONFIG_FEATURE_MAX_PROCESSORS]
	    [CONFIG_FEATURE_MAX_PROCESSORS];
	KRN_OS_T os[CONFIG_FEATURE_MAX_PROCESSORS];
	int wkPipe[2];
	int rxPipe[2];
	int nodes[CONFIG_FEATURE_MAX_PROCESSORS];
} IPM_T;

static IPM_T _IPM;

extern void _IRQ_synthesizeNP(IRQ_DESC_T * irqDesc);

static void _IPM_recv(int32_t sigNum)
{
	KRN_MSG_T msg;
	IRQ_ack(IRQ_cause(sigNum));

	while (read(_IPM.rxPipe[0], &msg.from, KRN_MSG_SIZE) == 32) {
		if (msg.from != 0)
			IPM_recv(&msg);
	}
}

IRQ_DESC_T _IPM_IRQ = {
	.intNum = IRQ_INTERNAL_IPM,.isrFunc = _IPM_recv
};

static void *_IPM_listen(void *dummy)
{
	KRN_MSG_T msg;
	int m, r, n, i;
	fd_set rfds;
	struct timeval tv;

	/* Read loop */
	for (;;) {
		FD_ZERO(&rfds);
		m = _IPM.wkPipe[0];
		FD_SET(m, &rfds);
		for (i = 0; i < CONFIG_FEATURE_MAX_PROCESSORS; i++) {
			n = _IPM.nodes[i];
			if (n) {
				FD_SET(n, &rfds);
				if (n > m)
					m = n;
			}
		}
		m++;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		r = select(m, &rfds, NULL, NULL, &tv);
		if (r == -1)
			break;
		else if (r) {
			if (FD_ISSET(_IPM.wkPipe[0], &rfds)) {
				if (read
				    (_IPM.wkPipe[0], &msg.from,
				     KRN_MSG_SIZE) > 0) {
					_IPM_impexpDebug("RL", &msg);
					if (write
					    (_IPM.rxPipe[1], &msg.from,
					     KRN_MSG_SIZE) > 0)
						_IRQ_synthesizeNP(&_IPM_IRQ);
				}
			}
			for (i = 0; i < CONFIG_FEATURE_MAX_PROCESSORS; i++) {
				n = _IPM.nodes[i];
				if ((n) && FD_ISSET(n, &rfds)) {
					if (read(n, &msg.from, KRN_MSG_SIZE) >
					    0) {
						_IPM_impexpDebug("RL", &msg);
						if (write
						    (_IPM.rxPipe[1], &msg.from,
						     KRN_MSG_SIZE) > 0)
							_IRQ_synthesizeNP
							    (&_IPM_IRQ);
					}
				}
			}
		}
	}

	return NULL;
}

void _IPM_open(char *name)
{
	int fd;
	KRN_MSG_T msg;
	fd_set rfds;
	struct timeval tv;

	PARACHECK();

	for (;;) {
		/* Open vport */
		fd = open(name, O_RDWR);
		/* Send an announce to wake the other end */
		msg.from = _KRN_schedule->hwThread;
		msg.to = -1;
		msg.cmd = KRN_COMMAND_MEM_ANNOUNCE;
		msg.sID = 0;
		msg.cID = 0;
		msg.p64a = 0;
		while (write(fd, &msg.from, KRN_MSG_SIZE) <= 0) ;
		/* Intercept first packet */
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(fd + 1, &rfds, NULL, NULL, &tv) == 1)
			break;
		close(fd);
	}
	if (read(fd, &msg.from, KRN_MSG_SIZE) > 0) {
		_IPM.nodes[msg.from] = fd;
		if (write(_IPM.rxPipe[1], &msg.from, KRN_MSG_SIZE) > 0)
			_IRQ_synthesizeNP(&_IPM_IRQ);
	}

	PARACHECK();
}

void *_IPM_watch(void *dummy);

/*
** FUNCTION:	IPM_start
**
** DESCRIPTION:	Deferred initialisation performed once the scheduler is running.
**
** RETURNS:	void
*/
void IPM_start()
{
	char *globspec;
	glob_t globbuf;
	int i;
	/* Register handler */
	IRQ_route(&_IPM_IRQ);
	/* Create read and wakeup pipe */
	if (!pipe2(_IPM.rxPipe, O_NONBLOCK)) {
		if (!pipe2(_IPM.wkPipe, O_NONBLOCK)) {
			/* create watcher thread */
			pthread_create(&_IPM_wtask, NULL,
				       (void *(*)(void *))_IPM_watch, NULL);
			/* glob preexisting devices */
			globspec = getenv("MEOS_VPORT_GLOB");
			if (!globspec)
				globspec = "/dev/vport*";
			glob(globspec, 0, NULL, &globbuf);
			for (i = 0; i < globbuf.gl_pathc; i++)
				_IPM_open(globbuf.gl_pathv[i]);
			/* create listener thread */
			pthread_create(&_IPM_ltask, NULL,
				       (void *(*)(void *))_IPM_listen, NULL);
		}
	}
}

/*
** FUNCTION:	IPM_send
**
** DESCRIPTION:	Send a message to another processor.
**
** RETURNS:	void
*/
void IPM_send(KRN_MSG_T * msg)
{
	PARACHECK();
	_IPM_impexpDebug("TX", msg);
	if (_IPM.nodes[msg->to]) {
		while (write(_IPM.nodes[msg->to], &msg->from, KRN_MSG_SIZE) <=
		       0) ;
	}
	PARACHECK();
}
#endif
