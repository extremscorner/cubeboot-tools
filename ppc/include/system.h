/*
 * system.h
 *
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */
#ifndef __SYSTEM_H
#define __SYSTEM_H

#define L1_CACHE_LINE_SIZE	32
#define LG_L1_CACHE_LINE_SIZE	5

#define GCN_VIDEO_LINES		480

#define GCN_RAM_SIZE		(24*1024*1024)
#define GCN_TOP_OF_RAM		(0x01200000)	/* up to apploader code */

#define GCN_XFB_SIZE		(2*640*GCN_VIDEO_LINES)

//#define GCN_XFB_END             (GCN_TOP_OF_RAM-1)
//#define GCN_XFB_START           (GCN_XFB_END-GCN_XFB_SIZE+1)
#define GCN_XFB_START		0x00f00000
#define GCN_XFB_END		(GCN_XFB_START+GCN_XFB_SIZE)

#ifndef __ASSEMBLY__

#include <sys/types.h>
#include <stdint.h>

#define GCN_VI_TFBL             ((void *)0xcc00201c)
#define GCN_VI_BFBL             ((void *)0xcc002024)

#define FLIPPER_ICR             ((void *)0xcc003000)

#define FLIPPER_RESET           ((void *)0xcc003024)
#define FLIPPER_RESET_DVD       0x00000004
#define FLIPPER_RESETCODE_MASK  0xFFFFFFF8
#define FLIPPER_RESETCODE_SHIFT 3

#define GCN_SI_C0OUTBUF		((void *)0xcc006400)
#define GCN_SI_SR		((void *)0xcc006438)

#define SICINBUFH(x)		((void *)(0xcc006404 + (x)*12))
#define SICINBUFL(x)		((void *)(0xcc006408 + (x)*12))

#define PAD_Y           (1 << 27)
#define PAD_X           (1 << 26)
#define PAD_B           (1 << 25)
#define PAD_A           (1 << 24)
#define PAD_Z           (1 << 20)


#define le16_to_cpus(addr) st_le16(addr, *addr)
#define le32_to_cpus(addr) st_le32(addr, *addr)

static inline void st_le16(volatile uint16_t * addr, const uint16_t val)
{
	asm volatile ("sthbrx %1,0,%2":"=m" (*addr):"r"(val), "r"(addr));
}

static inline void st_le32(volatile uint32_t * addr, const uint32_t val)
{
	asm volatile ("stwbrx %1,0,%2":"=m" (*addr):"r"(val), "r"(addr));
}

static inline unsigned long readl(volatile void *addr)
{
	return *(volatile unsigned long *)(addr);
}

static inline void writel(unsigned long b, volatile void *addr)
{
	*(volatile unsigned long *)(addr) = b;
}

static inline unsigned long ticks(void)
{
	unsigned long tbl;

	asm volatile ("mftb %0" : "=r" (tbl));
	return tbl;
}

extern void flush_dcache_range(void *start, void *stop);
extern void invalidate_dcache_range(void *start, void *stop);
extern void invalidate_icache_range(void *start, void *stop);

extern void rumble(int enable);
extern void rumble_on(void);
extern void panic(char *text);

#endif				/* __ASSEMBLY__ */

#endif				/* __SYSTEM_H */
