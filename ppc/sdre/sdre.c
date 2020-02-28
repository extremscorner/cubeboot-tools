/*
 * sdre.c
 *
 * Silly Dol Relocation Engine
 * This program is part of the cubeboot-tools package.
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

#include "../include/system.h"
#include "../include/debug.h"

#include "../../include/dolrel.h"


#define mtmsr(v)        asm volatile("mtmsr %0" : : "r" (v))
#define mfmsr()         ({unsigned long rval; \
                        asm volatile("mfmsr %0" : "=r" (rval)); rval;})

#define __MASK(X)       (1UL<<(X))

#define MSR_EE_LG       15              /* External Interrupt Enable */

#define MSR_EE          __MASK(MSR_EE_LG)

#define local_irq_save(flags)   local_irq_save_ptr(&flags)
#define local_irq_restore(flags)        mtmsr(flags)

static inline void local_irq_save_ptr(unsigned long *flags)
{
        unsigned long msr;
        msr = mfmsr();
        *flags = msr;
        mtmsr(msr & ~MSR_EE);
        __asm__ __volatile__("": : :"memory");
}

static inline void local_irq_disable(void)
{
        unsigned long msr;
        __asm__ __volatile__("": : :"memory");
        msr = mfmsr();
        mtmsr(msr & ~MSR_EE);
}

#define io_base ((void *)0xcc006000)

#define DI_DMA_ALIGN            0x1f /* 32 bytes */

/* DI Status Register */
#define DI_SR                   0x00
#define  DI_SR_BRK              (1<<0)
#define  DI_SR_DEINTMASK        (1<<1)
#define  DI_SR_DEINT            (1<<2)
#define  DI_SR_TCINTMASK        (1<<3)
#define  DI_SR_TCINT            (1<<4)
#define  DI_SR_BRKINTMASK       (1<<5)
#define  DI_SR_BRKINT           (1<<6)

/* DI Cover Register */
#define DI_CVR                  0x04
#define  DI_CVR_CVR             (1<<0)
#define  DI_CVR_CVRINTMASK      (1<<1)
#define  DI_CVR_CVRINT          (1<<2)

/* DI Command Buffers */
#define DI_CMDBUF0              0x08
#define DI_CMDBUF1              0x0c
#define DI_CMDBUF2              0x10

/* DI DMA Memory Address Register */
#define DI_MAR                  0x14

/* DI DMA Transfer Length Register */
#define DI_LENGTH               0x18

/* DI Control Register */
#define DI_CR                   0x1c
#define  DI_CR_TSTART           (1<<0)
#define  DI_CR_DMA              (1<<1)
#define  DI_CR_RW               (1<<2)

#define __DI_DONT_WAIT          (1<<7)

/* DI Immediate Data Buffer */
#define DI_DATA                 0x20

/* DI Configuration Register */
#define DI_CFG                  0x24

/* drive status, status */
#define DI_STATUS(s)            ((unsigned char)((s)>>24))

#define DI_STATUS_READY                 0x00
#define DI_STATUS_COVER_OPENED          0x01
#define DI_STATUS_DISK_CHANGE           0x02
#define DI_STATUS_NO_DISK               0x03
#define DI_STATUS_MOTOR_STOP            0x04
#define DI_STATUS_DISK_ID_NOT_READ      0x05


#define CMDBUF(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))


/*
 *
 */
static void mdelay(int msecs)
{
	const unsigned long ticks_per_msec = 162000 / 4;
	unsigned long start_ticks, loops;

	loops = msecs * ticks_per_msec;
	start_ticks = ticks();
	while(ticks() - start_ticks < loops)
		;
}


/*
 * DI
 * Drive Interface.
 */

/*
 * Hard resets the DVD unit.
 */
static void di_reset(void)
{
        unsigned long *reset_reg = (unsigned long *)0xcc003024;
        unsigned long reset, i;

#define FLIPPER_RESET_DVD 0x00000004

        reset = readl(reset_reg);
        writel((reset & ~FLIPPER_RESET_DVD) | 1, reset_reg);
        for (i = 0; i < 0xfffff; i++) ;
        writel((reset | FLIPPER_RESET_DVD) | 1, reset_reg);
}

/*
 * Calms down and brings the DVD unit to a known state.
 */
static void di_quiesce(void)
{
	unsigned long *sr_reg = io_base + DI_SR;
	unsigned long *cvr_reg = io_base + DI_CVR;
	unsigned long sr, cvr;
	unsigned long flags;

	local_irq_save(flags);

        /* ack and mask dvd io interrupts */
        sr = readl(sr_reg);
        sr |= DI_SR_BRKINT | DI_SR_TCINT | DI_SR_DEINT;
        sr &= ~(DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINTMASK);
        writel(sr, sr_reg);

        /* ack and mask dvd cover interrupts */
        cvr = readl(cvr_reg);
        writel((cvr | DI_CVR_CVRINT) & ~DI_CVR_CVRINTMASK, cvr_reg);

	local_irq_restore(flags);
}

/*
 * Runs a DI command.
 */
static int di_run_command(unsigned long *cmdbuf, void *data, int len, int mode)
{
        unsigned long *sr_reg = io_base + DI_SR;
        unsigned long *cr_reg = io_base + DI_CR;
        int result = 0;

        writel(cmdbuf[0], io_base + DI_CMDBUF0);
        writel(cmdbuf[1], io_base + DI_CMDBUF1);
        writel(cmdbuf[2], io_base + DI_CMDBUF2);

        if (mode & DI_CR_DMA) {
                /* setup address and length of transfer */
                writel(len, io_base + DI_LENGTH);
                writel((unsigned long)data, io_base + DI_MAR);

                flush_dcache_range(data, data + len);
        }

        if (mode & DI_CR_TSTART) {
                /* disable status related interrupts, we do polled io */
                writel(readl(sr_reg) &
                       ~(DI_SR_TCINTMASK | DI_SR_DEINTMASK), sr_reg);

                /* start the transfer */
                writel(mode & 0x7, cr_reg);

                if (!(mode & __DI_DONT_WAIT)) {
                        /* busy-wait */
                        while ((readl(cr_reg) & DI_CR_TSTART)) ;

                        if ((readl(sr_reg) & DI_SR_DEINT))
                                result = -1;
                }

                /* ack interrupts */
                writel(readl(sr_reg) | (DI_SR_DEINT | DI_SR_TCINT), sr_reg);
        }

        if (mode & DI_CR_DMA) {
                if (mode & DI_CR_RW)
                        invalidate_dcache_range(data, data + len);
        }

        return result;
}

/*
 * Stop the drive motor.
 */
static unsigned long di_stop_motor(void)
{
        unsigned long cmdbuf[3];

        cmdbuf[0] = 0xe3000000;
        cmdbuf[1] = cmdbuf[2] = 0;
        return di_run_command(cmdbuf, 0, 0, DI_CR_TSTART);
}

/*
 * Enables the "debug" command set.
 */
static int di_enable_debug_commands(void)
{
        unsigned long cmdbuf[3];

        cmdbuf[0] = CMDBUF(0xff, 0x01, 'M', 'A');
        cmdbuf[1] = CMDBUF('T', 'S', 'H', 'I');
        cmdbuf[2] = CMDBUF('T', 'A', 0x02, 0x00);
        di_run_command(cmdbuf, 0, 0, DI_CR_TSTART);

        cmdbuf[0] = CMDBUF(0xff, 0x00, 'D', 'V');
        cmdbuf[1] = CMDBUF('D', '-', 'G', 'A');
        cmdbuf[2] = CMDBUF('M', 'E', 0x03, 0x00);
        return di_run_command(cmdbuf, 0, 0, DI_CR_TSTART);
}

/*
 *
 * Firmware handling.
 */

#define DI_DRIVE_IRQ_VECTOR     0x00804c

/*
 * Reads a long word from drive addressable memory.
 * Requires debug mode enabled.
 */
static int di_fw_read_meml(unsigned long *data, unsigned long address)
{
        unsigned long *data_reg = io_base + DI_DATA;
        unsigned long cmdbuf[3];
        int result;

        cmdbuf[0] = 0xfe010000;
        cmdbuf[1] = address;
        cmdbuf[2] = 0x00010000;
        result = di_run_command(cmdbuf, 0, 0, DI_CR_TSTART);
        if (!result)
                *data = readl(data_reg);

        return result;
}

/*
 * Disables the xenogc drivechip.
 */
static void di_disable_xenogc(void)
{
        unsigned long cmdbuf[3];

        cmdbuf[0] = 0x25000000;
        cmdbuf[1] = cmdbuf[2] = 0;
        di_run_command(cmdbuf, 0, 0, DI_CR_TSTART);
}

/*
 * Ensures that the debug features of the drive firmware are working.
 */
static int di_test_debug_features(void)
{
        int result;

        result = di_enable_debug_commands();
        if (result) {
                di_reset();
                result = di_enable_debug_commands();
        }

        return result;
}

/*
 * Checks for a xenogc and cleanly disables it.
 */
static void sdre_disable_xenogc(void)
{
	int i;
	unsigned long val;

	if (!di_test_debug_features()) {
	        if (!di_fw_read_meml(&val, 0x40c60a)) {
	                if (val == 0xf710fff7) {
				/*
				 * Disable the drivechip, but leave it enough
				 * time to xfer the apploader patch.
				 * Otherwise, it will misbehave.
				 */
				di_disable_xenogc();
				mdelay(8000); /* 8 secs seems enough */

				/*
				 * Wait for reset logic to become zero.
				 * (Usually, not needed).
				 */
				for(i=0; i < 0x1111; i++) {
					val = 1;
        				di_enable_debug_commands();
	        			di_fw_read_meml(&val, 0x40d100);
					le32_to_cpus((uint32_t *)&val);
					val &= 0xffff;
					if (val == 0x0000)
						break;
					rumble(1);
				}
				rumble(0);
			}
		}
	}
}


/*
 * Ditto.
 */
static void relocate_sections(struct dolrel_control *dc)
{
	struct dolrel_section *section = (struct dolrel_section *)(dc + 1);
	void *src_address = dc->src_address;
	uint32_t nr_sections = dc->nr_sections;

	while (nr_sections > 0) {
		memcpy(section->dst_address, src_address, section->length);
		flush_dcache_range(section->dst_address,
				   section->dst_address + section->length);
		invalidate_icache_range(section->dst_address,
					section->dst_address + section->length);

		src_address += section->length;
		nr_sections--;

		section++;
	}
}

typedef void (*entry_point_t) (void);

int main(void)
{
	struct dolrel_control *dc = &__dolrel_control;
	entry_point_t f;
	unsigned long val;

	local_irq_disable();

	if (dc->flags & (DOLREL_FLAG_STOP_MOTOR|DOLREL_FLAG_DISABLE_XENOGC))
		di_quiesce();

	if (dc->flags & DOLREL_FLAG_STOP_MOTOR)
		di_stop_motor();

	if (dc->flags & DOLREL_FLAG_DISABLE_XENOGC)
		sdre_disable_xenogc();

	relocate_sections(dc);
	if (dc->size_bss) {
		memset(dc->address_bss, 0, dc->size_bss);
		flush_dcache_range(dc->address_bss,
				   dc->address_bss + dc->size_bss);
	}

	f = (entry_point_t) dc->entry_point;
	(*f) ();

	return 0;
}

