/***(C)2011***************************************************************
*
* Copyright (C) 2011 MIPS Tech, LLC
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
****(C)2011**************************************************************/

/*************************************************************************
*
*   Description:	Timer IRQ test
*
*************************************************************************/

#include <stdio.h>

#include <MEOS.h>

#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES-1)

#define TSTACKSIZE 2000

#define PRIORITIES 5

#define STACKSIZE 1000
static uint32_t istack[STACKSIZE];
static uint32_t timestack[TSTACKSIZE];

/* MeOS scheduler data structures */
KRN_SCHEDULE_T sched;
KRN_TASKQ_T schedQueues[PRIORITIES];

typedef void VOIDFUNC_T(void);
extern VOIDFUNC_T *_IntrusiveHook;

volatile static int numTicks = 0;

static void OnTimer(void)
{
	numTicks++;
}

int main()
{
	/* Standard MeOS startup */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0x73117a1e, istack,
		  STACKSIZE, NULL, 0);

	KRN_startOS("Startup task");

	KRN_startTimerTask("Timer Task", (uint32_t *) timestack,
			   TSTACKSIZE);

	BSP_init();

	_IntrusiveHook = OnTimer;

	printf("Hello world %" PRIx32 "\n", (uint32_t) mips_getcount());

	while (numTicks < 32) {
		__asm__("wait\n\t");
	}

	return 0;
}
