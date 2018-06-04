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
*   Description:        SRTC demo
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

#define STACKSIZE 2000		/* task stack size */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

/*------------------------------------------------------------*/
static KRN_TASK_T *bgtask;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static uint32_t istack[STACKSIZE];
static KRN_TASK_T *meos_timer_task;
static uint32_t meos_timer_task_stack[2048];

static SRTC_T srtc;

int main(void)
{
	struct netif *nif;
	time_t t;

	/* Initialise MeOS */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");

	meos_timer_task =
	    KRN_startTimerTask("timerTask", meos_timer_task_stack,
			       sizeof(meos_timer_task_stack));

	BSP_init();

	nif = netif_find("en0");
	netifapi_netif_set_default(nif);

	/* Specify host name, and start DHCP */
	netif_set_hostname(nif, "srtc");

	DBG_logF("Using DHCP to acquire IP address\r\n");
	netifapi_netif_set_up(nif);
	netifapi_dhcp_start(nif);

	while (nif->ip_addr.addr == 0L) {
		KRN_delay(DHCP_FINE_TIMER_MSECS * 1000);
		KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	}

	/* Start RTC */
	SRTC_init(&srtc);
	SRTC_ntp();

	/* Display time */
	for (;;) {
		t = SRTC_now();
		DBG_logF("%s", ctime(&t));
		KRN_delay(1000000);
	}

	return 0;
}
