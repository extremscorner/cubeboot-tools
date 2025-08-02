/**
 * lib.c
 *
 * Miscelaneous functions.
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
#include <unistd.h>

#include "../include/lib.h"
#include "../include/gcm.h"

/*
 *
 */
void die(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fflush(stdout);
	fflush(stderr);
	exit(1);
}

/*
 *
 */
void *xmalloc(size_t size)
{
	void *buf;
	buf = malloc(size);
	if (!buf) {
		die("Cannot malloc %ld bytes: %s\n",
		    size + 0UL, strerror(errno));
	}
	return buf;
}

/*
 *
 */
void *xrealloc(void *ptr, size_t size)
{
	void *buf;
	buf = realloc(ptr, size);
	if (!buf) {
		die("Cannot realloc %ld bytes: %s\n",
		    size + 0UL, strerror(errno));
	}
	return buf;
}

/*
 *
 */
int pad_file(int fd, int size)
{
	static char filler[32];
	int len, chunk;
	int result;

	for (len = size; len > 0; len -= chunk) {
		chunk = (len > 32) ? 32 : len;
		result = write(fd, filler, chunk);
		if (result < 0)
			return result;
	}
	return 0;

}

/*
 *
 */
char *slurp_file(const char *filename, off_t * r_size)
{
	int fd;
	char *buf;
	off_t aligned_size, size, progress;
	ssize_t result;
	struct stat stats;

	if (!filename) {
		*r_size = 0;
		return 0;
	}
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		die("Cannot open `%s': %s\n", filename, strerror(errno));
	}
	result = fstat(fd, &stats);
	if (result < 0) {
		die("Cannot stat: %s: %s\n", filename, strerror(errno));
	}
	size = stats.st_size;
	//aligned_size = di_align_size(size);
	aligned_size = size;
	*r_size = aligned_size;
	buf = xmalloc(aligned_size);
	memset(buf, 0, aligned_size);
	progress = 0;
	while (progress < size) {
		result = read(fd, buf + progress, size - progress);
		if (result < 0) {
			if ((errno == EINTR) || (errno == EAGAIN))
				continue;
			die("read on %s of %ld bytes failed: %s\n",
			    filename, (size - progress) + 0UL, strerror(errno));
		}
		progress += result;
	}
	result = close(fd);
	if (result < 0) {
		die("Close of %s failed: %s\n", filename, strerror(errno));
	}
	return buf;
}

