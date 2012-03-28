/* drivers/input/touchscreen/zforce.c
 *
 * Copyright (C) 2010 Barnes & Noble, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_ZFORCE_H
#define _LINUX_ZFORCE_H

#define ZFORCE_NAME "zforce-ts"
#define zforce_info(f, a...)		pr_info("%s:" f,  __func__ , ## a)
#define zforce_error(f, a...)		pr_err("%s:" f,  __func__ , ## a)
#define zforce_alert(f, a...)		pr_alert("%s:" f,  __func__ , ## a)

//#define ZF_USE_DEBUG
#ifdef ZF_USE_DEBUG
	#define zforce_debug(f, a...)	pr_alert("%s:" f,  __func__ , ## a)
#else
	#define zforce_debug(f, a...)
#endif /* ZF_USE_DEBUG */

struct zforce_platform_data {
	u32 width;
	u32 height;
	uint32_t flags;
	unsigned long irqflags;
};

struct touch_info_data_t {
	u16	x;
	u16	y;
	u8	id;
	u8	state;
	u8 prblty;
	u8 valid;
	u8 rsvrd;
	u16 z;
};
#define ZF_NUM_FINGER_SUPPORT 2

#endif /* _LINUX_ZFORCE_H */
