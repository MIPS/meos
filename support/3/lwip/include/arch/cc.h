#ifndef CC_H
#define CC_H

#include <mips/endian.h>
#include "meos/inttypes.h"
#include "meos/irq/irq.h"

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT      __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(X)	X

#define ALIGN_STRUCT_8_BEGIN
#define ALIGN_STRUCT_8          __attribute__ ((aligned (8)))
#define ALIGN_STRUCT_8_END

#define LWIP_PLATFORM_ASSERT(X)		sys_assert(X)
#define LWIP_PLATFORM_DIAG(X, ...)	do {sys_debug X;} while (0)

#define U16_F	PRIu16
#define S16_F	PRId16
#define X16_F	PRIx16
#define U32_F	PRIu32
#define S32_F	PRId32
#define X32_F	PRIx32

typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef uint32_t mem_ptr_t;
typedef IRQ_IPL_T sys_prot_t;

#include "sys_arch.h"

#endif				/* CC_H */
