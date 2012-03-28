/*
 * Display Sub-System Support for the OMAP3 Electronic Paper Framebuffer 
 *      Copyright (C) 2009 MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#ifndef OMAP3EP_DSS_H
#define OMAP3EP_DSS_H

#include <linux/dma-mapping.h>

#define OMAP3EPFB_SUBFRAME_DURATION_MS	20u

#define LCD_DB_ORDER_NORMAL     0
#define LCD_DB_ORDER_REVERSE    1

/*
 * 2 lines are what is currently needed. Keep the number low to use
 * less memory. This is not the current value but the maximum allowed.
 * The actual value is set by OMAP3EPFB_IO_SET_DSS_TIMING ioctl and
 * should be less than this value
 */
#define MAX_PREOFFSET_LINES	(2)

/* TODO - this HAL interface needs some serious refactoring */
struct fb_info;

extern void omap3epfb_newsubframe_inthandler(void *);
extern void omap3epfb_framedone_inthandler(void *);

extern int omap3epdss_alloc(struct fb_info *info, int lcd_db_order);
extern void omap3epdss_free(struct fb_info *info);

extern int omap3epdss_prepare_start(struct fb_info *info, unsigned int xres, unsigned int yres, unsigned int clk_khz, int panel_type);
extern void omap3epdss_start(struct fb_info *info);
extern void omap3epdss_request_stop(void);
extern void omap3epdss_stop(struct fb_info *info);


extern void omap3epdss_request_video_bufchange(dma_addr_t addr);

#define NUMBER_DSS_TIMES 11
extern int omap3epdss_set_dss_timing(int  *pdsst);
extern int dss_timing[NUMBER_DSS_TIMES];
#endif	/* OMAP3EP_DSS_H */

