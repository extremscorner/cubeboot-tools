/*
 * crt0.S
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

/*
 * Portions based on:
 *
 **
 * arch/ppc/boot/simple/head.S
 *
 * Initial board bringup code for many different boards.
 *
 * Author: Tom Rini
 *         trini@mvista.com
 * Derived from arch/ppc/boot/prep/head.S (Cort Dougan, many others).
 *
 * 2001-2004 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 **
 * arch/ppc/boot/common/util.S
 *
 * Useful bootup functions, which are more easily done in asm than C.
 *
 * NOTE:  Be very very careful about the registers you use here.
 *      We don't follow any ABI calling convention among the
 *      assembler functions that call each other, especially early
 *      in the initialization.  Please preserve at least r3 and r4
 *      for these early functions, as they often contain information
 *      passed from boot roms into the C decompress function.
 *
 * Author: Tom Rini
 *         trini@mvista.com
 * Derived from arch/ppc/boot/prep/head.S (Cort Dougan, many others).
 *
 * 2001-2004 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 **
 * ppc-setup-dol.S - setup glue for Nintendo's GameCube
 * Copyright (C) 2004 Albert Herranz
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 **
 * crt0.s file for the GameCube V1.1 by Costis (costis@gbaemu.com)!
 *
 *
 */

#include "ppc_asm.h"

	.text

	.globl _start
_start:
	bl	disable_6xx_mmu

        /* Try to reproduce the GameCube "native" environment */

        /* Setup BATs */
        isync
        li      r8, 0
        mtspr   DBAT0U, r8
        mtspr   DBAT0L, r8
        mtspr   DBAT1U, r8
        mtspr   DBAT1L, r8
        mtspr   DBAT2U, r8
        mtspr   DBAT2L, r8
        mtspr   DBAT3U, r8
        mtspr   DBAT3L, r8
        mtspr   IBAT0U, r8
        mtspr   IBAT0L, r8
        mtspr   IBAT1U, r8
        mtspr   IBAT1L, r8
        mtspr   IBAT2U, r8
        mtspr   IBAT2L, r8
        mtspr   IBAT3U, r8
        mtspr   IBAT3L, r8

        /*
         * Memory Map
         * start       end         size  description
         * 0x80000000  0x817fffff  24MB  RAM, uncached
         * 0xc0000000  0xc17fffff  24MB  RAM, cached
         * 0xc8000000  0xc81fffff   2MB  Embedded Framebuffer
         * 0xcc000000                    Hardware registers
         * 0xe0000000                    Layer 2 transfer cache ??? 256KB
         *
         */

        isync
        lis     r8,     0x8000 /* IBAT0,DBAT0 for first 16Mbytes */
        ori     r8, r8, 0x01ff /* 16MB */
        mtspr   IBAT0U, r8
        mtspr   DBAT0U, r8
        li      r8,     0x0002 /* rw */
        mtspr   IBAT0L, r8
        mtspr   DBAT0L, r8

        lis     r8,     0xc000  /* DBAT1 for IO mem */
        ori     r8, r8, 0x1fff  /* 256MB */
        mtspr   DBAT1U, r8
        li      r8,     0x002a  /* uncached, guarded ,rw */
        mtspr   DBAT1L, r8

        lis     r8,     0x8100  /* IBAT2,DBAT2 for next 8Mbytes */
        ori     r8, r8, 0x00ff  /* 8MB */
        mtspr   IBAT2U, r8
        mtspr   DBAT2U, r8
        lis     r8,     0x0100
        ori     r8, r8, 0x0002  /* rw */
        mtspr   IBAT2L, r8
        mtspr   DBAT2L, r8

        lis     r8,     0xe000  /* DBAT3 for layer 2 transfer cache ??? */
        ori     r8, r8, 0x01fe  /* 16MB ??? */
        mtspr   DBAT3U, r8
        lis     r8,     0xe000
        ori     r8, r8, 0x0002  /* rw */
        mtspr   DBAT3L, r8

        sync
        isync

/* AFAIK, this is not strictly needed, although seems sane */
#if 1
        li      r9, 0

        /* page table pointer */
        sync
        mtspr   SDR1, r9

        /* segment registers */
        li      r8, 16
        mtctr   r8
        li      r8, 0
1:      mtsrin  r9, r8          /* zero */
        sync
        addis   r8,r8,0x1000    /* next register */
        bdnz    1b
#endif

	/* stack pointer */
	lis	r1, 0x0160

        /* switch MMU on and continue */
        RELOC_SYM(1f)
        mfmsr   r0
        ori     r0, r0, MSR_RI|MSR_ME|MSR_DR|MSR_IR
        mtspr   SRR1, r0
        oris    r3, r3, 0x8000          /* adjust text address */
        mtspr   SRR0, r3
        oris    r1, r1, 0x8000          /* adjust stack */
        sync
        rfi

1:
        /* from now on we run in a DOL-like environment */

        /* invalidate L1 data and instructions caches */
        mfspr   r8, HID0
        ori     r8, r8, HID0_ICFI|HID0_DCI
        mtspr   HID0, r8
	sync

#if 1
        /* first, sanitize the hardware a little bit */

        /* audio */
        lis     r8, 0xcc00      /* io mem */
        li      r9, 0
        sth     r9, 0x5036(r8)  /* stop audio sample */
        stw     r9, 0x6c00(r8)  /* stop streaming */
        stw     r9, 0x6c04(r8)  /* mute */

        /* video */
        mfspr   r8, 920         /* spr920 = HID2 */
        rlwinm  r8, r8, 0, 4, 2 /* stop GX FIFO, and more */
        mtspr   920, r8

        /* exi */
        lis     r8, 0xcc00      /* io mem */
1:      lwz     r9,0x680c(r8)   /* wait for dma transfer to complete */
        andi.   r9,r9,1
        bne+    1b
        stw     r9,0x6800(r8)   /* disable exi interrupts */
        addi    r8,r8,0x14      /* next channel */
        andi.   r9,r8,0x40      /* XXX 4 channels? */
        beq+    1b

        /* pic */
        lis     r8, 0xcc00      /* io mem */
        li      r9, 0
        stw     r9, 0x3004(r8)  /* mask all interrupts */
        stw     r9, 0x3000(r8)  /* clear interrupt cause */
#endif

	bl	init_cache
//	bl	init_ps
//	bl	init_fpr

	/* Set the Small Data 2 (Read Only) base register */
        lis      r2,_SDA2_BASE_@h
        ori      r2,r2,_SDA2_BASE_@l

	/* Set the Small Data (Read\Write) base register */
        lis      r13,_SDA_BASE_@h
        ori      r13,r13,_SDA_BASE_@l

.extern __bss_start, _end

	/* clear the BSS section */
        lis	r3, __bss_start@h
        ori	r3, r3, __bss_start@l
        li	r4, 0
        lis	r5, _end@h
        ori	r5, r5, _end@l
        sub	r5, r5, r3
	srawi	r5, r5, 2
        cmpwi	r5, 0
        ble	2f
        mtctr	r5
        subi	r3, r3, 4
1:
        stwu r4, 4(r3)
        bdnz 1b

2:
        /* jump to our entry point */
        bl       main

9:
	b	9b


disable_6xx_mmu:
        /* save the Link Register for later... */
        mflr    r8

        /* load in r11 the MSR with address translation bits off */
        li      r10, MSR_IR|MSR_DR
        mfmsr   r11
        andc    r11, r11, r10

        /* load in r10 the address of label 2: in physical layout */
        bl      1f
1:
        mflr    r10
        addi    r10, r10, 2f - 1b
        clrlwi  r10, r10, 3 /* virt to phys in our current address space */

        /* this will disable the MMU and jump to 2: */
        mtspr   SPRN_SRR1, r11
        mtspr   SPRN_SRR0, r10
        sync
        rfi
2:
        /* restore, and fix up, the Link Register */
        clrlwi r8, r8, 3
        mtlr r8

        blr


init_cache:
        /* enable L1 data and instructions caches */
        mfspr   r8, SPRN_HID0
        ori     r8, r8, HID0_ICE|HID0_DCE
        mtspr   SPRN_HID0, r8
	sync

        /* disable speculative bus access */
        mfspr   r8, SPRN_HID0
        ori     r8, r8, HID0_SPD
        mtspr   SPRN_HID0, r8
	sync

	blr

#if 0

init_ps:
        mfspr    r3, 920 # (HID2)
        oris     r3, r3, 0xA000
        mtspr    920, r3 # (HID2)

        # Clear various Special Purpose Registers
        li       r3,0
        mtspr    912,r3
        mtspr    913,r3
        mtspr    914,r3
        mtspr    915,r3
        mtspr    916,r3
        mtspr    917,r3
        mtspr    918,r3
        mtspr    919,r3

        blr


init_fpr:
        /* enable the Floating Point Registers */
        mfmsr    r3
        ori      r3, r3, MSR_FP
        mtmsr    r3

        # Clear all of the FPR's to 0
        lis        r3, zfloat@h
        ori        r3, r3, zfloat@l
        lfd        0, 0(r3)
        fmr      1,0
        fmr      2,0
        fmr      3,0
        fmr      4,0
        fmr      5,0
        fmr      6,0
        fmr      7,0
        fmr      8,0
        fmr      9,0
        fmr      10,0
        fmr      11,0
        fmr      12,0
        fmr      13,0
        fmr      14,0
        fmr      15,0
        fmr      16,0
        fmr      17,0
        fmr      18,0
        fmr      19,0
        fmr      20,0
        fmr      21,0
        fmr      22,0
        fmr      23,0
        fmr      24,0
        fmr      25,0
        fmr      26,0
        fmr      27,0
        fmr      28,0
        fmr      29,0
        fmr      30,0
        fmr      31,0
        mtfsf    255,0

        blr

        .balign 8
zfloat:
        .double 0

#endif

