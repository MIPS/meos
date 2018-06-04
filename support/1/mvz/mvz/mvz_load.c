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
*   Description:    ELF loader
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "mvz_elf.h"
#include <string.h>

static inline void load(MVZ_GUEST_T * guest, MVZ_XFERFUNC_T * reade,
			void *priv, uintptr_t offset, uintptr_t destination,
			size_t amount)
{
	PARACHECK();
	MVZ_xfer(reade, priv, offset, amount, MVZ_writeGV, guest, destination);
	PARACHECK();
}

static inline void zero(MVZ_GUEST_T * guest, uintptr_t destination,
			size_t amount)
{
	PARACHECK();
	MVZ_zeroGV(guest, destination, amount);
	PARACHECK();
}

void MVZ_loadELF(MVZ_GUEST_T * guest, MVZ_XFERFUNC_T * reade, void *priv)
{
	PARACHECK();

#if SZREG ==4
	struct Elf32_Ehdr hdr;
	struct Elf32_Phdr phdr;
	struct Elf32_Shdr shdr;
#else
	struct Elf64_Ehdr hdr;
	struct Elf64_Phdr phdr;
	struct Elf64_Shdr shdr;
#endif
	reg_t pho, phs, i, m;

	/* Validate ELF */
#if SZREG == 4
	reade(0, &hdr, sizeof(struct Elf32_Ehdr), 1, priv);
#else
	reade(0, &hdr, sizeof(struct Elf64_Ehdr), 1, priv);
#endif
	DBG_assert(Elf32_Ehdr_magic(&hdr), "Not an ELF file!\n");
	/* This might want changing in future? */
#if SZREG == 4
	DBG_assert(Elf32_Ehdr_class(&hdr) == ELFCLASS32,
		   "Can't load 64 bit ELF on 32 bit platform!\n");
#if BYTE_ORDER==LITTLE_ENDIAN
	DBG_assert(Elf32_Ehdr_data(&hdr) == ELFDATA2LSB,
		   "Can't load big endian ELF on little endian platform!\n");
#else
	DBG_assert(Elf32_Ehdr_data(&hdr) == ELFDATA2MSB,
		   "Can't load little endian ELF on big endian platform!\n");
#endif
#else
	DBG_assert(Elf32_Ehdr_class(&hdr) == ELFCLASS64,
		   "Can't load 32 bit ELF on 64 bit platform!\n");
#if BYTE_ORDER==LITTLE_ENDIAN
	DBG_assert(Elf64_Ehdr_data(&hdr) == ELFDATA2LSB,
		   "Can't load big endian ELF on little endian platform!\n");
#else
	DBG_assert(Elf64_Ehdr_data(&hdr) == ELFDATA2MSB,
		   "Can't load little endian ELF on big endian platform!\n");
#endif
#endif
	DBG_assert(hdr.e_ident[EI_VERSION] == 1, "ELF version[0] wrong!\n");
	DBG_assert(hdr.e_ident[EI_OSABI] == 0, "ELF OS ABI unsupported!\n");
	DBG_assert(hdr.e_type == ET_EXEC, "Not an executable file!\n");
	DBG_assert(hdr.e_machine == EM_MIPS, "Not a MIPS executable file!\n");
	DBG_assert(hdr.e_version == 1, "ELF version[1] wrong!\n");

	/* Initialise guest */
	memset((void *)&guest->task.savedContext, 0, sizeof(struct gpctx));
	guest->task.savedContext.gp.epc = hdr.e_entry;
	guest->guest.EPC = hdr.e_entry;

	/* Load ELF */
	m = hdr.e_phnum;
	if (m == PN_XNUM) {
		reade((void *)hdr.e_shoff, &shdr, hdr.e_shentsize, 1, priv);
		m = shdr.sh_info;
	}
	phs = hdr.e_phentsize;
	for (i = 0, pho = hdr.e_phoff; i < m; i++, pho += phs) {
		reade((void *)pho, &phdr, phs, 1, priv);
		if (phdr.p_type == PT_LOAD) {
			/*DBG_assert(phdr.p_paddr,
			   "Segment with NULL load address!\n"); */
			load(guest, reade, priv, phdr.p_offset, phdr.p_vaddr,
			     phdr.p_filesz);
			zero(guest, phdr.p_vaddr + phdr.p_filesz,
			     phdr.p_memsz - phdr.p_filesz);
		}
	}

	PARACHECK();
}
