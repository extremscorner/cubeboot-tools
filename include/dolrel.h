/*
 * dolrel.h
 *
 * (simple) DOL relocator helper definitions.
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __DOLREL_H
#define __DOLREL_H

#include <sys/types.h>
#include <stdint.h>

#define DOLREL_FLAG_STOP_MOTOR     (1<<0)
#define DOLREL_FLAG_DISABLE_XENOGC (1<<1)

struct dolrel_section {
	void	*dst_address;
	size_t	length;
};

struct dolrel_control {
	uint32_t	version;
	unsigned long	flags;
	void		*entry_point;
	void		*address_bss;
	uint32_t	size_bss;
	void		*src_address;
	uint32_t	nr_sections;
};

extern struct dolrel_control __dolrel_control;

#endif /* __DOLREL_H */
