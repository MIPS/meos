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
*   Description:	Floating point simple test
*
*************************************************************************/

#include <stdlib.h>		/* for exit! */

/* we user assert to report failures.... */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 2000		/* task stack size */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define debug	DBG_logF

static int32_t errors = 0;

static KRN_TASK_T *bgtask;
static KRN_TASK_T *timetask;
static KRN_TASK_T Task0;
static KRN_TASK_T Task1;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static KRN_TASKQ_T sleepQueue;
static uint32_t timestack[STACKSIZE];
static uint32_t Task0Stack[STACKSIZE];
static uint32_t Task1Stack[STACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_LOCK_T lock;

void
notify ()
{
  errors++;
  DBG_logF ("Test failed\n");
  _DBG_stop (__FILE__, __LINE__);
}

static void
Task0_func (void)
{
  register float f0 asm ("$f0");
  register float f1 asm ("$f1");
  register float f2 asm ("$f2");
  register float f3 asm ("$f3");
  register float f4 asm ("$f4");
  register float f5 asm ("$f5");
  register float f6 asm ("$f6");
  register float f7 asm ("$f7");
  register float f8 asm ("$f8");
  register float f9 asm ("$f9");
  register float f10 asm ("$f10");
  register float f11 asm ("$f11");
  register float f12 asm ("$f12");
  register float f13 asm ("$f13");
  register float f14 asm ("$f14");
  register float f15 asm ("$f15");
  register float f16 asm ("$f16");
  register float f17 asm ("$f17");
  register float f18 asm ("$f18");
  register float f19 asm ("$f19");
  register float f20 asm ("$f20");
  register float f21 asm ("$f21");
  register float f22 asm ("$f22");
  register float f23 asm ("$f23");
  register float f24 asm ("$f24");
  register float f25 asm ("$f25");
  register float f26 asm ("$f26");
  register float f27 asm ("$f27");

  int32_t i = 0xabcd0123;

  debug ("set up task0\n");
  f0 = *(float *) &i;
  i += 0x11;
  f1 = *(float *) &i;
  i += 0x11;
  f2 = *(float *) &i;
  i += 0x11;
  f3 = *(float *) &i;
  i += 0x11;
  f4 = *(float *) &i;
  i += 0x11;
  f5 = *(float *) &i;
  i += 0x11;
  f6 = *(float *) &i;
  i += 0x11;
  f7 = *(float *) &i;
  i += 0x11;
  f8 = *(float *) &i;
  i += 0x11;
  f9 = *(float *) &i;
  i += 0x11;
  f10 = *(float *) &i;
  i += 0x11;
  f11 = *(float *) &i;
  i += 0x11;
  f12 = *(float *) &i;
  i += 0x11;
  f13 = *(float *) &i;
  i += 0x11;
  f14 = *(float *) &i;
  i += 0x11;
  f15 = *(float *) &i;
  i += 0x11;
  f16 = *(float *) &i;
  i += 0x11;
  f17 = *(float *) &i;
  i += 0x11;
  f18 = *(float *) &i;
  i += 0x11;
  f19 = *(float *) &i;
  i += 0x11;
  f20 = *(float *) &i;
  i += 0x11;
  f21 = *(float *) &i;
  i += 0x11;
  f22 = *(float *) &i;
  i += 0x11;
  f23 = *(float *) &i;
  i += 0x11;
  f24 = *(float *) &i;
  i += 0x11;
  f25 = *(float *) &i;
  i += 0x11;
  f26 = *(float *) &i;
  i += 0x11;
  f27 = *(float *) &i;

  /* Drop us below the other task */
  KRN_priority (NULL, PRIORITIES - 3);
  KRN_release ();
  debug ("check task0\n");

  i = 0xabcd0123;

  if (f0 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f1 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f2 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f3 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f4 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f5 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f6 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f7 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f8 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f9 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f10 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f11 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f12 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f13 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f14 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f15 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f16 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f17 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f18 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f19 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f20 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f21 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f22 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f23 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f24 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f25 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f26 != *(float *) &i)
    notify ();
  i += 0x11;
  if (f27 != *(float *) &i)
    notify ();

  debug ("clear out task0 finally\n");

  f0 = *(float *) &i;
  i += 0x11;
  f1 = *(float *) &i;
  i += 0x11;
  f2 = *(float *) &i;
  i += 0x11;
  f3 = *(float *) &i;
  i += 0x11;
  f4 = *(float *) &i;
  i += 0x11;
  f5 = *(float *) &i;
  i += 0x11;
  f6 = *(float *) &i;
  i += 0x11;
  f7 = *(float *) &i;
  i += 0x11;
  f8 = *(float *) &i;
  i += 0x11;
  f9 = *(float *) &i;
  i += 0x11;
  f10 = *(float *) &i;
  i += 0x11;
  f11 = *(float *) &i;
  i += 0x11;
  f12 = *(float *) &i;
  i += 0x11;
  f13 = *(float *) &i;
  i += 0x11;
  f14 = *(float *) &i;
  i += 0x11;
  f15 = *(float *) &i;
  i += 0x11;
  f16 = *(float *) &i;
  i += 0x11;
  f17 = *(float *) &i;
  i += 0x11;
  f18 = *(float *) &i;
  i += 0x11;
  f19 = *(float *) &i;
  i += 0x11;
  f20 = *(float *) &i;
  i += 0x11;
  f21 = *(float *) &i;
  i += 0x11;
  f22 = *(float *) &i;
  i += 0x11;
  f23 = *(float *) &i;
  i += 0x11;
  f24 = *(float *) &i;
  i += 0x11;
  f25 = *(float *) &i;
  i += 0x11;
  f26 = *(float *) &i;
  i += 0x11;
  f27 = *(float *) &i;
  i += 0x11;

  debug ("done task0\n");
  KRN_removeTask (NULL);
  return;
}

static void
Task1_func (void)
{
  register float f0 asm ("$f0");
  register float f1 asm ("$f1");
  register float f2 asm ("$f2");
  register float f3 asm ("$f3");
  register float f4 asm ("$f4");
  register float f5 asm ("$f5");
  register float f6 asm ("$f6");
  register float f7 asm ("$f7");
  register float f8 asm ("$f8");
  register float f9 asm ("$f9");
  register float f10 asm ("$f10");
  register float f11 asm ("$f11");
  register float f12 asm ("$f12");
  register float f13 asm ("$f13");
  register float f14 asm ("$f14");
  register float f15 asm ("$f15");
  register float f16 asm ("$f16");
  register float f17 asm ("$f17");
  register float f18 asm ("$f18");
  register float f19 asm ("$f19");
  register float f20 asm ("$f20");
  register float f21 asm ("$f21");
  register float f22 asm ("$f22");
  register float f23 asm ("$f23");
  register float f24 asm ("$f24");
  register float f25 asm ("$f25");
  register float f26 asm ("$f26");
  register float f27 asm ("$f27");

  int32_t i = 0x12345678;

  debug ("set up task1\n");
  f0 = *(float *) &i;
  i += 0x23;
  f1 = *(float *) &i;
  i += 0x23;
  f2 = *(float *) &i;
  i += 0x23;
  f3 = *(float *) &i;
  i += 0x23;
  f4 = *(float *) &i;
  i += 0x23;
  f5 = *(float *) &i;
  i += 0x23;
  f6 = *(float *) &i;
  i += 0x23;
  f7 = *(float *) &i;
  i += 0x23;
  f8 = *(float *) &i;
  i += 0x23;
  f9 = *(float *) &i;
  i += 0x23;
  f10 = *(float *) &i;
  i += 0x23;
  f11 = *(float *) &i;
  i += 0x23;
  f12 = *(float *) &i;
  i += 0x23;
  f13 = *(float *) &i;
  i += 0x23;
  f14 = *(float *) &i;
  i += 0x23;
  f15 = *(float *) &i;
  i += 0x23;
  f16 = *(float *) &i;
  i += 0x23;
  f17 = *(float *) &i;
  i += 0x23;
  f18 = *(float *) &i;
  i += 0x23;
  f19 = *(float *) &i;
  i += 0x23;
  f20 = *(float *) &i;
  i += 0x23;
  f21 = *(float *) &i;
  i += 0x23;
  f22 = *(float *) &i;
  i += 0x23;
  f23 = *(float *) &i;
  i += 0x23;
  f24 = *(float *) &i;
  i += 0x23;
  f25 = *(float *) &i;
  i += 0x23;
  f26 = *(float *) &i;
  i += 0x23;
  f27 = *(float *) &i;

  /* Drop us below the other task */
  KRN_priority (NULL, PRIORITIES - 4);
  KRN_release ();
  debug ("check task1\n");

  i = 0x12345678;

  if (f0 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f1 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f2 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f3 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f4 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f5 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f6 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f7 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f8 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f9 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f10 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f11 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f12 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f13 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f14 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f15 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f16 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f17 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f18 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f19 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f20 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f21 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f22 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f23 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f24 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f25 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f26 != *(float *) &i)
    notify ();
  i += 0x23;
  if (f27 != *(float *) &i)
    notify ();
  i += 0x23;

  debug ("done task1\n");

  KRN_unlock (&lock);
  KRN_removeTask (NULL);
  return;
}

/*
** FUNCTION:      main
**
** DESCRIPTION:   C main program for thread 1
**
** RETURNS:       int
**
*/
int
main ()
{
  DBG_logF ("Kernel FPU simple Test\n");

  KRN_reset (&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
	     0);
  DQ_init (&sleepQueue);

  /* Acquire the lock */
  KRN_initLock (&lock);
  KRN_lock (&lock, 0);

#ifdef TEST_FAST_INTS
#ifdef TEST_ISTACK
  KRN_fastInterruptStack (istack);
#else
  KRN_fastInterruptStack (NULL);
#endif
#endif
  bgtask = KRN_startOS ("Background Task");
  timetask =
    KRN_startTimerTask ("Timer Task", timestack, TSTACKSIZE);

  /* Make sure we complete this first */
  KRN_priority (NULL, PRIORITIES - 1);
  KRN_startTask (Task0_func, &Task0, Task0Stack, STACKSIZE,
		 PRIORITIES - 1, NULL, "Task0");
  KRN_startTask (Task1_func, &Task1, Task1Stack, STACKSIZE,
		 PRIORITIES - 2, NULL, "Task1");

  /* Drop us down, and wait for the lock to be released (signalling
   * completion. */
  KRN_priority (NULL, 0);
  KRN_lock (&lock, KRN_INFWAIT);

  debug ("Done\n");

  return errors;
}
