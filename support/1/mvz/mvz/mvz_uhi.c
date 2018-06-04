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
*   Description:    UHI hypercall handler
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/rings/ring.h"
#include <mips/uhi_syscalls.h>
#include <alloca.h>
#include <string.h>

struct uhi_stat {
	short st_dev;
	unsigned short st_ino;
	unsigned int st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	short st_rdev;
	unsigned long long st_size;
	unsigned long long st_atime;
	unsigned long long st_spare1;
	unsigned long long st_mtime;
	unsigned long long st_spare2;
	unsigned long long st_ctime;
	unsigned long long st_spare3;
	unsigned long long st_blksize;
	unsigned long long st_blocks;
	unsigned long long st_spare4[2];
};

static void uhisys(reg_t p0, reg_t p1, reg_t p2, reg_t p3,
		   reg_t op, struct gpctx *c)
{
	register reg_t a0 asm("a0") = p0;
	register reg_t a1 asm("a1") = p1;
	register reg_t a2 asm("a2") = p2;
	register reg_t a3 asm("a3") = p3;
	register reg_t t9 asm("t9") = op;
	register reg_t v0 asm("v0");
	register reg_t v1 asm("v1");

	asm volatile ("li $2, 1;"
		      "move $4, %2;"
		      "move $5, %3;"
		      "move $25, %4;"
		      "move $6, %5;"
		      "move $7, %6;"
		      "syscall 1;"
		      "move %0, $2;"
		      "move %1, $3;"
		      "move %2, $4;"
		      "move %3, $5;":"=r" (v0), "=r"(v1), "=r"(a0), "=r"(a1)
		      :"r"(t9), "r"(a2), "r"(a3)
		      :"t8");

	c->v[0] = v0;
	c->v[1] = v1;
	c->a[0] = a0;
	c->a[1] = a1;
}

#define RING_SIZE 1024

uint8_t UHI_wcBuf[RING_SIZE];
RING_T UHI_wc;
KRN_TIMER_T UHI_wcT;

void _UHI_wcCons(void) __attribute__ ((constructor, no_instrument_function));
void _UHI_wcCons(void)
{
	RING_init(&UHI_wc, UHI_wcBuf, RING_SIZE);
}

void _UHI_wcDump(KRN_TIMER_T * timer, void *timerPar)
{
	struct gpctx dummy;
	uint8_t buf[RING_SIZE], *p = buf;
	KRN_cancelTimer(&UHI_wcT);
	while (RING_read(&UHI_wc, p, 0))
		p++;
	*p = 0;
	if (p != buf)
		uhisys(1, (reg_t) buf, (reg_t) (p - buf), 0, __MIPS_UHI_WRITE,
		       &dummy);
}

void _UHI_wcQ(uint8_t c)
{
	PARACHECK();

	if (RING_full(&UHI_wc))
		_UHI_wcDump(NULL, NULL);
	RING_write(&UHI_wc, c, 0);
	KRN_cancelTimer(&UHI_wcT);
	KRN_setTimer(&UHI_wcT, (KRN_TIMERFUNC_T *) _UHI_wcDump, NULL, 1000);

	PARACHECK();
}

void MVZ_UHI(int sigNum, struct gpctx *c)
{
	PARACHECK();

	static KRN_TASK_T *last = NULL;
	MVZ_GUEST_T *gt = (MVZ_GUEST_T *) _KRN_current;
	char *one, *two;
	size_t l;
	reg_t op = c->t2[1];
	reg_t a0 = c->a[0], a1 = c->a[1], a2 = c->a[2], a3 = c->a[3];
	struct uhi_stat sbuf;
	struct uhi_stat *psbuf = &sbuf;

	if (c->v[0] != 1) {
		DBG_assert(0, "Expected UHI hypcall, got %u\n", c->v[0]);
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
	}

	switch (op) {
	case __MIPS_UHI_EXIT:
		MVZ_exit(gt, a0);
		break;
	case __MIPS_UHI_OPEN:
		l = MVZ_gstrlen(gt, a0) + 1;
		one = alloca(l);
		MVZ_readGV((void *)a0, one, l, 1, gt);
		uhisys((reg_t) one, a1, a2, 0, __MIPS_UHI_OPEN, c);
		break;
	case __MIPS_UHI_CLOSE:
		uhisys(a0, 0, 0, 0, __MIPS_UHI_CLOSE, c);
		break;
	case __MIPS_UHI_READ:
		one = alloca(a2);
		uhisys(a0, (reg_t) one, a2, 0, __MIPS_UHI_READ, c);
		MVZ_writeGV((void *)a1, one, a2, 1, gt);
		break;
	case __MIPS_UHI_WRITE:
		if (last != _KRN_current) {
			if (a0 == 1)
				_UHI_wcDump(NULL, NULL);
			const char *n = KRN_taskName(NULL);
			uhisys(a0, (reg_t) n, strlen(n), 0, __MIPS_UHI_WRITE,
			       c);
			uhisys(a0, (reg_t) ": ", 2, 0, __MIPS_UHI_WRITE, c);
			last = _KRN_current;
		}
		one = alloca(a2);
		MVZ_readGV((void *)a1, one, a2, 1, gt);
		if (a0 == 1) {
			if (a2 == 1)
				_UHI_wcQ(*(char *)one);
			else {
				_UHI_wcDump(NULL, NULL);
				uhisys(a0, (reg_t) one, a2, 0, __MIPS_UHI_WRITE,
				       c);
			}
		} else
			uhisys(a0, (reg_t) one, a2, 0, __MIPS_UHI_WRITE, c);
		break;
	case __MIPS_UHI_LSEEK:
		uhisys(a0, a1, a2, 0, __MIPS_UHI_LSEEK, c);
		break;
	case __MIPS_UHI_UNLINK:
		uhisys(a0, 0, 0, 0, __MIPS_UHI_UNLINK, c);
		break;
	case __MIPS_UHI_FSTAT:
		uhisys(a0, (reg_t) psbuf, 0, 0, __MIPS_UHI_FSTAT, c);
		MVZ_writeGV((void *)a1, psbuf, sizeof(sbuf), 1, gt);
		break;
	case __MIPS_UHI_ARGC:
		c->v[0] = gt->argc;
		break;
	case __MIPS_UHI_ARGLEN:
		c->v[0] = strlen(gt->argv[a0]);
		break;
	case __MIPS_UHI_ARGN:
		MVZ_writeGV((void *)a1, gt->argv[a0], strlen(gt->argv[a0]),
			    1, gt);
		break;
	case __MIPS_UHI_RAMRANGE:
		c->v[0] = -1;
		c->v[1] = -1;
		break;
	case __MIPS_UHI_LOG:
		l = MVZ_gstrlen(gt, a0);
		one = alloca(l);
		MVZ_readGV((void *)a0, one, l, 1, gt);
		uhisys((reg_t) one, a1, 0, 0, __MIPS_UHI_LOG, c);
		break;
	case __MIPS_UHI_ASSERT:
		l = MVZ_gstrlen(gt, a0);
		one = alloca(l);
		MVZ_readGV((void *)a0, one, l, 1, gt);
		l = MVZ_gstrlen(gt, a1);
		two = alloca(l);
		MVZ_readGV((void *)a1, two, l, 1, gt);
		uhisys((reg_t) one, (reg_t) two, a2, 0, __MIPS_UHI_ASSERT, c);
		break;
	case __MIPS_UHI_EXCEPTION:
		DBG_assert(0, "Guest '%s' took an exception!\n",
			   KRN_taskName(NULL));
		MVZ_restart((MVZ_GUEST_T *) _KRN_current);
		break;
	case 16:		// FINDFIRST
		l = MVZ_gstrlen(gt, a0);
		one = alloca(l);
		MVZ_readGV((void *)a0, one, l, 1, gt);
		l = MVZ_gstrlen(gt, a1);
		two = alloca(l);
		MVZ_readGV((void *)a1, two, l, 1, gt);
		uhisys((reg_t) one, (reg_t) two, 0, 0, 16, c);
		break;
	case 17:		// FINDNEXT
		l = MVZ_gstrlen(gt, a1);
		one = alloca(l);
		MVZ_readGV((void *)a1, one, l, 1, gt);
		uhisys(a0, (reg_t) one, 0, 0, 17, c);
		break;
	case 18:		// FINDCLOSE
		uhisys(a0, 0, 0, 0, 18, c);
		break;
	case __MIPS_UHI_PREAD:
		l = MVZ_gstrlen(gt, a1);
		one = alloca(l);
		MVZ_readGV((void *)a1, one, l, 1, gt);
		uhisys(a0, (reg_t) one, a2, a3, __MIPS_UHI_PREAD, c);
		break;
	case __MIPS_UHI_PWRITE:
		l = MVZ_gstrlen(gt, a1);
		one = alloca(l);
		MVZ_readGV((void *)a1, one, l, 1, gt);
		uhisys(a0, (reg_t) one, a2, a3, __MIPS_UHI_PWRITE, c);
		break;
	case 21:		//YIELD
		uhisys(a0, 0, 0, 0, 21, c);
		break;
	case __MIPS_UHI_LINK:
		l = MVZ_gstrlen(gt, a0);
		one = alloca(l);
		MVZ_readGV((void *)a0, one, l, 1, gt);
		l = MVZ_gstrlen(gt, a1);
		two = alloca(l);
		MVZ_readGV((void *)a1, two, l, 1, gt);
		uhisys((reg_t) one, (reg_t) two, 0, 0, __MIPS_UHI_LINK, c);
		break;
	default:
		DBG_assert(0, "Unknown UHI op %x\n", op);
		MVZ_restart(gt);
	}

	PARACHECK();
}
