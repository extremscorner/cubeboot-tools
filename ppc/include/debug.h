/*
 * debug.h
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

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG

#include "gcn-con.h"

#define dbg_rumble(enable)	rumble(enable)
#define dbg_puts(a)		gcn_con_puts(a)

extern void dbg_dump_memory(void *mem, int size);
#define dbg_dump_ref(a) dbg_dump_memory(a, sizeof(*a))

extern void dbg_print_val(void *mem, int size);

#else

#define dbg_rumble(enable)		do {} while(0)
#define dbg_puts(a)			do {} while(0)

#define dbg_dump_memory(mem, size)	do {} while(0)
#define dbg_dump_ref(a)			do {} while(0)

#define dbg_print_val(mem, size)	do {} while(0)

#endif

#endif /* __DEBUG_H */
