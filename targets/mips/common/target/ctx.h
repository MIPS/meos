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
*   Description:	MIPS common context specialisation
*
*************************************************************************/

#ifndef TARGET_CTX_H
#define TARGET_CTX_H

#include <mips/hal.h>
#include "meos/config.h"


#define CTX_NEST CTX_SIZE
#define CTX_GCTL0 CTX_NEST+SZREG

#ifndef __ASSEMBLER__

#include <reent.h>

typedef struct KRN_ctx_tag {
	struct gpctx gp;
	reg_t nest;
#ifdef CONFIG_ARCH_MIPS_VZ
	reg_t gctl0;
#endif
#ifdef CONFIG_ARCH_MIPS_DSP
	struct dspctx dsp;
#endif
#ifdef CONFIG_ARCH_MIPS_REENT
	struct _reent reent;
#endif
#if (defined(CONFIG_ARCH_MIPS_MSA) || defined (CONFIG_ARCH_MIPS_HARD_FLOAT))
	struct msactx fpa;
#endif
} KRN_CTX_T;

#define CTX_remove(X)

#endif

#endif
