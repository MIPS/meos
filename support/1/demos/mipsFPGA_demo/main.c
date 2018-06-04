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
*   Description:        mipsFPGA demo
*
*************************************************************************/

#include <string.h>
#include "MEOS.h"
#include "gpioTask.h"
#include "web_page.h"

/* Tasks */
static void MonitorTask(void *parameters);

#define PRIORITIES 5
#define STACKSIZE 2048		/* task stack size */
#define UART_TASK_PRIORITY           ( KRN_LOWEST_PRIORITY )
#define GPIO_TASK_PRIORITY           ( KRN_LOWEST_PRIORITY )
#define MONITOR_TASK_PRIORITY           ( KRN_LOWEST_PRIORITY )
#define WEBPAGE_TASK_PRIORITY           ( KRN_LOWEST_PRIORITY )
#define MAX_PRIORITY (PRIORITIES - 1)

/* references to variables stored in bsp.c */
extern NS16550A_T ttyS0;

/* Variables MeOS requires */
static KRN_TASK_T *bgtask;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static uint32_t istack[STACKSIZE];
static KRN_TASK_T *meos_timer_task;
static uint32_t meos_timer_task_stack[STACKSIZE];

/* Variables for MeOS task gpioTask */
KRN_TASK_T gpioTaskInfo;
uint32_t gpioWorkSpace[1024];

/* Variables for MeOS task monitorTask */
KRN_TASK_T monitorTaskInfo;
uint32_t monitorWorkSpace[1024];

/* Variables for MeOS task web_page */
KRN_TASK_T webpageTaskInfo;
uint32_t webpageWorkSpace[1024];

/* References to external functions */
void gpioTask(void *parameters);
static void MonitorTask(void *parameters);

int main(void)
{
	KRN_TASKQ_T queue;

	/* Initialise MeOS */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	DQ_init(&queue);

	meos_timer_task = KRN_startTimerTask("timerTask",
					     meos_timer_task_stack,
					     sizeof(meos_timer_task_stack));

	/* Initialise hardware */
	BSP_init();

	/* Setup debug output to go through the uart */
	UART_config((UART_T *) & ttyS0, "9600N81");
	_DBG_file = UART_fopen(&ttyS0.uart, "w");
	setvbuf(_DBG_file, NULL, _IOLBF, 0);

	/* Set the GPIO task going */
	KRN_startTask((KRN_TASKFUNC_T *) gpioTask,
		      &gpioTaskInfo,
		      gpioWorkSpace,
		      1024, GPIO_TASK_PRIORITY, NULL, "GPIO Task");

	/* Setup variables for the monitor task, and start the task */
	KRN_startTask((KRN_TASKFUNC_T *) MonitorTask,
		      &monitorTaskInfo,
		      monitorWorkSpace,
		      1024, MONITOR_TASK_PRIORITY, NULL, "Monitor Task");

	/* Start the web page server task */
	KRN_startTask((KRN_TASKFUNC_T *) web_page,
		      &webpageTaskInfo,
		      webpageWorkSpace,
		      1024, WEBPAGE_TASK_PRIORITY, NULL, "Web Page Task");

	KRN_hibernate(&queue, KRN_INFWAIT);

	return 0;
}

static void MonitorTask(void *parameters)
{
	struct netif *nif = netif_find("en0");
	uint8_t ch;
	uint8_t ip[4];

	for (;;) {
		DBG_logF("\nMonitor Task\n");
		DBG_logF("<n>     Network IP addresses\n");
		DBG_logF("\n");

		// block on rx
		while (UART_read(NULL, &ch, 1, 1, &ttyS0)
		       == 0)
			KRN_delay(250000);
		switch (ch) {
		case 'n':
			ip[0] = nif->ip_addr.addr & 0xFF;
			ip[1] = (nif->ip_addr.addr >> 8) & 0xFF;
			ip[2] = (nif->ip_addr.addr >> 16) & 0xFF;
			ip[3] = (nif->ip_addr.addr >> 24) & 0xFF;
			DBG_logF("      IP:%d.%d.%d.%d\n", ip[0], ip[1], ip[2],
				 ip[3]);
			ip[0] = nif->netmask.addr & 0xFF;
			ip[1] = (nif->netmask.addr >> 8) & 0xFF;
			ip[2] = (nif->netmask.addr >> 16) & 0xFF;
			ip[3] = (nif->netmask.addr >> 24) & 0xFF;
			DBG_logF("NET MASK:%d.%d.%d.%d\n", ip[0], ip[1], ip[2],
				 ip[3]);
			ip[0] = nif->gw.addr & 0xFF;
			ip[1] = (nif->gw.addr >> 8) & 0xFF;
			ip[2] = (nif->gw.addr >> 16) & 0xFF;
			ip[3] = (nif->gw.addr >> 24) & 0xFF;
			DBG_logF(" GATEWAY:%d.%d.%d.%d\n", ip[0], ip[1], ip[2],
				 ip[3]);
			break;
		}
	}
}
