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
*   Description:        LwIP porting layer
*
*************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "MEOS.h"

#include "arch/sys_arch.h"
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

LWIP_T *_lwip;

void LWIP_init(LWIP_T * lwip)
{
	uint32_t i;

	KRN_initPool(&lwip->mboxMsgsPool, lwip->mboxMsgsArray,
		     CONFIG_LWIP_MAX_MESSAGES, sizeof(SYS_MBOX_MSG_POOL_T));
	for (i = 0; i < CONFIG_LWIP_MAX_THREADS; i++)
		lwip->tcbs[i].reason = KRN_DEAD;
	lwip->lastTick = 0;
	lwip->msecOffset = 0;
	_lwip = lwip;
}

int LWIP_up()
{
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	return _lwip == NULL ? 0 : 1;
}

err_t sys_mbox_new(sys_mbox_t * mbox, int size)
{
	err_t r = ERR_MEM;
	(void)size;

	if (mbox != NULL) {
		KRN_initMbox(mbox);
		r = ERR_OK;
	}

	return r;
}

void sys_mbox_free(sys_mbox_t * mbox)
{
}

void sys_mbox_post(sys_mbox_t * mbox, void *msg)
{
	SYS_MBOX_MSG_POOL_T *mailMsg =
	    KRN_takePool(&_lwip->mboxMsgsPool, KRN_INFWAIT);

	if (mailMsg == NULL)
		return;
	mailMsg->ptr = msg;

	KRN_putMbox(mbox, mailMsg);
}

err_t sys_mbox_trypost(sys_mbox_t * mbox, void *msg)
{
	SYS_MBOX_MSG_POOL_T *mailMsg = KRN_takePool(&_lwip->mboxMsgsPool, 0);

	if (mailMsg == NULL)
		return ERR_MEM;

	mailMsg->ptr = msg;

	KRN_putMbox(mbox, mailMsg);

	return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t * mbox, void **msg, u32_t timeout)
{
	uint64_t start, end, diff;
	SYS_MBOX_MSG_POOL_T *mailMsg;

	start = TMR_getMonotonic();

	mailMsg = KRN_getMbox(mbox, timeout > 0 ? timeout * 1000 : KRN_INFWAIT);

	if (mailMsg == NULL) {
		return SYS_ARCH_TIMEOUT;
	} else {
		if (msg)
			*msg = mailMsg->ptr;
		KRN_returnPool(mailMsg);
	}

	end = TMR_getMonotonic();

	if (start > end)
		diff = (0xffffffff - start) + 1 + end;
	else
		diff = end - start;

	return diff / (TMR_clockSpeed() * 1000);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t * mbox, void **msg)
{
	SYS_MBOX_MSG_POOL_T *mailMsg;

	mailMsg = KRN_getMbox(mbox, 0);

	if (mailMsg == NULL) {
		return SYS_MBOX_EMPTY;
	} else {
		if (msg)
			*msg = mailMsg->ptr;
		KRN_returnPool(mailMsg);
	}

	return ERR_OK;
}

err_t sys_sem_new(sys_sem_t * sem, u8_t count)
{
	if (sem == NULL)
		return ERR_MEM;
	KRN_initSemaphore(sem, count);
	return ERR_OK;
}

u32_t sys_arch_sem_wait(sys_sem_t * sem, u32_t timeout)
{
	int result;

	if (timeout == 0) {
		KRN_testSemaphore(sem, 1, KRN_INFWAIT);

		return 0;
	} else {
		result = KRN_testSemaphore(sem, 1, timeout * 1000);
		if (result == 0) {
			return SYS_ARCH_TIMEOUT;
		} else {
			return result;
		}
	}
}

err_t sys_mutex_new(sys_mutex_t * mutex)
{
	return sys_sem_new(mutex, 1);
}

void sys_mutex_lock(sys_mutex_t * mutex)
{
	sys_arch_sem_wait(mutex, KRN_INFWAIT);
}

void sys_mutex_unlock(sys_mutex_t * mutex)
{
	KRN_setSemaphore(mutex, 1);
}

void sys_mutex_free(sys_mutex_t * mutex)
{
	KRN_returnPool(mutex);
}

void sys_sem_signal(sys_sem_t * sem)
{
	KRN_setSemaphore(sem, 1);
}

void sys_sem_free(sys_sem_t * sem)
{
}

static void sys_thread_start(void)
{
	uint32_t i = (uint32_t) KRN_taskParameter(NULL);
	void (*fn) (void *) = (void (*)(void *))_lwip->fn[i];
	fn(_lwip->par[i]);
}

sys_thread_t sys_thread_new(const char *name, void (*thread) (void *parameters),
			    void *arg, int stacksize, int priority)
{
	uint32_t i;
	KRN_TASK_T *t = NULL;
	uint32_t *s = NULL;

	for (i = 0; i < CONFIG_LWIP_MAX_THREADS; i++)
		if (_lwip->tcbs[i].reason == KRN_DEAD) {
			t = &_lwip->tcbs[i];
			s = _lwip->stacks[i];
			_lwip->fn[i] = (void *)thread;
			_lwip->par[i] = arg;
			break;
		}
	if (t == NULL) {
		DBG_assert(t, "lwIP ran out of tasks!\n");
		return NULL;
	}

	DBG_assert(stacksize <= CONFIG_LWIP_MAX_STACK,
		   "lwIP requested too much stack when creating task!\n");

	KRN_startTask(sys_thread_start, t, s, stacksize, priority, (void *)i,
		      name);

	return (sys_thread_t) t;
}

sys_prot_t sys_arch_protect(void)
{
	return IRQ_raiseIPL();
}

void sys_arch_unprotect(sys_prot_t ipl)
{
	IRQ_restoreIPL(ipl);
}

void sys_init(void)
{
}

__attribute__ ((weak))
struct netif *LWIP_filter(struct pbuf *pbuf, struct netif *netif, u16_t type)
{
	return netif;
}
