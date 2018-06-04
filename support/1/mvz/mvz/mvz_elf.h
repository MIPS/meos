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
*   Description:    elf.h compatible header
*
*************************************************************************/

#ifndef _MVZ_ELF_H
#define _MVZ_ELF_H

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

enum {
	EI_MAG0 = 0,
	EI_MAG1 = 1,
	EI_MAG2 = 2,
	EI_MAG3 = 3,
	EI_CLASS = 4,
	EI_DATA = 5,
	EI_VERSION = 6,
	EI_OSABI = 7,
	EI_ABIVERSION = 8,
	EI_PAD = 9,
	EI_NIDENT = 16
};

struct Elf32_Ehdr {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
};

static inline reg_t Elf32_Ehdr_magic(struct Elf32_Ehdr *hdr)
{
	return (hdr->e_ident[0] == 0x7f) && (hdr->e_ident[1] == 'E')
	    && (hdr->e_ident[2] == 'L') && (hdr->e_ident[3] == 'F');
}

static inline unsigned char Elf32_Ehdr_class(struct Elf32_Ehdr *hdr)
{
	return hdr->e_ident[EI_CLASS];
}

static inline unsigned char Elf32_Ehdr_data(struct Elf32_Ehdr *hdr)
{
	return hdr->e_ident[EI_DATA];
}

struct Elf64_Ehdr {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
};

static inline reg_t Elf64_Ehdr_magic(struct Elf64_Ehdr *hdr)
{
	return (hdr->e_ident[0] == 0x7f) && (hdr->e_ident[1] == 'E')
	    && (hdr->e_ident[2] == 'L') && (hdr->e_ident[3] == 'F');
}

static inline unsigned char Elf64_Ehdr_class(struct Elf64_Ehdr *hdr)
{
	return hdr->e_ident[EI_CLASS];
}

static inline unsigned char Elf64_Ehdr_data(struct Elf64_Ehdr *hdr)
{
	return hdr->e_ident[EI_DATA];
}

enum {
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4,
	ET_LOPROC = 0xff00,
	ET_HIPROC = 0xffff
};

enum {
	EV_NONE = 0,
	EV_CURRENT = 1
};

enum {
	EM_MIPS = 8
};

enum {
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2
};

enum {
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2
};

enum {
	ELFOSABI_NONE = 0
};

enum {
	R_MIPS_NONE = 0,
	R_MIPS_16 = 1,
	R_MIPS_32 = 2,
	R_MIPS_REL32 = 3,
	R_MIPS_26 = 4,
	R_MIPS_HI16 = 5,
	R_MIPS_LO16 = 6,
	R_MIPS_GPREL16 = 7,
	R_MIPS_LITERAL = 8,
	R_MIPS_GOT16 = 9,
	R_MIPS_PC16 = 10,
	R_MIPS_CALL16 = 11,
	R_MIPS_GPREL32 = 12,
	R_MIPS_UNUSED1 = 13,
	R_MIPS_UNUSED2 = 14,
	R_MIPS_UNUSED3 = 15,
	R_MIPS_SHIFT5 = 16,
	R_MIPS_SHIFT6 = 17,
	R_MIPS_64 = 18,
	R_MIPS_GOT_DISP = 19,
	R_MIPS_GOT_PAGE = 20,
	R_MIPS_GOT_OFST = 21,
	R_MIPS_GOT_HI16 = 22,
	R_MIPS_GOT_LO16 = 23,
	R_MIPS_SUB = 24,
	R_MIPS_INSERT_A = 25,
	R_MIPS_INSERT_B = 26,
	R_MIPS_DELETE = 27,
	R_MIPS_HIGHER = 28,
	R_MIPS_HIGHEST = 29,
	R_MIPS_CALL_HI16 = 30,
	R_MIPS_CALL_LO16 = 31,
	R_MIPS_SCN_DISP = 32,
	R_MIPS_REL16 = 33,
	R_MIPS_ADD_IMMEDIATE = 34,
	R_MIPS_PJUMP = 35,
	R_MIPS_RELGOT = 36,
	R_MIPS_JALR = 37,
	R_MIPS_TLS_DTPMOD32 = 38,
	R_MIPS_TLS_DTPREL32 = 39,
	R_MIPS_TLS_DTPMOD64 = 40,
	R_MIPS_TLS_DTPREL64 = 41,
	R_MIPS_TLS_GD = 42,
	R_MIPS_TLS_LDM = 43,
	R_MIPS_TLS_DTPREL_HI16 = 44,
	R_MIPS_TLS_DTPREL_LO16 = 45,
	R_MIPS_TLS_GOTTPREL = 46,
	R_MIPS_TLS_TPREL32 = 47,
	R_MIPS_TLS_TPREL64 = 48,
	R_MIPS_TLS_TPREL_HI16 = 49,
	R_MIPS_TLS_TPREL_LO16 = 50,
	R_MIPS_GLOB_DAT = 51,
	R_MIPS_PC21_S2 = 60,
	R_MIPS_PC26_S2 = 61,
	R_MIPS_PC18_S3 = 62,
	R_MIPS_PC19_S2 = 63,
	R_MIPS_PCHI16 = 64,
	R_MIPS_PCLO16 = 65,
	R_MIPS16_26 = 100,
	R_MIPS16_GPREL = 101,
	R_MIPS16_GOT16 = 102,
	R_MIPS16_CALL16 = 103,
	R_MIPS16_HI16 = 104,
	R_MIPS16_LO16 = 105,
	R_MIPS16_TLS_GD = 106,
	R_MIPS16_TLS_LDM = 107,
	R_MIPS16_TLS_DTPREL_HI16 = 108,
	R_MIPS16_TLS_DTPREL_LO16 = 109,
	R_MIPS16_TLS_GOTTPREL = 110,
	R_MIPS16_TLS_TPREL_HI16 = 111,
	R_MIPS16_TLS_TPREL_LO16 = 112,
	R_MIPS_COPY = 126,
	R_MIPS_JUMP_SLOT = 127,
	R_MICROMIPS_26_S1 = 133,
	R_MICROMIPS_HI16 = 134,
	R_MICROMIPS_LO16 = 135,
	R_MICROMIPS_GPREL16 = 136,
	R_MICROMIPS_LITERAL = 137,
	R_MICROMIPS_GOT16 = 138,
	R_MICROMIPS_PC7_S1 = 139,
	R_MICROMIPS_PC10_S1 = 140,
	R_MICROMIPS_PC16_S1 = 141,
	R_MICROMIPS_CALL16 = 142,
	R_MICROMIPS_GOT_DISP = 145,
	R_MICROMIPS_GOT_PAGE = 146,
	R_MICROMIPS_GOT_OFST = 147,
	R_MICROMIPS_GOT_HI16 = 148,
	R_MICROMIPS_GOT_LO16 = 149,
	R_MICROMIPS_SUB = 150,
	R_MICROMIPS_HIGHER = 151,
	R_MICROMIPS_HIGHEST = 152,
	R_MICROMIPS_CALL_HI16 = 153,
	R_MICROMIPS_CALL_LO16 = 154,
	R_MICROMIPS_SCN_DISP = 155,
	R_MICROMIPS_JALR = 156,
	R_MICROMIPS_HI0_LO16 = 157,
	R_MICROMIPS_TLS_GD = 162,
	R_MICROMIPS_TLS_LDM = 163,
	R_MICROMIPS_TLS_DTPREL_HI16 = 164,
	R_MICROMIPS_TLS_DTPREL_LO16 = 165,
	R_MICROMIPS_TLS_GOTTPREL = 166,
	R_MICROMIPS_TLS_TPREL_HI16 = 169,
	R_MICROMIPS_TLS_TPREL_LO16 = 170,
	R_MICROMIPS_GPREL7_S2 = 172,
	R_MICROMIPS_PC23_S2 = 173,
	R_MICROMIPS_PC21_S2 = 174,
	R_MICROMIPS_PC26_S2 = 175,
	R_MICROMIPS_PC18_S3 = 176,
	R_MICROMIPS_PC19_S2 = 177,
	R_MIPS_NUM = 218,
	R_MIPS_PC32 = 248,
	R_MIPS_EH = 249
};

enum {
	EF_MIPS_NOREORDER = 0x00000001,
	EF_MIPS_PIC = 0x00000002,
	EF_MIPS_CPIC = 0x00000004,
	EF_MIPS_ABI2 = 0x00000020,
	EF_MIPS_32BITMODE = 0x00000100,

	EF_MIPS_FP64 = 0x00000200,

	EF_MIPS_NAN2008 = 0x00000400,

	EF_MIPS_ABI_O32 = 0x00001000,
	EF_MIPS_ABI_O64 = 0x00002000,
	EF_MIPS_ABI_EABI32 = 0x00003000,
	EF_MIPS_ABI_EABI64 = 0x00004000,
	EF_MIPS_ABI = 0x0000f000,

	EF_MIPS_MACH_3900 = 0x00810000,
	EF_MIPS_MACH_4010 = 0x00820000,
	EF_MIPS_MACH_4100 = 0x00830000,
	EF_MIPS_MACH_4650 = 0x00850000,
	EF_MIPS_MACH_4120 = 0x00870000,
	EF_MIPS_MACH_4111 = 0x00880000,
	EF_MIPS_MACH_SB1 = 0x008a0000,
	EF_MIPS_MACH_OCTEON = 0x008b0000,
	EF_MIPS_MACH_XLR = 0x008c0000,
	EF_MIPS_MACH_OCTEON2 = 0x008d0000,
	EF_MIPS_MACH_OCTEON3 = 0x008e0000,
	EF_MIPS_MACH_5400 = 0x00910000,
	EF_MIPS_MACH_5900 = 0x00920000,
	EF_MIPS_MACH_5500 = 0x00980000,
	EF_MIPS_MACH_9000 = 0x00990000,
	EF_MIPS_MACH_LS2E = 0x00a00000,
	EF_MIPS_MACH_LS2F = 0x00a10000,
	EF_MIPS_MACH_LS3A = 0x00a20000,
	EF_MIPS_MACH = 0x00ff0000,

	EF_MIPS_MICROMIPS = 0x02000000,
	EF_MIPS_ARCH_ASE_M16 = 0x04000000,
	EF_MIPS_ARCH_ASE_MDMX = 0x08000000,
	EF_MIPS_ARCH_ASE = 0x0f000000,

	EF_MIPS_ARCH_1 = 0x00000000,
	EF_MIPS_ARCH_2 = 0x10000000,
	EF_MIPS_ARCH_3 = 0x20000000,
	EF_MIPS_ARCH_4 = 0x30000000,
	EF_MIPS_ARCH_5 = 0x40000000,
	EF_MIPS_ARCH_32 = 0x50000000,
	EF_MIPS_ARCH_64 = 0x60000000,
	EF_MIPS_ARCH_32R2 = 0x70000000,
	EF_MIPS_ARCH_64R2 = 0x80000000,
	EF_MIPS_ARCH_32R6 = 0x90000000,
	EF_MIPS_ARCH_64R6 = 0xa0000000,
	EF_MIPS_ARCH = 0xf0000000
};

enum {
	STO_MIPS_OPTIONAL = 0x04,
	STO_MIPS_PLT = 0x08,
	STO_MIPS_PIC = 0x20,
	STO_MIPS_MICROMIPS = 0x80,
	STO_MIPS_MIPS16 = 0xf0
};

enum {
	ODK_NULL = 0,
	ODK_REGINFO = 1,
	ODK_EXCEPTIONS = 2,
	ODK_PAD = 3,
	ODK_HWPATCH = 4,
	ODK_FILL = 5,
	ODK_TAGS = 6,
	ODK_HWAND = 7,
	ODK_HWOR = 8,
	ODK_GP_GROUP = 9,
	ODK_IDENT = 10,
	ODK_PAGESIZE = 11
};

struct Elf32_Shdr {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
};

struct Elf64_Shdr {
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
};

enum {
	SHN_UNDEF = 0,
	SHN_LORESERVE = 0xff00,
	SHN_LOPROC = 0xff00,
	SHN_HIPROC = 0xff1f,
	SHN_LOOS = 0xff20,
	SHN_HIOS = 0xff3f,
	SHN_ABS = 0xfff1,
	SHN_COMMON = 0xfff2,
	SHN_XINDEX = 0xffff,
	SHN_HIRESERVE = 0xffff
};

enum {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11,
	SHT_INIT_ARRAY = 14,
	SHT_FINI_ARRAY = 15,
	SHT_PREINIT_ARRAY = 16,
	SHT_GROUP = 17,
	SHT_SYMTAB_SHNDX = 18,
	SHT_LOOS = 0x60000000,
	SHT_GNU_ATTRIBUTES = 0x6ffffff5,
	SHT_GNU_HASH = 0x6ffffff6,
	SHT_GNU_verdef = 0x6ffffffd,
	SHT_GNU_verneed = 0x6ffffffe,
	SHT_GNU_versym = 0x6fffffff,
	SHT_HIOS = 0x6fffffff,
	SHT_LOPROC = 0x70000000,
	SHT_MIPS_REGINFO = 0x70000006,
	SHT_MIPS_OPTIONS = 0x7000000d,
	SHT_MIPS_ABIFLAGS = 0x7000002a,
	SHT_HIPROC = 0x7fffffff,
	SHT_LOUSER = 0x80000000,
	SHT_HIUSER = 0xffffffff
};

enum {
	SHF_WRITE = 0x1,
	SHF_ALLOC = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MERGE = 0x10,
	SHF_STRINGS = 0x20,
	SHF_INFO_LINK = 0x40U,
	SHF_LINK_ORDER = 0x80U,
	SHF_OS_NONCONFORMING = 0x100U,
	SHF_GROUP = 0x200U,
	SHF_TLS = 0x400U,
	SHF_EXCLUDE = 0x80000000U,
	SHF_MASKOS = 0x0ff00000,
	SHF_MASKPROC = 0xf0000000,
	SHF_MIPS_NODUPES = 0x01000000,
	SHF_MIPS_NAMES = 0x02000000,
	SHF_MIPS_LOCAL = 0x04000000,
	SHF_MIPS_NOSTRIP = 0x08000000,
	SHF_MIPS_GPREL = 0x10000000,
	SHF_MIPS_MERGE = 0x20000000,
	SHF_MIPS_ADDR = 0x40000000,
	SHF_MIPS_STRING = 0x80000000
};

enum {
	GRP_COMDAT = 0x1,
	GRP_MASKOS = 0x0ff00000,
	GRP_MASKPROC = 0xf0000000
};

struct Elf32_Sym {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;
};

struct Elf64_Sym {
	Elf64_Word st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf64_Half st_shndx;
	Elf64_Addr st_value;
	Elf64_Xword st_size;
};

#define ElfN_Sym_binding(SYM) ((SYM)->st_info >> 4)
#define ElfN_Sym_type(SYM) ((SYM)->st_info & 0xf)
#define ElfN_Sym_setBinding(SYM, BINDING) do {(SYM)->st_info = ((SYM)->st_info & 0xf) | ((BINDING) << 4);} while (0)
#define ElfN_Sym_setType(SYM, TYPE) do {(SYM)->st_info = ((TYPE)  & 0xf) | ((SYM)->st_info & ~0xf);} while (0)
#define ElfN_Sym_setBT(SYM, BINDING, TYPE) do {(SYM)->st_info = ((TYPE)  & 0xf) | ((BINDING) << 4);} while (0)

enum {
	SYMENTRY_SIZE32 = 16,
	SYMENTRY_SIZE64 = 24
};

enum {
	STB_LOCAL = 0,
	STB_GLOBAL = 1,
	STB_WEAK = 2,
	STB_GNU_UNIQUE = 10,
	STB_LOOS = 10,
	STB_HIOS = 12,
	STB_LOPROC = 13,
	STB_HIPROC = 15
};

enum {
	STT_NOTYPE = 0,
	STT_OBJECT = 1,
	STT_FUNC = 2,
	STT_SECTION = 3,
	STT_FILE = 4,
	STT_COMMON = 5,
	STT_TLS = 6,
	STT_GNU_IFUNC = 10,
	STT_LOOS = 10,
	STT_HIOS = 12,
	STT_LOPROC = 13,
	STT_HIPROC = 15
};

enum {
	STV_DEFAULT = 0,
	STV_INTERNAL = 1,
	STV_HIDDEN = 2,
	STV_PROTECTED = 3
};

enum {
	STN_UNDEF = 0
};

enum {
	RSS_UNDEF = 0,
	RSS_GP = 1,
	RSS_GP0 = 2,
	RSS_LOC = 3
};

struct Elf32_Rel {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
};

struct Elf32_Rela {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;
};

#define Elf32_Rel_symbol(REL) ((REL)->r_info >> 8)
#define Elf32_Rel_type(REL) ((unsigned char)((REL)->r_info & 0xff))
#define Elf32_Rel_setSymbol(REL, SYM) do {(REL)->r_info = ((REL)->r_info & 0xff) | ((SYM) << 8);} while (0)
#define Elf32_Rel_setType(REL, TYPE) do {(REL)->r_info = ((TYPE)  & 0xff) | ((REL)->r_info & ~0xff);} while (0)
#define Elf32_Rel_setST(REL, SYM, TYPE) do {(REL)->r_info = ((TYPE)  & 0xff) | ((SYM) << 8);} while (0)

#define Elf32_Rela_symbol ElfN_Rel_symbol
#define Elf32_Rela_type ElfN_Rel_type
#define Elf32_Rela_setSymbol ElfN_Rel_setSymbol
#define Elf32_Rela_setType ElfN_Rel_setType
#define Elf32_Rela_setST ElfN_Rel_setST

struct Elf64_Rel {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
};

struct Elf64_Rela {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
};

#define Elf64_Rel_symbol(REL) ((REL)->r_info >> 8)
#define Elf64_Rel_type(REL) ((unsigned char)((REL)->r_info & 0xffffffff))
#define Elf64_Rel_setSymbol(REL, SYM) do {(REL)->r_info = ((REL)->r_info & 0xffffffff) | ((SYM) << 32);} while (0)
#define Elf64_Rel_setType(REL, TYPE) do {(REL)->r_info = ((TYPE)  & 0xffffffff) | ((REL)->r_info & ~0xffffffff);} while (0)
#define Elf64_Rel_setST(REL, SYM, TYPE) do {(REL)->r_info = ((TYPE)  & 0xffffffff) | ((SYM) << 32);} while (0)

#define Elf64_Rela_symbol ElfN_Rel_symbol
#define Elf64_Rela_type ElfN_Rel_type
#define Elf64_Rela_setSymbol ElfN_Rel_setSymbol
#define Elf64_Rela_setType ElfN_Rel_setType
#define Elf64_Rela_setST ElfN_Rel_setST

struct Elf32_Phdr {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
};

struct Elf64_Phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr p_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
};

enum {
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,
	PT_PHDR = 6,
	PT_TLS = 7,
	PT_LOOS = 0x60000000,
	PT_HIOS = 0x6fffffff,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff,
	PT_MIPS_REGINFO = 0x70000000,
	PT_MIPS_RTPROC = 0x70000001,
	PT_MIPS_OPTIONS = 0x70000002,
	PT_MIPS_ABIFLAGS = 0x70000003
};

enum {
	PN_XNUM = 0xffff
};

enum {
	PF_X = 1,
	PF_W = 2,
	PF_R = 4,
	PF_MASKOS = 0x0ff00000,
	PF_MASKPROC = 0xf0000000
};

struct Elf32_Dyn {
	Elf32_Sword d_tag;
	union {
		Elf32_Word d_val;
		Elf32_Addr d_ptr;
	} d_un;
};

struct Elf64_Dyn {
	Elf64_Sxword d_tag;
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
};

enum {
	DT_NULL = 0,
	DT_NEEDED = 1,
	DT_PLTRELSZ = 2,
	DT_PLTGOT = 3,
	DT_HASH = 4,
	DT_STRTAB = 5,
	DT_SYMTAB = 6,
	DT_RELA = 7,
	DT_RELASZ = 8,
	DT_RELAENT = 9,
	DT_STRSZ = 10,
	DT_SYMENT = 11,
	DT_INIT = 12,
	DT_FINI = 13,
	DT_SONAME = 14,
	DT_RPATH = 15,
	DT_SYMBOLIC = 16,
	DT_REL = 17,
	DT_RELSZ = 18,
	DT_RELENT = 19,
	DT_PLTREL = 20,
	DT_DEBUG = 21,
	DT_TEXTREL = 22,
	DT_JMPREL = 23,
	DT_BIND_NOW = 24,
	DT_INIT_ARRAY = 25,
	DT_FINI_ARRAY = 26,
	DT_INIT_ARRAYSZ = 27,
	DT_FINI_ARRAYSZ = 28,
	DT_RUNPATH = 29,
	DT_FLAGS = 30,
	DT_ENCODING = 32,
	DT_PREINIT_ARRAY = 32,
	DT_PREINIT_ARRAYSZ = 33,
	DT_LOOS = 0x60000000,
	DT_HIOS = 0x6FFFFFFF,
	DT_LOPROC = 0x70000000,
	DT_HIPROC = 0x7FFFFFFF,
	DT_GNU_HASH = 0x6FFFFEF5,
	DT_RELACOUNT = 0x6FFFFFF9,
	DT_RELCOUNT = 0x6FFFFFFA,
	DT_FLAGS_1 = 0X6FFFFFFB,
	DT_VERSYM = 0x6FFFFFF0,
	DT_VERDEF = 0X6FFFFFFC,
	DT_VERDEFNUM = 0X6FFFFFFD,
	DT_VERNEED = 0X6FFFFFFE,
	DT_VERNEEDNUM = 0X6FFFFFFF,
	DT_MIPS_RLD_VERSION = 0x70000001,
	DT_MIPS_TIME_STAMP = 0x70000002,
	DT_MIPS_ICHECKSUM = 0x70000003,
	DT_MIPS_IVERSION = 0x70000004,
	DT_MIPS_FLAGS = 0x70000005,
	DT_MIPS_BASE_ADDRESS = 0x70000006,
	DT_MIPS_MSYM = 0x70000007,
	DT_MIPS_CONFLICT = 0x70000008,
	DT_MIPS_LIBLIST = 0x70000009,
	DT_MIPS_LOCAL_GOTNO = 0x7000000a,
	DT_MIPS_CONFLICTNO = 0x7000000b,
	DT_MIPS_LIBLISTNO = 0x70000010,
	DT_MIPS_SYMTABNO = 0x70000011,
	DT_MIPS_UNREFEXTNO = 0x70000012,
	DT_MIPS_GOTSYM = 0x70000013,
	DT_MIPS_HIPAGENO = 0x70000014,
	DT_MIPS_RLD_MAP = 0x70000016,
	DT_MIPS_DELTA_CLASS = 0x70000017,
	DT_MIPS_DELTA_CLASS_NO = 0x70000018,
	DT_MIPS_DELTA_INSTANCE = 0x70000019,
	DT_MIPS_DELTA_INSTANCE_NO = 0x7000001A,
	DT_MIPS_DELTA_RELOC = 0x7000001B,
	DT_MIPS_DELTA_RELOC_NO = 0x7000001C,
	DT_MIPS_DELTA_SYM = 0x7000001D,
	DT_MIPS_DELTA_SYM_NO = 0x7000001E,
	DT_MIPS_DELTA_CLASSSYM = 0x70000020,
	DT_MIPS_DELTA_CLASSSYM_NO = 0x70000021,
	DT_MIPS_CXX_FLAGS = 0x70000022,
	DT_MIPS_PIXIE_INIT = 0x70000023,
	DT_MIPS_SYMBOL_LIB = 0x70000024,
	DT_MIPS_LOCALPAGE_GOTIDX = 0x70000025,
	DT_MIPS_LOCAL_GOTIDX = 0x70000026,
	DT_MIPS_HIDDEN_GOTIDX = 0x70000027,
	DT_MIPS_PROTECTED_GOTIDX = 0x70000028,
	DT_MIPS_OPTIONS = 0x70000029,
	DT_MIPS_INTERFACE = 0x7000002A,
	DT_MIPS_DYNSTR_ALIGN = 0x7000002B,
	DT_MIPS_INTERFACE_SIZE = 0x7000002C,
	DT_MIPS_RLD_TEXT_RESOLVE_ADDR = 0x7000002D,
	DT_MIPS_PERF_SUFFIX = 0x7000002E,
	DT_MIPS_COMPACT_SIZE = 0x7000002F,
	DT_MIPS_GP_VALUE = 0x70000030,
	DT_MIPS_AUX_DYNAMIC = 0x70000031,
	DT_MIPS_PLTGOT = 0x70000032,
	DT_MIPS_RWPLT = 0x70000034
};

enum {
	DF_ORIGIN = 0x01,
	DF_SYMBOLIC = 0x02,
	DF_TEXTREL = 0x04,
	DF_BIND_NOW = 0x08,
	DF_STATIC_TLS = 0x10
};

enum {
	DF_1_NOW = 0x00000001,
	DF_1_GLOBAL = 0x00000002,
	DF_1_GROUP = 0x00000004,
	DF_1_NODELETE = 0x00000008,
	DF_1_LOADFLTR = 0x00000010,
	DF_1_INITFIRST = 0x00000020,
	DF_1_NOOPEN = 0x00000040,
	DF_1_ORIGIN = 0x00000080,
	DF_1_DIRECT = 0x00000100,
	DF_1_TRANS = 0x00000200,
	DF_1_INTERPOSE = 0x00000400,
	DF_1_NODEFLIB = 0x00000800,
	DF_1_NODUMP = 0x00001000,
	DF_1_CONFALT = 0x00002000,
	DF_1_ENDFILTEE = 0x00004000,
	DF_1_DISPRELDNE = 0x00008000,
	DF_1_DISPRELPND = 0x00010000,
	DF_1_NODIRECT = 0x00020000,
	DF_1_IGNMULDEF = 0x00040000,
	DF_1_NOKSYMS = 0x00080000,
	DF_1_NOHDR = 0x00100000,
	DF_1_EDITED = 0x00200000,
	DF_1_NORELOC = 0x00400000,
	DF_1_SYMINTPOSE = 0x00800000,
	DF_1_GLOBAUDIT = 0x01000000,
	DF_1_SINGLETON = 0x02000000
};

enum {
	RHF_NONE = 0x00000000,
	RHF_QUICKSTART = 0x00000001,
	RHF_NOTPOT = 0x00000002,
	RHS_NO_LIBRARY_REPLACEMENT = 0x00000004,
	RHF_NO_MOVE = 0x00000008,
	RHF_SGI_ONLY = 0x00000010,
	RHF_GUARANTEE_INIT = 0x00000020,
	RHF_DELTA_C_PLUS_PLUS = 0x00000040,
	RHF_GUARANTEE_START_INIT = 0x00000080,
	RHF_PIXIE = 0x00000100,
	RHF_DEFAULT_DELAY_LOAD = 0x00000200,
	RHF_REQUICKSTART = 0x00000400,
	RHF_REQUICKSTARTED = 0x00000800,
	RHF_CORD = 0x00001000,
	RHF_NO_UNRES_UNDEF = 0x00002000,
	RHF_RLD_ORDER_SAFE = 0x00004000
};

enum {
	VER_DEF_NONE = 0,
	VER_DEF_CURRENT = 1
};

enum {
	VER_FLG_BASE = 0x1,
	VER_FLG_WEAK = 0x2,
	VER_FLG_INFO = 0x4
};

enum {
	VER_NDX_LOCAL = 0,
	VER_NDX_GLOBAL = 1,
	VERSYM_VERSION = 0x7fff,
	VERSYM_HIDDEN = 0x8000
};

enum {
	VER_NEED_NONE = 0,
	VER_NEED_CURRENT = 1
};

#endif
