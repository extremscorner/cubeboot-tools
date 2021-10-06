/**
 * apploader.c
 *
 * Simple apploader enabling booting DOLs from "El Torito" iso9660 discs.
 * This program is part of the cubeboot-tools package.
 *
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 * Copyright (C) 2020-2021 Extrems
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#define PATCH_IPL 1

#include <stddef.h>
#include <string.h>

#include "../include/system.h"

#include "../../include/gcm.h"
#include "../../include/dol.h"

#define DI_ALIGN_SHIFT	5
#define DI_ALIGN_SIZE	(1UL << DI_ALIGN_SHIFT)
#define DI_ALIGN_MASK	(~((1 << DI_ALIGN_SHIFT) - 1))

#define di_align(addr)	(void *) \
			((((unsigned long)(addr)) + \
				 DI_ALIGN_SIZE - 1) & DI_ALIGN_MASK)

/*
 * DVD data structures
 */

struct di_boot_record {
	uint8_t zero;
	uint8_t standard_id[5];	/* "CD001" */
	uint8_t version;	/* 1 */
	uint8_t boot_system_id[32];	/* "EL TORITO SPECIFICATION" */
	uint8_t boot_id[32];
	uint32_t boot_catalog_offset;	/* in media sectors */
	uint8_t align_1[21];
} __attribute__ ((__packed__));

struct di_validation_entry {
	uint8_t header_id;	/* 1 */
	uint8_t platform_id;	/* 0=80x86,1=PowerPC,2=Mac */
	uint16_t reserved;
	uint8_t id_string[24];
	uint16_t checksum;
	uint8_t key_55;		/* 55 */
	uint8_t key_AA;		/* AA */
} __attribute__ ((__packed__));

struct di_default_entry {
	uint8_t boot_indicator;	/* 0x88=bootable */
	uint8_t boot_media_type;	/* 0=no emulation */
	uint16_t load_segment;	/* multiply by 10 to get actual address */
	uint8_t system_type;
	uint8_t unused_1;
	uint16_t sector_count;	/* emulated sectors to load at segment */
	uint32_t load_rba;	/* in media sectors */
	uint8_t unused_2[20];
} __attribute__ ((__packed__));

/*
 *
 */

struct dolphin_debugger_info {
	uint32_t		present;
	uint32_t		exception_mask;
	uint32_t		exception_hook_address;
	uint32_t		saved_lr;
	unsigned char		__pad1[0x10];
} __attribute__ ((__packed__));

struct dolphin_lowmem {
	struct gcm_disk_info	b_disk_info;

	uint32_t		a_boot_magic;
	uint32_t		a_version;

	uint32_t		b_physical_memory_size;
	uint32_t		b_console_type;

	uint32_t		a_arena_lo;
	uint32_t		a_arena_hi;
	void			*a_fst;
	uint32_t		a_fst_max_size;

	struct dolphin_debugger_info a_debugger_info;
	unsigned char		hook_code[0x60];

	uint32_t		o_current_os_context_phys;
	uint32_t		o_previous_os_interrupt_mask;
	uint32_t		o_current_os_interrupt_mask;

	uint32_t		tv_mode;
	uint32_t		b_aram_size;

	void			*o_current_os_context;
	void			*o_default_os_thread;
	void			*o_thread_queue_head;
	void			*o_thread_queue_tail;
	void			*o_current_os_thread;

	uint32_t		a_debug_monitor_size;
	void			*a_debug_monitor;

	uint32_t		a_simulated_memory_size;

	void			*a_bi2;

	uint32_t		b_bus_clock_speed;
	uint32_t		b_cpu_clock_speed;
} __attribute__ ((__packed__));

/*
 *
 */

static void al_enter(void (*report) (char *text, ...));
static int al_load(void **address, uint32_t * length, uint32_t * offset);
static void *al_exit(void);

/*
 * 
 */
struct apploader_control {
	unsigned 	step;
	unsigned long	fst_address;
	uint32_t	fst_offset;
	uint32_t	fst_size;
	unsigned long	bi2_address;
	void		(*report) (char *text, ...);
};

struct bootloader_control {
	void *entry_point;
	uint32_t offset;
	uint32_t size;
	uint32_t sects_bitmap;
	uint32_t all_sects_bitmap;
};


static struct dolphin_lowmem *lowmem =
			 (struct dolphin_lowmem *)0x80000000;

static struct apploader_control al_control = { .fst_size = ~0 };
static struct bootloader_control bl_control = { .size = ~0 };

static unsigned char di_buffer[DI_SECTOR_SIZE] __attribute__ ((aligned(32))) =
	"www.gc-linux.org";

#if PATCH_IPL
static void patch_ipl(void);
static void skip_ipl_animation(void);
#endif

/*
 * This is our particular "main".
 * It _must_ be the first function in this file.
 */
void al_start(void **enter, void **load, void **exit)
{
	al_control.step = 0;

	*enter = al_enter;
	*load = al_load;
	*exit = al_exit;

#if PATCH_IPL
	patch_ipl();
#endif
}




/*
 * Loads a bitmap mask with all non-void sections in a DOL file.
 */
static int al_load_dol_sects_bitmap(struct dol_header *h)
{
	int i, sects_bitmap;

	sects_bitmap = 0;
	for (i = 0; i < DOL_MAX_SECT; i++) {
		/* zero here means the section is not in use */
		if (dol_sect_size(h, i) == 0)
			continue;

		sects_bitmap |= (1 << i);
	}
	return sects_bitmap;
}

/*
 * Checks if the DOL we are trying to boot is appropiate enough.
 */
static void al_check_dol(struct dol_header *h, int dol_length)
{
	int i, valid = 0;
	uint32_t value;

	/* now perform some sanity checks */
	for (i = 0; i < DOL_MAX_SECT; i++) {
		/* DOL segment MAY NOT be physically stored in the header */
		if ((dol_sect_offset(h, i) != 0)
		    && (dol_sect_offset(h, i) < DOL_HEADER_SIZE)) {
			panic("detected segment offset within DOL header\n");
		}

		/* offsets must be aligned to 32 bytes */
		value = dol_sect_offset(h, i);
		if (value != (uint32_t) di_align(value)) {
			panic("detected unaligned section offset\n");
		}

		/* addresses must be aligned to 32 bytes */
		value = dol_sect_address(h, i);
		if (value != (uint32_t) di_align(value)) {
			panic("unaligned section address\n");
		}

		/* end of physical storage must be within file */
		if (dol_sect_offset(h, i) + dol_sect_size(h, i) > dol_length) {
			panic("segment past DOL file size\n");
		}

		if (dol_sect_address(h, i) != 0) {
			/* we only should accept DOLs with segments above 2GB */
			if (!(dol_sect_address(h, i) & 0x80000000)) {
				panic("segment below 2GB\n");
			}
			/* we only accept DOLs below 0x81200000 */
			if (dol_sect_address(h, i) > 0x81200000) {
				panic("segment above 0x81200000\n");
			}
		}

		if (i < DOL_SECT_MAX_TEXT) {
			/* remember that entrypoint was in a code segment */
			if (h->entry_point >= dol_sect_address(h, i)
			    && h->entry_point < dol_sect_address(h, i) +
			    dol_sect_size(h, i))
				valid = 1;
		}
	}

	/* if there is a BSS segment it must^H^H^H^Hshould be above 2GB, too */
	if (h->address_bss != 0 && !(h->address_bss & 0x80000000)) {
		panic("BSS segment below 2GB\n");
	}

	/* if entrypoint is not within a code segment reject this file */
	if (!valid) {
		panic("entry point out of text segment\n");
	}

	/* we've got a valid dol if we got here */
	return;
}

/*
 * Checks if the validation entry of the boot catalog is valid.
 */
static void al_check_validation_entry(struct di_validation_entry *ve)
{
	if (ve->header_id != 1 || ve->key_55 != 0x55 || ve->key_AA != 0xAA) {
		panic("Invalid validation entry\n");
	}
}

/*
 * Checks if the default entry in the boot catalog is valid.
 */
static void al_check_default_entry(struct di_default_entry *de)
{
	if (de->boot_indicator != 0x88) {
		panic("Default entry not bootable\n");
	}
}

/*
 * Initializes the apploader related stuff.
 * Called by the IPL.
 */
static void al_enter(void (*report) (char *text, ...))
{
	al_control.step = 1;
	al_control.report = report;
	if (report)
		report("\"El Torito\" apploader\n");

}

/*
 * This is the apploader main processing function.
 * Called by the IPL.
 */
static int al_load(void **address, uint32_t *length, uint32_t *offset)
{
	struct di_boot_record *br;
	struct di_validation_entry *validation_entry;
	struct di_default_entry *default_entry;

	struct gcm_disk_header *disk_header;
	struct gcm_disk_header_info *disk_header_info;

	struct dol_header *dh;
	unsigned long lowest_start;
	int j, k;

	int need_more = 1; /* this tells the IPL if we need more data or not */

	if (al_control.report)
		al_control.report("step %d\n", al_control.step);

	switch (al_control.step) {
	case 0:
		/* should not get here if al_enter was called */
	case 1:
		al_control.step = 1; /* fix it to a known value */

		/* read sector 17, containing Boot Record Volume */
		*address = di_buffer;
		*length = (uint32_t) di_align(sizeof(*br));
		*offset = 17 * DI_SECTOR_SIZE;
		invalidate_dcache_range(*address, *address + *length);

		al_control.step++;
		break;
	case 2:
		/* boot record volume loaded */
		br = (struct di_boot_record *)di_buffer;

		/* check "EL TORITO SPECIFICATION" id */
		if (memcmp(br->boot_system_id, "EL TORITO SPECIFICATION", 23)) {
			panic("Can't find EL TORITO boot record\n");
		}

		le32_to_cpus(&br->boot_catalog_offset);

		/* read the boot catalog */
		*address = di_buffer;
		*length = DI_SECTOR_SIZE;
		*offset = br->boot_catalog_offset * DI_SECTOR_SIZE;
		invalidate_dcache_range(*address, *address + *length);

		al_control.step++;
		break;
	case 3:
		/* boot catalog loaded */

		/* check validation entry */
		validation_entry = (struct di_validation_entry *)di_buffer;
		al_check_validation_entry(validation_entry);

		/* check default bootable entry */
		default_entry = (struct di_default_entry *)(di_buffer + 0x20);
		al_check_default_entry(default_entry);

		le16_to_cpus(&default_entry->sector_count);
		le32_to_cpus(&default_entry->load_rba);

		bl_control.size = default_entry->sector_count * 512;
		bl_control.offset = default_entry->load_rba * DI_SECTOR_SIZE;

		/* request the .dol header */
		*address = di_buffer;
		*length = DOL_HEADER_SIZE;
		*offset = bl_control.offset;
		invalidate_dcache_range(*address, *address + *length);

		bl_control.sects_bitmap = 0xdeadbeef;
		al_control.step++;
		break;
	case 4:
		/* .dol header loaded */

		dh = (struct dol_header *)di_buffer;

		/* extra work on first visit */
		if (bl_control.sects_bitmap == 0xdeadbeef) {
			/* sanity checks here */
			al_check_dol(dh, bl_control.size);

			/* save our entry point */
			bl_control.entry_point = (void *)dh->entry_point;

			/* pending and valid sections, respectively */
			bl_control.sects_bitmap = 0;
			bl_control.all_sects_bitmap = al_load_dol_sects_bitmap(dh);
		}

		/*
		 * Load the sections in ascending order.
		 * We need this because we are loading a bit more of data than
		 * strictly necessary on DOLs with unaligned lengths.
		 */

		/* find lowest start address for a pending section */
		lowest_start = 0xffffffff;
		for (j = -1, k = 0; k < DOL_MAX_SECT; k++) {
			/* continue if section is already done */
			if ((bl_control.sects_bitmap & (1 << k)) != 0)
				continue;
			/* do nothing for non sections */
			if (!(bl_control.all_sects_bitmap & (1 << k)))
				continue;
			/* found new candidate */
			if (dol_sect_address(dh, k) < lowest_start) {
				lowest_start = dol_sect_address(dh, k);
				j = k;
			}
		}
		/* mark section as being loaded */
		bl_control.sects_bitmap |= (1 << j);

		/* request a .dol section */
		*address = (void *)dol_sect_address(dh, j);;
		*length = (uint32_t) di_align(dol_sect_size(dh, j));
		*offset = bl_control.offset + dol_sect_offset(dh, j);

		invalidate_dcache_range(*address, *address + *length);
		if (dol_sect_is_text(dh, j))
			invalidate_icache_range(*address, *address + *length);

		/* check if we are going to be done with all sections */
		if (bl_control.sects_bitmap == bl_control.all_sects_bitmap) {
			/* setup .bss section */
			if (dh->size_bss)
				memset((void *)dh->address_bss, 0,
				       dh->size_bss);

			/* bye, bye */
			al_control.step++;
		}
		break;
	case 5:
		/* all .dol sections loaded */

		/* read sector 0, containing disk header and disk header info */
		*address = di_buffer;
		*length = (uint32_t) di_align(sizeof(*disk_header) + sizeof(*disk_header_info));
		*offset = 0;
		invalidate_dcache_range(*address, *address + *length);

		al_control.step++;
		break;
	case 6:
		/* boot.bin and bi2.bin header loaded */

		disk_header = (struct gcm_disk_header *)di_buffer;
		disk_header_info = (struct gcm_disk_header_info *)(di_buffer + sizeof(*disk_header));

		al_control.fst_offset = disk_header->layout.fst_offset;
		al_control.fst_size = disk_header->layout.fst_size;
		al_control.fst_address = (0x81800000 - al_control.fst_size) & DI_ALIGN_MASK;

		/* read fst.bin */
		*address = (void *)al_control.fst_address;
		*length = (uint32_t) di_align(al_control.fst_size);
		*offset = al_control.fst_offset;
		invalidate_dcache_range(*address, *address + *length);

		al_control.step++;
		break;
	case 7:
		/* fst.bin loaded */

		al_control.bi2_address = al_control.fst_address - 0x2000;

		/* read bi2.bin */
		*address = (void *)al_control.bi2_address;
		*length = 0x2000;
		*offset = 0x440;
		invalidate_dcache_range(*address, *address + *length);

		al_control.step++;
		break;
	case 8:
		/* bi2.bin loaded */

		lowmem->a_boot_magic = 0x0d15ea5e;
		lowmem->a_version = 1;

		lowmem->a_arena_hi = al_control.fst_address;
		lowmem->a_fst = (void *)al_control.fst_address;
		lowmem->a_fst_max_size = al_control.fst_size;
		//memset(&lowmem->a_debugger_info, 0, sizeof(struct dolphin_debugger_info));
		//lowmem->a_debug_monitor_size = 0;
		lowmem->a_debug_monitor = (void *)0x81800000;
		lowmem->a_simulated_memory_size = 0x01800000;
		lowmem->a_bi2 = (void *)al_control.bi2_address;
		flush_dcache_range(lowmem, lowmem+1);

#if PATCH_IPL
		skip_ipl_animation();
#endif
		*length = 0;
		need_more = 0;
		al_control.step++;
		break;
	default:
		al_control.step++;
		break;
	}

	return need_more;
}

/*
 *
 */
static void *al_exit(void)
{
	return bl_control.entry_point;
}

#if PATCH_IPL

/*
 *
 */
enum ipl_revision {
	IPL_UNKNOWN,
	IPL_NTSC_10_001,
	IPL_NTSC_10_002,
	IPL_DEV_10,
	IPL_NTSC_11_001,
	IPL_PAL_10_001,
	IPL_PAL_10_002,
	IPL_MPAL_11,
	IPL_TDEV_11,
	IPL_NTSC_12_001,
	IPL_NTSC_12_101,
	IPL_PAL_12_101
};

static enum ipl_revision get_ipl_revision(void)
{
	register uint32_t sdata2 asm ("r2");
	register uint32_t sdata asm ("r13");

	if (sdata2 == 0x81465cc0 && sdata == 0x81465320)
		return IPL_NTSC_10_001;
	if (sdata2 == 0x81468fc0 && sdata == 0x814685c0)
		return IPL_NTSC_10_002;
	if (sdata2 == 0x814695e0 && sdata == 0x81468bc0)
		return IPL_DEV_10;
	if (sdata2 == 0x81489c80 && sdata == 0x81489120)
		return IPL_NTSC_11_001;
	if (sdata2 == 0x814b5b20 && sdata == 0x814b4fc0)
		return IPL_PAL_10_001;
	if (sdata2 == 0x814b4fc0 && sdata == 0x814b4400)
		return IPL_PAL_10_002;
	if (sdata2 == 0x81484940 && sdata == 0x81483de0)
		return IPL_MPAL_11;
	if (sdata2 == 0x8148fbe0 && sdata == 0x8148ef80)
		return IPL_TDEV_11;
	if (sdata2 == 0x8148a660 && sdata == 0x8148b1c0)
		return IPL_NTSC_12_001;
	if (sdata2 == 0x8148aae0 && sdata == 0x8148b640)
		return IPL_NTSC_12_101;
	if (sdata2 == 0x814b66e0 && sdata == 0x814b7280)
		return IPL_PAL_12_101;

	return IPL_UNKNOWN;
}

/*
 *
 */
static void patch_ipl(void)
{
	uint32_t *start, *end;
	uint32_t *address;

	switch (get_ipl_revision()) {
	case IPL_NTSC_10_001:
		start = (uint32_t *)0x81300a70;
		end = (uint32_t *)0x813010b0;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x81300e88;
			if (*address == 0x38000000)
				*address |= 1;

			address = (uint32_t *)0x81300ea0;
			if (*address == 0x38000000)
				*address |= 1;

			address = (uint32_t *)0x81300ea8;
			if (*address == 0x38000000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_NTSC_10_002:
		start = (uint32_t *)0x813008d8;
		end = (uint32_t *)0x8130096c;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x8130092c;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x81300944;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x8130094c;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_DEV_10:
		start = (uint32_t *)0x81300dfc;
		end = (uint32_t *)0x81301424;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x8130121c;
			if (*address == 0x38000000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_NTSC_11_001:
	case IPL_PAL_10_001:
	case IPL_MPAL_11:
		start = (uint32_t *)0x813006e8;
		end = (uint32_t *)0x813007b8;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x8130077c;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x813007a0;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_PAL_10_002:
		start = (uint32_t *)0x8130092c;
		end = (uint32_t *)0x81300a10;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x813009d4;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x813009f8;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_TDEV_11:
		start = (uint32_t *)0x81300b58;
		end = (uint32_t *)0x81300c3c;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x81300c00;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x81300c24;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_NTSC_12_001:
	case IPL_NTSC_12_101:
		start = (uint32_t *)0x81300a24;
		end = (uint32_t *)0x81300b08;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x81300acc;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x81300af0;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	case IPL_PAL_12_101:
		start = (uint32_t *)0x813007d8;
		end = (uint32_t *)0x813008bc;
		if (start[0] == 0x7c0802a6 && end[-1] == 0x4e800020) {
			address = (uint32_t *)0x81300880;
			if (*address == 0x38600000)
				*address |= 1;

			address = (uint32_t *)0x813008a4;
			if (*address == 0x38600000)
				*address |= 1;

			flush_dcache_range(start, end);
			invalidate_icache_range(start, end);
		}
		break;
	default:
		break;
	}
}

/*
 *
 */
static void skip_ipl_animation(void)
{
	switch (get_ipl_revision()) {
	case IPL_NTSC_10_001:
		if (*(uint32_t *)0x8145d6d0 == 1
			&& !(*(uint16_t *)0x8145f14c & 0x0100)
			&& *(uint32_t *)0x8145d6f0 == 0x81465728)
			*(uint8_t *)0x81465747 = 1;
		break;
	case IPL_NTSC_10_002:
		if (*(uint32_t *)0x814609c0 == 1
			&& !(*(uint16_t *)0x814624ec & 0x0100)
			&& *(uint32_t *)0x814609e0 == 0x81468ac8)
			*(uint8_t *)0x81468ae7 = 1;
		break;
	case IPL_DEV_10:
		if (*(uint32_t *)0x81460fe0 == 1
			&& !(*(uint16_t *)0x81462b0c & 0x0100)
			&& *(uint32_t *)0x81461000 == 0x814690e8)
			*(uint8_t *)0x81469107 = 1;
		break;
	case IPL_NTSC_11_001:
		if (*(uint32_t *)0x81481518 == 1
			&& !(*(uint16_t *)0x8148370c & 0x0100)
			&& *(uint32_t *)0x81481538 == 0x81489e58)
			*(uint8_t *)0x81489e77 = 1;
		break;
	case IPL_PAL_10_001:
		if (*(uint32_t *)0x814ad3b8 == 1
			&& !(*(uint16_t *)0x814af60c & 0x0100)
			&& *(uint32_t *)0x814ad3d8 == 0x814b5d58)
			*(uint8_t *)0x814b5d77 = 1;
		break;
	case IPL_PAL_10_002:
		if (*(uint32_t *)0x814ac828 == 1
			&& !(*(uint16_t *)0x814aeb2c & 0x0100)
			&& *(uint32_t *)0x814ac848 == 0x814b5278)
			*(uint8_t *)0x814b5297 = 1;
		break;
	case IPL_MPAL_11:
		if (*(uint32_t *)0x8147c1d8 == 1
			&& !(*(uint16_t *)0x8147e3cc & 0x0100)
			&& *(uint32_t *)0x8147c1f8 == 0x81484b18)
			*(uint8_t *)0x81484b37 = 1;
		break;
	case IPL_TDEV_11:
		if (*(uint32_t *)0x81487438 == 1
			&& !(*(uint16_t *)0x8148972c & 0x0100)
			&& *(uint32_t *)0x81487458 == 0x8148fe78)
			*(uint8_t *)0x8148fe97 = 1;
		break;
	case IPL_NTSC_12_001:
		if (*(uint32_t *)0x814835f0 == 1
			&& !(*(uint16_t *)0x81484cec & 0x0100)
			&& *(uint32_t *)0x81483610 == 0x8148b438)
			*(uint8_t *)0x8148b457 = 1;
		break;
	case IPL_NTSC_12_101:
		if (*(uint32_t *)0x81483a70 == 1
			&& !(*(uint16_t *)0x8148518c & 0x0100)
			&& *(uint32_t *)0x81483a90 == 0x8148b8d8)
			*(uint8_t *)0x8148b8f7 = 1;
		break;
	case IPL_PAL_12_101:
		if (*(uint32_t *)0x814af6b0 == 1
			&& !(*(uint16_t *)0x814b0dcc & 0x0100)
			&& *(uint32_t *)0x814af6d0 == 0x814b7518)
			*(uint8_t *)0x814b7537 = 1;
		break;
	default:
		break;
	}
}

#endif

