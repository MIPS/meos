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
*   Description:	MIPS GIC interrupt specialisation
*
*************************************************************************/

#ifndef TARGET_IRQ1_H
#define TARGET_IRQ1_H

#define IRQ_ACTIVE_HIGH 1
#define IRQ_ACTIVE_LOW	0
#define IRQ_RISING_EDGE IRQ_ACTIVE_HIGH
#define IRQ_FALLING_EDGE	IRQ_ACTIVE_LOW
#define IRQ_LEVEL_SENSITIVE	0
#define IRQ_EDGE_TRIGGERED	1
#define IRQ_EDGE_DOUBLE_TRIGGERED	2

typedef struct {
	int32_t core; /* *NB* 1 indexed, unlike the rest of MEOS, 0 means self */
	int32_t extNum;
	int32_t polarity;
	int32_t trigger;
#ifdef CONFIG_ARCH_MIPS_VZ
	int32_t guest;
#endif
	/* Internal to MEOS, do not use */
	volatile uint32_t *pend;
	uint32_t bit;
} IRQ_IMPDESC_T;

#endif
