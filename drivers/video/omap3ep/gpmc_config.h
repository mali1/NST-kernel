/*
 * GPMC Configuration for epaper display
 *
 *      Copyright (C) 2010 Vladimir Krushkin, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */
#ifndef __GPMC_CONFIG_H
#define __GPMC_CONFIG_H

#include <linux/io.h>
#include <linux/fb.h>

struct gpmc_sess {
	int		cs;
	unsigned long	phys_base;
	void __iomem	*cs_baseaddr;
	void __iomem	*baseaddr;
	unsigned long	size;
//	struct clk *gpmc_fck;
};

extern int gpmc_probe(struct gpmc_sess **sess);
extern void gpmc_remove(struct gpmc_sess **sess);
extern void gpmc_clock_request(struct fb_info *info);
extern void gpmc_clock_release(struct fb_info *info);

#endif
