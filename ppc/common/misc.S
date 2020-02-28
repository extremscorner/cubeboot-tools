/*
 * This file contains miscellaneous low-level functions.
 *    Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *
 * Largely rewritten by Cort Dougan (cort@cs.nmt.edu)
 * and Paul Mackerras.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#define __ASSEMBLY__

#include "../include/system.h"

	.text

/*
 * Write any modified data cache blocks out to memory and invalidate them.
 * Does not invalidate the corresponding instruction cache blocks.
 *
 * flush_dcache_range(unsigned long start, unsigned long stop)
 */
.global flush_dcache_range
flush_dcache_range:
	li	5,L1_CACHE_LINE_SIZE-1
	andc	3,3,5
	subf	4,3,4
	add	4,4,5
	srwi.	4,4,LG_L1_CACHE_LINE_SIZE
	beqlr
	mtctr	4

1:	dcbf	0,3
	addi	3,3,L1_CACHE_LINE_SIZE
	bdnz	1b
	sync				/* wait for dcbf's to get to ram */
	blr

/*
 * Like above, but invalidate the D-cache.  This is used by the 8xx
 * to invalidate the cache so the PPC core doesn't get stale data
 * from the CPM (no cache snooping here :-).
 *
 * invalidate_dcache_range(unsigned long start, unsigned long stop)
 */
.global invalidate_dcache_range
invalidate_dcache_range:
	li	5,L1_CACHE_LINE_SIZE-1
	andc	3,3,5
	subf	4,3,4
	add	4,4,5
	srwi.	4,4,LG_L1_CACHE_LINE_SIZE
	beqlr
	mtctr	4

1:	dcbi	0,3
	addi	3,3,L1_CACHE_LINE_SIZE
	bdnz	1b
	sync				/* wait for dcbi's to get to ram */
	blr

.global invalidate_icache_range
invalidate_icache_range:
	li	5,L1_CACHE_LINE_SIZE-1
	andc	3,3,5
	subf	4,3,4
	add	4,4,5
	srwi.	4,4,LG_L1_CACHE_LINE_SIZE
	beqlr
	mtctr	4

1:	icbi	0,3
	addi	3,3,L1_CACHE_LINE_SIZE
	bdnz	1b
	sync				/* wait for icbi's to get to ram */
	isync
	blr

