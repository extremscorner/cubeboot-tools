/*
 * gcn-con.h
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __GCN_CON_H
#define __GCN_CON_H

extern void gcn_con_init(void);
extern void gcn_con_exit(void);

extern void gcn_con_puts(const char *s);
extern void gcn_con_putc(char c);

#endif /* __GCN_CON_H */
