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
*   Description:	Linux kernel specialisation
*
*************************************************************************/

#ifndef TARGET_KRN_H
#define TARGET_KRN_H

#include <pthread.h>
#include <signal.h>
#ifdef CONFIG_ARCH_LINUX_MIPS
#include <sys/cachectl.h>
#endif

#define INLINE

typedef struct {
	PARAHEAD
	pthread_t thread;
	volatile int32_t first;
	volatile int32_t die;
	pthread_barrier_t bar;
	PARATAIL
} _KRN_DISPOSABLE_CTX_T;

typedef struct KRN_ctx_tag {
	PARAHEAD
	_KRN_DISPOSABLE_CTX_T *disp;
	uint32_t magic;
	uint32_t trace;
	KRN_TASKFUNC_T *task_func;
	PARATAIL
} KRN_CTX_T;

typedef struct sigaction KRN_ISRTABLE_T[32];

static inline void KRN_schedulerInterrupt(void)
 __attribute__ ((no_instrument_function));
static inline void KRN_schedulerInterrupt(void)
{
	pthread_kill(pthread_self(), SIGUSR1);
}

/*
* Call the scheduler from a non-interruptible context. Restore the requested IPL, and wait
* for the schedule event to occur if we are now back at interruptible level.
*/
static inline void KRN_scheduleUnprotect(IRQ_IPL_T oldIPL)
__attribute__ ((no_instrument_function));
static inline void KRN_scheduleUnprotect(IRQ_IPL_T oldIPL)
{
	extern volatile int32_t _KRN_schedNeeded;
	_KRN_schedNeeded = 1;
	IRQ_restoreIPL(oldIPL);
	KRN_schedulerInterrupt();
}

static inline void KRN_schedule()
__attribute__ ((no_instrument_function));
static inline void KRN_schedule()
{
	extern volatile int32_t _KRN_schedNeeded;
	_KRN_schedNeeded = 1;
	KRN_schedulerInterrupt();
}

/*
* Call the scheduler from a non-interruptible context. Do not wait for the underlying KICK to complete
* or we will deadlock. Rely on the fact that a enough code will be executed to get back to interruptible
* level to let the KICK filter through and arrive in a timely manner.
*/
static inline void KRN_scheduleProtected()
__attribute__ ((no_instrument_function));
static inline void KRN_scheduleProtected()
{
	extern volatile int32_t _KRN_schedNeeded;
	_KRN_schedNeeded = 1;
}

#define KRN_refreshCache(ADDR, SIZE) __extension__ ({void *addr = (ADDR); size_t size = (SIZE); KRN_flushCache(addr, size, KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D); KRN_preloadCache(addr, size, 0);})

/* Test for GCC >= 4.3.6 */
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ >= 6)))
#define KRN_syncCache(ADDR, SIZE) __builtin___clear_cache((char*)(ADDR), (char*)((ADDR)+(SIZE)))
#else
#define KRN_syncCache(ADDR, SIZE)
#warning No __builtin___clear_cache() - consider upgrading tool chain
#endif

#define KRN_barrier(FLAGS) __sync_synchronize()

static inline void KRN_flushCache(void *address, int32_t size, int32_t flags)
{
#ifdef CONFIG_ARCH_LINUX_MIPS
	int cache = 0;
	if (flags && KRN_FLUSH_FLAG_I)
		cache |= ICACHE;
	if (flags && KRN_FLUSH_FLAG_D)
		cache |= DCACHE;
	_flush_cache((char*)address, (int)size, cache);
#else
	/* Test for GCC >= 4.3.6 */
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ >= 6)))
	/* Can't flush dcache generically from user space */
	if (flags & KRN_FLUSH_FLAG_I)
		__builtin___clear_cache((int8_t *)address,
					(int8_t *)(((uintptr_t) address) + size));
#else
#warning No __builtin___clear_cache() - consider upgrading tool chain
#endif
#endif
}

static inline void KRN_preloadCache(void *paddr, size_t size, int32_t flags)
{
	int8_t *addr = (int8_t *)paddr;
	for (; size; size--) {
		switch (flags) {
		default:
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_NORMAL):
			__builtin_prefetch(addr, 0, 1);
			break;
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_STREAMED):
			__builtin_prefetch(addr, 0, 0);
			break;
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_RETAINED):
			__builtin_prefetch(addr, 0, 3);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_NORMAL):
			__builtin_prefetch(addr, 1, 1);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_STREAMED):
			__builtin_prefetch(addr, 1, 0);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_RETAINED):
			__builtin_prefetch(addr, 1, 3);
			break;
		}
		addr++;
	}
}

#endif
