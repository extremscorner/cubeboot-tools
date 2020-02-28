/*
 * dol.h
 *
 * DolphinOS executable file format definitions.
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __DOL_H
#define __DOL_H

#include <stdint.h>

/*
 * DOL related stuff
 */

#define DOL_HEADER_SIZE		0x100

#define DOL_SECT_MAX_TEXT	7	/* text sections */
#define DOL_SECT_MAX_DATA	11	/* data sections */
#define DOL_MAX_SECT		(DOL_SECT_MAX_TEXT+DOL_SECT_MAX_DATA)

#define dol_sect_offset(hptr, index) \
                ((index >= DOL_SECT_MAX_TEXT)? \
                        hptr->offset_data[index - DOL_SECT_MAX_TEXT] \
                        :hptr->offset_text[index])
#define dol_sect_address(hptr, index) \
                ((index >= DOL_SECT_MAX_TEXT)? \
                        hptr->address_data[index - DOL_SECT_MAX_TEXT] \
                        :hptr->address_text[index])
#define dol_sect_size(hptr, index) \
                ((index >= DOL_SECT_MAX_TEXT)? \
                        hptr->size_data[index - DOL_SECT_MAX_TEXT] \
                        :hptr->size_text[index])
#define dol_sect_is_text(hptr, index) \
                ((index >= DOL_SECT_MAX_TEXT) ? 0 : 1)

struct dol_header {
	uint32_t offset_text[DOL_SECT_MAX_TEXT];	/* in the file */
	uint32_t offset_data[DOL_SECT_MAX_DATA];
	uint32_t address_text[DOL_SECT_MAX_TEXT];	/* in memory */
	uint32_t address_data[DOL_SECT_MAX_DATA];
	uint32_t size_text[DOL_SECT_MAX_TEXT];
	uint32_t size_data[DOL_SECT_MAX_DATA];
	uint32_t address_bss;
	uint32_t size_bss;
	uint32_t entry_point;
	uint8_t __pad[0x1c];
} __attribute__ ((__packed__));

#endif /* __DOL_H */

