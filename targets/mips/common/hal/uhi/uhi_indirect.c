/*
 * Copyright 2014-2015MIPS Tech, LLC and/or its
 *                      affiliated group companies.
 * All rights reserved.
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
*/

#include <mips/hal.h>

/* Forward a UHI SYSCALL operation to SDBPP interface.  */
int __uhi_indirect(struct gpctx *ctx)
{
	register reg_t arg1 asm("$4") = ctx->a[0];
	register reg_t arg2 asm("$5") = ctx->a[1];
	register reg_t arg3 asm("$6") = ctx->a[2];
	register reg_t arg4 asm("$7") = ctx->a[3];
	register reg_t op asm("$25") = ctx->t2[1];
	register reg_t ret1 asm("$2") = 1;
	register reg_t ret2 asm("$3");

	__asm__ __volatile__(" # UHI indirect\n"
			     "\tsdbbp 1":"+r"(ret1), "=r"(ret2), "+r"(arg1),
			     "+r"(arg2)
			     :"r"(arg3), "r"(arg4), "r"(op));

	ctx->v[0] = ret1;
	ctx->v[1] = ret2;
	ctx->a[0] = arg1;
	ctx->a[1] = arg2;
	/* Handled, move on.  SYSCALL is 4-bytes in all ISAs.  */
	ctx->epc += 4;

	return 1;		/* exception handled */
}
