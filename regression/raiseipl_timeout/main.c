/***(C)2009***************************************************************
*
* Copyright (C) 2009 MIPS Tech, LLC
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
****(C)2009**************************************************************/

/*************************************************************************
*
*   Description:	Raise IPL test
*
*************************************************************************/
#include <stdlib.h>		/* for exit! */
#include <string.h>		/* for exit! */

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000		/* task stack size */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)
#define CLEARFLAGS 1

#define debug	DBG_logF

static int32_t errors = 0;

void notify()
{
	errors++;
	DBG_logF("Test failed\n");
	_DBG_stop(__FILE__, __LINE__);
}

int main2(void);

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
uint32_t istack[STACKSIZE];

typedef struct {
	LST_LINK;
	int8_t *text;
} MESSAGE_T;
MESSAGE_T *message;
KRN_MAILBOX_T mbox;

#define POOLITEMS 1
typedef struct {
	KRN_POOLLINK;
	int32_t itemNumber;
} POOLITEM_T;
KRN_POOL_T pool;
POOLITEM_T poolbuf[POOLITEMS];
KRN_FLAGCLUSTER_T efcluster;
KRN_SEMAPHORE_T sem;

IRQ_IPL_T IPL_preamble(const char *test)
{
	IRQ_IPL_T oldipl;
	/* Set the current task to have not timed out */
	_KRN_current->timedOut = (KRN_TIMEOUT_T) 0;
	/* Show that we have not yet timed out */
	DBG_logF("Timed out value before %s test: %d\n", test,
		 _KRN_current->timedOut);

	/* Raise IPL to simulate an ISR or a critical region */
	oldipl = IRQ_raiseIPL();

	return oldipl;
}

void IPL_postamble(IRQ_IPL_T oldipl, const char *test)
{
	/* Exit the critical region */
	IRQ_restoreIPL(oldipl);

	/* Now did it try and run the time out system */
	DBG_logF("Timed out value after %s test: %d\n", test,
		 _KRN_current->timedOut);

	if (_KRN_current->timedOut != 0)
		notify();
}

/*
** FUNCTION:	  main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:	   int
**				This program will check that getMbox does not attempt to
**				timeout when in an elevated IPL
*/
int main(void)
{
	IRQ_IPL_T oldipl;

	DBG_logF("Kernel IPL mbox Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	KRN_startOS("Background Task");

	BSP_init();

	/* Create a mailbox */
	KRN_initMbox(&mbox);

	/* create a pool and allocate all the items */
	KRN_initPool(&pool, poolbuf, POOLITEMS, sizeof(POOLITEM_T));

	/* initialise an event flag cluster */
	KRN_initFlags(&efcluster);

	/* Initialise a semaphore */
	KRN_initSemaphore(&sem, 0);

	message = (MESSAGE_T *) malloc(sizeof(MESSAGE_T));
	/* Put a message in the mailbox */
	KRN_putMbox(&mbox, message);

	/* Set up the correct IPL */
	oldipl = IPL_preamble("mailbox");

	/* This should not try and time out. However, if MEOS treats a non-zero IPL
	   as meaning interrupts enabled it will take the interruptable path and
	   attempt to set up the time out system. With a timeout of zero this does
	   not actually happen but it does set the timedOut flag */
	message = (MESSAGE_T *) KRN_getMbox(&mbox, 0);

	/* restore IPL */
	IPL_postamble(oldipl, "mailbox");

	/* Set up the correct IPL */
	oldipl = IPL_preamble("pool");

	/* Now do the same test for pools */
	KRN_takePool(&pool, 0);

	/* restore IPL */
	IPL_postamble(oldipl, "pool");

	/* Set up the correct IPL */
	oldipl = IPL_preamble("flags");

	/* Now do the same test for flags */
	KRN_testFlags(&efcluster, 0x0ff, KRN_ANY, CLEARFLAGS, 0);

	/* restore IPL */
	IPL_postamble(oldipl, "flags");

	/* Set up the correct IPL */
	oldipl = IPL_preamble("semaphore");

	/* Now do the same test for semaphores */
	KRN_testSemaphore(&sem, 3, 0);

	/* restore IPL */
	IPL_postamble(oldipl, "semaphore");

	debug("Done\n");

	return errors;
}
