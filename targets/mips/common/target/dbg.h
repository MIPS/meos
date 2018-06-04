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
*   Description:	MIPS common interrupt specialisation
*
*************************************************************************/

#ifndef TARGET_DBG_H
#define TARGET_DBG_H

#include <assert.h>
#include <stdio.h>
#include "meos/irq/irq.h"

extern int32_t _IRQ_log;

extern FILE* _DBG_file;
#define DBG_logF(...) __extension__ ({IRQ_init(NULL, 0); int32_t en = IRQ_bg(); if (en) _IRQ_disable(); _IRQ_log = 1; fprintf(_DBG_file, __VA_ARGS__); fflush(_DBG_file); _IRQ_log = 0;if (en) _IRQ_enable();})
#define _DBG_stop(F, L) __extension__ ({asm ("sdbbp");})

#ifdef CONFIG_DEBUG_TRACE_HARD
/* Output via PDTrace */
#define DBG_RTT(E) _mips32_usertrace1((E))
#define DBG_RTTValue(V) _mips32_usertrace2((V))
#define DBG_RTTPair(E, V) do {_mips32_usertrace1((E)); _mips32_usertrace2((V));} while (0)
#endif

extern int __memory_base[];
extern int __memory_size[];
#define DBG_goodPtr(X)
#define DBG_badPtr(X)  __extension__ ({ extern int __memory_base[]; extern int __memory_size[]; (((uintptr_t)(X) >= (uintptr_t)__memory_base) && ((uintptr_t)(X) < (uintptr_t)__memory_base + (uintptr_t)__memory_size)) ? 0 : 1; })
#define DBG_paranoiaAllowed() 1
#define DBG_extra() 0
#define DBG_PC(X) ((X) ? ((X)->savedContext.gp.epc) : 0)

#endif
