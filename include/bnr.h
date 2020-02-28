/**
 * bnr.h
 *
 * GameCube .BNR related definitions
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __BNR_H
#define __BNR_H

#define BNR_WIDTH 96
#define BNR_HEIGHT 32

#define BNR_TILE_SIZE (4*4) /* 4 x 4 16 bits = 32 bytes */

#define BNR_MAGIC1 "BNR1"
#define BNR_MAGIC2 "BNR2"

struct banner_header {
	char magic[4];
	unsigned char pad[28];
};

#define BNR_NAME_LEN		32
#define BNR_COMPANY_LEN		32
#define BNR_FULL_NAME_LEN	64
#define BNR_FULL_COMPANY_LEN	64
#define BNR_DESCRIPTION_LEN	128

struct banner_description {
	char name[BNR_NAME_LEN];
	char company[BNR_COMPANY_LEN];
	char full_name[BNR_FULL_NAME_LEN];
	char full_company[BNR_FULL_COMPANY_LEN];
	char description[BNR_DESCRIPTION_LEN];
};

#endif /* __BNR_H */
