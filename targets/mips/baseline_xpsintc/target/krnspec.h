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
*   Description:	MIPS baseline kernel specialisation
*
*************************************************************************/

#ifndef TARGET_KRN_H
#define TARGET_KRN_H

#include <mips/cpu.h>

#define KRN_refreshCache(ADDR, SIZE) __extension__ ({void *addr = (ADDR); size_t size = (SIZE); KRN_flushCache(addr, size, KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_WRITEBACK_D); KRN_preloadCache(addr, size, 0);})

#define KRN_syncCache(ADDR, SIZE) _KRN_cache((uint32_t)(ADDR), (SIZE), Hit_Sync_I)

#define KRN_barrier(FLAGS) do { \
	if ((FLAGS) == KRN_BARRIER_FLAG_READ) \
	 	__asm__ __volatile__ ("sync 19" : : : "memory"); \
	else if ((FLAGS) == KRN_BARRIER_FLAG_WRITE) \
		__asm__ __volatile__ ("sync 4" : : : "memory"); \
	else if ((FLAGS) == (KRN_BARRIER_FLAG_READ | KRN_BARRIER_FLAG_WRITE)) \
		__asm__ __volatile__ ("sync 16" : : : "memory"); \
	else \
		_mips_sync(); \
} while (0)

#define _L1ILS() ((mips32_getconfig0() & CFG0_M) ? (((mips32_getconfig1() & CFG1_IL_MASK) >> CFG1_IL_SHIFT) ? (2 << ((mips32_getconfig1() & CFG1_IL_MASK) >> CFG1_IL_SHIFT)) : 0) : 32)
#define _L1DLS() ((mips32_getconfig0() & CFG0_M) ? (((mips32_getconfig1() & CFG1_DL_MASK) >> CFG1_DL_SHIFT) ? (2 << ((mips32_getconfig1() & CFG1_DL_MASK) >> CFG1_DL_SHIFT)) : 0) : 32)
#define _L2LS() (((mips32_getconfig0() & CFG0_M) && (mips32_getconfig1() & CFG1_M)) ? (((mips32_getconfig2() & CFG2_SL_MASK) >> CFG2_SL_SHIFT) ? (2 << ((mips32_getconfig2() & CFG2_SL_MASK) >> CFG2_SL_SHIFT)) : 0) : 0)
#define _L3LS() (((mips32_getconfig0() & CFG0_M) && (mips32_getconfig1() & CFG1_M)) ? (((mips32_getconfig2() & CFG2_TL_MASK) >> CFG2_TL_SHIFT) ? (2 << ((mips32_getconfig2() & CFG2_TL_MASK) >> CFG2_TL_SHIFT)) : 0) : 0)

#define Hit_Sync_I 0xffffffff

static inline void _KRN_cache(uint32_t addr, size_t size, uint32_t mode)
{
	if (size > 0) {
		register uint32_t linesize;
		register uint32_t mask;
		register uint32_t caddr;
		register uint32_t maxaddr;
		switch (mode) {
		case Hit_Sync_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr = addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("synci 0(%0)"::"r"(caddr));
			break;
		case Index_Invalidate_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr = addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x00,0(%0)"::"r"(caddr));
			break;
		case Index_Writeback_Inv_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr = addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x01,0(%0)"::"r"(caddr));
			break;
		case Index_Writeback_Inv_T:
			for (linesize = _L3LS(), mask = ~(linesize - 1), caddr =
			     addr & mask, maxaddr = ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x02,0(%0)"::"r"(caddr));
			break;
		case Index_Writeback_Inv_S:
			for (linesize = _L2LS(), mask = ~(linesize - 1), caddr =
			     addr & mask, maxaddr = ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x03,0(%0)"::"r"(caddr));
			break;
		case Index_Load_Tag_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr = addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
			asm("cache 0x04,0(%0)"::"r"(caddr));
			break;
		case Index_Load_Tag_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x05,0(%0)"::"r"(caddr));
			break;
		case Index_Load_Tag_T:
			for (linesize = _L3LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x06,0(%0)"::"r"(caddr));
			break;
		case Index_Load_Tag_S:
			for (linesize = _L2LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x07,0(%0)"::"r"(caddr));
			break;
		case Index_Store_Tag_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x08,0(%0)"::"r"(caddr));
			break;
		case Index_Store_Tag_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x09,0(%0)"::"r"(caddr));
			break;
		case Index_Store_Tag_T:
			for (linesize = _L3LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x0a,0(%0)"::"r"(caddr));
			break;
		case Index_Store_Tag_S:
			for (linesize = _L2LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x0b,0(%0)"::"r"(caddr));
			break;
		case Hit_Invalidate_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x10,0(%0)"::"r"(caddr));
			break;
		case Hit_Invalidate_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x11,0(%0)"::"r"(caddr));
			break;
		case Hit_Invalidate_T:
			for (linesize = _L3LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x12,0(%0)"::"r"(caddr));
			break;
		case Hit_Invalidate_S:
			for (linesize = _L2LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x13,0(%0)"::"r"(caddr));
			break;
		case Fill_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x14,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_Inv_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x15,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_Inv_T:
			for (linesize = _L3LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x16,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_Inv_S:
			for (linesize = _L2LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x17,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x19,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_T:
			for (linesize = _L3LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x1a,0(%0)"::"r"(caddr));
			break;
		case Hit_Writeback_S:
			for (linesize = _L2LS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x1b,0(%0)"::"r"(caddr));
			break;
		case Fetch_Lock_I:
			for (linesize = _L1ILS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x1c,0(%0)"::"r"(caddr));
			break;
		case Fetch_Lock_D:
			for (linesize = _L1DLS(), mask =
			     ~(linesize - 1), caddr =
			     addr & mask, maxaddr =
			     ((addr + size) - 1) & mask;
			     (linesize) && (caddr <= maxaddr);
			     caddr += linesize)
				asm("cache 0x1d,0(%0)"::"r"(caddr));
			break;
		default:
			break;
		}
	}
	_mips_sync();
}

static inline void KRN_flushCache(void *address, size_t size, int32_t flags)
{
	if (flags ==
	    (KRN_FLUSH_FLAG_I | KRN_FLUSH_FLAG_D | KRN_FLUSH_FLAG_WRITEBACK_D))
	{
		_KRN_cache((uintptr_t) address, size, Hit_Writeback_Inv_D);
		_KRN_cache((uintptr_t) address, size, Hit_Writeback_Inv_S);
		_KRN_cache((uintptr_t) address, size, Hit_Writeback_Inv_T);
		_KRN_cache((uintptr_t) address, size, Hit_Invalidate_I);
	} else {
		if (flags & KRN_FLUSH_FLAG_D) {
			if (flags & KRN_FLUSH_FLAG_WRITEBACK_D) {
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_Inv_D);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_Inv_S);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_Inv_T);
			} else {
				_KRN_cache((uintptr_t) address, size,
					   Hit_Invalidate_D);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Invalidate_S);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Invalidate_T);
			}
		}
        else if (flags & KRN_FLUSH_FLAG_WRITEBACK_D) {
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_D);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_S);
				_KRN_cache((uintptr_t) address, size,
					   Hit_Writeback_T);
			}
		if (flags & KRN_FLUSH_FLAG_I)
			_KRN_cache((uintptr_t) address, size, Hit_Invalidate_I);
	}
}

static inline void KRN_preloadCache(void *paddr, size_t osize, int32_t flags)
{
	int8_t *addr;
	size_t size;
	size_t linesize = _L1DLS();
	if (flags & KRN_PRELOAD_FLAG_DATA) {
		addr = (int8_t *) paddr;
		size = osize;

		switch (flags &
			~(KRN_PRELOAD_FLAG_DATA | KRN_PRELOAD_FLAG_INST)) {
		default:
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_NORMAL):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 0, 1);
			break;
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_STREAMED):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 0, 0);
			break;
		case (KRN_PRELOAD_FLAG_READ | KRN_PRELOAD_FLAG_RETAINED):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 0, 3);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_NORMAL):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 1, 1);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_STREAMED):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 1, 0);
			break;
		case (KRN_PRELOAD_FLAG_WRITE | KRN_PRELOAD_FLAG_RETAINED):
			for (; size;
			     size =
			     ((size > linesize) ? (size - linesize) : size),
			     addr += linesize)
				mips_prefetch(addr, 1, 3);
			break;
		}
	}
	if (flags & KRN_PRELOAD_FLAG_INST) {
		_KRN_cache((uintptr_t) paddr, osize, Fill_I);
	}
}

#undef _L1ILS
#undef _L1DLS
#undef _L2LS
#undef _L3LS

#endif
