/*
 * lib.h
 *
 * Miscelaneous functions.
 * Copyright (C) 2005 The GameCube Linux Team
 * Copyright (C) 2005 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __LIB_H
#define __LIB_H

#include <sys/types.h>

#include <endian.h>
#include <byteswap.h>

#if defined(__LITTLE_ENDIAN)
#define cpu_to_be32(val) bswap_32(val)
#define be32_to_cpu(val) bswap_32(val)
#define cpu_to_be16(val) bswap_16(val)
#define be16_to_cpu(val) bswap_16(val)
#else
#define cpu_to_be32(val) (val)
#define be32_to_cpu(val) (val)
#define cpu_to_be16(val) (val)
#define be16_to_cpu(val) (val)
#endif

void die(char *fmt, ...);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

int pad_file(int fd, int size);
char *slurp_file(const char *filename, off_t * r_size);

#endif /* __LIB_H */

