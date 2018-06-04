/*
 * Copyright 2014-2015MIPS Tech, LLC and/or its
 *			affiliated group companies.
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

#ifndef _HAL_H
#define _HAL_H

#include <mips/asm.h>
#include <mips/m32c0.h>

#if _MIPS_SIM == _ABIO32
#define UHI_ABI 0
#elif _MIPS_SIM == _ABIN32
#define UHI_ABI 1
#elif _MIPS_SIM == _ABI64
#define UHI_ABI 2
#else
#error "UHI context structure is not defined for current ABI"
#endif

#define CTX_REG(REGNO)	((SZREG)*((REGNO)-1))

#define CTX_AT		((SZREG)*0)
#define CTX_V0		((SZREG)*1)
#define CTX_V1		((SZREG)*2)
#define CTX_A0		((SZREG)*3)
#define CTX_A1		((SZREG)*4)
#define CTX_A2		((SZREG)*5)
#define CTX_A3		((SZREG)*6)
#if _MIPS_SIM==_ABIN32 || _MIPS_SIM==_ABI64 || _MIPS_SIM==_ABIEABI
# define CTX_A4		((SZREG)*7)
# define CTX_A5		((SZREG)*8)
# define CTX_A6		((SZREG)*9)
# define CTX_A7		((SZREG)*10)
# define CTX_T0		((SZREG)*11)
# define CTX_T1		((SZREG)*12)
# define CTX_T2		((SZREG)*13)
# define CTX_T3		((SZREG)*14)
#else
# define CTX_T0		((SZREG)*7)
# define CTX_T1		((SZREG)*8)
# define CTX_T2		((SZREG)*9)
# define CTX_T3		((SZREG)*10)
# define CTX_T4		((SZREG)*11)
# define CTX_T5		((SZREG)*12)
# define CTX_T6		((SZREG)*13)
# define CTX_T7		((SZREG)*14)
#endif
#define CTX_S0		((SZREG)*15)
#define CTX_S1		((SZREG)*16)
#define CTX_S2		((SZREG)*17)
#define CTX_S3		((SZREG)*18)
#define CTX_S4		((SZREG)*19)
#define CTX_S5		((SZREG)*20)
#define CTX_S6		((SZREG)*21)
#define CTX_S7		((SZREG)*22)
#define CTX_T8		((SZREG)*23)
#define CTX_T9		((SZREG)*24)
#define CTX_K0		((SZREG)*25)
#define CTX_K1		((SZREG)*26)
#define CTX_GP		((SZREG)*27)
#define CTX_SP		((SZREG)*28)
#define CTX_FP		((SZREG)*29)
#define CTX_RA		((SZREG)*30)
#define CTX_EPC		((SZREG)*31)
#define CTX_BADVADDR	((SZREG)*32)
#define CTX_HI0		((SZREG)*33)
#define CTX_LO0		((SZREG)*34)
#define CTX_LINK	((SZREG)*35)
#define CTX_STATUS	(((SZREG)*35)+SZPTR)
#define CTX_CAUSE	(((SZREG)*35)+SZPTR+4)
#define CTX_BADINSTR	(((SZREG)*35)+SZPTR+8)
#define CTX_BADPINSTR	(((SZREG)*35)+SZPTR+12)
#define CTX_SIZE	(((SZREG)*35)+SZPTR+16)

#define LINKCTX_ID	(0)
#define LINKCTX_NEXT	(LINKCTX_ID + SZREG)
#if _MIPS_SIM==_ABIN32
#define LINKCTX_SIZE	(LINKCTX_NEXT + SZPTR + 4)
#else
#define LINKCTX_SIZE	(LINKCTX_NEXT + SZPTR)
#endif

#define SWAPCTX_LINK	(0)
#define SWAPCTX_OLDSP	(SWAPCTX_LINK + LINKCTX_SIZE)
#define SWAPCTX_SIZE	(SWAPCTX_OLDSP + SZREG)

#define DSPCTX_LINK	(0)
#define DSPCTX_DSPC	(DSPCTX_LINK + LINKCTX_SIZE)
#define DSPCTX_HI1	(DSPCTX_DSPC + SZREG)
#define DSPCTX_HI2	(DSPCTX_HI1 + SZREG)
#define DSPCTX_HI3	(DSPCTX_HI2 + SZREG)
#define DSPCTX_LO1	(DSPCTX_HI3 + SZREG)
#define DSPCTX_LO2	(DSPCTX_LO1 + SZREG)
#define DSPCTX_LO3	(DSPCTX_LO2 + SZREG)
#define DSPCTX_SIZE	(DSPCTX_LO3 + SZREG)

#define FPCTX_LINK	(0)
#define FPCTX_FCSR	(FPCTX_LINK + LINKCTX_SIZE)
#define FPCTX_RESERVED	(FPCTX_FCSR + SZREG)
#define _FPCTX_SIZE	(FPCTX_RESERVED + SZREG)
#if defined(__ASSEMBLER__)
#define FPCTX_SIZE	(_FPCTX_SIZE)
#else
#define FPCTX_SIZE	(mips_getsr() & SR_FR ? FP64CTX_SIZE : FP32CTX_SIZE)
#endif

#define MSACTX_LINK	(0)
#define MSACTX_FCSR	(MSACTX_LINK + LINKCTX_SIZE)
#define MSACTX_MSACSR	(MSACTX_FCSR + SZREG)
#define MSACTX_W	(MSACTX_MSACSR + SZREG)
#define MSACTX_D	(MSACTX_W)
#define MSACTX_S	(MSACTX_W)
#define MSACTX_SIZE	(MSACTX_W + 8 * (64))

#define MSACTX_0	(MSACTX_W)
#define MSACTX_1	(MSACTX_0 + (1 * 16))
#define MSACTX_2	(MSACTX_0 + (2 * 16))
#define MSACTX_3	(MSACTX_0 + (3 * 16))
#define MSACTX_4	(MSACTX_0 + (4 * 16))
#define MSACTX_5	(MSACTX_0 + (5 * 16))
#define MSACTX_6	(MSACTX_0 + (6 * 16))
#define MSACTX_7	(MSACTX_0 + (7 * 16))
#define MSACTX_8	(MSACTX_0 + (8 * 16))
#define MSACTX_9	(MSACTX_0 + (9 * 16))
#define MSACTX_10	(MSACTX_0 + (10 * 16))
#define MSACTX_11	(MSACTX_0 + (11 * 16))
#define MSACTX_12	(MSACTX_0 + (12 * 16))
#define MSACTX_13	(MSACTX_0 + (13 * 16))
#define MSACTX_14	(MSACTX_0 + (14 * 16))
#define MSACTX_15	(MSACTX_0 + (15 * 16))
#define MSACTX_16	(MSACTX_0 + (16 * 16))
#define MSACTX_17	(MSACTX_0 + (17 * 16))
#define MSACTX_18	(MSACTX_0 + (18 * 16))
#define MSACTX_19	(MSACTX_0 + (19 * 16))
#define MSACTX_20	(MSACTX_0 + (20 * 16))
#define MSACTX_21	(MSACTX_0 + (21 * 16))
#define MSACTX_22	(MSACTX_0 + (22 * 16))
#define MSACTX_23	(MSACTX_0 + (23 * 16))
#define MSACTX_24	(MSACTX_0 + (24 * 16))
#define MSACTX_25	(MSACTX_0 + (25 * 16))
#define MSACTX_26	(MSACTX_0 + (26 * 16))
#define MSACTX_27	(MSACTX_0 + (27 * 16))
#define MSACTX_28	(MSACTX_0 + (28 * 16))
#define MSACTX_29	(MSACTX_0 + (29 * 16))
#define MSACTX_30	(MSACTX_0 + (30 * 16))
#define MSACTX_31	(MSACTX_0 + (31 * 16))

#define FP32CTX_FP	(0)
#define FP32CTX_D	(FP32CTX_FP + _FPCTX_SIZE)
#define FP32CTX_S	(FP32CTX_D)
#define FP32CTX_SIZE	(FP32CTX_D + 8 * 16)

#define FP32CTX_0	(FP32CTX_D)
#define FP32CTX_2	(FP32CTX_0 + (1 * 8))
#define FP32CTX_4	(FP32CTX_0 + (2 * 8))
#define FP32CTX_6	(FP32CTX_0 + (3 * 8))
#define FP32CTX_8	(FP32CTX_0 + (4 * 8))
#define FP32CTX_10	(FP32CTX_0 + (5 * 8))
#define FP32CTX_12	(FP32CTX_0 + (6 * 8))
#define FP32CTX_14	(FP32CTX_0 + (7 * 8))
#define FP32CTX_16	(FP32CTX_0 + (8 * 8))
#define FP32CTX_18	(FP32CTX_0 + (9 * 8))
#define FP32CTX_20	(FP32CTX_0 + (10 * 8))
#define FP32CTX_22	(FP32CTX_0 + (11 * 8))
#define FP32CTX_24	(FP32CTX_0 + (12 * 8))
#define FP32CTX_26	(FP32CTX_0 + (13 * 8))
#define FP32CTX_28	(FP32CTX_0 + (14 * 8))
#define FP32CTX_30	(FP32CTX_0 + (15 * 8))

#define FP64CTX_FP	(0)
#define FP64CTX_D	(FP64CTX_FP + _FPCTX_SIZE)
#define FP64CTX_S	(FP64CTX_D)
#define FP64CTX_SIZE	(FP64CTX_D + 8 * 32)

#define FP64CTX_0	(FP64CTX_D)
#define FP64CTX_2	(FP64CTX_0 + (1 * 8))
#define FP64CTX_4	(FP64CTX_0 + (2 * 8))
#define FP64CTX_6	(FP64CTX_0 + (3 * 8))
#define FP64CTX_8	(FP64CTX_0 + (4 * 8))
#define FP64CTX_10	(FP64CTX_0 + (5 * 8))
#define FP64CTX_12	(FP64CTX_0 + (6 * 8))
#define FP64CTX_14	(FP64CTX_0 + (7 * 8))
#define FP64CTX_16	(FP64CTX_0 + (8 * 8))
#define FP64CTX_18	(FP64CTX_0 + (9 * 8))
#define FP64CTX_20	(FP64CTX_0 + (10 * 8))
#define FP64CTX_22	(FP64CTX_0 + (11 * 8))
#define FP64CTX_24	(FP64CTX_0 + (12 * 8))
#define FP64CTX_26	(FP64CTX_0 + (13 * 8))
#define FP64CTX_28	(FP64CTX_0 + (14 * 8))
#define FP64CTX_30	(FP64CTX_0 + (15 * 8))
#define FP64CTX_1	(FP64CTX_30 + (1 * 8))
#define FP64CTX_3	(FP64CTX_30 + (2 * 8))
#define FP64CTX_5	(FP64CTX_30 + (3 * 8))
#define FP64CTX_7	(FP64CTX_30 + (4 * 8))
#define FP64CTX_9	(FP64CTX_30 + (5 * 8))
#define FP64CTX_11	(FP64CTX_30 + (6 * 8))
#define FP64CTX_13	(FP64CTX_30 + (7 * 8))
#define FP64CTX_15	(FP64CTX_30 + (8 * 8))
#define FP64CTX_17	(FP64CTX_30 + (9 * 8))
#define FP64CTX_19	(FP64CTX_30 + (10 * 8))
#define FP64CTX_21	(FP64CTX_30 + (11 * 8))
#define FP64CTX_23	(FP64CTX_30 + (12 * 8))
#define FP64CTX_25	(FP64CTX_30 + (13 * 8))
#define FP64CTX_27	(FP64CTX_30 + (14 * 8))
#define FP64CTX_29	(FP64CTX_30 + (15 * 8))
#define FP64CTX_31	(FP64CTX_30 + (16 * 8))

#define XPACTX_LINK	(0)
#define XPACTX_BADVADDR (XPACTX_LINK + LINKCTX_SIZE)
#define XPACTX_SIZE	(XPACTX_BADVADDR + 8)

#define VZROOTCTX_LINK		(0)
#define VZROOTCTX_GUESTCTL0	(VZROOTCTX_LINK + LINKCTX_SIZE)
#define VZROOTCTX_GUESTCTL1	(VZROOTCTX_GUESTCTL0 + 4)
#define VZROOTCTX_GUESTCTL2	(VZROOTCTX_GUESTCTL1 + 4)
#define VZROOTCTX_GUESTCTL3	(VZROOTCTX_GUESTCTL2 + 4)
#define VZROOTCTX_GUESTCTL0EXT	(VZROOTCTX_GUESTCTL3 + 4)
#define VZROOTCTX_GTOFFSET	(VZROOTCTX_GUESTCTL0EXT + 4)
#define VZROOTCTX_SIZE		(VZROOTCTX_GTOFFSET + 4)

#define VZGUESTCTX_ENTRYLO0	  (LINKCTX_SIZE)
#define VZGUESTCTX_ENTRYLO1	  (VZGUESTCTX_ENTRYLO0 + 8)
#define VZGUESTCTX_MAARI	  (VZGUESTCTX_ENTRYLO1 + 8)
#define VZGUESTCTX_CONTEXT	  (VZGUESTCTX_MAARI  + 8)
#define VZGUESTCTX_USERLOCAL	  (VZGUESTCTX_CONTEXT + SZREG)
#define VZGUESTCTX_PAGEMASK	  (VZGUESTCTX_USERLOCAL + SZREG)
#define VZGUESTCTX_SEGCTL0	  (VZGUESTCTX_PAGEMASK + SZREG)
#define VZGUESTCTX_SEGCTL1	  (VZGUESTCTX_SEGCTL0 + SZREG)
#define VZGUESTCTX_SEGCTL2	  (VZGUESTCTX_SEGCTL1 + SZREG)
#define VZGUESTCTX_PWBASE	  (VZGUESTCTX_SEGCTL2 + SZREG)
#define VZGUESTCTX_PWFIELD	  (VZGUESTCTX_PWBASE + SZREG)
#define VZGUESTCTX_PWSIZE	  (VZGUESTCTX_PWFIELD + SZREG)
#define VZGUESTCTX_BADVADDR	  (VZGUESTCTX_PWSIZE + SZREG)
#define VZGUESTCTX_ENTRYHI	  (VZGUESTCTX_BADVADDR + SZREG)
#define VZGUESTCTX_EPC		  (VZGUESTCTX_ENTRYHI + SZREG)
#define VZGUESTCTX_NESTEDEPC	  (VZGUESTCTX_EPC + SZREG)
#define VZGUESTCTX_EBASE	  (VZGUESTCTX_NESTEDEPC + SZREG)
#define VZGUESTCTX_LLADDR	  (VZGUESTCTX_EBASE + SZREG)
#define VZGUESTCTX_WATCHLO0	  (VZGUESTCTX_LLADDR + SZREG)
#define VZGUESTCTX_WATCHLO1	  (VZGUESTCTX_WATCHLO0 + SZREG)
#define VZGUESTCTX_WATCHLO2	  (VZGUESTCTX_WATCHLO1 + SZREG)
#define VZGUESTCTX_WATCHLO3	  (VZGUESTCTX_WATCHLO2 + SZREG)
#define VZGUESTCTX_WATCHLO4	  (VZGUESTCTX_WATCHLO3 + SZREG)
#define VZGUESTCTX_WATCHLO5	  (VZGUESTCTX_WATCHLO4 + SZREG)
#define VZGUESTCTX_WATCHLO6	  (VZGUESTCTX_WATCHLO5 + SZREG)
#define VZGUESTCTX_WATCHLO7	  (VZGUESTCTX_WATCHLO6 + SZREG)
#define VZGUESTCTX_ERROREPC	  (VZGUESTCTX_WATCHLO7 + SZREG)
#define VZGUESTCTX_KSCRATCH1	  (VZGUESTCTX_ERROREPC + SZREG)
#define VZGUESTCTX_KSCRATCH2	  (VZGUESTCTX_KSCRATCH1 + SZREG)
#define VZGUESTCTX_KSCRATCH3	  (VZGUESTCTX_KSCRATCH2 + SZREG)
#define VZGUESTCTX_KSCRATCH4	  (VZGUESTCTX_KSCRATCH3 + SZREG)
#define VZGUESTCTX_KSCRATCH5	  (VZGUESTCTX_KSCRATCH4 + SZREG)
#define VZGUESTCTX_KSCRATCH6	  (VZGUESTCTX_KSCRATCH5 + SZREG)
#define VZGUESTCTX_INDEX	  (VZGUESTCTX_KSCRATCH6 + SZREG)
#define VZGUESTCTX_CONTEXTCONFIG  (VZGUESTCTX_INDEX + 4)
#define VZGUESTCTX_PAGEGRAIN	  (VZGUESTCTX_CONTEXTCONFIG + 4)
#define VZGUESTCTX_WIRED	  (VZGUESTCTX_PAGEGRAIN + 4)
#define VZGUESTCTX_PWCTL	  (VZGUESTCTX_WIRED + 4)
#define VZGUESTCTX_HWRENA	  (VZGUESTCTX_PWCTL + 4)
#define VZGUESTCTX_BADINSTR	  (VZGUESTCTX_HWRENA + 4)
#define VZGUESTCTX_BADINSTRP	  (VZGUESTCTX_BADINSTR + 4)
#define VZGUESTCTX_COUNT	  (VZGUESTCTX_BADINSTRP + 4)
#define VZGUESTCTX_COMPARE	  (VZGUESTCTX_COUNT + 4)
#define VZGUESTCTX_STATUS	  (VZGUESTCTX_COMPARE + 4)
#define VZGUESTCTX_INTCTL	  (VZGUESTCTX_STATUS + 4)
#define VZGUESTCTX_SRSCTL	  (VZGUESTCTX_INTCTL + 4)
#define VZGUESTCTX_SRSMAP	  (VZGUESTCTX_SRSCTL + 4)
#define VZGUESTCTX_CAUSE	  (VZGUESTCTX_SRSMAP + 4)
#define VZGUESTCTX_NESTEDEXC	  (VZGUESTCTX_CAUSE + 4)
#define VZGUESTCTX_CONFIG	  (VZGUESTCTX_NESTEDEXC + 4)
#define VZGUESTCTX_CONFIG1	  (VZGUESTCTX_CONFIG + 4)
#define VZGUESTCTX_CONFIG2	  (VZGUESTCTX_CONFIG1 + 4)
#define VZGUESTCTX_CONFIG3	  (VZGUESTCTX_CONFIG2 + 4)
#define VZGUESTCTX_CONFIG4	  (VZGUESTCTX_CONFIG3 + 4)
#define VZGUESTCTX_CONFIG5	  (VZGUESTCTX_CONFIG4 + 4)
#define VZGUESTCTX_CONFIG6	  (VZGUESTCTX_CONFIG5 + 4)
#define VZGUESTCTX_CONFIG7	  (VZGUESTCTX_CONFIG6 + 4)
#define VZGUESTCTX_WATCHHI0	  (VZGUESTCTX_CONFIG7 + 4)
#define VZGUESTCTX_WATCHHI1	  (VZGUESTCTX_WATCHHI0 + 4)
#define VZGUESTCTX_WATCHHI2	  (VZGUESTCTX_WATCHHI1 + 4)
#define VZGUESTCTX_WATCHHI3	  (VZGUESTCTX_WATCHHI2 + 4)
#define VZGUESTCTX_WATCHHI4	  (VZGUESTCTX_WATCHHI3 + 4)
#define VZGUESTCTX_WATCHHI5	  (VZGUESTCTX_WATCHHI4 + 4)
#define VZGUESTCTX_WATCHHI6	  (VZGUESTCTX_WATCHHI5 + 4)
#define VZGUESTCTX_WATCHHI7	  (VZGUESTCTX_WATCHHI6 + 4)
// Bump by 4 to account for the gap between the WatchHI7 and MAAR fields.
#define VZGUESTCTX_MAAR		  (VZGUESTCTX_WATCHHI7 + 4 + 4)

#define VZTLBCTXENTRY_PAGEMASK	(0)
#define VZTLBCTXENTRY_ENTRYHI	(VZTLBCTXENTRY_PAGEMASK + SZREG)
#define VZTLBCTXENTRY_ENTRYLO0	(VZTLBCTXENTRY_ENTRYHI + SZREG)
#define VZTLBCTXENTRY_ENTRYLO1	(VZTLBCTXENTRY_ENTRYLO0 + 8)
#define VZTLBCTXENTRY_GUESTCTL1	(VZTLBCTXENTRY_ENTRYLO1 + 8)
#define VZTLBCTXENTRY_PAD	(VZTLBCTXENTRY_GUESTCTL1 + SZREG)
#define VZTLBCTXENTRY_SIZE	(VZTLBCTXENTRY_PAD + SZREG)

#define VZTLBCTX_LINK		(0)
#define VZTLBCTX_ENTRIES	(VZTLBCTX_LINK + LINKCTX_SIZE)

#define LINKCTX_TYPE_MSA	0x004D5341
#define LINKCTX_TYPE_FP32	0x46503332
#define LINKCTX_TYPE_FP64	0x46503634
#define LINKCTX_TYPE_FMSA	0x463D5341
#define LINKCTX_TYPE_DSP	0x00445350
#define LINKCTX_TYPE_STKSWP	0x53574150
#define LINKCTX_TYPE_XPA	0x00585041
#define LINKCTX_TYPE_VZROOT	0x565a4254
#define LINKCTX_TYPE_VZGUEST	0x565a4754
#define LINKCTX_TYPE_VZTLBCTX	0x47544c42

#define LINKCTX_TYPE(X) (((struct linkctx *)(X))->id)

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct linkctx;

struct gpctx
{
  union
  {
    struct
    {
      reg_t at;
      reg_t v[2];
# if _MIPS_SIM==_ABIN32 || _MIPS_SIM==_ABI64 || _MIPS_SIM==_ABIEABI
      reg_t a[8];
      reg_t t[4];
# else
      reg_t a[4];
      reg_t t[8];
# endif
      reg_t s[8];
      reg_t t2[2];
      reg_t k[2];
      reg_t gp;
      reg_t sp;
      reg_t fp;
      reg_t ra;
    };
    reg_t r[31];
  };

  reg_t epc;
  reg_t badvaddr;
  reg_t hi;
  reg_t lo;
  /* This field is for future extension */
  struct linkctx *link;
  /* Status at the point of the exception.  This may not be restored
     on return from exception if running under an RTOS */
  uint32_t status;
  /* These fields should be considered read-only */
  uint32_t cause;
  uint32_t badinstr;
  uint32_t badpinstr;
};

struct linkctx
{
  reg_t id;
  struct linkctx *next;
};

struct swapctx
{
  struct linkctx link;
  reg_t oldsp;
};

static inline void
_linkctx_append (struct gpctx *gp, struct linkctx *nu)
{
  struct linkctx **ctx = (struct linkctx **)&gp->link;
  while (*ctx)
    ctx = &(*ctx)->next;
  *ctx = nu;
}

struct dspctx
{
  struct linkctx link;
  reg_t dspc;
  reg_t hi[3];
  reg_t lo[3];
};

struct fpctx
{
  struct linkctx link;
  reg_t fcsr;
  reg_t reserved;
};

typedef char _msareg[16] __attribute__ ((aligned(16)));

struct msactx
{
  struct linkctx link;
  reg_t fcsr;
  reg_t msacsr;
  union
    {
      _msareg w[32];
      double d[64];
      float s[128];
    };
};

#define MSAMSACTX_D(CTX, N) (CTX)->w[(N)]
#define MSACTX_DBL(CTX, N) (CTX)->d[(N) << 1]
#ifdef __MIPSEL__
#define MSACTX_SGL(CTX, N) (CTX)->s[(N) << 2]
#else
#define MSACTX_SGL(CTX, N) (CTX)->s[((N) << 2) | 1]
#endif

struct fp32ctx
{
  struct fpctx fp;
  union
    {
      double d[16];	/* even doubles */
      float s[32];	/* even singles, padded */
    };
};

#define FP32CTX_DBL(CTX, N) (CTX)->d[(N)]
#ifdef __MIPSEL__
#define FP32CTX_SGL(CTX, N) (CTX)->s[(N)]
#else
#define FP32CTX_SGL(CTX, N) (CTX)->s[(N) ^ 1]
#endif

struct fp64ctx
{
  struct fpctx fp;
  union
    {
      double d[32];	/* even doubles, followed by odd doubles */
      float s[64];	/* even singles, followed by odd singles, padded */
    };
};

#define FP64CTX_DBL(CTX, N) (CTX)->d[((N) >> 1) + (((N) & 1) << 4)]
#ifdef __MIPSEL__
#define FP64CTX_SGL(CTX, N) (CTX)->s[((N) & ~1) + (((N) & 1) << 5)]
#else
#define FP64CTX_SGL(CTX, N) (CTX)->s[((N) | 1) + (((N) & 1) << 5)]
#endif

struct xpactx
{
  struct linkctx link;
  reg64_t badvaddr;
};

extern reg_t _xpa_save (struct xpactx *ptr);

extern reg_t _fpctx_save (struct fpctx *ptr);
extern reg_t _fpctx_load (struct fpctx *ptr);
extern reg_t _msactx_save (struct msactx *ptr);
extern reg_t _msactx_load (struct msactx *ptr);
extern reg_t _dspctx_save (struct dspctx *ptr);
extern reg_t _dspctx_load (struct dspctx *ptr);

struct vzrootctx
{
  struct linkctx link;
  reg32_t  GuestCtl0;
  reg32_t  GuestCtl1;
  reg32_t  GuestCtl2;
  reg32_t  GuestCtl3;
  reg32_t  GuestCtl0Ext;
  reg32_t  GTOffset;
};

struct vzguestctx
{
  struct linkctx link;

  reg64_t EntryLo0;
  reg64_t EntryLo1;
  reg64_t MAARI;

  reg_t   Context;
  reg_t   UserLocal;
  reg_t   PageMask;
  reg_t   SegCtl0;
  reg_t   SegCtl1;
  reg_t   SegCtl2;
  reg_t   PWBase;
  reg_t   PWField;
  reg_t   PWSize;
  reg_t   BadVAddr;
  reg_t   EntryHi;
  reg_t   EPC;
  reg_t   NestedEPC;
  reg_t   EBase;
  reg_t   LLAddr;
  reg_t   WatchLo0;
  reg_t   WatchLo1;
  reg_t   WatchLo2;
  reg_t   WatchLo3;
  reg_t   WatchLo4;
  reg_t   WatchLo5;
  reg_t   WatchLo6;
  reg_t   WatchLo7;
  reg_t   ErrorEPC;
  reg_t   KScratch1;
  reg_t   KScratch2;
  reg_t   KScratch3;
  reg_t   KScratch4;
  reg_t   KScratch5;
  reg_t   KScratch6;

  reg32_t Index;
  reg32_t ContextConfig;
  reg32_t PageGrain;
  reg32_t Wired;
  reg32_t PWCtl;
  reg32_t HWREna;
  reg32_t BadInstr;
  reg32_t BadInstrP;
  reg32_t Count;
  reg32_t Compare;
  reg32_t Status;
  reg32_t IntCtl;
  reg32_t SRSCtl;
  reg32_t SRSMap;
  reg32_t Cause;
  reg32_t NestedExc;
  reg32_t Config;
  reg32_t Config1;
  reg32_t Config2;
  reg32_t Config3;
  reg32_t Config4;
  reg32_t Config5;
  reg32_t Config6;
  reg32_t Config7;
  reg32_t WatchHi0;
  reg32_t WatchHi1;
  reg32_t WatchHi2;
  reg32_t WatchHi3;
  reg32_t WatchHi4;
  reg32_t WatchHi5;
  reg32_t WatchHi6;
  reg32_t WatchHi7;

  reg64_t MAAR[];
};

#define VZGUESTCTX_SIZE()	  __extension__ ({ \
  reg_t maari, max = 0; \
  if (mips32_getconfig5() & CFG5_MRP) { \
	maari =_m32c0_mfc0(C0_MAARI); \
	_m32c0_mtc0(C0_MAARI, -1); \
	max = _m32c0_mfc0(C0_MAARI); \
	_m32c0_mtc0(C0_MAARI, maari); \
  }; \
  VZGUESTCTX_MAAR + (max * 8); \
)}

struct vzguestctxmax
{
  struct linkctx link;

  reg64_t EntryLo0;
  reg64_t EntryLo1;
  reg64_t MAARI;

  reg_t   Context;
  reg_t   UserLocal;
  reg_t   PageMask;
  reg_t   SegCtl0;
  reg_t   SegCtl1;
  reg_t   SegCtl2;
  reg_t   PWBase;
  reg_t   PWField;
  reg_t   PWSize;
  reg_t   BadVAddr;
  reg_t   EntryHi;
  reg_t   EPC;
  reg_t   NestedEPC;
  reg_t   EBase;
  reg_t   LLAddr;
  reg_t   WatchLo0;
  reg_t   WatchLo1;
  reg_t   WatchLo2;
  reg_t   WatchLo3;
  reg_t   WatchLo4;
  reg_t   WatchLo5;
  reg_t   WatchLo6;
  reg_t   WatchLo7;
  reg_t   ErrorEPC;
  reg_t   KScratch1;
  reg_t   KScratch2;
  reg_t   KScratch3;
  reg_t   KScratch4;
  reg_t   KScratch5;
  reg_t   KScratch6;

  reg32_t Index;
  reg32_t ContextConfig;
  reg32_t PageGrain;
  reg32_t Wired;
  reg32_t PWCtl;
  reg32_t HWREna;
  reg32_t BadInstr;
  reg32_t BadInstrP;
  reg32_t Count;
  reg32_t Compare;
  reg32_t Status;
  reg32_t IntCtl;
  reg32_t SRSCtl;
  reg32_t SRSMap;
  reg32_t Cause;
  reg32_t NestedExc;
  reg32_t Config;
  reg32_t Config1;
  reg32_t Config2;
  reg32_t Config3;
  reg32_t Config4;
  reg32_t Config5;
  reg32_t Config6;
  reg32_t Config7;
  reg32_t WatchHi0;
  reg32_t WatchHi1;
  reg32_t WatchHi2;
  reg32_t WatchHi3;
  reg32_t WatchHi4;
  reg32_t WatchHi5;
  reg32_t WatchHi6;
  reg32_t WatchHi7;

  reg64_t MAAR[64];
};

#define VZGUESTCTX_MAXSIZE	(VZGUESTCTX_MAAR + (64 * 8))

struct vztlbctx_entry
{
  reg_t PageMask;
  reg_t EntryHi;
  reg64_t EntryLo0;
  reg64_t EntryLo1;
  reg_t GuestCtl1;
  reg_t _pad;
};

struct vztlbctx
{
  struct linkctx link;
  struct vztlbctx_entry entries[];
};

#define VZTLBCTX_SIZE() (((((mips32_getconfig1() & CFG1_MMUS_MASK) >> \
			  CFG1_MMUS_SHIFT) + 1) * VZTLBCTXENTRY_SIZE) + \
			  LINKCTX_SIZE)

struct vztlbctxmax
{
  struct linkctx link;
  struct vztlbctx_entry entries[64];
};

#define VZTLBCTXMAX_SIZE      (VZTLBCTX_ENTRIES + (VZTLBCTXENTRY_SIZE) * 64)

extern reg_t _vzrootctx_save (struct vzrootctx *ptr);
extern reg_t _vzrootctx_load (struct vzrootctx *ptr);
extern reg_t _vzguestctx_save (struct vzguestctx *ptr);
extern reg_t _vzguestctx_load (struct vzguestctx *ptr);
extern reg_t _vztlbctx_save (struct vztlbctx *ptr);
extern reg_t _vztlbctx_load (struct vztlbctx *ptr);

/* Fall back exception handlers:
   _mips_handle_exception - May be implemented by a user but is aliased
			    to __exception_handle by default.
   __exception_handle	  - Toolchain provided fallback handler.
*/
extern void _mips_handle_exception (struct gpctx *ctx, int exception);
extern void __exception_handle (struct gpctx *ctx, int exception);
extern void __exception_handle_quiet (struct gpctx *ctx, int exception);
extern void __exception_handle_verbose (struct gpctx *ctx, int exception);

/* Obtain the largest available region of RAM */
extern void _get_ram_range (void **ram_base, void **ram_extent);

/* Invoke a UHI operation via SDBBP using the provided context */
extern int __uhi_indirect (struct gpctx *);

/* Report an unhandled exception */
extern int32_t __uhi_exception (struct gpctx *, int32_t);

/* Print a message to a logger.  Minimal formatting support for one
   integer argument.  */
extern int32_t __plog (int8_t *, int32_t);

/* Boot context support functions */
extern int __get_startup_BEV (void) __attribute__((weak));
extern int __chain_uhi_excpt (struct gpctx *) __attribute__((weak));

/* Emergency exit, use it when unrecoverable errors occur */
extern int __exit (int);

#endif

#ifdef __cplusplus
}
#endif

#endif // _HAL_H
