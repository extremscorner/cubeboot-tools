/**
 * bnr2ppm.c
 *
 * Simple Nintendo GameCube .BNR to .PPM converter
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/lib.h"

#define BUF_SIZE 4096
static char buf[BUF_SIZE];

#define BNR_WIDTH 96
#define BNR_HEIGHT 32

unsigned int rgb32_image[BNR_WIDTH*BNR_HEIGHT];

void bnr2ppm(void *image, off_t image_size)
{
	unsigned char *p, *outp;
	unsigned short *rgba, rgba_cpu;
	unsigned int r, g, b;

#define BNR_TILE_SIZE (4*4) /* 4 x 4 16 bits = 32 bytes */

	int width = BNR_WIDTH, height = BNR_HEIGHT;
	int tiles = (width * height) / BNR_TILE_SIZE;

	int tile, pixel_count;
	int row, col;
	int i, j, pos;

	p = image;
	if (memcmp(p, "BNR1", 4) && memcmp(p, "BNR2", 4))
		die("not a banner file\n");

	pixel_count = 0x1800 / sizeof(*rgba);
	rgba = (unsigned short *)(image + 0x20);

	printf("P6 %d %d %d\n", BNR_WIDTH, BNR_HEIGHT, 255);
	if (pixel_count != BNR_WIDTH * BNR_HEIGHT)
		die("wrong pixel count\n");

	memset(rgb32_image, 0, sizeof(rgb32_image));

	col = row = 0;
	for (tile = 0; tile < tiles; tile++, rgba += BNR_TILE_SIZE) {
		for(i=0; i<4; i++) { /* Y */
			for(j=0; j<4; j++) { /* X */
				rgba_cpu = be16_to_cpu(rgba[4*i+j]);

				/* retrieve components */
				r = (rgba_cpu >> 10) & 0x1f;
				g = (rgba_cpu >> 5) & 0x1f;
				b = (rgba_cpu >> 0) & 0x1f;

				/* aproximate to 8 bits */
				r = (r << 3) | (r >> 2);
				g = (g << 3) | (g >> 2);
				b = (b << 3) | (b >> 2);

				pos = (row+i)*width + (col+j);
				outp = (unsigned char *)&rgb32_image[pos];

				*outp++ = r;
				*outp++ = g;
				*outp++ = b;
			}
		}

		col += 4;
		if (col >= BNR_WIDTH) {
			col = 0;
			row += 4;
		}
	}

	outp = (unsigned char *)rgb32_image;
	while(pixel_count-- > 0) {
		r = *outp++;
		g = *outp++;
		b = *outp++;
		outp++;
		printf("%c%c%c", r, g, b);
	}
	printf("\n");
}

/*
 *
 */
int main(int argc, char *argv[])
{
	int result;
	void *image;
	off_t image_size;

	image = slurp_file("opening.bnr", &image_size);

	bnr2ppm(image, image_size);
}

