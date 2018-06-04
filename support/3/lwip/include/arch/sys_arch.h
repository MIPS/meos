#ifndef LWIP_SYS_ARCH_H
#define LWIP_SYS_ARCH_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "meos/debug/dbg.h"
#include "meos/kernel/krn.h"

#define NUM_SEMS	16
#define NUM_MBOXES	16
#define NUM_MBOX_MSGS	32

typedef struct {
	KRN_POOLLINK;
	int32_t tag;
	void *ptr;
} SYS_MBOX_MSG_POOL_T;

typedef struct {
	KRN_POOL_T mboxMsgsPool;
	SYS_MBOX_MSG_POOL_T mboxMsgsArray[CONFIG_LWIP_MAX_MESSAGES];
	uint32_t stacks[CONFIG_LWIP_MAX_THREADS][CONFIG_LWIP_MAX_STACK];
	KRN_TASK_T tcbs[CONFIG_LWIP_MAX_THREADS];
	void *fn[CONFIG_LWIP_MAX_THREADS];
	void *par[CONFIG_LWIP_MAX_THREADS];
	uint32_t lastTick;
	uint32_t msecOffset;
} LWIP_T;

void LWIP_init(LWIP_T * lwip);
int LWIP_up(void);

typedef KRN_MAILBOX_T sys_mbox_t;
typedef KRN_SEMAPHORE_T sys_sem_t;
typedef KRN_SEMAPHORE_T sys_mutex_t;
typedef KRN_TASK_T *sys_thread_t;

#define SYS_MBOX_NULL	NULL
#define SYS_SEM_NULL	NULL
#define SYS_DEFAULT_THREAD_STACK_DEPTH	1024

static inline int sys_mbox_valid(sys_mbox_t * mbox)
{
	/* Work around for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119 */
	struct {
		char f0;
		char fn[sizeof(sys_mbox_t) - 1];
	} invalidMbox = {0};
	return memcmp(mbox, (sys_mbox_t*)&invalidMbox, sizeof(sys_mbox_t));
}

static inline void sys_mbox_set_invalid(sys_mbox_t * mbox)
{
	const struct {
		char f0;
		char fn[sizeof(sys_mbox_t) - 1];
	} invalidMbox = {0};
	*mbox = *(sys_mbox_t*)&invalidMbox;
}

static inline int sys_sem_valid(sys_sem_t * sem)
{
	struct {
		char f0;
		char fn[sizeof(sys_sem_t) - 1];
	} invalidSem = {0};
	return memcmp(sem, (sys_sem_t*)&invalidSem, sizeof(sys_sem_t));
}

static inline void sys_sem_set_invalid(sys_sem_t * sem)
{
	const struct {
		char f0;
		char fn[sizeof(sys_sem_t) - 1];
	} invalidSem = {0};
	*sem = *(sys_sem_t*)&invalidSem;
}

static inline void sys_assert(const char *pcMessage)
{
	DBG_assert(0, "LWIP: %s\n", pcMessage);
}

static inline void sys_debug(const char *const fmt, ...)
{
	va_list ap;
	static char buff[2048];

	va_start(ap, fmt);
	(void)vsprintf(buff, fmt, ap);
	strcat(buff, "\r");
	va_end(ap);
	DBG_logF("%s", buff);
}

static inline u32_t sys_now(void)
{
	extern LWIP_T *_lwip;
	uint32_t now = TMR_getMonotonic();
	uint32_t factor = TMR_clockSpeed() * 1000;
	if (now < _lwip->lastTick)
	_lwip->msecOffset += 0x100000000ULL / factor;
	_lwip->lastTick = now;
	return _lwip->msecOffset + (now/factor);
}

#endif				/* LWIP_SYS_ARCH_H */
