/*
 * ppm2bnr.c
 *
 * Converts a 96x32 ppm image file to a Nintendo GameCube .BNR file
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

#include <stdio.h>
#include <string.h>

#include <netpbm/pam.h>

#include "../include/lib.h"
#include "../include/bnr.h"

#define _GNU_SOURCE
#include <getopt.h>

#define PPM2BNR_VERSION "V0.1-20122005"

#define DEFAULT_GAME_NAME	"Bootable iso9660 Disc"
#define DEFAULT_COMPANY		"(company name)"
#define DEFAULT_FULL_GAME_TITLE	"(full game title)"
#define DEFAULT_GAME_DESCR	"Built with cubeboot-tools,\n" \
				"have fun! :)"


const char *__progname;


struct banner_header bh;
struct banner_description bd;

unsigned short banner_raster[BNR_WIDTH*BNR_HEIGHT];


/**
 *
 */
int convert_ppm_to_bnr(FILE *fout, FILE *fin, struct banner_description *bd)
{
	int tiles, cols, rows;
	int tile, col, row;
	int x, y;
	unsigned long r ,g ,b;
	pixel **pixbuf, p;
	pixval maxval;

	unsigned short *outp;

	pixbuf = ppm_readppm(fin, &cols, &rows, &maxval);
	if (cols != BNR_WIDTH || rows != BNR_HEIGHT) {
		die("can only convert %dx%d ppm images, sorry\n",
			BNR_WIDTH, BNR_HEIGHT);
	}

	memset(&bh, 0, sizeof(bh));
	memcpy(bh.magic, BNR_MAGIC1, 4);

	if (fwrite(&bh, sizeof(bh), 1, fout) != 1)
		die("write failed: %s\n", strerror(errno));

	tiles = (cols * rows) / BNR_TILE_SIZE;
	col = row = 0;

	outp = banner_raster;
	for(tile = 0; tile < tiles; tile++) {	
		for(y = 0; y < 4; y++) {
			for(x = 0; x < 4; x++) {
				p = pixbuf[row+y][col+x];
				r = PPM_GETR(p);
				g = PPM_GETG(p);
				b = PPM_GETB(p);

				/* convert from 8 to 5 bits */
				r = (r * (1<<5)) / 256;
				g = (g * (1<<5)) / 256;
				b = (b * (1<<5)) / 256;

				*outp++ = cpu_to_be16((1<<15) | (r << 10) | (g << 5) | b);
			}
		}

		col += 4;
		if (col >= cols) {
			col = 0;
			row += 4;
		}
	}

	ppm_freearray(pixbuf, rows);

	fwrite(banner_raster, sizeof(banner_raster), 1, fout);

	fwrite(bd, sizeof(*bd), 1, fout);
}

/**
 *
 */
void version(void)
{
        printf("version %s\n", PPM2BNR_VERSION);
        exit(2);
}

/**
 *
 */
void usage(void)
{
        fprintf(stderr,
                "Usage: %s [OPTION] [FILE] -o [OUTFILE]" "\n"
                "  -n, --name=TEXT         set name (32 chars max)" "\n"
                "  -c, --company=TEXT      set company (32 chars max)" "\n"
                "  -N, --full_name=TEXT    set full name (64 chars max)" "\n"
                "  -C, --full_company=TEXT set full company (64 chars max)" "\n"
                "  -d, --description=TEXT  set description (128 chars max)" "\n"
                "  -o, --outfile=PATH      output file (default stdout)" "\n"
                , __progname);
        exit(1);
}

/**
 *
 */
int set_name(struct banner_description *bd, char *optarg)
{
	if (strlen(optarg) >= BNR_NAME_LEN) {
		fprintf(stderr, "name length exceeds %d chars\n",
			BNR_NAME_LEN);
		return -1;
	}

	strcpy(bd->name, optarg);
	return 0;
}

/**
 *
 */
int set_company(struct banner_description *bd, char *optarg)
{
	if (strlen(optarg) >= BNR_COMPANY_LEN) {
		fprintf(stderr, "company length exceeds %d chars\n",
			BNR_COMPANY_LEN);
		return -1;
	}

	strcpy(bd->company, optarg);
	return 0;
}

/**
 *
 */
int set_full_name(struct banner_description *bd, char *optarg)
{
	if (strlen(optarg) >= BNR_FULL_NAME_LEN) {
		fprintf(stderr, "full name length exceeds %d chars\n",
			BNR_FULL_NAME_LEN);
		return -1;
	}

	strcpy(bd->full_name, optarg);
	return 0;
}

/**
 *
 */
int set_full_company(struct banner_description *bd, char *optarg)
{
	if (strlen(optarg) >= BNR_FULL_COMPANY_LEN) {
		fprintf(stderr, "full company length exceeds %d chars\n",
			BNR_FULL_COMPANY_LEN);
		return -1;
	}

	strcpy(bd->full_company, optarg);
	return 0;
}

/**
 *
 */
int set_description(struct banner_description *bd, char *optarg)
{
	if (strlen(optarg) >= BNR_DESCRIPTION_LEN) {
		fprintf(stderr, "description length exceeds %d chars\n",
			BNR_DESCRIPTION_LEN);
		return -1;
	}

	strcpy(bd->description, optarg);
	return 0;
}

/**
 *
 */
int main(int argc, char *argv[])
{
	char *outfile = NULL, *infile = NULL;
	FILE *fout, *fin;
        char *p;
	int ch;
	int result;

        struct option long_options[] = {
                {"name", 1, NULL, 'n'},
                {"company", 1, NULL, 'c'},
                {"full_name", 1, NULL, 'n'},
                {"full_company", 1, NULL, 'C'},
                {"description", 1, NULL, 'D'},
                {"outfile", 1, NULL, 'o'},
                {"version", 0, NULL, 'v'},
                {"help", 0, NULL, 'h'},
                {0,0,0,0}
        };
#define SHORT_OPTIONS "n:c:N:C:d:o:vh"

	ppm_init(&argc, argv);

	memset(&bd, 0, sizeof(bd));

        p = strrchr(argv[0], '/');
        __progname = (p && p[1]) ? p+1 : argv[0];

       while((ch = getopt_long(argc, argv, SHORT_OPTIONS,
                                long_options, NULL)) != -1) {
                switch(ch) {
                        case 'n':
				if (set_name(&bd, optarg) < 0)
					usage();
                                break;
                        case 'c':
                                if (set_company(&bd, optarg) < 0)
                                        usage();
                                break;
                        case 'N':
				if (set_full_name(&bd, optarg) < 0)
					usage();
                                break;
                        case 'C':
                                if (set_full_company(&bd, optarg) < 0)
                                        usage();
                                break;
                        case 'd':
                                if (set_description(&bd, optarg) < 0)
                                        usage();
                                break;
                        case 'o':
				outfile = optarg;
                                break;
                        case 'v':
                                version();
                                break;
                        case 'h':
                        case '?':
                        default:
                                usage();
                                break;
                }
        }

        if (argc-optind == 1) {
		infile = argv[optind];
        } else if (argc-optind > 1) {
                usage();
	}

	if (!infile) {
		infile = "*stdin*";
		fin = stdin;
	} else {
		if (!strcmp(infile, "-")) {
			infile = "*stdin*";
			fin = stdin;
		} else {
			fin = fopen(infile, "r");
			if (!fin) {
				die("%s: can't open input file: %s\n",
					infile, strerror(errno));
			}
		}
	}

	if (!outfile) {
		outfile = "*stdout*";
		fout = stdout;
	} else {
		if (!strcmp(outfile, "-")) {
			outfile = "*stdout*";
			fout = stdout;
		} else {
			fout = fopen(outfile, "w");
			if (!fout) {
				die("%s: can't open output file: %s\n",
					outfile, strerror(errno));
			}
		}
	}

	if (!bd.name[0])
		strcpy(bd.name, DEFAULT_GAME_NAME);
	if (!bd.company[0])
		strcpy(bd.company, DEFAULT_COMPANY);
	if (!bd.full_name[0])
		strcpy(bd.full_name, DEFAULT_FULL_GAME_TITLE);
	if (!bd.full_company[0])
		strcpy(bd.full_company, DEFAULT_COMPANY);
	if (!bd.description[0])
		strcpy(bd.description, DEFAULT_GAME_DESCR);

	convert_ppm_to_bnr(fout, fin, &bd);

	fclose(fout);
	fclose(fin);
}

