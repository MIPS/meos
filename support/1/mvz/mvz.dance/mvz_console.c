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
*   Description:    VirtIO common
*
*************************************************************************/

#include "mvz/MVZ.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

static inline void MC_list()
{
	fputs(GREEN);
	//for each task
	//if guest, printf("%3" PRIu32 ": %s\n", guest->gid, guest->task->name);
	fputs(RESET);
}

static inline MVZ_GUEST_T *MVZ_guestByGid(uint8_t gid)
{
}

static inline void MC_halt()
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec());
	DQ_remove(gt);
	_KRN_hibernateTask(gt, _MVZ_ > haltedTasks);
}

static inline void MC_continue()
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec());
	DQ_remove(gt);
	_KRN_wakeTask(gt);
}

static inline void MC_restart()
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec());
	DQ_remove(gt);
	_KRN_wakeTask(gt);
	MVZ_restart(gt);
}

static inline void MC_nop(MVZ_GUEST_T * gt)
{
	(void);
}

static inline void MC_stop(MVZ_GUEST_T * gt)
{
	_KRN_hibernateTask(gt, _MVZ_ > haltedTasks);
}

static inline void MC_tftp()
{
	MVZ_GUEST_T *gt = MVZ_guestByGid(MC_readDec());
	char *host = MC_readStr();
	char *file = MC_readStr();
	TFTP_T t;

	DQ_remove(gt);
	/* Reset guest */
	gt->start = MC_nop;
	MVZ_restart(gt);
	/* Load ELF */
	TFTP_init(&t, host, file);
	MVZ_loadELF(gt, TFTP_read, &t);
	TFTP_close(&t);
	/* Prevent future reboots */
	gt->start = MC_stop;
}

void MVZ_console()
{
	char buf[256], *p = buf, *cmd;
	uint32_t ok = 1;
	for (;;) {
		if (ok)
			fputs(GREEN);
		else
			fputs(RED);
		fputs("> " RESET);
		fgets(cmd, 255, stdin);
		cmd = readStr(&p);
		ok = 1;
		if ((!strstr(cmd, "list")) || (!strstr(cmd, "ls"))
		    || (!strstr(cmd, "l")))
			MC_list();
		else if ((!strstr(cmd, "continue")) || (!strstr(cmd, "cont"))
			 || (!strstr(cmd, "c")))
			MC_continue();
		else if ((!strstr(cmd, "halt")) || (!strstr(cmd, "h"))
			 || (!strstr(cmd, "stop")) || (!strstr(cmd, "s")))
			MC_halt();
		else if ((!strstr(cmd, "restart"))
			 || (!strstr(cmd, "reboot") (!strstr(cmd, "r"))))
			MC_restart();
		else if ((!strstr(cmd, "tftp")) || (!strstr(cmd, "t")))
			MC_tftp();
		else {
			fputs(YELLOW "Bad command!\n" RESET);
			ok = 0;
		}

	}
}
