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

/*
 * @Synopsis     int __uhi_exception (struct gp_ctx *ctx);
 *
 *               Parameters:
 *                 $4 - GP context
 *
 *               Arguments to syscall:
 *                 $25 - Operation code for __uhi_exception
 *                 $4 - GP context
 *
 *               Return from syscall:
 *                 $2 - 
 *
 * @Description  Handle an unhandled exception
*/

#include <stdint.h>
#include <mips/uhi_syscalls.h>
#include <mips/hal.h>

int32_t __uhi_exception(struct gpctx *ctx, int32_t abi)
{
	register struct gpctx *arg1 asm("$4") = ctx;
	register int32_t arg2 asm("$5") = abi;
	register int32_t op asm("$25") = __MIPS_UHI_EXCEPTION;
	register int32_t ret asm("$2") = __MIPS_UHI_SYSCALL_NUM;

	__asm__ __volatile__(" # %0 = __uhi_exception(%1, %2) op=%3\n"
			     SYSCALL(__MIPS_UHI_SYSCALL_NUM)
			     :"+r"(ret)
			     :"r"(arg1), "r"(arg2), "r"(op)
			     :"$3");
	return ret;
}
