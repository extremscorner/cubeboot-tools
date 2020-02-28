/*
 * udolrel.c
 *
 * Converts a zImage.dol into a self-relocatable lowmem .dol
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

/*
 * CAUTION:
 * This program is currently really dumb and lacks lots of checks.
 * Use it _ONLY_ if you know what you're doing.
 *
 * Currently, the relocation engine will work _only_ if the memory areas used
 * by the original and resulting DOLs do not overlap.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "../include/lib.h"

#include "../include/dol.h"
#include "../include/dolrel.h"

#define _GNU_SOURCE
#include <getopt.h>

#define UDOLREL_VERSION "V0.1-20060310"

const char *__progname;

const unsigned int max_nr_sections = DOL_SECT_MAX_TEXT + DOL_SECT_MAX_DATA;
struct dolrel_control control;
struct dolrel_section sections[DOL_SECT_MAX_TEXT + DOL_SECT_MAX_DATA];

void *reloc_code;
unsigned int reloc_code_size;
unsigned long reloc_flags;

#define DOL_ALIGN_SHIFT  5
#define DOL_ALIGN_SIZE   (1UL << DOL_ALIGN_SHIFT)
#define DOL_ALIGN_MASK   (~((1 << DOL_ALIGN_SHIFT) - 1))

#define dol_align(addr)  (void *) \
                        ((((unsigned long)(addr)) + \
                                 DOL_ALIGN_SIZE - 1) & DOL_ALIGN_MASK)

/**
 *
 */
void calc_section_sizes(struct dol_header *dh, uint32_t *largest, uint32_t *total)
{
	int k;
	uint32_t sect_size;

	*largest = 0;
	*total = 0;

	for( k = 0; k < max_nr_sections; k++) {
		sect_size = be32_to_cpu(dol_sect_size(dh, k));
		*total += sect_size;
		if (sect_size > *largest)
			*largest = sect_size;
	}
}

/**
 *
 */
int pad(FILE *f, size_t count)
{
	uint8_t data = 0xaa;
	size_t nr_items;

        while (count--) {
		nr_items = fwrite(&data, 1, 1, f);
		if (nr_items != 1)
			return -1;
	}
	return 0;
}

/**
 *
 */
int transform_dol(FILE *fout, FILE *fin)
{
	struct dol_header dol_header, *dol;
	struct dol_header new_dol_header, *new_dol;
	struct dolrel_section *reloc_entry;
	unsigned int nr_reloc_entries;
	uint32_t largest_sect_size, total_sects_size, code_size, len;
	uint32_t aligned_total_sects_size, aligned_code_size;
	void *sect_buf;
	unsigned int sects_bitmap;
	unsigned long load_address_code, load_address_data;
	unsigned long lowest_start;
	size_t nr_items;
	int i, j, k;
	int result;

	/* retrieve the original .dol header */
	dol = &dol_header;
	nr_items= fread(dol, sizeof(*dol), 1, fin);
	if (nr_items != 1) {
		die("can't read dol header: %s\n", strerror(errno));
	}

#if 0
	if (dol_check_header(dol)) {
		
	}
#endif

	/* pretty self explanatory */
	calc_section_sizes(dol, &largest_sect_size, &total_sects_size);
	aligned_total_sects_size = (uint32_t)dol_align(total_sects_size);

	/* allocate a buffer large enough for the largest section */
	sect_buf = malloc(largest_sect_size);
	if (!sect_buf) {
		die("can't allocate section buffer: %s\n", strerror(errno));
	}

	sects_bitmap = (1 << max_nr_sections) - 1;
	memset(sections, 0, sizeof(sections));
	reloc_entry = &sections[0];
	nr_reloc_entries = 0;

	/* calculate the final stub size */
	code_size = reloc_code_size + sizeof(control) + sizeof(sections);
	aligned_code_size = (uint32_t)dol_align(code_size);

	/*
	 * The resulting .dol will contain just a data and a text section.
	 */

	/* the relocation stub will be loaded at this address */
	load_address_code = 0x80003100;

	/* the original sections will be loaded right after the stub */
	load_address_data = load_address_code + aligned_code_size;

	/* this is the new .dol header */
	new_dol = &new_dol_header;
	memset(new_dol, 0, sizeof(*new_dol));

	/*
	 * The data section contains all original sections packed one after
	 * another.
	 */
	new_dol->address_text[0] = cpu_to_be32(load_address_data);
	new_dol->offset_text[0] = cpu_to_be32(sizeof(*new_dol));
	new_dol->size_text[0] = cpu_to_be32(aligned_total_sects_size);

	/*
	 * The text section contains the relocation stub and the control
	 * structures.
	 */
	new_dol->address_text[1] = cpu_to_be32(load_address_code);
	new_dol->offset_text[1] = cpu_to_be32(be32_to_cpu(new_dol->offset_text[0]) + be32_to_cpu(new_dol->size_text[0]));
	new_dol->size_text[1] = cpu_to_be32(aligned_code_size);

	/* we don't need a bss section here */
	new_dol->size_bss = 0;
	new_dol->address_bss = 0;

	/* our entry point becomes our relocation stub */
	new_dol->entry_point = cpu_to_be32(load_address_code);

	/* write the new .dol header */
	nr_items = fwrite(new_dol, sizeof(*new_dol), 1, fout);
	if (nr_items != 1) {
		die("can't write dol header: %s\n", strerror(errno));
	}

	/* write all sections into the new .dol data section */
	while(sects_bitmap) {
		lowest_start = 0xffffffff;
		for (j = -1, k = 0; k < max_nr_sections; k++) {
			/* continue if section is already done */
			if ((sects_bitmap & (1 << k)) == 0)
				continue;

			/* mark section as done if empty */
			if (be32_to_cpu(dol_sect_size(dol, k)) == 0) {
				sects_bitmap &= ~(1 << k);
				continue;
			}

			/* found new candidate */
			if (be32_to_cpu(dol_sect_address(dol, k)) < lowest_start) {
				lowest_start = be32_to_cpu(dol_sect_address(dol, k));
				j = k;
			}
		}

		/* mark section as being loaded */
		sects_bitmap &= ~(1 << j);

		result = fseek(fin, be32_to_cpu(dol_sect_offset(dol, j)),
			       SEEK_SET);
		if (result < 0) {
			die("can't seek to section: %s\n", strerror(errno));
		}

		len = be32_to_cpu(dol_sect_size(dol, j));
		nr_items = fread(sect_buf, len, 1, fin);
		if (nr_items != 1) {
			die("can't read section: %s\n", strerror(errno));
		}

		nr_items = fwrite(sect_buf, len, 1, fout);
		if (nr_items != 1) {
			die("can't write section: %s\n", strerror(errno));
		}

		reloc_entry->dst_address = (void *)dol_sect_address(dol, j);
		reloc_entry->length = cpu_to_be32(len);
		reloc_entry++;
		nr_reloc_entries++;
	}

	/* data section padding */
	result = pad(fout, aligned_total_sects_size - total_sects_size);
	if (result) {
		die("can't write data section padding: %s\n", strerror(errno));
	}
	
	/* write our stub into the new .dol code section */

	/* stub code */
	nr_items = fwrite(reloc_code, reloc_code_size, 1, fout);
	if (nr_items != 1) {
		die("can't write relocation code: %s\n", strerror(errno));
	}

	/* stub control header */
	control.version = cpu_to_be32(0xdead0001);

	control.flags = cpu_to_be32(reloc_flags);

	control.entry_point = (void *)dol->entry_point;
	control.address_bss = (void *)dol->address_bss;
	control.size_bss = dol->size_bss;
	control.src_address = (void *)cpu_to_be32(load_address_data);

	control.nr_sections = cpu_to_be32(nr_reloc_entries);

	nr_items = fwrite(&control, sizeof(control), 1, fout);
	if (nr_items != 1) {
		die("can't write relocation control: %s\n", strerror(errno));
	}

	/* stub relocation table */
	nr_items = fwrite(sections, sizeof(sections), 1, fout);
	if (nr_items != 1) {
		die("can't write relocation table: %s\n", strerror(errno));
	}

	/* code section padding */
	result = pad(fout, aligned_code_size - code_size);
	if (result) {
		die("can't write text section padding: %s\n", strerror(errno));
	}
}

/**
 *
 */
void version(void)
{
        printf("version %s\n", UDOLREL_VERSION);
        exit(2);
}

/**
 *
 */
void usage(void)
{
        fprintf(stderr,
                "Usage: %s [OPTION] [FILE] -o [OUTFILE]" "\n"
                "  -s, --stop-motor        stop dvd motor (default don't stop)"
						"\n"
                "  -x, --disable-xenogc    disable xenogc on startup"
						" (implies -s)" "\n"
                "  -r, --releng=PATH       relocation engine image"
						" (default sdre.bin)" "\n"
                "  -o, --outfile=PATH      output file (default stdout)" "\n"
                , __progname);
        exit(1);
}

/**
 *
 */
int main(int argc, char *argv[])
{
	char *outfile = NULL, *infile = NULL;
	FILE *fout, *fin;
	char *sdre_bin = "sdre.bin";
	void *sdre_image;
	off_t sdre_size;
        char *p;
	int ch;
	int result;

        struct option long_options[] = {
                {"stop-motor", 0, NULL, 's'},
                {"disable-xenogc", 0, NULL, 'x'},
                {"releng", 1, NULL, 'r'},
                {"outfile", 1, NULL, 'o'},
                {"version", 0, NULL, 'v'},
                {"help", 0, NULL, 'h'},
                {0,0,0,0}
        };
#define SHORT_OPTIONS "sxr:o:vh"

        p = strrchr(argv[0], '/');
        __progname = (p && p[1]) ? p+1 : argv[0];

	reloc_flags = 0;

       while((ch = getopt_long(argc, argv, SHORT_OPTIONS,
                                long_options, NULL)) != -1) {
                switch(ch) {
			case 's':
				reloc_flags |= DOLREL_FLAG_STOP_MOTOR;
				break;
 			case 'x':
				reloc_flags |= DOLREL_FLAG_DISABLE_XENOGC;
				break;
			case 'r':
				sdre_bin = optarg;
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

	sdre_image = slurp_file(sdre_bin, &sdre_size);

	reloc_code_size = sdre_size - sizeof(struct dolrel_control);
	reloc_code = sdre_image;

	transform_dol(fout, fin);

	fclose(fout);
	fclose(fin);
}

