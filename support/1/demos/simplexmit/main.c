/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
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
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	Demonstration of interguest comms
*
*************************************************************************/

/*
 * This demonstrates simple interguest communication.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mips/m32c0.h>
#include "meos/config.h"

/* Shared memory, as mapped by hypervisor */
volatile unsigned int *shared = (unsigned int *)0xa1000000;
#ifndef CONFIG_MVZ_ACK_VIA_CAUSE
volatile unsigned int *cleardown = (unsigned int *)0xa2000000;
#endif
volatile unsigned int kick;

/*
** FUNCTION:      isr
**
** DESCRIPTION:   interrupt handler
**
** RETURNS:       void
*/
void __attribute__ ((interrupt("vector=hw0"))) _mips_isr_hw0(void)
{
	if (shared[32]) {
		kick = shared[32];
		shared[32] = 0;
#ifdef CONFIG_MVZ_ACK_VIA_CAUSE
		mips_biccr(CR_HWINT0);
#else
		*cleardown = 1;
#endif
	}
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
*/
int main()
{
	printf("Simple transmitter demo\n");

	unsigned int i, j;
	/* Enable interrupt */
	mips32_bissr(SR_IM2);
	for (i = 0; i < 32; i++) {
		/* Write some random nonsense */
		printf("X%u: ", i);
		for (j = 0; j < 32; j++) {
			shared[j] = rand();
			printf("%u ", shared[j]);
		}
		printf("\n");
		__sync_synchronize();
		/* Signal the other side that we're finished - the hypervisor will automatically kick the other side */
		shared[33] = i + 1;
		__sync_synchronize();
		/* Wait for the other side to acknowledge */
		while (kick != i + 1) ;
		__sync_synchronize();
	}

	return 0;
}
