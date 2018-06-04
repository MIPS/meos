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
*   Description:	ChipKit Wi-Fire GPIO demo
*
*************************************************************************/

/*
 * This demonstrates GPIO facilities on the ChipKit Wi-Fire. An interrupt
 * is associated with button presses, the buttons are read, and the LEDs
 * are changed in response.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

IRQ_DESC_T portAInt;

#define BTN1 (1 << 5)
#define BTN2 (1 << 4)
#define LD1	(1 << 6)
#define LD2 (1 << 4)
#define LD3 (1 << 11)
#define LD4 (1 << 15)

void portAISR(int32_t sig)
{
	static uint64_t lastTime;
	IRQ_DESC_T *desc = IRQ_ack(IRQ_cause(sig));
	uint32_t *count = (uint32_t *) desc->priv;

	/* Very naive debouncing! */
	if (labs(TMR_getMonotonic() - lastTime) < 1000)
		return;
	lastTime = TMR_getMonotonic();

	if (PORTA & BTN1)
		(*count)--;
	if (PORTA & BTN2)
		(*count)++;
	if (*count & 1)
		LATGSET = LD1;
	else
		LATGCLR = LD1;
	if (*count & 2)
		LATDSET = LD2;
	else
		LATDCLR = LD2;
	if (*count & 4)
		LATBSET = LD3;
	else
		LATBCLR = LD3;
	if (*count & 8)
		LATGSET = LD4;
	else
		LATGCLR = LD4;
}

uint32_t count;

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main()
{
	DBG_logF("ChipKit Wi-Fire GPIO Demo\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	//KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);
	(void)timestack;

	/* Pin control */
	RPD4R = 0;
	RPG6R = 0;
	/* Buttons */
	ANSELACLR = BTN1 | BTN2;
	PORTACLR = BTN1 | BTN2;
	TRISASET = BTN1 | BTN2;
	ODCACLR = BTN1 | BTN2;
	CNENASET = BTN1 | BTN2;
	CNCONASET = 0x8000;
	/* LEDs */
	LATBCLR = LD3;
	TRISBCLR = LD3;
	ODCBCLR = LD3;
	LATDCLR = LD2;
	TRISDCLR = LD2;
	ODCDCLR = LD2;
	LATGCLR = LD1 | LD4;
	TRISGCLR = LD1 | LD4;
	ODCGCLR = LD1 | LD4;

	/* Attach interrupt */
	portAInt.intNum = 118;
	portAInt.priv = (void *)&count;
	portAInt.isrFunc = portAISR;
	portAInt.impSpec.priority = 0;
	portAInt.impSpec.polarity = IRQ_RISING_EDGE;
	IRQ_route(&portAInt);

	/* Sleep forever */
	KRN_TASKQ_T q;
	DQ_init(&q);
	KRN_hibernate(&q, KRN_INFWAIT);

	return 0;
}
