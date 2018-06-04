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
*          File:	$File: //meta/fw/meos2/DEV/LISA.PARRATT/regression/template/main.c $
* Revision date:	$Date: 2015/10/06 $
*   Description:	Hypervisor communications demo
*
*************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
KRN_TASKQ_T foreverq;

MVZ_T MVZ_hyper;

extern reg_t _binary_guest0_elf[];
extern reg_t _binary_guest1_elf[];

MVZ_GUEST_T MVZ_guest0, MVZ_guest1;

/* The memory to be shared between guests */
uint32_t *shared = (uint32_t *) 0xa6080000;

/* Helper function when loading a compile in ELF */
static int _loadELF(void *paddr, void *buf, int size, int n, void *priv)
{
	memcpy((void *)buf, (void *)((uintptr_t) priv + (uintptr_t) paddr),
	       size * n);
	return size * n;
}

void MVZ_guest0Load(MVZ_GUEST_T * guest)
{
	MVZ_zeroGP(&MVZ_guest0, 0, 262144);
	MVZ_loadELF(&MVZ_guest0, _loadELF, _binary_guest0_elf);
}

void MVZ_guest1Load(MVZ_GUEST_T * guest)
{
	MVZ_zeroGP(&MVZ_guest1, 0, 262144);
	MVZ_loadELF(&MVZ_guest1, _loadELF, _binary_guest1_elf);
}

/* Check if the magic value in shared memory is set, and synthesize an
 * interrupt if so.
 */
static void prepareShared0(MVZ_VREGS_T * vregs)
{
	if ((shared[32] != 0) && (MVZ_intSet(&MVZ_guest0, 2) == 0)) {
		MVZ_upInt(&MVZ_guest0, 2);
	}
}

static void prepareShared1(MVZ_VREGS_T * vregs)
{
	if ((shared[33] != 0) && (MVZ_intSet(&MVZ_guest1, 2) == 0)) {
		MVZ_upInt(&MVZ_guest1, 2);
	}
}

#ifndef CONFIG_MVZ_ACK_VIA_CAUSE
/* Acknowledge interrupt via virtual register */
static int accessShared(void *address, void *buffer, int size, int n,
			void *priv)
{
	MVZ_downInt(((MVZ_VREGS_T *) priv)->guest, 2);
	return 0;
}
#endif

/* Virtual devices that inject and ack interrupts */
MVZ_VREGS_T shared0 = {
#ifndef CONFIG_MVZ_ACK_VIA_CAUSE
	.start = 0x2000000,
	.stop = 0x2000004,
	.read = accessShared,
	.write = accessShared,
#else
	.start = -1,
	.stop = -1,
#endif
	.prepare = prepareShared0
};

MVZ_VREGS_T shared1 = {
#ifndef CONFIG_MVZ_ACK_VIA_CAUSE
	.start = 0x2000000,
	.stop = 0x2000004,
	.read = accessShared,
	.write = accessShared,
#else
	.start = -1,
	.stop = -1,
#endif
	.prepare = prepareShared1
};

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program
**
** RETURNS:       int
*/
int main()
{
	uint32_t tlbIndex = 0;

	DBG_logF("Hypervisor comms demo\n");

	/* MEOS boiler plate */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	/* Initialise board - this configures middleware and device drivers */
	BSP_init();

	/* Timeslicing required for hypervision */
	KRN_setTimeSlice(10000);

	/* Initialise hypervisor */
	MVZ_hypervise(&MVZ_hyper, (KRN_ISRFUNC_T *) MVZ_UHIHLT);

	/* Initialise guests, provide them with memory */
	memset(shared, 0, 34 * sizeof(uint32_t));

	/* Create simplexmit guest */
	MVZ_initGuest(&MVZ_guest0, 1, &MVZ_guest0Load);
	MVZ_fixMapping(&tlbIndex, 0x6000000, 0x40000, &MVZ_guest0, 0,
		       MVZ_MEM_FLAG_ALLOWALL);
	MVZ_fixMapping(&tlbIndex, 0x6080000, 0x40000, &MVZ_guest0, 0x1000000, MVZ_MEM_FLAG_ALLOWALL);	/* Shared */
	MVZ_addRegs(&MVZ_guest0, &shared0);

	/* Create simplerecv guest */
	MVZ_initGuest(&MVZ_guest1, 2, &MVZ_guest1Load);
	MVZ_fixMapping(&tlbIndex, 0x6040000, 0x40000, &MVZ_guest1, 0,
		       MVZ_MEM_FLAG_ALLOWALL);
	MVZ_fixMapping(&tlbIndex, 0x6080000, 0x40000, &MVZ_guest1, 0x1000000, MVZ_MEM_FLAG_ALLOWALL);	/* Shared */
	MVZ_addRegs(&MVZ_guest1, &shared1);

	MVZ_splitTLBs(tlbIndex);

	MVZ_startGuest(&MVZ_guest0, KRN_LOWEST_PRIORITY, "guest 0");
	MVZ_startGuest(&MVZ_guest1, KRN_LOWEST_PRIORITY, "guest 1");

	/* Everything is prepared, so sleep and let it happen */
	DQ_init(&foreverq);
	KRN_hibernate(&foreverq, KRN_INFWAIT);
	DBG_assert(0, "Can't happen!");
	return -1;
}
