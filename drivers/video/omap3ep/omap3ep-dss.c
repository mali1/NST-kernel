/*
 * Display Sub-System Support for the OMAP3 Electronic Paper Framebuffer
 *      Copyright (C) 2009 MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
  #include <asm/gpio.h>
#else
  #include <linux/gpio.h>
#endif

#include "omap3epfb.h"

#include "omap3ep-dss.h"
#include "halDSS.h"
#include "OMAP3430.h"
#include "omap3epfb-user.h"

#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
#define BORDER_CTRL0_GPIO	(92)
#define BORDER_CTRL1_GPIO	(93)
#endif

static uint32_t *dss_cmbuf;
static dma_addr_t dss_cmbuf_phys;
static size_t dss_cmbuf_size;

int dss_timing[NUMBER_DSS_TIMES] = {0};


/*
 * Generate the CLUT table for mapping of internal video memory 8bpp pixels
 * to the external data bus.
 */
static void omap3epdss_fill_cmbuf(uint32_t *buf, int lcd_db_order)
{
	unsigned int i;

	for (i=0; i<256; i++) {
		/* "unpack" pixels */
		const unsigned int p0 = (i >> 0) & 3;
		const unsigned int p1 = (i >> 2) & 3;
		const unsigned int p2 = (i >> 4) & 3;
		const unsigned int p3 = (i >> 6) & 3;

		/*
		 * DSS converts the 24-bit RGB888 CLUT entry to 12-bit RGB444
		 * in order to put it on the external LCD data bus.
		 * So the mapping goes as follows:
		 * 	---------------------------------------------------
		 * 	SRC | CLUT entry    |  RGB444 conv.  | LCD Data Bus
		 * 	---------------------------------------------------
		 * 	p0 -> RGB888.G[7:6] -> RGB444.G[3:2] -> LCD_D[7:6]
		 * 	p1 -> RGB888.G[5:4] -> RGB444.G[1:0] -> LCD_D[5:4]
		 * 	p2 -> RGB888.B[7:6] -> RGB444.B[3:2] -> LCD_D[3:2]
		 * 	p3 -> RGB888.B[5:4] -> RGB444.B[1:0] -> LCD_D[1:0]
		 * 	---------------------------------------------------
		 */
		if(lcd_db_order == LCD_DB_ORDER_NORMAL){
            buf[i] =  (p3 << 4)
                    | (p2 << 6)
                    | (p1 << 12)
                    | (p0 << 14);
		}else if (lcd_db_order == LCD_DB_ORDER_REVERSE)
		{
            buf[i] =  (p0 << 4)
                    | (p1 << 6)
                    | (p2 << 12)
                    | (p3 << 14);
		}else{
	        printk("LCD Data Bus order invalid \n");
		}

	}
	wmb();
}


int omap3epdss_alloc(struct fb_info *info, int lcd_db_order)
{
	int stat;

	dss_cmbuf_size = PAGE_ALIGN(4 * 256);
	dss_cmbuf = dma_alloc_writecombine(info->device, dss_cmbuf_size,
			&dss_cmbuf_phys, GFP_KERNEL);

	if (!dss_cmbuf) {
		pr_err("omap3epfb-dss: cannot allocate DSS color map\n");
		stat = -ENODEV;
		return stat;
	}
	omap3epdss_fill_cmbuf(dss_cmbuf, lcd_db_order);
	dev_dbg(info->device, "fake DSS color map @%08lx phys\n",
			(unsigned long)dss_cmbuf_phys);

	return 0;
}


static int omap3epdss_init(struct fb_info *info)
{
	int stat;

	stat = halDSS_Init();
	if (stat)
		return stat;

	return 0;
}


void omap3epdss_free(struct fb_info *info)
{
	dma_free_writecombine(info->device, dss_cmbuf_size, dss_cmbuf, dss_cmbuf_phys);
}


static void omap3epdss_cleanup(struct fb_info *info)
{
	halDSS_Reset();
	halDSS_Cleanup();
}

#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
static void omap3epdss_set_border_color(struct fb_info *info)
{
	struct omap3epfb_par *devpar = info->par;
	unsigned int val;

	val = devpar->epd_varpar.border_color;

	switch (val) {
		case EPD_BORDER_FLOATING:
			gpio_direction_output(BORDER_CTRL0_GPIO, 0);
			gpio_direction_output(BORDER_CTRL1_GPIO, 0);
			break;
		case EPD_BORDER_WHITE:
			gpio_direction_output(BORDER_CTRL0_GPIO, 0);
			gpio_direction_output(BORDER_CTRL1_GPIO, 1);
			break;
		case EPD_BORDER_BLACK:
			gpio_direction_output(BORDER_CTRL0_GPIO, 1);
			gpio_direction_output(BORDER_CTRL1_GPIO, 0);
			break;
		default:
			break;
	}
}
#endif

static void *dss_intparnsf;
static void *dss_intparfd;

void line1_isr(void)
{
	BUG_ON( halDSS_check_go() ); //we assume that LCDGO is 0 and we can change settings
	BUG_ON(!dss_intparnsf);
	omap3epfb_newsubframe_inthandler(dss_intparnsf);

}

void framedone_isr(void)
{
	BUG_ON(!dss_intparfd);
	omap3epfb_framedone_inthandler(dss_intparfd);
}


int omap3epdss_prepare_start(struct fb_info *info, unsigned int xres, unsigned int yres, unsigned int clk_khz, int panel_type)
{
	dss_intparnsf = info;
	dss_intparfd = info;

	BUG_ON(xres & 3);

	omap3epdss_init(info);

	/* TODO - DSS should not be reconfigured each time
	 * a new screen sequence is started.
	 */
	if(panel_type == OMAP3EPFB_PANELTYPE_AUO){
		halDSS_DISPC_Config(clk_khz, /* fill Display clock frequency (in MHz) */
							xres / 4, /* fill Display H resolution (in pixels) */
							yres + 1, /* fill Display V resolution (in lines) */
							12, /* fill Display data lines count (12, 16, 18 or 24) */
							1, /* fill Horizontal Back Porch (in pixel clocks) */
							201, /* fill Horizontal Front Porch (in pixel clocks) */
							31, /* fill HSYNC Pulse Width (in pixel clocks) */
							0, /* fill Vertical Back Porch (in HSYNCs) */
							0, /* fill Vertical Front Porch (in HSYNCs) */
							1, /* fill VSYNC Pulse Width (in HSYNCs) */
							1, /* Invert pixel clock? (TRUE / FALSE) */
							1, /* Invert HSYNC? (TRUE / FALSE) */
							1 /* Invert VSYNC? (TRUE / FALSE) */
		);
	} else {
		/* WARNING: Four pixels assumed on data bus per pixel clock tick! */
			halDSS_DISPC_Config(clk_khz, /* fill Display clock frequency (in MHz) */
								xres / 4, /* fill Display H resolution (in pixels) */
								dss_timing[10] + yres + 1, /* fill Display V resolution (in lines) */
								dss_timing[0], /* fill Display data lines count (12, 16, 18 or 24) */
								dss_timing[1], /* fill Horizontal Back Porch (in pixel clocks) */
								dss_timing[2], /* fill Horizontal Front Porch (in pixel clocks) */
								dss_timing[3], /* fill HSYNC Pulse Width (in pixel clocks) */
								dss_timing[4], /* fill Vertical Back Porch (in HSYNCs) */
								dss_timing[5], /* fill Vertical Front Porch (in HSYNCs) */
								dss_timing[6], /* fill VSYNC Pulse Width (in HSYNCs) */
								dss_timing[7], /* Invert pixel clock? (TRUE / FALSE) */
								dss_timing[8], /* Invert HSYNC? (TRUE / FALSE) */
								dss_timing[9] /* Invert VSYNC? (TRUE / FALSE) */
			);



	}
	GFX_Layer_Config(
			HALDSS_CONST_GFX_FORMAT_BITMAP8 << GFXFORMAT_BITPOS, /* Image format */
			0, /* fill Number of bytes to increment at the end of each row */
			xres/4, /* fill VID Layer H image size */
			yres + dss_timing[10], /* fill VID Layer V image size */
			0, /* fill VID Layer H position */
			0 /* fill VID Layer V position */
			);

	halDSS_DISPC_SetColorMap((u32*)dss_cmbuf_phys);

#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
	omap3epdss_set_border_color (info);
#endif

	return 0;
}

void omap3epdss_start(struct fb_info *info)
{
	GFX_Layer_enable();

	LCD_enable();
}

void omap3epdss_request_stop(void)
{
        LCD_disable();

#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
	/* Leave border floating */
	gpio_direction_output(BORDER_CTRL0_GPIO, 0);
	gpio_direction_output(BORDER_CTRL1_GPIO, 0);
#endif

       // halDSS_set_go();
}

void omap3epdss_stop(struct fb_info *info)
{
	omap3epdss_cleanup(info);
}


void omap3epdss_request_video_bufchange(dma_addr_t addr)
{
	GFX_Layer_Address_Set((u32*)addr);
	halDSS_set_go();
}

int omap3epdss_set_dss_timing(int  *pdsst)
{

	if(pdsst)
		memcpy(dss_timing,pdsst,sizeof(dss_timing));

	if (dss_timing[10] > MAX_PREOFFSET_LINES)
		return -EINVAL;

	return 0;
}
