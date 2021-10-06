/**
 * mkgbi.c
 *
 * Generic Boot Image builder for bootable iso9660 discs
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
#include <sys/stat.h>

#include "../include/lib.h"
#include "../include/gcm.h"

#define _GNU_SOURCE
#include <getopt.h>

#define MKGBI_VERSION "V0.2-20060103"

const char *__progname;

char *apploader_bin;
char *opening_bnr;

#define DEFAULT_OPENING_BNR GCM_OPENING_BNR
#define DEFAULT_APPLOADER_BIN "apploader.bin"

/*
 *
 */
static void default_disk_header(struct gcm_disk_header *dh)
{
	memset(dh, 0, sizeof(*dh));

	memcpy(dh->info.game_code, "GBLP", 4);	/*Gamecube BootLoader PAL */
	memcpy(dh->info.maker_code, "GL", 2);	/* gc-linux */
	dh->info.magic = cpu_to_be32(0xc2339f3d);

	strcpy(dh->game_name, "GAMECUBE \"EL TORITO\" BOOTLOADER");

//      dh->debug_monitor_offset = cpu_to_be32(0x0001a7f4);
//      dh->debug_monitor_address = cpu_to_be32(0x80280060);
//      dh->layout.user_offset = cpu_to_be32(0x803ff900);
	dh->layout.user_size = cpu_to_be32(4*1024*1024); /* 4MB */

	dh->layout.disk_size = cpu_to_be32(0x56fe8000);	/* 1.4GB */
}

/*
 * 
 */
static void default_disk_header_info(struct gcm_disk_header_info *dhi)
{
	memset(dhi, 0, sizeof(*dhi));

	dhi->simulated_memory_size = cpu_to_be32(0x01800000);
	dhi->country_code = cpu_to_be32(1); /* 1=ntsc, 2=pal */
	dhi->unknown_1 = cpu_to_be32(1);
}

/*
 *
 */
static void default_apploader_header(struct gcm_apploader_header *ah)
{
	memset(ah, 0, sizeof(*ah));

	memcpy(ah->date, "2021/10/06", 10);
	ah->entry_point = 0x81200000;	/* gets proper endianness later */
}

/*
 *
 */
static void default_system_area(struct gcm_system_area *sa)
{
	memset(sa, 0, sizeof(*sa));

	default_disk_header(&sa->dh);
	default_disk_header_info(&sa->dhi);
	default_apploader_header(&sa->al_header);
}

/*
 *
 */
static void fixup_file_offsets(struct gcm_file_entry *fe, unsigned int num,
			       unsigned long offset)
{
	while (num-- > 0)
		fe->file.file_offset =
		    cpu_to_be32(fe->file.file_offset + offset);
}

/*
 *
 */
static int write_and_pad(int fd, const void *buf, size_t count, size_t align)
{
	static char pad_buf[DI_ALIGN + 1];
	int pad_len = (count & align);
	int bytes_written, result;

	if (pad_len)
		pad_len = (align + 1) - pad_len;

	result = write(fd, buf, count);
	if (result < 0)
		return result;
	bytes_written = result;

	result = write(fd, pad_buf, pad_len);
	if (result < 0)
		return result;
	bytes_written += result;

	return bytes_written;
}

/*
 *
 */
static int write_system_area(int fd, struct gcm_system_area *sa)
{
	struct gcm_file_entry *fe;
	uint32_t fst_offset;
	int result;
	int length;

	unsigned long written_length = 0;

	fst_offset = sa->dh.layout.fst_offset = sizeof(sa->dh) + 0x2000 +
	    sizeof(sa->al_header) + di_align_size(sa->al_size);

	/* disc header */
	sa->dh.layout.fst_size = sa->dh.layout.fst_max_size = sa->fst_size;

	sa->dh.layout.fst_offset = cpu_to_be32(sa->dh.layout.fst_offset);
	sa->dh.layout.fst_size = cpu_to_be32(sa->dh.layout.fst_size);
	sa->dh.layout.fst_max_size = cpu_to_be32(sa->dh.layout.fst_max_size);

	result = write(fd, &sa->dh, sizeof(sa->dh));
	if (result < 0)
		return result;

	written_length += sizeof(sa->dh);

	/* disc header information... */
	result = write(fd, &sa->dhi, sizeof(sa->dhi));
	if (result < 0)
		return result;
	/* ... with padding */
	result = pad_file(fd, 0x2000 - sizeof(sa->dhi));
	if (result < 0)
		return result;

	written_length += 0x2000;

	/* apploader */
	sa->al_header.size = sa->al_size;

	sa->al_header.entry_point = cpu_to_be32(sa->al_header.entry_point);
	sa->al_header.size = cpu_to_be32(sa->al_header.size);

	result = write(fd, &sa->al_header, sizeof(sa->al_header));
	if (result < 0)
		return result;
	result = write_and_pad(fd, sa->al_image, sa->al_size, DI_ALIGN);
	if (result < 0)
		return result;

	written_length += sizeof(sa->al_header) + result;

	/* fst */

	/* fixup file offsets */
	fe = (struct gcm_file_entry *)sa->fst_image;
	fixup_file_offsets(fe + 1, 1, fst_offset + di_align_size(sa->fst_size));

	result = write_and_pad(fd, sa->fst_image, sa->fst_size, DI_ALIGN);
	if (result < 0)
		return result;

	written_length += result;

	/* opening.bnr */
	result = write_and_pad(fd, sa->bnr_image, sa->bnr_size, DI_ALIGN);
	if (result < 0)
		return result;

	written_length += result;

	/* padding */
	result = pad_file(fd, SYSTEM_AREA_SIZE - written_length);
	if (result < 0)
		return result;

	return 0;
}

/*
 *
 */
static int build_single_file_fst(void **fst, uint32_t * fst_size,
				 char *fname, int flen)
{
	struct gcm_file_entry *fe;
	void *new_fst;

	int file_entry_table_size = 2 * sizeof(struct gcm_file_entry);
	int string_table_size = strlen(fname) + 1;
	int new_fst_size = file_entry_table_size + string_table_size;

	new_fst = malloc(new_fst_size);
	if (!new_fst)
		die("not enough memory for fst\n");

	/* zero out values */
	memset(new_fst, 0, new_fst_size);

	fe = new_fst;

	/* root directory */
	fe[0].flags = 1;	/* directory */
	fe[0].root_dir.num_entries = cpu_to_be32(2);

	/*
	 * The file_offset value needs to be fixed afterwards adding the
	 * final location of the file in the disc image.
	 */
	fe[1].file.fname_offset = 0;	/* in string table */
	fe[1].file.file_offset = 0;	/* needs fixup afterwards */
	fe[1].file.file_length = cpu_to_be32(flen);
	strcpy(new_fst + file_entry_table_size, fname);

	*fst = new_fst;
	*fst_size = new_fst_size;

	return 0;
}

/*
 *
 */
void version(void)
{
	printf("version %s\n", MKGBI_VERSION);
	exit(2);
}

/*
 *
 */
void usage(void)
{
	fprintf(stderr,
		"Usage: %s [OPTION] -o [OUTFILE]" "\n"
		"  -a, --apploader=FILE    use apploader from file"
		"      (default `apploader.bin')" "\n"
		"  -b, --banner=FILE       use banner from file" "\n"
		"      (default `openning.bnr')" "\n"
		"  -o, --outfile=PATH      output file (default stdout)" "\n",
		__progname);
	exit(1);
}

/*
 *
 */
int set_apploader(char *optarg)
{
	apploader_bin = optarg;
	return 0;
}

/*
 *
 */
int set_banner(char *optarg)
{
	opening_bnr = optarg;
	return 0;
}

/*
 *
 */
int main(int argc, char *argv[])
{
	char *outfile = NULL;
	FILE *fout;
	char *p;
	int ch;
	int result;

	struct gcm_system_area sa;
	void *fst;
	uint32_t fst_size;

	struct option long_options[] = {
		{"apploader", 1, NULL, 'a'},
		{"banner", 1, NULL, 'b'},
		{"outfile", 1, NULL, 'o'},
		{"version", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0}
	};
#define SHORT_OPTIONS "a:b:o:vh"

	p = strrchr(argv[0], '/');
	__progname = (p && p[1]) ? p + 1 : argv[0];

	default_system_area(&sa);

	while ((ch = getopt_long(argc, argv, SHORT_OPTIONS,
				 long_options, NULL)) != -1) {
		switch (ch) {
		case 'a':
			if (set_apploader(optarg) < 0)
				usage();
			break;
		case 'b':
			if (set_banner(optarg) < 0)
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

	if (argc - optind > 0) {
		usage();
	}

	if (!apploader_bin)
		apploader_bin = DEFAULT_APPLOADER_BIN;
	if (!opening_bnr)
		opening_bnr = DEFAULT_OPENING_BNR;

	sa.al_image = slurp_file(apploader_bin, &sa.al_size);

	sa.bnr_image = slurp_file(opening_bnr, &sa.bnr_size);
	build_single_file_fst(&fst, &fst_size, GCM_OPENING_BNR, sa.bnr_size);

	sa.fst_image = fst;
	sa.fst_size = fst_size;

	if (sizeof(sa.dh) + 0x2000 + sizeof(sa.al_header) +
	    di_align_size(sa.al_size) + di_align_size(sa.fst_size) +
	    di_align_size(sa.bnr_size) > SYSTEM_AREA_SIZE) {
		die("system area overflowed"
		    " (apploader size = %ld, fst size = %ld, banner size = %ld)\n",
		    sa.al_size + 0UL, sa.fst_size + 0UL, sa.bnr_size + 0UL);
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

	fflush(fout);
	write_system_area(fileno(fout), &sa);
	fclose(fout);

	free(fst);

	return 0;
}
