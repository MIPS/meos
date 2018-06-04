/*
 * Copyright 2015MIPS Tech, LLC and/or its
 *                 affiliated group companies.
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
 * m64tlb.h: MIPS64 / XPA TLB support functions
*/
#ifndef _M64TLB_H_
#define _M64TLB_H_

#if __mips != 32 && __mips != 64
#error use -mips32 or -mips64 option with this file
#endif

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>
#include <mips/notlb.h>

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long tlbhi64_t;
typedef unsigned long long tlblo64_t;
// Returns the size of the TLB.
int m64_tlb_size (void);

// Probes the TLB for an entry matching hi, and if present invalidates it.
void m64_tlbinval (tlbhi64_t hi);

// Invalidate the whole TLB.
void m64_tlbinvalall (void);

// Reads the TLB entry with specified by index, and returns the EntryHi,
// EntryLo0, EntryLo1 and PageMask parts in *phi, *plo0, *plo1 and *pmsk
// respectively.
void m64_tlbri2 (tlbhi64_t *phi, tlblo64_t *plo0, tlblo64_t *plo1,
		 unsigned long long *pmsk, int index);

// Writes hi, lo0, lo1 and msk into the TLB entry specified by index.
void m64_tlbwi2 (tlbhi64_t hi, tlblo64_t lo0, tlblo64_t lo1,
		 unsigned long long msk, int index);

// Writes hi, lo0, lo1 and msk into the TLB entry specified by the
// Random register.
void m64_tlbwr2 (tlbhi64_t hi, tlblo64_t lo0, tlblo64_t lo1,
		 unsigned long long msk);

// Probes the TLB for an entry matching hi and returns its index, or -1 if
// not found. If found, then the EntryLo0, EntryLo1 and PageMask parts of the
// entry are also returned in *plo0, *plo1 and *pmsk respectively
int m64_tlbprobe2 (tlbhi64_t hi, tlblo64_t *plo0, tlblo64_t *plo1,
		   unsigned long long *pmsk);

// Probes the TLB for an entry matching hi and if present rewrites that entry,
// otherwise updates a random entry. A safe way to update the TLB.
int m64_tlbrwr2 (tlbhi64_t hi, tlblo64_t lo0, tlblo64_t lo1,
		 unsigned long long  msk);

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* _M64TLB_H_ */
