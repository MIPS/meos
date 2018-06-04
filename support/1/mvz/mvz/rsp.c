#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include <mips/cpu.h>
#include <mips/m32c0.h>
#include <mips/m32c1.h>
#include <mips/endian.h>
#include <sys/signal.h>

/* TODO:
 Use standard set_debug_traps(), handle_exception(), breakpoint() entry points. Use int getDebugChar(), void putDebugchar(int), and void exceptionHandler(int exception_number, void *exception_address).
 */

static reg_t excSig[] = {
	SIGINT,			/* EXC_INTR */
	SIGSEGV,		/* EXC_MOD */
	SIGSEGV,		/* EXC_TLBL */
	SIGSEGV,		/* EXC_TLBS */
	SIGSEGV,		/* EXC_ADEL */
	SIGSEGV,		/* EXC_ADES */
	SIGBUS,			/* EXC_IBE */
	SIGBUS,			/* EXC_DBE */
	SIGSYS,			/* EXC_SYS */
	SIGTRAP,		/* EXC_BP */
	SIGILL,			/* EXC_RI */
	SIGFPE,			/* EXC_CPU */
	SIGFPE,			/* EXC_OVF */
	SIGTRAP,		/* EXC_TRAP */
	SIGFPE,			/* EXC_MSA */
	SIGFPE,			/* EXC_FPE */
	SIGILL,			/* EXC_IS1 */
	SIGILL,			/* EXC_IS2 */
	SIGFPE,			/* EXC_C2E */
	SIGSEGV,		/* EXC_TLBRI */
	SIGSEGV,		/* EXC_TLBXI */
	SIGFPE,			/* EXC_MSAU */
	SIGFPE,			/* EXC_MDMX */
	SIGTRAP,		/* EXC_WATCH */
	SIGILL,			/* EXC_MCHECK */
	SIGTRAP,		/* EXC_THREAD */
	SIGFPE,			/* EXC_DSPU */
	SIGTRAP,		/* EXC_RES27 GEXC */
	SIGTRAP,		/* EXC_RES28 */
	SIGTRAP,		/* EXC_RES29 */
	SIGBUS,			/* EXC_RES30 CACHEERR */
	SIGTRAP			/* EXC_RES31 */
};

static uintptr_t goffsets[] = {
	VZGUESTCTX_INDEX, -1, -1, -1, -1, -1, -1, -1,	/* 0 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 1 */
	VZGUESTCTX_ENTRYLO0, -1, -1, -1, -1, -1, -1, -1,	/* 2 */
	VZGUESTCTX_ENTRYLO1, -1, -1, -1, -1, -1, -1, -1,	/* 3 */
	VZGUESTCTX_CONTEXT, VZGUESTCTX_CONTEXTCONFIG, VZGUESTCTX_USERLOCAL, -1, -1, -1, -1, -1,	/* 4 */
	VZGUESTCTX_PAGEMASK, VZGUESTCTX_PAGEGRAIN, VZGUESTCTX_SEGCTL0, VZGUESTCTX_SEGCTL1, VZGUESTCTX_SEGCTL2, VZGUESTCTX_PWBASE, VZGUESTCTX_PWFIELD, VZGUESTCTX_PWSIZE,	/* 5 */
	VZGUESTCTX_WIRED, -1, -1, -1, -1, -1, VZGUESTCTX_PWCTL, -1,	/* 6 */
	VZGUESTCTX_HWRENA, -1, -1, -1, -1, -1, -1, -1,	/* 7 */
	VZGUESTCTX_BADVADDR, VZGUESTCTX_BADINSTR, VZGUESTCTX_BADINSTRP, -1, -1, -1, -1, -1,	/* 8 */
	VZGUESTCTX_COUNT, -1, -1, -1, -1, -1, -1, -1,	/* 9 */
	VZGUESTCTX_ENTRYHI, -1, -1, -1, -1, -1, -1, -1,	/* 10 */
	VZGUESTCTX_COMPARE, -1, -1, -1, -1, -1, -1, -1,	/* 11 */
	VZGUESTCTX_STATUS, VZGUESTCTX_INTCTL, VZGUESTCTX_SRSCTL, VZGUESTCTX_SRSMAP, -1, -1, -1, -1,	/* 12 */
	VZGUESTCTX_CAUSE, -1, -1, -1, -1, VZGUESTCTX_NESTEDEXC, -1, -1,	/* 13 */
	VZGUESTCTX_EPC, -1, VZGUESTCTX_NESTEDEPC, -1, -1, -1, -1, -1,	/* 14 */
	-1, VZGUESTCTX_EBASE, -1, -1, -1, -1, -1, -1,	/* 15 */
	VZGUESTCTX_CONFIG, VZGUESTCTX_CONFIG1, VZGUESTCTX_CONFIG2, VZGUESTCTX_CONFIG3, VZGUESTCTX_CONFIG4, VZGUESTCTX_CONFIG5, VZGUESTCTX_CONFIG6, VZGUESTCTX_CONFIG7,	/* 16 */
	VZGUESTCTX_LLADDR, VZGUESTCTX_MAAR, VZGUESTCTX_MAARI, -1, -1, -1, -1, -1,	/* 17 */
	VZGUESTCTX_WATCHLO0, VZGUESTCTX_WATCHLO1, VZGUESTCTX_WATCHLO2, VZGUESTCTX_WATCHLO3, VZGUESTCTX_WATCHLO4, VZGUESTCTX_WATCHLO5, VZGUESTCTX_WATCHLO6, VZGUESTCTX_WATCHLO7,	/* 18 */
	VZGUESTCTX_WATCHHI0, VZGUESTCTX_WATCHHI1, VZGUESTCTX_WATCHHI2, VZGUESTCTX_WATCHHI3, VZGUESTCTX_WATCHHI4, VZGUESTCTX_WATCHHI5, VZGUESTCTX_WATCHHI6, VZGUESTCTX_WATCHHI7,	/* 19 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 20 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 21 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 22 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 23 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 24 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 25 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 26 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 27 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 28 */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* 29 */
	VZGUESTCTX_ERROREPC, -1, -1, -1, -1, -1, -1, -1,	/* 30 */
	-1, -1, VZGUESTCTX_KSCRATCH1, VZGUESTCTX_KSCRATCH2, VZGUESTCTX_KSCRATCH3, VZGUESTCTX_KSCRATCH4, VZGUESTCTX_KSCRATCH5, VZGUESTCTX_KSCRATCH6	/* 31 */
};

void cp0name(int index, char *name)
{
	reg_t sel, reg;
	switch (index) {
	case 8 * 8:
		memcpy(name, "badvaddr", 9);
		break;
	case 12 * 8:
		memcpy(name, "status", 7);
		break;
	case 13 * 8:
		memcpy(name, "cause", 6);
		break;
	default:
		sel = index & 0x7;
		reg = index >> 3;
		sprintf(name, "cp0.%u.%u", reg, sel);
	}
}

extern uintptr_t _mips_nextpc(struct gpctx *ctx, uint32_t one, uint32_t two);

#define NOSAVE
#define MEOS

/*
 * Short cut hardware detection - can be overridden, for optimisation purposes
 */

#ifndef MSA
#define MSA ((mips32_getconfig() & CFG0_M) && (mips32_getconfig1() & CFG1_M) && (mips32_getconfig2() & CFG2_M) && (mips32_getconfig3() & CFG3_MSAP))
#endif
#ifndef FP
#define FP ((mips32_getconfig() & CFG0_M) && (mips32_getconfig1() & CFG1_FP))
#endif
#ifndef FP64
#define FP64 ((mips32_getconfig() & CFG0_M) && (mips32_getconfig1() & CFG1_FP)&& (fpa_getfir() & FPA_FIR_F64))
#endif
#ifndef FP32
#define FP32 ((mips32_getconfig() & CFG0_M) && (mips32_getconfig1() & CFG1_FP)&& !(fpa_getfir() & FPA_FIR_F64))
#endif
#ifndef DSP
#define DSP ((mips32_getconfig() & CFG0_M) && (mips32_getconfig() & CFG1_M) && (mips32_getconfig2() & CFG2_M) && (mips32_getconfig3() & CFG3_DSPP))
#endif

/*
 * I/O functions
 */

static inline void setWriteWindow(RSP_T * rsp, size_t offset, size_t len,
				  char startChar)
{
	rsp->startChar = startChar;
	rsp->writeOffset = offset;
	rsp->writeLen = (reg_t) len;
}

static inline reg_t clearWriteWindow(RSP_T * rsp)
{
	reg_t r = rsp->writeOffset ? 0 : (rsp->startChar ? 0 : 1);
	rsp->startChar = 0;
	rsp->writeOffset = 0;
	rsp->writeLen = -1;
	return r;
}

static inline void writeChar(RSP_T * rsp, uint8_t value)
{
	if (rsp->writeOffset) {
		rsp->writeOffset--;
	} else {
		if (rsp->startChar) {
			rsp->writeCheckSum += rsp->startChar;
			rsp->writeChar(rsp, rsp->startChar);
			rsp->startChar = 0;
		}
		if (rsp->writeLen) {
			if (rsp->writeLen > 0)
				rsp->writeLen--;
			rsp->writeCheckSum += value;
			rsp->writeChar(rsp, value);
		}
	}
}

static inline void writeHex(RSP_T * rsp, uint8_t value)
{
	writeChar(rsp, "0123456789ABCDEF"[value >> 4]);
	writeChar(rsp, "0123456789ABCDEF"[value & 0xf]);
}

static inline void writeHexes(RSP_T * rsp, const uint8_t * buffer, size_t len)
{
	reg_t i;
	for (i = 0; i < len; i++)
		writeHex(rsp, buffer[i]);
}

static inline void writeStart(RSP_T * rsp)
{
	rsp->writeCheckSum = 0;
	rsp->writeChar(rsp, '$');
}

static inline void writeEnd(RSP_T * rsp)
{
	clearWriteWindow(rsp);
	rsp->writeChar(rsp, '#');
	writeHex(rsp, rsp->writeCheckSum);
}

static inline void writeBuffer(RSP_T * rsp, const uint8_t * buffer, size_t len)
{
	reg_t i;
	for (i = 0; i < len; i++)
		writeChar(rsp, buffer[i]);
}

static inline uint8_t readChar(RSP_T * rsp)
{
	uint8_t value = rsp->readChar(rsp);
	if (value != '#')
		rsp->readCheckSum += value;
	return value;
}

static inline reg_t hexDigit(uint8_t digit)
{
	if ((digit >= '0') && (digit <= '9'))
		return digit - '0';
	else if ((digit >= 'a') && (digit <= 'f'))
		return 10 + digit - 'a';
	else if ((digit >= 'A') && (digit <= 'F'))
		return 10 + digit - 'a';
	else
		return -1;
}

static inline reg_t readHexDigit(RSP_T * rsp)
{
	return hexDigit(readChar(rsp));
}

static inline reg_t readHex(RSP_T * rsp, size_t maxbyte)
{
	reg_t result = 0;
	maxbyte *= 2;
	while (maxbyte--) {
		sreg_t value = readHexDigit(rsp);
		if (value < 0)
			return result;
		result = result << 4 | value;
	}
	return result;
}

static inline void readHexes(RSP_T * rsp, uint8_t * buffer, size_t len)
{
	while (len--)
		*buffer++ = readHex(rsp, 1);
}

static inline void readBuffer(RSP_T * rsp, uint8_t * buffer, size_t len)
{
	while (len--)
		*buffer++ = readChar(rsp);
}

static inline sreg_t readStart(RSP_T * rsp)
{
	uint8_t value = rsp->readChar(rsp);
	rsp->readCheckSum = 0;
	switch (value) {
	case 0x03:
		return -1;
	case '$':
		return 1;
	default:
		return 0;
	}
}

static inline reg_t ackEnd(RSP_T * rsp)
{
	reg_t calculated = rsp->readCheckSum;
	reg_t expected = readHex(rsp, 1);
	reg_t r = calculated == expected;
	writeChar(rsp, r ? '+' : '-');
	return r;
}

static inline reg_t readEnd(RSP_T * rsp)
{
	if (readChar(rsp) == '#')
		return ackEnd(rsp);
	else
		return 0;
}

static inline reg_t readConfirm(RSP_T * rsp, const char *test)
{
	uint8_t value;
	/* Try reading matching characters */
	while (*test != 0) {
		value = readChar(rsp);
		if (*test != value) {
			/* Failed - read the rest of the command, and report failure */
			while (!readEnd(rsp)) ;
			writeStart(rsp);
			writeEnd(rsp);
			return 0;
		}
		test++;
	}
	/* Suceeded */
	return 1;
}

static inline void writeStr(RSP_T * rsp, const char *str)
{
	writeBuffer(rsp, (const uint8_t *)str, strlen(str));
}

static inline void writeDec(RSP_T * rsp, reg_t value)
{
	char buf[40];
	sprintf(buf, "%llu", (long long unsigned)value);
	writeStr(rsp, buf);
}

static inline void writeOk(RSP_T * rsp)
{
	writeStart(rsp);
	writeStr(rsp, "OK");
	writeEnd(rsp);
}

static inline void writeSimple(RSP_T * rsp, const char response,
			       const uint8_t result)
{
	writeStart(rsp);
	writeChar(rsp, response);
	writeHex(rsp, result);
	writeEnd(rsp);
}

/*
 * Register manipulation
 */

size_t regSize(reg_t index)
{
	if ((index < 291) || (index > 322) || (FP32))
		return 4;
	if (MSA)
		return 16;
	if (FP64)
		return 8;
	return 4;
}

/* Search a chain of contexts for a DSP context */
static inline struct dspctx *getdspctx(struct gpctx *ctx)
{
#ifdef NOSAVE
	/* Tickle the DSP to ensure its context has been saved */
	asm(".set push\n.set dsp\nrddsp $0\n.set pop");
#endif
	struct dspctx *dctx = (struct dspctx *)ctx->link;
	while (dctx && (LINKCTX_TYPE(dctx) != LINKCTX_TYPE_DSP))
		dctx = (struct dspctx *)dctx->link.next;
	return dctx;
}

/* Search a chain of contexts for an FPU context */
static inline struct msactx *getfpctx(struct gpctx *ctx)
{
	struct linkctx *lctx = (struct linkctx *)ctx->link;
	while (lctx && (LINKCTX_TYPE(lctx) != LINKCTX_TYPE_FP32)
	       && (LINKCTX_TYPE(lctx) != LINKCTX_TYPE_FP64)
	       && (LINKCTX_TYPE(lctx) != LINKCTX_TYPE_FMSA)
	       && (LINKCTX_TYPE(lctx) != LINKCTX_TYPE_MSA))
		lctx = lctx->next;
	return (struct msactx *)lctx;
}

static inline struct vzguestctx *getgctx(struct gpctx *ctx)
{
	struct linkctx *lctx = (struct linkctx *)ctx->link;
	while (lctx && (LINKCTX_TYPE(lctx) != LINKCTX_TYPE_VZGUEST))
		lctx = lctx->next;
	return (struct vzguestctx *)lctx;
}

static inline reg_t getcp0(struct gpctx *ctx, reg_t index)
{
	struct vzguestctx *gctx = getgctx(ctx);
	if (gctx) {
		if (goffsets[index] != -1)
			return *(reg_t *) (((uintptr_t) gctx) +
					   goffsets[index]);
		else
			return 0;
	} else {
		switch (index) {
		case (14 * 8) + 0:
			return ctx->epc;
		case (8 * 8) + 0:
			return ctx->badvaddr;
		case (12 * 8) + 0:
			return ctx->status;
		case (13 * 8) + 0:
			return ctx->cause;
		case (8 * 8) + 1:
			return ctx->badinstr;
		case (8 * 8) + 2:
			return ctx->badpinstr;
#define CP0C(REG, SEL) case ((REG)*8)+(SEL): return _m32c0_mfc0((REG),(SEL))
			CP0C(0, 0);
			CP0C(0, 1);
			CP0C(0, 2);
			CP0C(0, 3);
			CP0C(0, 4);
			CP0C(0, 5);
			CP0C(0, 6);
			CP0C(0, 7);
			CP0C(1, 0);
			CP0C(1, 1);
			CP0C(1, 2);
			CP0C(1, 3);
			CP0C(1, 4);
			CP0C(1, 5);
			CP0C(1, 6);
			CP0C(1, 7);
			CP0C(2, 0);
			CP0C(2, 1);
			CP0C(2, 2);
			CP0C(2, 3);
			CP0C(2, 4);
			CP0C(2, 5);
			CP0C(2, 6);
			CP0C(2, 7);
			CP0C(3, 0);
			CP0C(3, 1);
			CP0C(3, 2);
			CP0C(3, 3);
			CP0C(3, 4);
			CP0C(3, 5);
			CP0C(3, 6);
			CP0C(3, 7);
			CP0C(4, 0);
			CP0C(4, 1);
			CP0C(4, 2);
			CP0C(4, 3);
			CP0C(4, 4);
			CP0C(4, 5);
			CP0C(4, 6);
			CP0C(4, 7);
			CP0C(5, 0);
			CP0C(5, 1);
			CP0C(5, 2);
			CP0C(5, 3);
			CP0C(5, 4);
			CP0C(5, 5);
			CP0C(5, 6);
			CP0C(5, 7);
			CP0C(6, 0);
			CP0C(6, 1);
			CP0C(6, 2);
			CP0C(6, 3);
			CP0C(6, 4);
			CP0C(6, 5);
			CP0C(6, 6);
			CP0C(6, 7);
			CP0C(7, 0);
			CP0C(7, 1);
			CP0C(7, 2);
			CP0C(7, 3);
			CP0C(7, 4);
			CP0C(7, 5);
			CP0C(7, 6);
			CP0C(7, 7);
			CP0C(8, 3);
			CP0C(8, 4);
			CP0C(8, 5);
			CP0C(8, 6);
			CP0C(8, 7);
			CP0C(9, 0);
			CP0C(9, 1);
			CP0C(9, 2);
			CP0C(9, 3);
			CP0C(9, 4);
			CP0C(9, 5);
			CP0C(9, 6);
			CP0C(9, 7);
			CP0C(10, 0);
			CP0C(10, 1);
			CP0C(10, 2);
			CP0C(10, 3);
			CP0C(10, 4);
			CP0C(10, 5);
			CP0C(10, 6);
			CP0C(10, 7);
			CP0C(11, 0);
			CP0C(11, 1);
			CP0C(11, 2);
			CP0C(11, 3);
			CP0C(11, 4);
			CP0C(11, 5);
			CP0C(11, 6);
			CP0C(11, 7);
			CP0C(12, 1);
			CP0C(12, 2);
			CP0C(12, 3);
			CP0C(12, 4);
			CP0C(12, 5);
			CP0C(12, 6);
			CP0C(12, 7);
			CP0C(13, 1);
			CP0C(13, 2);
			CP0C(13, 3);
			CP0C(13, 4);
			CP0C(13, 5);
			CP0C(13, 6);
			CP0C(13, 7);
			CP0C(14, 1);
			CP0C(14, 2);
			CP0C(14, 3);
			CP0C(14, 4);
			CP0C(14, 5);
			CP0C(14, 6);
			CP0C(14, 7);
			CP0C(15, 0);
			CP0C(15, 1);
			CP0C(15, 2);
			CP0C(15, 3);
			CP0C(15, 4);
			CP0C(15, 5);
			CP0C(15, 6);
			CP0C(15, 7);
			CP0C(16, 0);
			CP0C(16, 1);
			CP0C(16, 2);
			CP0C(16, 3);
			CP0C(16, 4);
			CP0C(16, 5);
			CP0C(16, 6);
			CP0C(16, 7);
			CP0C(17, 0);
			CP0C(17, 1);
			CP0C(17, 2);
			CP0C(17, 3);
			CP0C(17, 4);
			CP0C(17, 5);
			CP0C(17, 6);
			CP0C(17, 7);
			CP0C(18, 0);
			CP0C(18, 1);
			CP0C(18, 2);
			CP0C(18, 3);
			CP0C(18, 4);
			CP0C(18, 5);
			CP0C(18, 6);
			CP0C(18, 7);
			CP0C(19, 0);
			CP0C(19, 1);
			CP0C(19, 2);
			CP0C(19, 3);
			CP0C(19, 4);
			CP0C(19, 5);
			CP0C(19, 6);
			CP0C(19, 7);
			CP0C(20, 0);
			CP0C(20, 1);
			CP0C(20, 2);
			CP0C(20, 3);
			CP0C(20, 4);
			CP0C(20, 5);
			CP0C(20, 6);
			CP0C(20, 7);
			CP0C(21, 0);
			CP0C(21, 1);
			CP0C(21, 2);
			CP0C(21, 3);
			CP0C(21, 4);
			CP0C(21, 5);
			CP0C(21, 6);
			CP0C(21, 7);
			CP0C(22, 0);
			CP0C(22, 1);
			CP0C(22, 2);
			CP0C(22, 3);
			CP0C(22, 4);
			CP0C(22, 5);
			CP0C(22, 6);
			CP0C(22, 7);
			CP0C(23, 0);
			CP0C(23, 1);
			CP0C(23, 2);
			CP0C(23, 3);
			CP0C(23, 4);
			CP0C(23, 5);
			CP0C(23, 6);
			CP0C(23, 7);
			CP0C(24, 0);
			CP0C(24, 1);
			CP0C(24, 2);
			CP0C(24, 3);
			CP0C(24, 4);
			CP0C(24, 5);
			CP0C(24, 6);
			CP0C(24, 7);
			CP0C(25, 0);
			CP0C(25, 1);
			CP0C(25, 2);
			CP0C(25, 3);
			CP0C(25, 4);
			CP0C(25, 5);
			CP0C(25, 6);
			CP0C(25, 7);
			CP0C(26, 0);
			CP0C(26, 1);
			CP0C(26, 2);
			CP0C(26, 3);
			CP0C(26, 4);
			CP0C(26, 5);
			CP0C(26, 6);
			CP0C(26, 7);
			CP0C(27, 0);
			CP0C(27, 1);
			CP0C(27, 2);
			CP0C(27, 3);
			CP0C(27, 4);
			CP0C(27, 5);
			CP0C(27, 6);
			CP0C(27, 7);
			CP0C(28, 0);
			CP0C(28, 1);
			CP0C(28, 2);
			CP0C(28, 3);
			CP0C(28, 4);
			CP0C(28, 5);
			CP0C(28, 6);
			CP0C(28, 7);
			CP0C(29, 0);
			CP0C(29, 1);
			CP0C(29, 2);
			CP0C(29, 3);
			CP0C(29, 4);
			CP0C(29, 5);
			CP0C(29, 6);
			CP0C(29, 7);
			CP0C(30, 0);
			CP0C(30, 1);
			CP0C(30, 2);
			CP0C(30, 3);
			CP0C(30, 4);
			CP0C(30, 5);
			CP0C(30, 6);
			CP0C(30, 7);
			CP0C(31, 0);
			CP0C(31, 1);
			CP0C(31, 2);
			CP0C(31, 3);
			CP0C(31, 4);
			CP0C(31, 5);
			CP0C(31, 6);
			CP0C(31, 7);
#undef CP0C
		}
	}
	return ctx->cause;
}

static inline reg_t setcp0(struct gpctx *ctx, reg_t index, reg_t value)
{
	struct vzguestctx *gctx = getgctx(ctx);
	if (gctx) {
		if (goffsets[index] != -1)
			*(reg_t *) (((uintptr_t) gctx) + goffsets[index]) =
			    value;
	} else {
		switch (index) {
		case (14 * 8) + 0:
			ctx->epc = value;
			break;
		case (8 * 8) + 0:
			ctx->badvaddr = value;
			break;
		case (12 * 8) + 0:
			ctx->status = value;
			break;
		case (13 * 8) + 0:
			ctx->cause = value;
			break;
		case (8 * 8) + 1:
			ctx->badinstr = value;
			break;
		case (8 * 8) + 2:
			ctx->badpinstr = value;
			break;
#define CP0C(REG, SEL) case ((REG)*8)+(SEL): _m32c0_mtc0((REG),(SEL), value); break;
			CP0C(0, 0);
			CP0C(0, 1);
			CP0C(0, 2);
			CP0C(0, 3);
			CP0C(0, 4);
			CP0C(0, 5);
			CP0C(0, 6);
			CP0C(0, 7);
			CP0C(1, 0);
			CP0C(1, 1);
			CP0C(1, 2);
			CP0C(1, 3);
			CP0C(1, 4);
			CP0C(1, 5);
			CP0C(1, 6);
			CP0C(1, 7);
			CP0C(2, 0);
			CP0C(2, 1);
			CP0C(2, 2);
			CP0C(2, 3);
			CP0C(2, 4);
			CP0C(2, 5);
			CP0C(2, 6);
			CP0C(2, 7);
			CP0C(3, 0);
			CP0C(3, 1);
			CP0C(3, 2);
			CP0C(3, 3);
			CP0C(3, 4);
			CP0C(3, 5);
			CP0C(3, 6);
			CP0C(3, 7);
			CP0C(4, 0);
			CP0C(4, 1);
			CP0C(4, 2);
			CP0C(4, 3);
			CP0C(4, 4);
			CP0C(4, 5);
			CP0C(4, 6);
			CP0C(4, 7);
			CP0C(5, 0);
			CP0C(5, 1);
			CP0C(5, 2);
			CP0C(5, 3);
			CP0C(5, 4);
			CP0C(5, 5);
			CP0C(5, 6);
			CP0C(5, 7);
			CP0C(6, 0);
			CP0C(6, 1);
			CP0C(6, 2);
			CP0C(6, 3);
			CP0C(6, 4);
			CP0C(6, 5);
			CP0C(6, 6);
			CP0C(6, 7);
			CP0C(7, 0);
			CP0C(7, 1);
			CP0C(7, 2);
			CP0C(7, 3);
			CP0C(7, 4);
			CP0C(7, 5);
			CP0C(7, 6);
			CP0C(7, 7);
			CP0C(8, 3);
			CP0C(8, 4);
			CP0C(8, 5);
			CP0C(8, 6);
			CP0C(8, 7);
			CP0C(9, 0);
			CP0C(9, 1);
			CP0C(9, 2);
			CP0C(9, 3);
			CP0C(9, 4);
			CP0C(9, 5);
			CP0C(9, 6);
			CP0C(9, 7);
			CP0C(10, 0);
			CP0C(10, 1);
			CP0C(10, 2);
			CP0C(10, 3);
			CP0C(10, 4);
			CP0C(10, 5);
			CP0C(10, 6);
			CP0C(10, 7);
			CP0C(11, 0);
			CP0C(11, 1);
			CP0C(11, 2);
			CP0C(11, 3);
			CP0C(11, 4);
			CP0C(11, 5);
			CP0C(11, 6);
			CP0C(11, 7);
			CP0C(12, 1);
			CP0C(12, 2);
			CP0C(12, 3);
			CP0C(12, 4);
			CP0C(12, 5);
			CP0C(12, 6);
			CP0C(12, 7);
			CP0C(13, 1);
			CP0C(13, 2);
			CP0C(13, 3);
			CP0C(13, 4);
			CP0C(13, 5);
			CP0C(13, 6);
			CP0C(13, 7);
			CP0C(14, 1);
			CP0C(14, 2);
			CP0C(14, 3);
			CP0C(14, 4);
			CP0C(14, 5);
			CP0C(14, 6);
			CP0C(14, 7);
			CP0C(15, 0);
			CP0C(15, 1);
			CP0C(15, 2);
			CP0C(15, 3);
			CP0C(15, 4);
			CP0C(15, 5);
			CP0C(15, 6);
			CP0C(15, 7);
			CP0C(16, 0);
			CP0C(16, 1);
			CP0C(16, 2);
			CP0C(16, 3);
			CP0C(16, 4);
			CP0C(16, 5);
			CP0C(16, 6);
			CP0C(16, 7);
			CP0C(17, 0);
			CP0C(17, 1);
			CP0C(17, 2);
			CP0C(17, 3);
			CP0C(17, 4);
			CP0C(17, 5);
			CP0C(17, 6);
			CP0C(17, 7);
			CP0C(18, 0);
			CP0C(18, 1);
			CP0C(18, 2);
			CP0C(18, 3);
			CP0C(18, 4);
			CP0C(18, 5);
			CP0C(18, 6);
			CP0C(18, 7);
			CP0C(19, 0);
			CP0C(19, 1);
			CP0C(19, 2);
			CP0C(19, 3);
			CP0C(19, 4);
			CP0C(19, 5);
			CP0C(19, 6);
			CP0C(19, 7);
			CP0C(20, 0);
			CP0C(20, 1);
			CP0C(20, 2);
			CP0C(20, 3);
			CP0C(20, 4);
			CP0C(20, 5);
			CP0C(20, 6);
			CP0C(20, 7);
			CP0C(21, 0);
			CP0C(21, 1);
			CP0C(21, 2);
			CP0C(21, 3);
			CP0C(21, 4);
			CP0C(21, 5);
			CP0C(21, 6);
			CP0C(21, 7);
			CP0C(22, 0);
			CP0C(22, 1);
			CP0C(22, 2);
			CP0C(22, 3);
			CP0C(22, 4);
			CP0C(22, 5);
			CP0C(22, 6);
			CP0C(22, 7);
			CP0C(23, 0);
			CP0C(23, 1);
			CP0C(23, 2);
			CP0C(23, 3);
			CP0C(23, 4);
			CP0C(23, 5);
			CP0C(23, 6);
			CP0C(23, 7);
			CP0C(24, 0);
			CP0C(24, 1);
			CP0C(24, 2);
			CP0C(24, 3);
			CP0C(24, 4);
			CP0C(24, 5);
			CP0C(24, 6);
			CP0C(24, 7);
			CP0C(25, 0);
			CP0C(25, 1);
			CP0C(25, 2);
			CP0C(25, 3);
			CP0C(25, 4);
			CP0C(25, 5);
			CP0C(25, 6);
			CP0C(25, 7);
			CP0C(26, 0);
			CP0C(26, 1);
			CP0C(26, 2);
			CP0C(26, 3);
			CP0C(26, 4);
			CP0C(26, 5);
			CP0C(26, 6);
			CP0C(26, 7);
			CP0C(27, 0);
			CP0C(27, 1);
			CP0C(27, 2);
			CP0C(27, 3);
			CP0C(27, 4);
			CP0C(27, 5);
			CP0C(27, 6);
			CP0C(27, 7);
			CP0C(28, 0);
			CP0C(28, 1);
			CP0C(28, 2);
			CP0C(28, 3);
			CP0C(28, 4);
			CP0C(28, 5);
			CP0C(28, 6);
			CP0C(28, 7);
			CP0C(29, 0);
			CP0C(29, 1);
			CP0C(29, 2);
			CP0C(29, 3);
			CP0C(29, 4);
			CP0C(29, 5);
			CP0C(29, 6);
			CP0C(29, 7);
			CP0C(30, 0);
			CP0C(30, 1);
			CP0C(30, 2);
			CP0C(30, 3);
			CP0C(30, 4);
			CP0C(30, 5);
			CP0C(30, 6);
			CP0C(30, 7);
			CP0C(31, 0);
			CP0C(31, 1);
			CP0C(31, 2);
			CP0C(31, 3);
			CP0C(31, 4);
			CP0C(31, 5);
			CP0C(31, 6);
			CP0C(31, 7);
#undef CP0C
		}
	}
	return ctx->cause;
}

void readReg(struct gpctx *ctx, reg_t index, void *value)
{
	struct msactx *mctx;
	struct dspctx *dctx;
	if ((index >= 1) && (index <= 31)) {
		/* GP reg */
		*((reg_t *) value) = ((reg_t *) ctx)[index - 1];
		return;
	} else if ((index >= 35) && (index <= 290)) {
		*((reg_t *) value) = getcp0(ctx, index - 35);
		return;
	} else if ((index >= 291) && (index <= 322)) {
		if (MSA || FP) {
			/* FP/MSA reg */
#ifdef NOSAVE
			/* Tickle the FPU to ensure its context has been saved */
			fpa_getfir();
#endif
			mctx = getfpctx(ctx);
			switch LINKCTX_TYPE
				(mctx) {
			case LINKCTX_TYPE_MSA:
			case LINKCTX_TYPE_FMSA:
				memcpy(value,
				       MSAMSACTX_D(mctx, index - 291),
				       sizeof(MSAMSACTX_D(mctx, index - 291)));
			case LINKCTX_TYPE_FP64:
				*(double *)value =
				    FP64CTX_DBL((struct fp64ctx *)mctx,
						index - 291);
				return;
			case LINKCTX_TYPE_FP32:
				*(float *)value =
				    FP32CTX_SGL((struct fp32ctx *)mctx,
						index - 291);
				return;
			default:
				memset(value, 0, 8);
				return;
				}
		} else {
			memset(value, 0, 8);
			return;
		}
	}
	/* Assorted exceptional/status registers */
	switch (index) {
	case 0:
		*(reg_t *) value = 0;
		return;
	case 32:
		*(reg_t *) value = ctx->lo;
		return;
	case 33:
		*(reg_t *) value = ctx->hi;
		return;
	case 34:
		*(reg_t *) value = ctx->epc;
		return;
	case 323:
		if (FP) {
#ifdef NOSAVE
/* Tickle the FPU to ensure its context has been saved */
			fpa_getfir();
#endif
			mctx = getfpctx(ctx);
			*(reg_t *) value = ((struct fpctx *)mctx)->fcsr;
		} else
			memset(value, 0, 8);
		return;
	case 324:
		if (FP)
			*(reg_t *) value = fpa_getfir();
		else
			memset(value, 0, 8);
		return;
	case 325:
		if (MSA) {
#ifdef NOSAVE
/* Tickle the MSA to ensure its context has been saved */
			msa_getmir();
#endif
			mctx = getfpctx(ctx);
			*(reg_t *) value = mctx->msacsr;
		} else
			memset(value, 0, 8);
		return;
	case 326:
		if (MSA)
			*(reg_t *) value = msa_getmir();
		else
			memset(value, 0, 8);
		return;
	case 327:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->hi[0];
		} else
			memset(value, 0, 8);
		return;
	case 328:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->lo[0];
		} else
			memset(value, 0, 8);
		return;
	case 329:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->hi[1];
		} else
			memset(value, 0, 8);
		return;
	case 330:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->lo[1];
		} else
			memset(value, 0, 8);
		return;
	case 331:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->hi[2];
		} else
			memset(value, 0, 8);
		return;
	case 332:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->lo[2];
		} else
			memset(value, 0, 8);
		return;
	case 333:
		if (DSP) {
			dctx = getdspctx(ctx);
			*(reg_t *) value = dctx->dspc;
		}
		memset(value, 0, 8);
		return;
	}
}

void writeReg(struct gpctx *ctx, reg_t index, void *value)
{
	struct msactx *mctx;
	struct dspctx *dctx;
	if ((index >= 1) && (index <= 31)) {
		/* GP reg */
		((reg_t *) ctx)[index - 1] = *((reg_t *) value);
		return;
	} else if ((index >= 35) && (index <= 290)) {
		setcp0(ctx, index - 35, *((reg_t *) value));
	} else if ((index >= 291) && (index <= 324)) {
		/* FP/MSA reg */
#ifdef NOSAVE
		/* Tickle the FPU to ensure its context has been saved */
		fpa_getfir();
#endif
		mctx = getfpctx(ctx);
		switch LINKCTX_TYPE
			(mctx) {
		case LINKCTX_TYPE_MSA:
		case LINKCTX_TYPE_FMSA:
			memcpy(MSAMSACTX_D(mctx, index - 291),
			       value, sizeof(MSAMSACTX_D(mctx, index - 291)));
		case LINKCTX_TYPE_FP64:
			FP64CTX_DBL((struct fp64ctx *)mctx, index - 291) =
			    *(double *)value;
			return;
		case LINKCTX_TYPE_FP32:
			FP32CTX_SGL((struct fp32ctx *)mctx, index - 291) =
			    *(float *)value;
			return;
		default:
			return;
			}
	}
	/* Assorted exceptional/status registers */
	switch (index) {
	case 0:
		return;
	case 32:
		ctx->lo = *(reg_t *) value;
		return;
	case 33:
		ctx->hi = *(reg_t *) value;
		return;
	case 34:
		ctx->epc = *(reg_t *) value;
		return;
	case 323:
#ifdef NOSAVE
/* Tickle the FPU to ensure its context has been saved */
		fpa_getfir();
#endif
		mctx = getfpctx(ctx);
		((struct fpctx *)mctx)->fcsr = *(reg_t *) value;
		return;
	case 324:
		return;
	case 325:
#ifdef NOSAVE
/* Tickle the MSA to ensure its context has been saved */
		msa_getmir();
#endif
		mctx = getfpctx(ctx);
		mctx->msacsr = *(reg_t *) value;
		return;
	case 326:
		return;
	case 327:
		dctx = getdspctx(ctx);
		*(reg_t *) value = dctx->hi[0];
		return;
	case 328:
		dctx = getdspctx(ctx);
		dctx->lo[0] = *(reg_t *) value;
		return;
	case 329:
		dctx = getdspctx(ctx);
		dctx->hi[1] = *(reg_t *) value;
		return;
	case 330:
		dctx = getdspctx(ctx);
		dctx->lo[1] = *(reg_t *) value;
		return;
	case 331:
		dctx = getdspctx(ctx);
		dctx->hi[2] = *(reg_t *) value;
		return;
	case 332:
		dctx = getdspctx(ctx);
		dctx->lo[2] = *(reg_t *) value;
		return;
	case 333:
		dctx = getdspctx(ctx);
		dctx->dspc = *(reg_t *) value;
		return;
	}
}

/*
 * XML register definitions
 */

void xmlStart(RSP_T * rsp)
{
	writeStr(rsp,
		 "<?xml version=\"1.0\"?><!DOCTYPE feature SYSTEM \"gdb-target.dtd\"><target><architecture>mips</architecture>");
}

void xmlEnd(RSP_T * rsp)
{
	writeStr(rsp, "</target>");
}

void xmlStartFeature(RSP_T * rsp, const char *name)
{
	writeStr(rsp, "<feature name=\"org.gnu.gdb.mips.");
	writeStr(rsp, name);
	writeStr(rsp, "\">");
}

void xmlEndFeature(RSP_T * rsp)
{
	writeStr(rsp, "</feature>");
}

void xmlReg(RSP_T * rsp, const char *name, const sreg_t numeral,
	    size_t size, reg_t index, const char *extra)
{
	writeStr(rsp, "<reg name=\"");
	writeStr(rsp, name);
	if (numeral >= 0)
		writeDec(rsp, numeral);
	writeStr(rsp, "\" bitsize=\"");
	writeDec(rsp, size);
	writeStr(rsp, "\" regnum=\"");
	writeDec(rsp, index);
	writeStr(rsp, "\" ");
	writeStr(rsp, extra);
	writeStr(rsp, "/>");
}

/* It looks like more hassle to use this to linearly access context structs than it's worth */
/* This'll want some polishing for 64-bit compatibility */
void xmlDesc(RSP_T * rsp)
{
	reg_t i;
	char buf[16];

	xmlStart(rsp);
	xmlStartFeature(rsp, "cpu");
	for (i = 0; i < 32; i++)
		xmlReg(rsp, "r", i, sizeof(reg_t) * 8, i, "");
	xmlReg(rsp, "lo", -1, 32, 32, "");
	xmlReg(rsp, "hi", -1, 32, 33, "");
	xmlReg(rsp, "pc", -1, 32, 34, "");

	xmlEndFeature(rsp);

	xmlStartFeature(rsp, "cp0");
	for (i = 0; i < 32 * 8; i++) {
		cp0name(i, buf);
		xmlReg(rsp, buf, -1, regSize(i + 35) * 8, i + 35, "");
	}
	xmlEndFeature(rsp);
	xmlStartFeature(rsp, "fpu");

	if (MSA) {
		writeStr
		    (rsp,
		     "<vector id=\"msa128\" type=\"ieee_double\" count=\"2\"/>");
		for (i = 0; i < 32; i++)
			xmlReg(rsp, "f", i, 128, 291 + i, "type=\"msa128\"");
	} else if (FP64)
		for (i = 0; i < 32; i++)
			xmlReg(rsp, "f", i, 64, 291 + i,
			       "type=\"ieee_double\"");
	else
		for (i = 0; i < 32; i++)
			xmlReg(rsp, "f", i, 32, 291 + i,
			       "type=\"ieee_single\"");

	xmlReg(rsp, "fcsr", -1, 32, 323, "group=\"float\"");
	xmlReg(rsp, "fir", -1, 32, 324, "group=\"float\"");
	xmlEndFeature(rsp);
	if (MSA) {
		xmlStartFeature(rsp, "msa");
		xmlReg(rsp, "msacsr", -1, 32, 325, "group=\"vector\"");
		xmlReg(rsp, "msair", -1, 32, 326, "group=\"vector\"");
		xmlEndFeature(rsp);
	}
	if (DSP) {
		xmlStartFeature(rsp, "dsp");
		xmlReg(rsp, "hi1", -1, 32, 327, "");
		xmlReg(rsp, "lo1", -1, 32, 328, "");
		xmlReg(rsp, "hi1", -1, 32, 329, "");
		xmlReg(rsp, "lo1", -1, 32, 330, "");
		xmlReg(rsp, "hi1", -1, 32, 331, "");
		xmlReg(rsp, "lo1", -1, 32, 332, "");
		xmlReg(rsp, "dspctl", -1, 32, 333, "");
		xmlEndFeature(rsp);
	}

	xmlEnd(rsp);
}

void RSP_addBreak(RSP_T * rsp, uintptr_t addr)
{
	size_t i;

	for (i = 0; i <= rsp->nBreaks; i++) {
		if (rsp->breakTable[i * 2] == addr)
			return;
	}
	for (i = 0; i <= rsp->nBreaks; i++) {
		if (rsp->breakTable[i * 2] == 0) {
			rsp->breakTable[i * 2] = addr;
			mips_flush_dcache();
			mips_flush_icache();
			rsp->readMem((void *)(addr & ~1),
				     &rsp->breakTable[(i * 2) + 1], 1, 4,
				     rsp->readPriv);
			rsp->inject(rsp, addr);
			mips_flush_dcache();
			mips_flush_icache();
			return;
		}
	}
}

void RSP_removeBreak(RSP_T * rsp, uintptr_t addr)
{
	size_t i;
	for (i = 0; i <= rsp->nBreaks; i++) {
		if (rsp->breakTable[i * 2] == addr) {
			rsp->breakTable[i * 2] = 0;
			rsp->writeMem((void *)(addr & ~1),
				      &rsp->breakTable[(i * 2) + 1], 1, 4,
				      rsp->readPriv);
			mips_flush_dcache();
			mips_flush_icache();
			return;
		}
	}
}

/*
 * RSP handler - point exceptions here
 */

/* Ideally, the compiler would not generate float/dsp/msa instructions in this */
void RSP_loop(RSP_T * rsp, int sig, struct gpctx *ctx)
{
	uint8_t *buffer;
	size_t len;
	reg_t index;
	uint8_t value[16];
	sreg_t start;
#ifndef NOSAVE
	struct msactx mctx;
	struct dspctx dctx;
	if (MSA) {
		_msactx_save(&mctx);
		_linkctx_append(ctx, (struct linkctx *)&mctx);
	} else if (FP) {
		_fpctx_save((struct fpctx *)&mctx);
		_linkctx_append(ctx, (struct linkctx *)&mctx);

	}
	_dspctx_save(&dctx);
	_linkctx_append(ctx, (struct linkctx *)&dctx);
#endif
	if (rsp->stepAddr != 0xffffffff) {
		RSP_removeBreak(rsp, rsp->stepAddr);
		rsp->stepAddr = 0xffffffff;
		rsp->broken = 5;
	}
	if (rsp->broken) {
		writeSimple(rsp, 'S', rsp->broken);
		rsp->broken = 0;
	}
	for (;;) {
		while ((start = readStart(rsp)) == 0) ;
		if (start < 0)
			writeSimple(rsp, 'S',
				    excSig[(getcp0(ctx, (13 * 8) + 0) &
					    CR_X_MASK) >> CR_X_SHIFT]);
		else
			switch (readChar(rsp)) {
			case 'q':
				switch (readChar(rsp)) {
				case 'f':
					if (!readConfirm(rsp, "ThreadInfo"))
						break;
					while (!readEnd(rsp)) ;
					writeStart(rsp);
					writeStr(rsp, "l");
					writeEnd(rsp);
					break;
				case 'S':
					if (!readConfirm(rsp, "upported"))
						break;
					while (!readEnd(rsp)) ;
					writeStart(rsp);
					writeStr(rsp, "qXfer:features:read+");
					writeEnd(rsp);
					break;
				case 'X':
					if (!readConfirm
					    (rsp,
					     "fer:features:read:target.xml:"))
						break;
					index = readHex(rsp, sizeof(reg_t) + 1);
					len = readHex(rsp, sizeof(reg_t) + 1);
					if (!ackEnd(rsp))
						break;
					writeStart(rsp);
					setWriteWindow(rsp, index, len, 'm');
					xmlDesc(rsp);
					if (clearWriteWindow(rsp) == 0)
						writeChar(rsp, 'l');
					writeEnd(rsp);
					break;
				default:
					while (!readEnd(rsp)) ;
					writeStart(rsp);
					writeEnd(rsp);
					break;
				}
				break;
			case 'm':
				buffer =
				    (uint8_t *) readHex(rsp, sizeof(reg_t) + 1);
				len = readHex(rsp, sizeof(reg_t) + 1);
				if (!ackEnd(rsp))
					break;
				do {
					writeStart(rsp);
					if (rsp->readMem) {
						for (index = 0;
						     index < len; index++) {
							rsp->readMem((void
								      *)(((uintptr_t) buffer) + index), &value, 1, 1, rsp->readPriv);
							writeHex(rsp, value[0]);
						}
					} else {
						writeHexes(rsp,
							   (const uint8_t *)
							   buffer, len);
					}
					writeEnd(rsp);
				} while (readChar(rsp) == '-');
				break;
			case 'M':
				buffer =
				    (uint8_t *) readHex(rsp, sizeof(reg_t) + 1);
				len = readHex(rsp, sizeof(reg_t) + 1);
				if (!rsp->writeMem) {
					readHexes(rsp, buffer, len);
					readEnd(rsp);
					writeOk(rsp);
					break;
				}
				for (index = 0; index < len; index++) {
					value[0] = readHex(rsp, 1);
					rsp->writeMem((void *)(((uintptr_t)
								buffer) +
							       index),
						      &value, 1, 1,
						      rsp->writePriv);
				}
				mips_flush_dcache();
				mips_flush_icache();
				//mips_sync_icache((vaddr_t) buffer, len);
				readEnd(rsp);
				writeOk(rsp);
				break;
			case 'g':
				if (!readEnd(rsp))
					break;
				do {
					writeStart(rsp);
					for (index = 0; index <= 34; index++) {
						readReg(ctx, index, &value);
						writeHexes(rsp,
							   (const uint8_t *)
							   &value,
							   regSize(index));
					}
					writeEnd(rsp);
				} while (readChar(rsp) == '-');
				break;
			case 'G':
				for (index = 0; index <= 34; index++) {
					readHexes(rsp, value, regSize(index));
					writeReg(ctx, index, value);
				}
				readEnd(rsp);
				writeOk(rsp);
				break;
			case 'p':
				index = readHex(rsp, sizeof(reg_t) + 1);
				if (!ackEnd(rsp))
					break;
				do {
					writeStart(rsp);
					readReg(ctx, index, value);
					writeHexes(rsp, value, regSize(index));
					writeEnd(rsp);
				} while (readChar(rsp) == '-');
				break;
			case 'P':
				index = readHex(rsp, sizeof(reg_t) + 1);
				readHexes(rsp, value, regSize(index));
				readChar(rsp);
				if (!ackEnd(rsp))
					break;
				writeReg(ctx, index, value);
				writeOk(rsp);
				break;
			case 's':{
					uint32_t one, two;
					while (!readEnd(rsp)) ;
					rsp->readMem((void *)(ctx->epc &
							      ~1), &one,
						     1, 4, rsp->readPriv);
					rsp->readMem((void *)(ctx->epc &
							      ~1) + 4,
						     &two, 1, 4, rsp->readPriv);
					rsp->stepAddr = _mips_nextpc(ctx, one,
								     two);
					RSP_addBreak(rsp, rsp->stepAddr);
					goto out;
				}
				break;
			case '?':
				while (!readEnd(rsp)) ;
				writeSimple(rsp, 'S',
					    excSig[(getcp0(ctx, (13 * 8) + 0) &
						    CR_X_MASK) >> CR_X_SHIFT]);
				break;
			case 'c':
				if (readEnd(rsp))
					goto out;
			default:
				while (!readEnd(rsp)) ;
				writeStart(rsp);
				writeEnd(rsp);
			}
	}
      out:
#ifndef NOSAVE
	switch (LINKCTX_TYPE(&mctx)) {
	case LINKCTX_TYPE_MSA:
	case LINKCTX_TYPE_FMSA:
		_msactx_load(&mctx);
		break;
	case LINKCTX_TYPE_FP32:
	case LINKCTX_TYPE_FP64:
		_fpctx_load((struct fpctx *)&mctx);
		break;
	default:
		break;
	}
	_dspctx_load(&dctx);
#endif
	return;
}

void RSP_break(RSP_T * rsp, int sig)
{
	rsp->broken = 5;
}

void RSP_injectBREAK(RSP_T * rsp, uintptr_t addr)
{
	uint32_t brk = 0x0000000d;
	if (addr & 1) {
		if (mips32_getconfig1()
		    & CFG1_CA)
			brk = 0xe805e805;
		else if (mips32_getconfig3()
			 & CFG3_ISA_MASK)
			brk = 0x441b441b;
	}
	rsp->writeMem((void *)(addr & ~1), &brk, 1, 4, rsp->writePriv);
	//mips_sync_icache((vaddr_t) (addr & ~1), 4);
	mips_flush_dcache();
	mips_flush_icache();
}

/*
 * Init function
 */
void RSP_init(RSP_T *
	      rsp,
	      RSP_READFUNC_T
	      *
	      readChar,
	      RSP_WRITEFUNC_T
	      * writeChar, uintptr_t * breakTable, size_t nBreaks)
{
	rsp->readChar = readChar;
	rsp->writeChar = writeChar;
	rsp->inject = &RSP_injectBREAK;
	rsp->stepAddr = 0xffffffff;
	rsp->broken = 0;
	rsp->breakTable = breakTable;
	rsp->nBreaks = nBreaks;
	memset(rsp->breakTable, 0, sizeof(uintptr_t)
	       * 2 * nBreaks);
	clearWriteWindow(rsp);
}

void RSP_setRW(RSP_T * rsp, int (*r)
	        (void *, void *, int, int, void *), void *rp, int (*w)
	        (void *, void *, int, int, void *), void *wp)
{
	rsp->readMem = r;
	rsp->readPriv = rp;
	rsp->writeMem = w;
	rsp->writePriv = wp;
}
