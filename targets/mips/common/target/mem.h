#include <mips/endian.h>
#include "meos/config.h"

#ifdef CONFIG_ARCH_MIPS_BIG_ENDIAN
#define beswap16(X) (X)
#define beswap32(X) (X)
#define leswap16(X) __swap16md(X)
#define leswap32(X) __swap32md(X)
#else
#define beswap16(X) __swap16md(X)
#define beswap32(X) __swap32md(X)
#define leswap16(X) (X)
#define leswap32(X) (X)
#endif

static inline uint8_t MEM_rb(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint8_t MEM_rb(const void *vaddr)
{
	return *(volatile uint8_t *) vaddr;
}

static inline uint16_t MEM_rh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_rh(const void *vaddr)
{
	return *(volatile uint16_t *) vaddr;
}

static inline uint32_t MEM_rw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_rw(const void *vaddr)
{
	return *(volatile uint32_t *) vaddr;
}

static inline uint16_t MEM_urh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_urh(const void *vaddr)
{
	uint16_t v;
	memcpy(&v, vaddr, sizeof(v));
	return v;
}

static inline uint32_t MEM_urw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_urw(const void *vaddr)
{
	uint32_t v;
	memcpy(&v, vaddr, sizeof(v));
	return v;
}

static inline void MEM_wb(void *vaddr, uint8_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_wb(void *vaddr, uint8_t v)
{
	*(volatile uint8_t *) vaddr = v;
}

static inline void MEM_wh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_wh(void *vaddr, uint16_t v)
{
	*(volatile uint16_t *) vaddr = v;
}

static inline void MEM_ww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_ww(void *vaddr, uint32_t v)
{
	*(volatile uint32_t *) vaddr = v;
}

static inline void MEM_uwh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_uwh(void *vaddr, uint16_t v)
{
	memcpy(vaddr, &v, sizeof(v));
}

static inline void MEM_uww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_uww(void *vaddr, uint32_t v)
{
	memcpy(vaddr, &v, sizeof(v));
}

static inline uint16_t MEM_lerh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_lerh(const void *vaddr)
{
	return leswap16(*(volatile uint16_t*) vaddr);
}

static inline uint32_t MEM_lerw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_lerw(const void *vaddr)
{
	return leswap32(*(volatile uint32_t*) vaddr);
}

static inline uint16_t MEM_leurh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_leurh(const void *vaddr)
{
	uint16_t v;
	memcpy(&v, vaddr, sizeof(v));
	return leswap16(v);
}

static inline uint32_t MEM_leurw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_leurw(const void *vaddr)
{
	uint32_t v;
	memcpy(&v, vaddr, sizeof(v));
	return leswap32(v);
}

static inline void MEM_lewh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_lewh(void *vaddr, uint16_t v)
{
	*(volatile uint16_t *) vaddr = leswap16(v);
}

static inline void MEM_leww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leww(void *vaddr, uint32_t v)
{
	*(volatile uint32_t *) vaddr = leswap32(v);
}

static inline void MEM_leuwh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leuwh(void *vaddr, uint16_t v)
{
	v = leswap16(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline void MEM_leuww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leuww(void *vaddr, uint32_t v)
{
	v = leswap32(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline uint16_t MEM_berh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_berh(const void *vaddr)
{
	return beswap16(*(volatile uint16_t *) vaddr);
}

static inline uint32_t MEM_berw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_berw(const void *vaddr)
{
	return beswap32(*(volatile uint32_t *) vaddr);
}

static inline uint16_t MEM_beurh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_beurh(const void *vaddr)
{
	uint16_t v;
	memcpy(&v, vaddr, sizeof(v));
	return beswap16(v);
}

static inline uint32_t MEM_beurw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_beurw(const void *vaddr)
{
	uint32_t v;
	memcpy(&v, vaddr, sizeof(v));
	return beswap32(v);
}

static inline void MEM_bewh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_bewh(void *vaddr, uint16_t v)
{
	*(volatile uint16_t *) vaddr = beswap16(v);
}

static inline void MEM_beww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beww(void *vaddr, uint32_t v)
{
	*(volatile uint32_t *) vaddr = beswap32(v);
}

static inline void MEM_beuwh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beuwh(void *vaddr, uint16_t v)
{
	v = beswap16(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline void MEM_beuww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beuww(void *vaddr, uint32_t v)
{
	v = beswap32(v);
	memcpy(vaddr, &v, sizeof(v));
}

#undef beswap16
#undef beswap32
#undef leswap16
#undef leswap32
