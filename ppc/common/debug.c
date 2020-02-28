/*
 * debug.c
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

#ifdef DEBUG

#include "../include/debug.h"
#include "../include/system.h"

static const char digits[] = "0123456789abcdef";

static void dump_byte(unsigned char data)
{
	gcn_con_putc(digits[(data & 0xf0) >> 4]);
	gcn_con_putc(digits[data & 0x0f]);
}

void dbg_dump_memory(void *mem, int size)
{
	int chunk;

	while (size > 0) {
		dump_byte((((unsigned long)mem) & 0xff000000) >> 24);
		dump_byte((((unsigned long)mem) & 0x00ff0000) >> 16);
		dump_byte((((unsigned long)mem) & 0x0000ff00) >> 8);
		dump_byte((((unsigned long)mem) & 0x000000ff));
		gcn_con_putc(':');
		gcn_con_putc(' ');
		chunk = (size > 16) ? 16 : size;
		size -= chunk;
		while (chunk) {
			dump_byte(*(unsigned char *)mem);
			gcn_con_putc(' ');
			mem++;
			chunk--;
		}
		gcn_con_putc('\n');
	}
}

void dbg_print_val(void *mem, int size)
{
	int chunk;

	while (size > 0) {
		chunk = (size > 16) ? 16 : size;
		size -= chunk;
		while (chunk) {
			dump_byte(*(unsigned char *)mem);
			mem++;
			chunk--;
		}
	}
}

#endif /* DEBUG */
