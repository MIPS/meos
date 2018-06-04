#include <endian.h>
#include "meos/config.h"

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
	return le16toh(*(volatile uint16_t*) vaddr);
}

static inline uint32_t MEM_lerw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_lerw(const void *vaddr)
{
	return le32toh(*(volatile uint32_t*) vaddr);
}

static inline uint16_t MEM_leurh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_leurh(const void *vaddr)
{
	uint16_t v;
	memcpy(&v, vaddr, sizeof(v));
	return le16toh(v);
}

static inline uint32_t MEM_leurw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_leurw(const void *vaddr)
{
	uint32_t v;
	memcpy(&v, vaddr, sizeof(v));
	return le32toh(v);
}

static inline void MEM_lewh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_lewh(void *vaddr, uint16_t v)
{
	*(volatile uint16_t *) vaddr = htole16(v);
}

static inline void MEM_leww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leww(void *vaddr, uint32_t v)
{
	*(volatile uint32_t *) vaddr = htole32(v);
}

static inline void MEM_leuwh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leuwh(void *vaddr, uint16_t v)
{
	v = htole16(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline void MEM_leuww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_leuww(void *vaddr, uint32_t v)
{
	v = htole32(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline uint16_t MEM_berh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_berh(const void *vaddr)
{
	return be16toh(*(volatile uint16_t *) vaddr);
}

static inline uint32_t MEM_berw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_berw(const void *vaddr)
{
	return be32toh(*(volatile uint32_t *) vaddr);
}

static inline uint16_t MEM_beurh(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint16_t MEM_beurh(const void *vaddr)
{
	uint16_t v;
	memcpy(&v, vaddr, sizeof(v));
	return be16toh(v);
}

static inline uint32_t MEM_beurw(const void *vaddr)
    __attribute__ ((optimize("-O3")));
static inline uint32_t MEM_beurw(const void *vaddr)
{
	uint32_t v;
	memcpy(&v, vaddr, sizeof(v));
	return be32toh(v);
}

static inline void MEM_bewh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_bewh(void *vaddr, uint16_t v)
{
	*(volatile uint16_t *) vaddr = htobe16(v);
}

static inline void MEM_beww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beww(void *vaddr, uint32_t v)
{
	*(volatile uint32_t *) vaddr = htobe32(v);
}

static inline void MEM_beuwh(void *vaddr, uint16_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beuwh(void *vaddr, uint16_t v)
{
	v = htobe16(v);
	memcpy(vaddr, &v, sizeof(v));
}

static inline void MEM_beuww(void *vaddr, uint32_t v)
    __attribute__ ((optimize("-O3")));
static inline void MEM_beuww(void *vaddr, uint32_t v)
{
	v = htobe32(v);
	memcpy(vaddr, &v, sizeof(v));
}
