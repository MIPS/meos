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

#ifndef _M64C0_H_
#define _M64C0_H_

/* Superset of MIPS32 */
#include <mips/m32c0.h>

/*
 * Define macros for accessing the MIPS coprocessor 0 registers which are
 * 64 bits wide.
 * Most apart from "set" return the original register value.
 */

#define mips64_get_c0(selreg) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("dmfc0 %0,$%1,%2" \
		      : "=d" (__r) \
		      : "JK" (selreg & 0x1F), "JK" (selreg >> 8)); \
  __r; \
})

#define mips64_set_c0(selreg, val) \
do { \
    __asm__ __volatile (".set push \n"\
			".set noreorder\n"\
			"dmtc0 %z0,$%1,%2\n"\
			"ehb\n" \
			".set pop" \
			: \
			: "dJ" ((reg64_t)(val)), "JK" (selreg & 0x1F),\
			  "JK" (selreg >> 8) \
			: "memory"); \
} while (0)

#define mips64_xch_c0(selreg, val) \
__extension__ ({ \
    register reg64_t __o; \
    __o = mips64_get_c0 (selreg); \
    mips64_set_c0 (selreg, val); \
    __o; \
})

#define mips64_bc_c0(selreg, clr) \
__extension__ ({ \
    register reg64_t __o; \
    __o = mips64_get_c0 (selreg); \
    mips64_set_c0 (selreg, __o & ~(clr)); \
    __o; \
})

#define mips64_bs_c0(selreg, set) \
__extension__ ({ \
    register reg64_t __o; \
    __o = mips64_get_c0 (selreg); \
    mips64_set_c0 (selreg, __o | (set)); \
    __o; \
})

#define mips64_bcs_c0(selreg, clr, set) \
__extension__ ({ \
    register reg64_t __o; \
    __o = mips64_get_c0 (selreg); \
    mips64_set_c0 (selreg, (__o & ~(clr)) | (set)); \
    __o; \
})

/* MIPS64 TagLo register */
#define mips64_getitaglo()	mips64_get_c0(C0_TAGLO)		/* alias define */
#define mips64_setitaglo(x)	mips64_set_c0(C0_TAGLO,x)	/* alias define */
#define mips64_xchitaglo(x)	mips64_xch_c0(C0_TAGLO,x)	/* alias define */
#define mips64_getdtaglo()	mips64_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 2))
#define mips64_setdtaglo(x)	mips64_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 2),x)
#define mips64_xchdtaglo(x)	mips64_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 2),x)
#define mips64_gettaglo2()	mips64_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 4))
#define mips64_settaglo2(x)	mips64_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 4),x)
#define mips64_xchtaglo2(x)	mips64_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 4),x)

/* MIPS64 DataLo register */
#define mips64_getdatalo()	mips64_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 1))
#define mips64_setdatalo(x)	mips64_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 1),x)
#define mips64_xchdatalo(x)	mips64_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 1),x)
#define mips64_getidatalo()	mips64_getdatalo()	/* alias define */
#define mips64_setidatalo(x)	mips64_setdatalo(x)	/* alias define */
#define mips64_xchidatalo(x)	mips64_xchdatalo(x)	/* alias define */
#define mips64_getddatalo()	mips64_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 3))
#define mips64_setddatalo(x)	mips64_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 3),x)
#define mips64_xchddatalo(x)	mips64_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 3),x)
#define mips64_getdatalo2()	mips64_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 5))
#define mips64_setdatalo2(x)	mips64_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 5),x)
#define mips64_xchdatalo2(x)	mips64_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 5),x)

/* CP0 TagHi register */
#define mips64_gettaghi()	mips64_get_c0(C0_TAGHI)
#define mips64_settaghi(x)	mips64_set_c0(C0_TAGHI, x)
#define mips64_xchtaghi(x)	mips64_xch_c0(C0_TAGHI, x)

/* CP0 WatchLo register */
#define mips64_getwatchlo()	mips64_get_c0(C0_WATCHLO)
#define mips64_setwatchlo(x)	mips64_set_c0(C0_WATCHLO, x)
#define mips64_xchwatchlo(x)	mips64_xch_c0(C0_WATCHLO, x)

#define _m64c0_mfc0(reg, sel) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("dmfc0 %0,$%1,%2" \
		      : "=d" (__r) \
      		      : "JK" (reg), "JK" (sel)); \
  __r; \
})

#define _m64c0_mtc0(reg, sel, val) \
do { \
    __asm__ __volatile (".set push \n"\
			".set noreorder\n"\
			"dmtc0 %z0,$%1,%2\n"\
			"ehb\n" \
			".set pop" \
			: \
			: "dJ" ((reg64_t)(val)), "JK" (reg), "JK" (sel) \
			: "memory"); \
} while (0)

#define _m64c0_mxc0(reg, sel, val) \
__extension__ ({ \
    register reg64_t __o; \
    __o = _m64c0_mfc0 (reg, sel); \
    _m64c0_mtc0 (reg, sel, val); \
    __o; \
})

#define _m64c0_bcc0(reg, sel, clr) \
__extension__ ({ \
    register reg64_t __o; \
    __o = _m64c0_mfc0 (reg, sel); \
    _m64c0_mtc0 (reg, sel, __o & ~(clr)); \
    __o; \
})

#define _m64c0_bsc0(reg, sel, set) \
__extension__ ({ \
    register reg64_t __o; \
    __o = _m64c0_mfc0 (reg, sel); \
    _m64c0_mtc0 (reg, sel, __o | (set)); \
    __o; \
})

#define _m64c0_bcsc0(reg, sel, clr, set) \
__extension__ ({ \
    register reg64_t __o; \
    __o = _m64c0_mfc0 (reg, sel); \
    _m64c0_mtc0 (reg, sel, (__o & ~(clr)) | (set)); \
    __o; \
})

/* Define MIPS64 user-level intrinsics */
#include <mips/mips64.h>

/* MIPS64-specific MMU interface */
#include <mips/m64tlb.h>

#endif /* _M64C0_H_ */
