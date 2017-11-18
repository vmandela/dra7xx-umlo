/*
 * (C) Copyright 2017
 * Texas Instruments Incorporated, <www.ti.com>
 *
 * Venkateswara Rao Mandela <venkat.mandela@ti.com>
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#ifndef __COMMON_H
#define __COMMON_H
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint8_t u8;

#define FLD_MASK(start, end)	(((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))
#define FLD_GET(val, start, end) (((val) & FLD_MASK(start, end)) >> (end))
#define FLD_MOD(orig, val, start, end) (((orig) & ~FLD_MASK(start, end)) | FLD_VAL(val, start, end))

static inline u32 reg_read32(u32 addr) {
	u32 reg;
	reg = *((volatile u32 *) addr);
	return reg;
}

static inline void reg_write32(u32 addr, u32 val) {
	*((volatile u32 *) addr) = val;
}

static inline u16 reg_read16(u32 addr) {
	u16 reg;
	reg = *((volatile u16 *) addr);
	return reg;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif
