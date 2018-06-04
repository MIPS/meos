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
*   Description:	Stack overflow test
*
*************************************************************************/

/*
 * This test checks that a stack overflow asserts
 */

#include <alloca.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEOS.h"

#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static KRN_TASK_T testTask;
static volatile uint32_t gap1[STACKSIZE];
static uint32_t testStack[STACKSIZE];
static volatile uint32_t gap2[STACKSIZE];
static uint32_t istack[STACKSIZE];
static volatile uint32_t gap3[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

int32_t _DBG_assert(const char *file, const int line, const char *message, ...)
{
	(void)file;
	(void)line;
	(void)message;
	DBG_logF("Overflowed! Success!\n");
	exit(0);
}

static void inner_test_func(volatile uint32_t * dummy)
    __attribute__ ((noreturn));
static void inner_test_func(volatile uint32_t * dummy)
{
	*dummy = 0;
	KRN_priority(NULL, KRN_LOWEST_PRIORITY);
	KRN_release();
	DBG_logF("Shouldn't happen, internal exit!");
	exit(-1);
}

static void test_func(void) __attribute__ ((noreturn));
static void test_func(void)
{
	void *dummy = alloca(1 + sizeof(*testStack) * STACKSIZE);
	inner_test_func((uint32_t *) dummy);
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
	DBG_logF("Stack overflow Test\n");

	(void)gap1;
	(void)gap2;
	(void)gap3;

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);

	bgtask = KRN_startOS("Background Task");
	BSP_init();
	KRN_startTask(test_func, &testTask, testStack, STACKSIZE,
		      MAX_PRIORITY, NULL, "Test");
	KRN_priority(NULL, KRN_LOWEST_PRIORITY + 1);
	KRN_release();
	DBG_logF("Shouldn't happen, external exit!");
	return -1;
}
