/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Dynamic demo console
*
*************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "MEOS.h"

#define GRANULARITY (CONFIG_MVZ_PAGESIZE * 2)

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

extern void MVZ_scheduleIn();

static KRN_TASKQ_T haltedTasks;
static void *gram[256];
static size_t gbytes[256];

uint32_t freeGIDs[] =
    { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff
};

#ifdef CONFIG_DRIVER_FDC
extern FDC_T *FDC_Ts[16];
reg_t freeFDCs;
#endif

char *MC_readStr(char **p)
{
	while ((**p != 0) && (isspace((int)**p)))
		(*p)++;
	char *q = strpbrk(*p, " \r\n\t"), *r = *p;
	if (q) {
		*q = 0;
		q++;
		*p = q;
	}
	return r;
}

reg_t MC_readDec(char **p)
{
	reg_t r = 0;
	while ((**p != 0) && (isspace((int)**p)))
		(*p)++;
	while (isdigit((int)**p)) {
		r = (r * 10) + (**p - '0');
		(*p)++;
	}
	return r;
}

static inline void MC_list(char *p)
{
	/* Early return if there are no live tasks */
	if (DQ_empty(&_KRN_schedule->liveTasks))
		return;
	/* Abuse taskLinks to scan tasks */
	void *last =
	    (void *)(((uintptr_t) DQ_last(&_KRN_schedule->liveTasks)) & ~1);
	void *link = DQ_first(&_KRN_schedule->liveTasks);
	MVZ_GUEST_T *guest =
	    (MVZ_GUEST_T *) (((uintptr_t) link) -
			     offsetof(MVZ_GUEST_T, task.taskLink));
	DBG_logF(GREEN);
	for (;;) {
		/* If the task is scheduled in by MVZ, it's a guest */
		if (guest->task.schedIn == MVZ_scheduleIn) {
			/* Display output */
			DBG_logF("%3" PRIu32 ": %s\n", (uint32_t) guest->gid,
				 guest->task.name);
		}
		/* Bail if last task */
		if (link == last)
			break;
		/* Otherwise follow the taskLink for the next task */
		link = DQ_next(link);
		guest =
		    (MVZ_GUEST_T *) (((uintptr_t) link) -
				     offsetof(MVZ_GUEST_T, task.taskLink));
	}
	DBG_logF(RESET);
}

static inline MVZ_GUEST_T *MVZ_guestByGid(uint8_t gid)
{
	/* Early return if there are no live tasks */
	if (DQ_empty(&_KRN_schedule->liveTasks))
		return NULL;
	/* Abuse taskLinks to scan tasks */
	void *last =
	    (void *)(((uintptr_t) DQ_last(&_KRN_schedule->liveTasks)) & ~1);
	void *link = DQ_first(&_KRN_schedule->liveTasks);
	MVZ_GUEST_T *guest =
	    (MVZ_GUEST_T *) (((uintptr_t) link) -
			     offsetof(MVZ_GUEST_T, task.taskLink));
	for (;;) {
		/* If the task is scheduled in by MVZ_scheduleIn, it's a guest */
		if ((guest->task.schedIn == MVZ_scheduleIn)
		    && (guest->gid == gid))
			return guest;
		/* Bail if last task */
		if (link == last)
			break;
		/* Otherwise follow the taskLink for the next task */
		link = DQ_next(link);
		guest =
		    (MVZ_GUEST_T *) (((uintptr_t) link) -
				     offsetof(MVZ_GUEST_T, task.taskLink));
	}
	return NULL;
}

static inline void MC_halt(char *p)
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec(&p));
	if (!gt) {
		DBG_logF(YELLOW "No such guest!" RESET "\n");
		return;
	}
	DQ_remove(gt);
	_KRN_hibernateTask((KRN_TASK_T *) gt, &haltedTasks);
}

static inline void MC_continue(char *p)
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec(&p));
	if (!gt) {
		DBG_logF(YELLOW "No such guest!" RESET "\n");
		return;
	}
	DQ_remove(gt);
	_KRN_wakeTask((KRN_TASK_T *) gt);
}

static inline void MC_restart(char *p)
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec(&p));
	if (!gt) {
		DBG_logF(YELLOW "No such guest!" RESET "\n");
		return;
	}
	DQ_remove(gt);
	_KRN_wakeTask((KRN_TASK_T *) gt);
	MVZ_restart(gt);
}

static inline void MC_nop(MVZ_GUEST_T * gt)
{
	(void)gt;
}

static void MC_stop(MVZ_GUEST_T * gt)
{
	_KRN_hibernateTask((KRN_TASK_T *) gt, &haltedTasks);
}

typedef struct {
	int file;
	size_t fsize;
	uint8_t *data;
	size_t dsize;
	uintptr_t offset;
} MC_CACHED_T;

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

static int MC_read(void *offset, void *buffer, int size, int n, void *f)
{
	uintptr_t off = (uintptr_t) offset;
	MC_CACHED_T *file = (MC_CACHED_T *) f;
	if ((off < file->offset) || (off >= (file->offset + file->dsize))) {
		lseek(file->file, off, SEEK_SET);
		read(file->file, file->data,
		     MIN(file->fsize - off, file->dsize));
		file->offset = off;
	}
	size_t len = MIN(size * n, file->dsize - (off - file->offset));
	memcpy(buffer, &file->data[off - file->offset], len);
	return len;
}

static inline void MC_open(char *p)
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec(&p));
	char *file = MC_readStr(&p);
	MC_CACHED_T f;
	if (!gt) {
		DBG_logF(YELLOW "No such guest!" RESET "\n");
		return;
	}
	DQ_remove(gt);
	/* Reset guest */
	gt->start = MC_nop;
	MVZ_restart(gt);
	/* Initialise cached file I/O */
	f.file = open(file, O_RDONLY);
	struct stat s;
	fstat(f.file, &s);
	size_t sz = s.st_size;
	f.fsize = sz;
	f.offset = sz * 2;
	sz = MIN(sz, 16777216);
	for (f.data = NULL; f.data == NULL; sz /= 2)
		f.data = malloc(sz);
	f.dsize = sz;
	/* Load it into VM */
	MVZ_loadELF(gt, MC_read, &f);
	/* Discard ELF */
	free(f.data);
	close(f.file);
	/* Prevent future reboots */
	gt->start = MC_stop;
	/* Schedule */
	_KRN_wakeTask((KRN_TASK_T *) gt);
}

void MC_dynGuestLoad(MVZ_GUEST_T * gt)
{
	reg_t gid = gt->gid;
	MVZ_zeroGP(gt, 0, gbytes[gid]);
	/* Boot and stick at KSEG0 */
	MVZ_stick(gt, 0x80000000);
	gt->task.savedContext.gp.epc = 0x80000000;
}

static inline void MC_new(char *p)
{
	reg_t gid;
	MVZ_GUEST_T *gt;
	char name[8];
	/* Get amount */
	reg_t bytes = MC_readDec(&p);
	/* Find an unused GID */
	for (gid = 1; gid < 256; gid++)
		if ((freeGIDs[gid >> 5] & (1 << (gid & 0x1f))) != 0)
			break;
	if (gid == 256) {
		DBG_logF(YELLOW "Insufficient free GIDs!" RESET "\n");
		return;
	}
	/* Allocate memory */
	void *ram = malloc(bytes + GRANULARITY);
	if (ram == NULL) {
		DBG_logF(YELLOW "Insufficient free heap!" RESET "\n");
		return;
	}
	memset(ram, 0, bytes + GRANULARITY);

	/* Allocate guest */
	gt = (MVZ_GUEST_T *) malloc(sizeof(MVZ_GUEST_T));
	if (gt == NULL) {

		free(ram);
		return;
	}

	/* Initialise guest */
	MVZ_initGuest(gt, gid, &MC_dynGuestLoad);

	/* Add TLB entries */
	MVZ_PTE_T *pte = malloc(sizeof(MVZ_PTE_T));
	DBG_logF("paddr start: %p(%p/%p)\n",
		 (void *)((MEM_v2p(ram) + GRANULARITY) & ~(GRANULARITY - 1)),
		 (void *)MEM_v2p(ram), (void *)GRANULARITY);
	MVZ_addMapping(pte, (MEM_v2p(ram) + GRANULARITY) & ~(GRANULARITY - 1),
		       bytes, gt, 0, MVZ_MEM_FLAG_ALLOWALL);

	/* Assign GID */
	freeGIDs[gid >> 5] &= ~(1 << (gid & 0x1f));

	/* Assign RAM */
	gbytes[gid] = bytes;
	gram[gid] = ram;

	VIO_T *console, *net;

#ifdef CONFIG_DRIVER_FDC
	/* Allocate FDC channel */
	if (freeFDCs) {
		reg_t ch = __builtin_ctz(freeFDCs);
		freeFDCs &= ~(1 << ch);
		/* Allocate VIO device */
		console = (VIO_T *) malloc(sizeof(VIO_CONSOLE_T));
		if (console == NULL) {
			DBG_logF(YELLOW
				 "Insufficient free heap for Virtio console device!"
				 RESET "\n");
			gram[gid] = 0;
			gbytes[gid] = 0;
			freeGIDs[gid >> 5] |= 1 << (gid & 0x1f);
			free(gt);
			free(ram);
			free(pte);
			return;
		}
		VIO_initConsole((VIO_CONSOLE_T *) console, 0x1d000000,
				5, (UART_T *) FDC_Ts[ch]);
		VIO_attach(console, gt);
		DBG_logF("Console attached to FDC channel %" PRIu32
			 "\n", (uint32_t) ch);
	} else {
#endif
		/* FDC unavailable - allocate dummy device */
		console = (VIO_T *) malloc(sizeof(VIO_DUMMY_T));
		if (console == NULL) {
			DBG_logF(YELLOW
				 "Insufficient free heap for Virtio dummy device!"
				 RESET "\n");
			gram[gid] = 0;
			gbytes[gid] = 0;
			freeGIDs[gid >> 5] |= 1 << (gid & 0x1f);
			free(gt);
			free(ram);
			free(pte);
			return;
		}
		VIO_initDummy((VIO_DUMMY_T *) console, 0x1d000000);
		VIO_attach(console, gt);
		DBG_logF("FDCs unavailable, dummy attached\n");
#ifdef CONFIG_DRIVER_FDC
	}
#endif

#ifdef CONFIG_LWIP
	struct netif *nif = netif_find("en0");
	if (nif) {
		/* Allocate VIO device */
		net = (VIO_T *) malloc(sizeof(VIO_NET_T));
		if (console == NULL) {
			DBG_logF(YELLOW
				 "Insufficient free heap for Virtio net device!"
				 RESET "\n");
			free(console);
			gram[gid] = 0;
			gbytes[gid] = 0;
			freeGIDs[gid >> 5] |= 1 << (gid & 0x1f);
			free(gt);
			free(ram);
			free(pte);
			return;
		}
		VIO_initNet((VIO_NET_T *) net, 0x1d001000, 5, nif);
		VIO_attach(net, gt);
		DBG_logF("en0 attached\n");
	} else {
#endif
		/* FDC unavailable - allocate dummy device */
		net = (VIO_T *) malloc(sizeof(VIO_DUMMY_T));
		if (console == NULL) {
			DBG_logF(YELLOW
				 "Insufficient free heap for Virtio dummy device!"
				 RESET "\n");
			free(console);
			gram[gid] = 0;
			gbytes[gid] = 0;
			freeGIDs[gid >> 5] |= 1 << (gid & 0x1f);
			free(gt);
			free(ram);
			free(pte);
			return;
		}
		VIO_initDummy((VIO_DUMMY_T *) net, 0x1d001000);
		VIO_attach(net, gt);
		DBG_logF("en0 unavailable, dummy attached\n");
#ifdef CONFIG_LWIP
	}
#endif

	gt->ipti = 7;
	sprintf(name, "VM %u", gid);
	MVZ_startGuest(gt, KRN_LOWEST_PRIORITY, strdup(name));
	DBG_logF("New VM created, gid %u\n", gid);
}

static inline void MC_kill(char *p)
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec(&p));
	if (!gt) {
		DBG_logF(YELLOW "No such guest!" RESET "\n");
		return;
	}
	reg_t gid = gt->gid;
	KRN_removeTask((KRN_TASK_T *) gt);
	mips_tlbinvalall();
	MVZ_VREGS_T *handler = (MVZ_VREGS_T *) LST_first(&gt->mappings);
	while (handler) {
		VIO_T *vio = handler->priv;
		if (handler->start == 0x1d000001) {
			freeFDCs |=
			    1 << ((FDC_T *) ((VIO_CONSOLE_T *) vio)->
				  rp)->channel;
		}
		if (handler->start & 1) {
			VIO_kill(vio);
			handler = (MVZ_VREGS_T *) LST_next((LST_T *) handler);
			free(vio);
		} else {
			void *f = handler;
			handler = (MVZ_VREGS_T *) LST_next((LST_T *) handler);
			free(f);
		}
	}
	free(gram[gid]);
	gram[gid] = 0;
	gbytes[gid] = 0;
	freeGIDs[gid >> 5] |= 1 << (gid & 0x1f);
	free((void *)gt->task.name);
	free(gt);
}

static inline void MC_help(char *p)
{
	DBG_logF("help|h: Display this help\n");
	DBG_logF("list|l: List running VMs and GIDs\n");
	DBG_logF("halt|stop|s <gid>: Halt a VM\n");
	DBG_logF("continue|cont|c <gid>: Resume a halted VM\n");
	DBG_logF("restart|reboot|r <gid>: Reboot a VM\n");
	DBG_logF("open <gid> <file>: Load an ELF via semi-hosting into a VM\n");
	DBG_logF("new|n: Allocate and create a new VM\n");
	DBG_logF("kill|k < gid>: Kill and deallocate a VM\n");
}

void MC_console()
{
	char buf[256], *p = buf, *cmd;
	uint32_t ok = 1;

	reg_t i;

	DQ_init(&haltedTasks);

#ifdef CONFIG_DRIVER_FDC
	/* Assume 5 is in use for console */
	for (i = 6; i < 16; i++)
		if (FDC_Ts[i] != NULL)
			freeFDCs |= 1 << i;
#endif

	for (;;) {
		if (ok)
			DBG_logF(GREEN);
		else
			DBG_logF(RED);
		DBG_logF("> " RESET);
		fflush(_DBG_file);
		fgets(buf, 255, _DBG_file);
		p = buf;
		cmd = MC_readStr(&p);
		ok = 1;
		if ((!strcmp(cmd, "help")) || (!strcmp(cmd, "h"))
		    || (!strcmp(cmd, "?")))
			MC_help(p);
		else if ((!strcmp(cmd, "list")) || (!strcmp(cmd, "ls"))
			 || (!strcmp(cmd, "l")))
			MC_list(p);
		else if ((!strcmp(cmd, "continue"))
			 || (!strcmp(cmd, "cont"))
			 || (!strcmp(cmd, "c")))
			MC_continue(p);
		else if ((!strcmp(cmd, "halt"))
			 || (!strcmp(cmd, "stop"))
			 || (!strcmp(cmd, "s")))
			MC_halt(p);
		else if ((!strcmp(cmd, "restart"))
			 || (!strcmp(cmd, "reboot") || (!strcmp(cmd, "r"))))
			MC_restart(p);
		else if ((!strcmp(cmd, "open")) || (!strcmp(cmd, "f")))
			MC_open(p);
		else if ((!strcmp(cmd, "new")) || (!strcmp(cmd, "n")))
			MC_new(p);
		else if ((!strcmp(cmd, "kill")) || (!strcmp(cmd, "k")))
			MC_kill(p);
		else {
			DBG_logF(YELLOW "Bad command!" RESET "\n");
			ok = 0;
		}

	}
}
