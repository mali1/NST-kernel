/*
 * OMAP3 Electronic Paper Framebuffer driver.
 *
 * Based on linux/drivers/video/vfb.c
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, Vladimir Krushkin, MM Solutions
 *      Portions Copyright (c) 2010 Barnes & Noble
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 *  TODO/Ideas:
 *  	- Currently there is a race condition bug in DSS code. If module
 *  	  is unloaded and immediately loaded again, the new module instance
 *  	  might start using DSS code before the previous instance has
 *  	  cleaned up (framebuffer has already been unregistered).
 *  	  As a temporary fix, just delay new module load sufficiently to
 *  	  let DSS be cleaned up. As a proper fix, DSS must be treated
 *  	  as a resource.
 *
 *  	- Add select/poll support for this driver to augment the update
 *  	  area polling IOCTL calls.
 *
 *  	- Investigate whether the current mb() calls in the subframe
 *  	  queue/dequeue code are enough to ensure race-condition-free
 *  	  execution in SMP environment.
 *
 *  	- The current usage of dma_alloc_writecombine() for allocating
 *  	  megabytes of memory IS INSANE! In future add a platform_device
 *  	  implementation in the BSP code. This way we could allocate as
 *  	  large contiguous memory buffer as we like, and pass it to the
 *  	  driver via a platform resource (see how other drivers pass the
 *  	  address of internal SRAM when configured so).
 *
 *  	  Don't forget to test both for CONFIG_OMAP3EP and
 *  	  CONFIG_OMAP3EP_MODULE (or sth) so that omap3epfb module will be
 *  	  able to see that resource. Reference counting might be in order,
 *  	  too.
 *
 *  	  ***UPDATE***: Is the current "hack" (as stolen from omapfb)
 *  	  sufficient?
 *
 *  	- When are we going to initialize DSS and the associated clocks?
 *  	  If DSS startup is in the order of 10ms we should do start/stop the
 *  	  DSS with each page update. Otherwise we should devise a scheme with
 *  	  a timeout after which to stop DSS, similar to the deferred I/O.
 *
 *  	- Request the DSS base address from the platform and resource
 *  	  structures. Don't forget to io-remap it.
 *
 *  	- Request driver configuration (BPP, XRES/YRES) from the platform
 *  	  device structure.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include "omap3epfb.h"
#include "omap3epfb-user.h"
#include "subfq.h"
#include "pmic.h"
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
#include "omap3ep-dss.h"
#endif
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
#include "gpmc_config.h"
#endif
static int deferred_io_modparam = -1;
module_param_named(deferred_io, deferred_io_modparam, int, S_IRUGO);
MODULE_PARM_DESC(deferred_io, "Set deferred I/O timeout in milliseconds");

static int omap3epfb_vcom_modparam = -1220;
module_param_named(vcom, omap3epfb_vcom_modparam, int, S_IRUGO);
MODULE_PARM_DESC(vcom, "Set static VCOM voltage in millivolts");

static char *omap3epfb_pmic_modparam = "tps65180-1p1-i2c";
module_param_named(pmic, omap3epfb_pmic_modparam, charp, S_IRUGO);
MODULE_PARM_DESC(pmic, "Set panel Power Management IC to use");

/*
 * How many milliseconds to wait after pmic_release_powerup_req()
 * before actually powering down the epaper display.
 */
static uint omap3epfb_pmic_dwell_modparam = PMIC_DEFAULT_DWELL_TIME_MS;
module_param_named(pmic_dwell, omap3epfb_pmic_dwell_modparam, uint, S_IRUGO);
MODULE_PARM_DESC(pmic_dwell, "Set PMIC power down dwell time in milliseconds");

/*
 * How many milliseconds to wait after pmic_release_powerup_req()
 * before vcom off "call pmic_vcom_switch(...,false)"
 */
static uint omap3epfb_pmic_vcomoff_modparam = PMIC_DEFAULT_VCOMOFF_TIME_MS;
module_param_named(pmic_vcomoff, omap3epfb_pmic_vcomoff_modparam, uint, S_IRUGO);
MODULE_PARM_DESC(pmic_vcomoff, "Set PMIC vcom off time in milliseconds");

static char *omap3epfb_pmicopt_modparam = "";
module_param_named(pmicopt, omap3epfb_pmicopt_modparam, charp, S_IRUGO);
MODULE_PARM_DESC(pmicopt, "Set panel Power Management IC custom options");

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
/*
 * Mode parameters:
 * X	- Panel X resolution
 * Y	- Panel Y resolution
 * BPP	- What pixel video format to present to user space:
 * 		0  - Packed 4bpp
 * 		4  - Byte-aligned 4bpp
 * 		8  - 8bit grayscale
 * 		16 - RGB565
 * F	- Pixel clock, in MHz. Please see user manual for calculating this.
 * ROT	- Rotation, in degrees. If 0 then panel and video resolutions match.
 * 	  If 90 then video_xres=panel_yres and video_yres=panel_xres.
 *      - Rotations 180 and 270 supports also.
 * PT   - Panel Type
 *              0 - Eink
 *              1 - AUO
 */

static char *omap3epfb_mode_modparam = "800x600x16x9x0x0";
module_param_named(mode, omap3epfb_mode_modparam, charp, S_IRUGO);
MODULE_PARM_DESC(mode, "Panel mode of operation (<X>x<Y>x<BPP>x<F>[x<ROT>)][x<PT>]");
#else
/*
 * Mode parameters:
 * X	- Panel X resolution
 * Y	- Panel Y resolution
 * BPP	- What pixel video format to present to user space:
 * 		0  - Packed 4bpp
 * 		4  - Byte-aligned 4bpp
 * 		8  - 8bit grayscale
 * 		16 - RGB565
 * ROT	- Rotation, in degrees. If 0 then panel and video resolutions match.
 * 	  If 90 then video_xres=panel_yres and video_yres=panel_xres.
 *      - Rotations 180 and 270 supports also.
 */
static char *omap3epfb_mode_modparam = "800x600x16x0";
module_param_named(mode, omap3epfb_mode_modparam, charp, S_IRUGO);
MODULE_PARM_DESC(mode, "Panel mode of operation (<X>x<Y>x<BPP>[x<ROT>)]");
#endif

/* ------------------------------------------------------------------------- */

#define OMAP3EPFB_REQQ_ITEM_IMASK	(OMAP3EPFB_REQQ_DEPTH-1u)

static ssize_t daemonconn_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct fb_info *info = dev_get_drvdata(dev);
	int st;
	char *tbuf = buf;

	/*
	 * Fake, non-intrusive, pop request that we use to block the
	 * calling process until some request data arrives.
	 */
	st = omap3epfb_reqq_pop_front_sync(info, 0);

	/*
	 * This read is used only for blocking so it doesn't matter what
	 * is printed.
	 */
	strncpy(tbuf, "42\n", PAGE_SIZE);
	tbuf += 3;

	return tbuf - buf;
}

static ssize_t emptysync_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct fb_info *info = dev_get_drvdata(dev);
    struct omap3epfb_par *devpar = info->par;
	int val;

	val =  devpar->epd_varpar.emptysync;

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t emptysync_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct fb_info *info = dev_get_drvdata(dev);
    struct omap3epfb_par *devpar = info->par;
	int val;

	val = simple_strtoul(buf, NULL, 10);
	devpar->epd_varpar.emptysync = val;

	return size;
}


#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
static ssize_t border_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct fb_info *info = dev_get_drvdata(dev);
    struct omap3epfb_par *devpar = info->par;
	int val;

	val =  devpar->epd_varpar.border_color;
 
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t border_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct fb_info *info = dev_get_drvdata(dev);
    struct omap3epfb_par *devpar = info->par;
	int val;

	val = simple_strtoul(buf, NULL, 10);

	switch (val) {
		case EPD_BORDER_WHITE:
		case EPD_BORDER_BLACK:
			devpar->epd_varpar.border_color = val;
			break;
		default:
			devpar->epd_varpar.border_color = EPD_BORDER_FLOATING;
			break;
	}
	return size;
}

static DEVICE_ATTR(border, S_IRUGO|S_IWUSR, border_show, border_store);
#endif

static DEVICE_ATTR(daemonconn, S_IRUGO, daemonconn_show, NULL);
static DEVICE_ATTR(emptysync, S_IRUGO|S_IWUSR, emptysync_show, emptysync_store);

static struct device_attribute *epd_sysfs_attrs[] = {
	&dev_attr_daemonconn,
#ifdef CONFIG_FB_OMAP3EP_MANAGE_BORDER
	&dev_attr_border,
#endif
	&dev_attr_emptysync,
	NULL
};

int omap3epfb_send_recovery_signal()
{
	struct task_struct *p;
	bool found = false;
	int st = -1;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if ( strcmp(p->comm, "omap-edpd.elf") == 0) {
			found = true;
			break;
		}
	}
	read_unlock(&tasklist_lock);

	if (found) {
		/* send recovery signal */
		struct siginfo sig_info;
		memset(&sig_info, 0, sizeof(struct siginfo));
		sig_info.si_signo = SIGUSR1;
		sig_info.si_code = SI_KERNEL;
		st = send_sig_info(SIGUSR1, &sig_info, p);
		pr_info("Sent sleep-awake signal to daemon:%d!\n", st);
		if (st < 0)
			printk("Error sending sleep-awake signal to daemon\n");
	}

	return st;
}

int omap3epfb_reqq_push_back_async(struct fb_info *info,
					const struct omap3epfb_update_area *a)
{
	struct omap3epfb_par *par = info->par;
	int st = 0;
	static time_t prev_nobufs_time = -1;
	long temp_time;	/* ms */

	BUG_ON(!a);

	spin_lock(&par->reqq.xi_lock);
	if ((par->reqq.wi - par->reqq.ri) == OMAP3EPFB_REQQ_DEPTH) {
		st = -ENOBUFS;
		temp_time = CURRENT_TIME.tv_sec * 1000 + CURRENT_TIME.tv_nsec/1000000;
		if (prev_nobufs_time != -1) {
			if (temp_time - prev_nobufs_time >= FULL_QUEUE_PERIOD
					|| par->num_missed_subframes >= SLEEPAWAKE_MISSED_THRESHOLD) {
				pr_info("Not removed request from reqq from daemon for %ldms\n ",
					temp_time - prev_nobufs_time);
				omap3epfb_send_recovery_signal();
				prev_nobufs_time = temp_time;
			}
		} else
			prev_nobufs_time = temp_time;
	} else {
		prev_nobufs_time = -1;
		par->reqq.bufs[par->reqq.wi++ & OMAP3EPFB_REQQ_ITEM_IMASK] = *a;

		if(par->reqq.bufs[(par->reqq.wi-1) & OMAP3EPFB_REQQ_ITEM_IMASK].wvfid == DSP_PM_AWAKE)
			par->reqq.cmd_in_queue++;
	}

	spin_unlock(&par->reqq.xi_lock);

	/*
	 * Use interruptible wait queue. Otherwise kernel core would
	 * complain that our process has hanged.
	 */
	wake_up_interruptible(&par->reqq.waitreq);

	return st;
}

int omap3epfb_reqq_pop_front_async(struct fb_info *info,
					struct omap3epfb_update_area *a)
{
	struct omap3epfb_par *par = info->par;
	int st = 0;

	spin_lock(&par->reqq.xi_lock);
	if (par->reqq.wi == par->reqq.ri)
		st = -EAGAIN;
	else if (a) {
		*a = par->reqq.bufs[par->reqq.ri++ & OMAP3EPFB_REQQ_ITEM_IMASK];

		if(par->reqq.bufs[(par->reqq.ri-1) & OMAP3EPFB_REQQ_ITEM_IMASK].wvfid == DSP_PM_AWAKE)
			par->reqq.cmd_in_queue--;
	}

	spin_unlock(&par->reqq.xi_lock);

	return st;
}

int omap3epfb_reqq_purge(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	spin_lock(&par->reqq.xi_lock);
	if(!par->reqq.cmd_in_queue)
		par->reqq.ri = par->reqq.wi;
	spin_unlock(&par->reqq.xi_lock);

	return 0;
}

int omap3epfb_reqq_pop_front_sync(struct fb_info *info,
					struct omap3epfb_update_area *a)
{
	struct omap3epfb_par *par = info->par;
	int st, wst;

	wst = wait_event_interruptible(par->reqq.waitreq,
		(st = omap3epfb_reqq_pop_front_async(info, a)) != -EAGAIN);

	return wst ? -EAGAIN : st;
}

/* ------------------------------------------------------------------------- */
static void omap3epfb_reg_daemon(struct fb_info *info,bool r)
{
	struct omap3epfb_par *par = info->par;
	int t;
	if(r){
		if (pmic_get_temperature(par->pwr_sess, &t) == 0){
			par->subfq.shrd.p->rq.temperature = t;
		}
	}
	par->reg_daemon = r;
}

static void omap3epfb_set_loaded_daemon_st(struct fb_info *info, bool loaded)
{
	struct omap3epfb_par *par = info->par;
	par->loaded_daemon = loaded;
}

static bool omap3epfb_get_loaded_daemon_st(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	return par->loaded_daemon;
}

static void omap3epfb_tracking_timer(unsigned long arg)
{
}

static void omap3epfb_update_area_retry(struct fb_info *info,
				unsigned int x, unsigned int y,
				unsigned int w, unsigned int h)
{
	struct omap3epfb_update_area area = {
		.x0	= x,
		.x1	= x + w - 1,
		.y0	= y,
		.y1	= y + h - 1,
		.wvfid	= OMAP3EPFB_WVFID_AUTO,
	};
	int stat;

	dev_dbg(info->device, "%s: %ux%u/%ux%u\n", __func__, x, y, w, h);
	do {

        stat = omap3epfb_update_area(info, &area);
		if (stat)
			msleep(50);
	} while ((stat == -ENOBUFS) || (stat == -EAGAIN));

	if (stat)
		dev_err(info->device, "update_area_retry stat=%d\n", stat);
}


static void omap3epfb_fillrect(struct fb_info *info,
				const struct fb_fillrect *rect)
{
	dev_dbg(info->device, "%s\n", __func__);
	sys_fillrect(info, rect);

	omap3epfb_update_area_retry(info, rect->dx, rect->dy, rect->width,
								rect->height);
}

static void omap3epfb_copyarea(struct fb_info *info,
				const struct fb_copyarea *area)
{
	dev_dbg(info->device, "%s\n", __func__);
	sys_copyarea(info, area);

	omap3epfb_update_area_retry(info, area->dx, area->dy, area->width,
								area->height);
}

static void omap3epfb_imageblit(struct fb_info *info,
				const struct fb_image *image)
{
	dev_dbg(info->device, "%s\n", __func__);
	sys_imageblit(info, image);

	omap3epfb_update_area_retry(info, image->dx, image->dy, image->width,
								image->height);
}


/* Code copied from boardsheetfb.c */
static ssize_t omap3epfb_write(struct fb_info *info, const char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	void *dst;
	int err = 0;
	unsigned long total_size;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	dst = (void *)(info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		err = -EFAULT;

	if  (!err)
		*ppos += count;

	omap3epfb_update_screen(info, OMAP3EPFB_WVFID_AUTO, true);

	return (err) ? err : count;
}


/* Ensure XRES/YRES/BPP are constant, and ignore the video timing fluff. */
static int omap3epfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	if (var->xres != par->mode.vxres)
		return -EINVAL;
	if (var->yres != par->mode.vyres)
		return -EINVAL;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = par->mode.vyres;

	if (par->mode.bpp == 0) {
		/* 4bpp, packed */
		if (var->bits_per_pixel != 4)
			var->bits_per_pixel = 4;
		if (!var->grayscale)
			var->grayscale = 1;

		/* GRAY16, packed */
		var->red.offset = 0;
		var->red.length = 4;
		var->green.offset = 0;
		var->green.length = 4;
		var->blue.offset = 0;
		var->blue.length = 4;
		var->transp.offset = 0;
		var->transp.length = 0;

		var->red.msb_right = 0;
		var->green.msb_right = 0;
		var->blue.msb_right = 0;
		var->transp.msb_right = 0;
	} else if (par->mode.bpp == 4) {
		/* 4bpp, byte-aligned */
		if (var->bits_per_pixel != 8)
			var->bits_per_pixel = 8;
		if (!var->grayscale)
			var->grayscale = 1;

		/* GRAY16 */
		var->red.offset = 0;
		var->red.length = 4;
		var->green.offset = 0;
		var->green.length = 4;
		var->blue.offset = 0;
		var->blue.length = 4;
		var->transp.offset = 0;
		var->transp.length = 0;

		var->red.msb_right = 0;
		var->green.msb_right = 0;
		var->blue.msb_right = 0;
		var->transp.msb_right = 0;
	} else if (par->mode.bpp == 8) {
		/* 8 bits per pixel */
		if (var->bits_per_pixel != 8)
			var->bits_per_pixel = 8;
		if (!var->grayscale)
			var->grayscale = 1;

		/* GRAY256 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 0;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;

		var->red.msb_right = 0;
		var->green.msb_right = 0;
		var->blue.msb_right = 0;
		var->transp.msb_right = 0;
	} else if (par->mode.bpp == 16) {
		/* RGB565 */
		if (var->bits_per_pixel != 16)
			var->bits_per_pixel = 16;
		if (var->grayscale)
			var->grayscale = 0;

		/* RGB 565 */
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;

		var->red.msb_right = 0;
		var->green.msb_right = 0;
		var->blue.msb_right = 0;
		var->transp.msb_right = 0;
	}

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;
	if (var->xoffset)
		var->xoffset = 0;
	if (var->yres_virtual > (2 * var->yres))
		var->yres_virtual = 2 * par->mode.vyres;

	/* support only page flipping and no pixel-position panning */
	if ((var->yoffset != 0) && (var->yoffset != par->mode.vyres))
		var->yoffset = 0;

	return 0;
}

static int omap3epfb_set_par(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	/* No parameters to really set, just check whether we have to
	 * force an update if user has requested page flipping.
	 *
	 * Note that this scheme has the following prerequisites:
	 * 	- Deferred I/O must be turned off.
	 * 	- The whole screen is updated with each page flip.
	 */
	if (par->last_yoffset != info->var.yoffset) {
		par->last_yoffset = info->var.yoffset;

		if (par->pgflip_refresh)
		    omap3epfb_update_screen_bursty(info);
	}

	return 0;
}


static inline unsigned int chan_to_field(unsigned int chan, const struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

/**
 * The framebuffer console (fbcon) requires a pseudo palette even for
 * TRUECOLOR drivers. Apart from that, the color map should never be used in
 * TRUECOLOR mode.
 */
static int omap3epfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int transp, struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	unsigned int val = 0;
	uint32_t *pal;
	int ret = 1;

	if (info->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green
				+ 7471 * blue) >> 16;

	switch (info->fix.visual) {
		case FB_VISUAL_STATIC_PSEUDOCOLOR:
			if ((par->mode.bpp == 8) && (regno < 256)) {
				BUG_ON(info->var.red.length != 8);

				/* use the gamma correction table
				 * as a CLUT if some client demands one */
				//TODO - add support for CLUT!
				ret = 0;
			}
			break;
		case FB_VISUAL_TRUECOLOR:
			if (regno < 64) {
				pal = info->pseudo_palette;

				val  = chan_to_field(red, &info->var.red);
				val |= chan_to_field(green, &info->var.green);
				val |= chan_to_field(blue, &info->var.blue);

				pal[regno] = val;
				ret = 0;
			}
			break;
	}

#if 0
	dev_dbg(info->device, "regno=%d, r=%d, g=%d, b=%d, t=%d, val=%08x\n",
				regno, red, green, blue, transp, val);
#endif

	return ret;
}


static int omap3epfb_blank(int blank_mode, struct fb_info *info)
{
	/* nothing to do - we should be in power down mode between
	 * screen updates */
	return 0;
}

/* ------------------------------------------------------------------------- */


static int omap3epfb_sync(struct fb_info *info)
{
    struct omap3epfb_par *devpar = info->par;

	dev_dbg(info->device, "omap3epfb_sync()\n");
	if (devpar->epd_varpar.emptysync == 0)
		return omap3epfb_update_screen(info, OMAP3EPFB_WVFID_AUTO, true);

	return 0;
}

static int omap3epfb_get_buffaddrs(struct fb_info *info, struct omap3epfb_buffaddrs *sfba)
{
	struct omap3epfb_par *par = info->par;

	memcpy(&sfba->bfbmem,&par->bfb.mem,sizeof(struct omap3epfb_buffer));
	memcpy(&sfba->bfbquemem,&par->bfb.que.mem,sizeof(struct omap3epfb_buffer));

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	memcpy(sfba->subf,par->subfq.bufs,sizeof(struct omap3epfb_subfbuf)*OMAP3EPFB_SUBFQ_DEPTH);
#else
	memset(sfba->subf,0,sizeof(struct omap3epfb_subfbuf)*OMAP3EPFB_SUBFQ_DEPTH);
	sfba->shmem.gpmc.phys =  par->gpmc->phys_base;
	sfba->shmem.gpmc.p = NULL;
#endif
	sfba->shmem.rd.phys = par->subfq.shrd.phys;
	sfba->shmem.rd.p = NULL;
	sfba->shmem.wr.phys = par->subfq.shwr.phys;
	sfba->shmem.wr.p = NULL;
	sfba->shmem.stats.phys = par->subfq.shstats.phys;
	sfba->shmem.stats.p = NULL;
	sfba->shmem.size = OMAP3EPFB_SHARED_MEM_SIZE;
	
	return 0;
}

extern int user_update(struct fb_info *info, struct omap3epfb_update_area *p);
extern int user_init_device(struct fb_info *fb_info);
extern void user_cleanup_device(struct fb_info *fb_info);
extern int omap3epfb_set_region(struct fb_info *info, struct omap3epfb_area *p);
extern int omap3epfb_get_region(struct fb_info *info, struct omap3epfb_area *p);
extern int omap3epfb_reset_region(struct fb_info *info, int mask);
extern int omap3epfb_fill_region(struct fb_info *info, struct omap3epfb_area *p);

static int omap3epfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct omap3epfb_par *par = info->par;
	union {
		struct omap3epfb_update_area update_area;
		struct omap3epfb_epd_fixpar epd_fixpar;
		struct omap3epfb_epd_varpar epd_varpar;
		struct omap3epfb_statistics stats;
		struct omap3epfb_buffaddrs buff_addrs;
		struct omap3epfb_buffer bfbmem;
		struct omap3epfb_bfb_update_area bfb_update_area;
		struct omap3epfb_area region;
//		int enable;
		int version;
//		unsigned int updateid;
		int hwstate;
		int last;
		int index;
		int wvfid;
		int temp;
		int daemon_status;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		int dsst[NUMBER_DSS_TIMES];
#endif
	} p;
	int stat = 0;

	/* avoid sending random data to user space */
	memset(&p, 0, sizeof(p));

	switch (cmd) {
		case OMAP3EPFB_IO_GET_API_VERSION:
			p.version = OMAP3EPFB_IOCTL_API_VERSION;
			if (put_user(p.version, (int __user *)arg))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_REQ_SCREEN_CLEAR:
		dev_err(info->device, "OMAP3EPFB_IO_REQ_SCREEN_CLEAR not supported now\n");
#if 0
///#TODO
			stat = omap3epfb_update_screen(info,
						OMAP3EPFB_WVFID_INITCLEAR,
						false);
#endif
			break;

		case OMAP3EPFB_IO_REQ_SCREEN_UPDATE:
			stat = omap3epfb_update_screen(info, arg, false);
			break;

		case OMAP3EPFB_IO_REQ_AREA_UPDATE:

			if (copy_from_user(&p.update_area, (void __user *)arg,
					sizeof(struct omap3epfb_update_area)))
				stat = -EFAULT;
			else {
				if (!user_update(info, &p.update_area))
					break;
				stat = omap3epfb_update_area(info,
							&p.update_area);
				if (copy_to_user((void __user *)arg,
							&p.update_area,
							sizeof(struct omap3epfb_update_area)))
					stat = -EFAULT;
			}
			break;

		case OMAP3EPFB_IO_POLL_SCREEN_ACTIVE:
			if (omap3epfb_screen_is_flipping(info))
				stat = -EAGAIN;
			else
				stat = 0;
			break;

		case OMAP3EPFB_IO_GET_EPDFIXPAR:
			omap3epfb_get_epd_fixpar(info, &p.epd_fixpar);
			if (copy_to_user((void __user *)arg, &p.epd_fixpar,
					sizeof(struct omap3epfb_epd_fixpar)))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_GET_EPDVARPAR:
			omap3epfb_get_epd_varpar(info, &p.epd_varpar);
			if (copy_to_user((void __user *)arg, &p.epd_varpar,
					sizeof(struct omap3epfb_epd_varpar)))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_SET_EPDVARPAR:
			if (copy_from_user(&p.epd_varpar, (void __user *)arg,
					sizeof(struct omap3epfb_epd_varpar)))
				stat = -EFAULT;
			else
				stat = omap3epfb_set_epd_varpar(info,
							&p.epd_varpar);
			break;

		case OMAP3EPFB_IO_GET_HWSTATE:
			p.hwstate = par->hwstate;
			if (put_user(p.hwstate, (int __user *)arg))
				stat = -EFAULT;
			break;
			
		case OMAP3EPFB_IO_GET_STATS:
			stat = omap3epfb_get_statistics(info, &p.stats);
			if (copy_to_user((void __user *)arg, &p.stats,
					sizeof(struct omap3epfb_statistics)))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_RESET_STATS:
			stat = omap3epfb_reset_statistics(info);
			break;

		case OMAP3EPFB_IO_BFB_GET_ADDRS:
			memcpy(&p.bfbmem,&par->bfb.mem,sizeof(struct omap3epfb_buffer));

			if (copy_to_user((void __user *)arg, &p.bfbmem,
					sizeof(struct omap3epfb_buffer)))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_BFB_AREA_UPDATE:
			if (copy_from_user(&p.bfb_update_area, (void __user *)arg,
					sizeof(struct omap3epfb_bfb_update_area)))
				stat = -EFAULT;
			else {
				stat = omap3epfb_bfb_update_area(info,
							&p.bfb_update_area);
			}
			break;


		case OMAP3EPFB_IO_GET_UPDATE_AREA:
			stat = omap3epfb_reqq_pop_front_async(info,
							&p.update_area);
			if (stat)
				memset(&p.update_area, 0,
					sizeof(struct omap3epfb_update_area));

			if (copy_to_user((void __user *)arg,
					&p.update_area,
					sizeof(struct omap3epfb_update_area)))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_START_UPDATE:
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
			stat = omap3epfb_run_screen_sequence(info);
#elif defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
			stat =  omap3epfb_pmic_powerup(info);
#endif
			break;

		case OMAP3EPFB_IO_DEQUEUE_SUBF:
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
			p.index = subf_producer_get(par);
			if (put_user(p.index, (int __user *)arg))
				stat = -EFAULT;
#else
			stat = -ENOSYS;
#endif
			break;

		case OMAP3EPFB_IO_QUEUE_SUBF:
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
			if (get_user(p.last, (int __user*)arg)){
				stat = -EFAULT;
			} else {
				subf_producer_queue(par,p.last);
			}
#else
			stat = -ENOSYS;
#endif
			break;

		case OMAP3EPFB_IO_GET_BUFFADDRS:
			stat = omap3epfb_get_buffaddrs(info, &p.buff_addrs);
			if (copy_to_user((void __user *)arg, &p.buff_addrs,
				sizeof(struct omap3epfb_buffaddrs)))
				stat = -EFAULT;

			break;

		case OMAP3EPFB_IO_GET_TEMPERATURE:
			stat = pmic_get_temperature(par->pwr_sess, &p.temp);
			if(!stat){
				if (put_user(p.temp, (int __user *)arg))
					stat = -EFAULT;
			}
			break;

		case OMAP3EPFB_IO_REGISTER_DEMON:
			omap3epfb_reg_daemon(info,true);
			break;
			
		case OMAP3EPFB_IO_UNREGISTER_DEMON:
			omap3epfb_reg_daemon(info,false);
			break;

		case OMAP3EPFB_IO_SET_READY_DEMON:
			omap3epfb_set_loaded_daemon_st(info, true);
			break;

		case OMAP3EPFB_IO_CHECK_READY_DEMON:
			p.daemon_status = omap3epfb_get_loaded_daemon_st(info);
			if (put_user(p.daemon_status, (int __user *) arg))
				stat = -EFAULT;
			break;

		case OMAP3EPFB_IO_SET_DSS_TIMING:
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
			if (copy_from_user(p.dsst, (int __user *)arg,
					sizeof(p.dsst)))
				stat = -EFAULT;
			else
				stat = omap3epdss_set_dss_timing(p.dsst);
			break;
#else
			stat = -ENOSYS;
#endif
		case OMAP3EPFB_IO_SET_ROTATE:
			stat = omap3epfb_set_rotate(info, (int __user)arg);
			break;

		case OMAP3EPFB_IO_SET_REGION:
			if (copy_from_user(&p.region, (void __user *)arg, sizeof(struct omap3epfb_area)))
				stat = -EFAULT;
			else 
				stat = omap3epfb_set_region(info, &p.region);
			break;

		case OMAP3EPFB_IO_GET_REGION:
			if (copy_from_user(&p.region, (void __user *)arg, sizeof(struct omap3epfb_area)))
				stat = -EFAULT;
			else {
				stat = omap3epfb_get_region(info, &p.region);
				if (stat)
					break;

				if (copy_to_user((void __user *)arg, &p.region, sizeof(struct omap3epfb_area)))
					stat = -EFAULT;
			}
			break;

		case OMAP3EPFB_IO_RESET_REGION:
			stat = omap3epfb_reset_region(info, (int __user) arg);
			break;

		case OMAP3EPFB_IO_FILL_REGION:
			if (copy_from_user(&p.region, (void __user *)arg, sizeof(struct omap3epfb_area)))
				stat = -EFAULT;
			else 
				stat = omap3epfb_fill_region(info, &p.region);
			break;

		default:
			stat = -EINVAL;
	}

	return stat;
}

/* ------------------------------------------------------------------------- */

#define __dummy_video_timings				\
	.height 	= -1,				\
	.width 		= -1,				\
	.pixclock	= 27778,			\
	.left_margin 	= 128,				\
	.right_margin 	= 24,				\
	.upper_margin 	= 22,				\
	.lower_margin 	= 1,				\
	.hsync_len 	= 72,				\
	.vsync_len 	= 2,				\
	.vmode 		= FB_VMODE_NONINTERLACED,

static const struct fb_var_screeninfo omap3epfb_default_4bpp_packed __initdata = {
	.bits_per_pixel	= 4,
	.red 		= { 0, 4, 0 },
	.green 		= { 0, 4, 0 },
	.blue 		= { 0, 4, 0 },
	.transp 	= { 0, 0, 0 },
	.grayscale 	= 1,

	__dummy_video_timings
};

static const struct fb_var_screeninfo omap3epfb_default_4bpp __initdata = {
	.bits_per_pixel	= 8,
	.red 		= { 0, 4, 0 },
	.green 		= { 0, 4, 0 },
	.blue 		= { 0, 4, 0 },
	.transp 	= { 0, 0, 0 },
	.grayscale 	= 1,

	__dummy_video_timings
};

static const struct fb_var_screeninfo omap3epfb_default_8bpp __initdata = {
	.bits_per_pixel	= 8,
	.red 		= { 0, 8, 0 },
	.green 		= { 0, 8, 0 },
	.blue 		= { 0, 8, 0 },
	.transp 	= { 0, 0, 0 },
	.grayscale 	= 1,

	__dummy_video_timings
};

static const struct fb_var_screeninfo omap3epfb_default_16bpp __initdata = {
	.bits_per_pixel	= 16,
	.red 		= { 11, 5, 0 },
	.green 		= {  5, 6, 0 },
	.blue 		= {  0, 5, 0 },
	.transp 	= {  0, 0, 0 },
	.grayscale 	= 0,

	__dummy_video_timings
};

static const struct fb_fix_screeninfo omap3epfb_fix __initdata = {
	.id		= "omap3epfb",
	.type 		= FB_TYPE_PACKED_PIXELS,
	.xpanstep	= 0,
	.ypanstep	= 0,
	.ywrapstep	= 0,
	.accel		= FB_ACCEL_NONE,
};


static const struct fb_ops omap3epfb_ops __initdata = {
	.owner		= THIS_MODULE,
	.fb_read        = fb_sys_read,
	.fb_write       = omap3epfb_write,
	.fb_check_var	= omap3epfb_check_var,
	.fb_set_par	= omap3epfb_set_par,
	.fb_setcolreg	= omap3epfb_setcolreg,
	.fb_blank	= omap3epfb_blank,
	.fb_fillrect	= omap3epfb_fillrect,
	.fb_copyarea	= omap3epfb_copyarea,
	.fb_imageblit	= omap3epfb_imageblit,
	.fb_sync	= omap3epfb_sync,
	.fb_ioctl	= omap3epfb_ioctl,
};


/*
 * This is called back from the deferred io workqueue.
 *
 * Algorithm copied from broadsheet.c
 */
static void omap3epfb_dpy_deferred_io(struct fb_info *info,
				struct list_head *pagelist)
{
	dev_dbg(info->device, "----- DEFERREDIO module fired screen update\n");

#if 0
	const unsigned int yres = info->var.yres;
	const unsigned int xres = info->var.xres;
	unsigned int y1 = 0, h = 0, npages = 0;
	int prev_index = -1;
	struct page *cur;
	struct fb_deferred_io *fbdefio = info->fbdefio;

	BUG_ON(!fbdefio);

	/* TODO - page walking algorithm needs fixing! */
	/* walk the written page list and swizzle the data */
	list_for_each_entry(cur, &fbdefio->pagelist, lru) {
		if (prev_index < 0) {
			/* just starting so assign first page */
			y1 = (cur->index << PAGE_SHIFT) / xres;
			npages = 1;
		} else if ((prev_index + 1) == cur->index) {
			/* count pages instead of page heights because
			 * the integer divide error might accumulate
			 * too fast with large screens
			 */
			npages++;
		} else {
			/* page not consecutive, issue previous update first */
			h = DIV_ROUND_UP(npages*PAGE_SIZE, xres);
			omap3epfb_update_area_retry(info, 0, y1, xres-1, h);
			/* start over with our non consecutive page */
			y1 = (cur->index << PAGE_SHIFT) / xres;
			npages = 1;
		}
		prev_index = cur->index;
	}

	/* if we still have any pages to update we do so now */
	h = DIV_ROUND_UP(npages*PAGE_SIZE, xres);
	if (h >= yres) {
		/* its a full screen update, just do it */
		omap3epfb_update_screen(info,OMAP3EPFB_WVFID_AUTO,true);
	} else {
		BUG_ON(!h);
		omap3epfb_update_area_retry(info, 0, y1, xres-1,
					(y1 + h) >= yres ? yres-1 : h);
	}
#else
  #if defined(CONFIG_FB_OMAP3EPFB_DEFERREDIO)
	omap3epfb_update_screen_bursty(info);
  #endif /* CONFIG_FB_OMAP3EPFB_DEFERREDIO */
#endif /* 0 */
	dev_dbg(info->device, "----- screen update done\n");
}


#ifndef MODULE
static char omap3epfb_pmic_modparam_buf[32];
static char omap3epfb_pmicopt_modparam_buf[64];
static char omap3epfb_mode_modparam_buf[64];

static int __init omap3epfb_setup(char *options)
{
	char *opt;

	if (!options || !*options)
		return 0;

	while ((opt = strsep(&options, ",")) != NULL) {
		if (!*opt)
			continue;
		if (!strncmp(opt, "deferred_io=", 12)) {
			deferred_io_modparam =
					simple_strtol(opt + 12, NULL, 0);
		} else if (!strncmp(opt, "vcom=", 5)) {
			omap3epfb_vcom_modparam =
					simple_strtol(opt + 5, NULL, 0);
		} else if (!strncmp(opt, "pmic=", 5)) {
			omap3epfb_pmic_modparam = omap3epfb_pmic_modparam_buf;
			strncpy(omap3epfb_pmic_modparam_buf, opt+5, 31);
		} else if (!strncmp(opt, "pmic_dwell=", 11)) {
			omap3epfb_pmic_dwell_modparam =
					simple_strtoul(opt + 11, NULL, 0);
		} else if (!strncmp(opt, "pmic_vcomoff=", 13)) {
			omap3epfb_pmic_vcomoff_modparam =
					simple_strtoul(opt + 13, NULL, 0);
		} else if (!strncmp(opt, "pmicopt=", 8)) {
			omap3epfb_pmicopt_modparam = omap3epfb_pmicopt_modparam_buf;
			strncpy(omap3epfb_pmicopt_modparam_buf, opt+8, 63);
		} else if (!strncmp(opt, "mode=", 5)) {
			omap3epfb_mode_modparam = omap3epfb_mode_modparam_buf;
			strncpy(omap3epfb_mode_modparam_buf, opt+5, 63);
		} else
			pr_err("omap3epfb: unrecognized option %s\n", opt);
	}

	return 0;
}
#endif  /*  MODULE  */


static int omap3epfb_parse_mode(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	char *vxres, *vyres, *bpp, *rotation,  *p;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	char *paneltype, *f_pixclk;
#endif
	unsigned int r;

	p = omap3epfb_mode_modparam;
	vxres = strsep(&p, "x");
	vyres = strsep(&p, "x");
	bpp = strsep(&p, "x");
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	f_pixclk = strsep(&p, "x");
#endif
	rotation = strsep(&p, "x");
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	paneltype = strsep(&p, "x");
#endif
	par->mode.pxres = simple_strtoul(vxres, NULL, 0);
	par->mode.pyres = simple_strtoul(vyres, NULL, 0);
	par->mode.bpp = simple_strtoul(bpp, NULL, 0);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	par->mode.f_pixclk_khz = simple_strtoul(f_pixclk, NULL, 0) * 1000;
#endif
	if (!rotation)
		rotation = "0";
	r = simple_strtoul(rotation, NULL, 0);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	if (!paneltype)
		paneltype = "0";

	par->epd_fixpar.panel_type = simple_strtoul(paneltype, NULL, 0);

	dev_dbg(info->device, "Reverse-parsed mode=%ux%ux%ux%ux%u%u\n",
					par->mode.pxres, par->mode.pyres,
					par->mode.bpp, par->mode.f_pixclk_khz,
					r,par->epd_fixpar.panel_type);
#else
	dev_dbg(info->device, "Reverse-parsed mode=%ux%ux%ux%u\n",
					par->mode.pxres, par->mode.pyres,
					par->mode.bpp,r);
#endif

	if ((r == 0)||(r == 180)) {
		par->mode.vxres = par->mode.pxres;
		par->mode.vyres = par->mode.pyres;
	} else if ((r == 90)||(r == 270)) {
		par->mode.vxres = par->mode.pyres;
		par->mode.vyres = par->mode.pxres;
	} else {
		dev_err(info->device, "Invalid rotation %u\n", r);
		return -EINVAL;	/* only 0 and 90 degrees rotation supported */
	}

	par->epd_fixpar.rotation = r;

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	/* some sanity checks */
	if ((par->mode.f_pixclk_khz < 5000) || (par->mode.f_pixclk_khz > 34000)) {
		dev_err(info->device, "Invalid pixel clock %u\n",
						par->mode.f_pixclk_khz / 1000);
		return -EINVAL;
	}
#endif

	if ((par->mode.pxres < 10) || (par->mode.pxres > 3000)) {
		dev_err(info->device, "Invalid X resolution %u\n",
							par->mode.pxres);
		return -EINVAL;
	}
	if ((par->mode.pyres < 10) || (par->mode.pyres > 3000)) {
		dev_err(info->device, "Invalid Y resolution %u\n",
							par->mode.pyres);
		return -EINVAL;
	}
	if (par->mode.pxres % 4) {
		dev_err(info->device, "X resolution must be multiple of 4!\n");
		return -EINVAL;		/* observe pixel output packing */
	}
	if ((par->mode.pxres * par->mode.pyres) % 16) {
		dev_err(info->device, "Number of pixels must be multiple of 16!\n");
		return -EINVAL;		/* observe alignment */
	}

	switch (par->mode.bpp) {
		case 0:
		case 4:
		case 8:
		case 16:
			break;	/* OK */
		default:
			dev_err(info->device, "Invalid bits per pixel %u\n",
								par->mode.bpp);
			return -EINVAL;
	}

	return 0;
}

static int __init omap3epfb_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	struct omap3epfb_par *par;
	struct device *dev = &pdev->dev;
	struct device_attribute *attr;
	int retval = -ENOMEM;
	int i;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	int lcd_db_order;
#endif
	dev_dbg(dev, "Probing OMAP3EP FB, compiled on " __DATE__", "__TIME__"\n");

	/* inform kernel that we support DMA */
	if (dma_set_mask(dev, DMA_BIT_MASK(32))) {
		dev_err(dev, "could not set DMA mask\n");
	}

	info = framebuffer_alloc(sizeof(*par), dev);
	if (!info)
		goto out;

	par = info->par;

	/* initialize the private driver data */
	par->info = info;

	/* first of all we have to setup the driver parameters */
	retval = omap3epfb_parse_mode(info);
	if (retval) {
		dev_err(dev, "mode parse error - check arguments!\n");
		goto free_info;
	}

	/* ... only then we're allowed to setup subframe queue state */
	omap3epfb_init_subfq_state(info);

	retval = omap3epfb_alloc_buffers(info);
	if (retval) {
		dev_err(dev, "cannot allocate contiguous memory\n");
		goto free_info;
	}
	par->fbvars.fbops = omap3epfb_ops;
	memset(&par->fbvars.defio, 0, sizeof(par->fbvars.defio));
	par->fbvars.defio.deferred_io = omap3epfb_dpy_deferred_io;
	if (deferred_io_modparam <= 0)
		par->fbvars.defio.delay = -1;
	else
		par->fbvars.defio.delay = deferred_io_modparam * HZ / 1000;

	par->epd_varpar.border_color = EPD_BORDER_FLOATING;
	par->epd_varpar.emptysync = 1;

	retval = omap3epfb_bfb_init(info);

	omap3epfb_reg_daemon(info,false);
	omap3epfb_set_loaded_daemon_st(info, false);
	par->dsp_pm_sleep = false;

	par->last_yoffset = 0;

	init_timer(&(par->timer));
	par->timer.function = &omap3epfb_tracking_timer;
	par->timer.data = (u_long)par;
	mod_timer(&par->timer,1*HZ);

	par->reqq.ri = par->reqq.wi = (unsigned int)(-10);
	init_waitqueue_head(&par->reqq.waitreq);
	spin_lock_init(&par->reqq.xi_lock);
	par->reqq.cmd_in_queue = 0;

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	par->stamp_mask = 0;
#endif

	memset(par->pseudo_palette, 0, sizeof(par->pseudo_palette));

	/* parse platform config */
	if (dev->platform_data) {
		dev_dbg(dev, "platform data available\n");
	} else {
		dev_dbg(dev, "platform data NOT available\n");
	}

	/* setup video memory addresses */
	info->screen_base = par->vmem.virt;
	info->fbops = &par->fbvars.fbops;

	info->fbdefio = &par->fbvars.defio;
	fb_deferred_io_init(info);

	info->pseudo_palette = par->pseudo_palette;
	info->flags = FBINFO_DEFAULT;

	/* fill-in init-time configurable parameters */
	info->fix = omap3epfb_fix;
	info->fix.smem_start = (unsigned long)par->vmem.phys;
	info->fix.ypanstep = par->mode.vyres;
	info->fix.smem_len = omap3epfb_videomem_size(info);

	switch (par->mode.bpp) {
		case 0:
			info->var = omap3epfb_default_4bpp_packed;
			/* we don't support this corner setup */
			BUG_ON((par->mode.vxres % 2) == 1);
			info->fix.visual = FB_VISUAL_STATIC_PSEUDOCOLOR;
			info->fix.line_length = par->mode.vxres / 2;
			break;
		case 4:
			info->var = omap3epfb_default_4bpp;
			info->fix.visual = FB_VISUAL_STATIC_PSEUDOCOLOR;
			info->fix.line_length = par->mode.vxres;
			break;
		case 8:
			info->var = omap3epfb_default_8bpp;
			info->fix.visual = FB_VISUAL_STATIC_PSEUDOCOLOR;
			info->fix.line_length = par->mode.vxres;
			break;
		case 16:
			info->var = omap3epfb_default_16bpp;
			info->fix.visual = FB_VISUAL_TRUECOLOR;
			info->fix.line_length = 2 * par->mode.vxres;
			break;
		default:
			BUG();
	}

	info->var.xres = par->mode.vxres;
	info->var.yres = par->mode.vyres;
	info->var.xres_virtual = par->mode.vxres;
	info->var.yres_virtual = par->mode.vyres;


	retval = omap3epfb_create_screenupdate_workqueue(info);
	if (retval)
		goto free_buffers;

	if ((par->mode.bpp == 4) || (par->mode.bpp == 0)) {
		retval = fb_alloc_cmap(&info->cmap, 16, 0);
		if (retval < 0)
			goto destroy_screenupdate_workqueue;

		/* generate a static color map (taken from broadsheetfb) */
		for (i = 0; i < 16; i++)
			info->cmap.red[i] = (((2*i)+1)*(0xFFFF))/32;
		memcpy(info->cmap.green, info->cmap.red, sizeof(u16)*16);
		memcpy(info->cmap.blue, info->cmap.red, sizeof(u16)*16);
	} else {
		retval = fb_alloc_cmap(&info->cmap, 256, 0);
		if (retval < 0)
			goto destroy_screenupdate_workqueue;

		/* generate a static color map (taken from broadsheetfb) */
		for (i = 0; i < 256; i++)
			info->cmap.red[i] = (((2*i)+1)*(0xFFFF))/512;
		memcpy(info->cmap.green, info->cmap.red, sizeof(u16)*256);
		memcpy(info->cmap.blue, info->cmap.red, sizeof(u16)*256);
	}

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	if(par->epd_fixpar.panel_type == OMAP3EPFB_PANELTYPE_AUO){
		lcd_db_order = LCD_DB_ORDER_REVERSE;
	}else{
		lcd_db_order = LCD_DB_ORDER_NORMAL;
	}


	retval = omap3epdss_alloc(info,lcd_db_order);
	if (retval < 0)
		goto free_cmap;
#endif
	retval = pmic_probe(&par->pwr_sess, omap3epfb_pmic_modparam,
						omap3epfb_pmicopt_modparam,
						omap3epfb_pmic_dwell_modparam,
						omap3epfb_pmic_vcomoff_modparam);
	if (retval < 0)
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		goto dss_cleanup;
#else
		goto free_cmap;
#endif
	retval = pmic_set_vcom(par->pwr_sess, omap3epfb_vcom_modparam);
	if (retval)
		dev_err(dev,
			"VCOM voltage %d mV is invalid\n",
			omap3epfb_vcom_modparam);

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
	retval = gpmc_probe(&par->gpmc);
	if (retval){
		dev_err(dev,"Cannot configure GPMC\n");
		goto pmic_cleanup;
	}

#endif

	retval = register_framebuffer(info);
	if (retval < 0) {
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
		goto gpmc_cleanup;
#else
		goto pmic_cleanup;
#endif
	}
	platform_set_drvdata(pdev, info);

	i = 0;
	while ((attr = epd_sysfs_attrs[i++]) != NULL) {
		retval = device_create_file(info->dev, attr);
		if (retval) {
			printk (KERN_ERR "Failed to create sysfs file\n");
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
			goto gpmc_cleanup;
#else
			goto pmic_cleanup;
#endif
		}
	}

	if (user_init_device(info)) {
		printk (KERN_ERR "Failed to create user sysfs files\n");
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
		goto gpmc_cleanup;
#else
		goto pmic_cleanup;
#endif
	}

	dev_dbg(dev, "fb%d: OMAP3EP frame buffer device, using %dK of video memory\n",
	       info->node, info->fix.smem_len >> 10);
	return 0;

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)
gpmc_cleanup:
	gpmc_remove(&par->gpmc);
#endif
pmic_cleanup:
	user_cleanup_device(info);
	pmic_remove(&par->pwr_sess);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
dss_cleanup:
	omap3epdss_free(info);
#endif
free_cmap:
	fb_dealloc_cmap(&info->cmap);

destroy_screenupdate_workqueue:
	omap3epfb_destroy_screenupdate_workqueue(info);

free_buffers:
	omap3epfb_free_buffers(info);
free_info:
	framebuffer_release(info);
out:
	return retval;
}


static int omap3epfb_remove(struct platform_device *pdev)
{
	struct fb_info *info;
	struct omap3epfb_par *par;
	struct device_attribute *attr;
	int i = 0;

	info = platform_get_drvdata(pdev);
	par = info->par;

	if (info) {
		while ((attr = epd_sysfs_attrs[i++]) != NULL) {
			device_remove_file(info->dev, attr);
		}
		user_cleanup_device(info);

		del_timer_sync(&par->timer);
		
		unregister_framebuffer(info);

		fb_deferred_io_cleanup(info);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		cancel_work_sync(&par->subfq.ss_work);
		flush_work(&par->subfq.ss_work);

#if defined(DEBUG)
		BUG_ON(down_trylock(&par->screen_update_mutex) == 1);
		up(&par->screen_update_mutex);

#endif
//		BUG_ON(waitqueue_active(&par->subfq.waitfree));
		BUG_ON(waitqueue_active(&par->subfq.waitframe));
#endif

		cancel_work_sync(&par->subfq.dspck_work);
		flush_work(&par->subfq.dspck_work);

		omap3epfb_destroy_screenupdate_workqueue(info);

		pmic_remove(&par->pwr_sess);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		/* TODO:  IRQ inhibit? */
		omap3epdss_free(info);
#endif
		omap3epfb_free_buffers(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}

	return 0;
}

#ifdef CONFIG_PM
static int omap3epfb_prepare(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct fb_info *info = platform_get_drvdata(pdev);
	struct omap3epfb_par *par = info->par;

mb();
	if(par->pwr_sess->powered) {
		return -EBUSY;
	}

	/* skip the callback if dwell time is zero */
	if (PAPYRUS_STANDBY_DWELL_TIME && !pmic_standby_dwell_time_ready(par->pwr_sess)) {
		return -EBUSY;
	}

	/*
	 * We send SLEEP to DSP in @prepare and not in @suspend because we
	 * want to avoid unnecessary wakeup of all drivers put to sleep if
	 * DSP is not ready to go off.
	 */
	if(omap3epfb_send_dsp_pm(info,true,NULL)) {
		return -EBUSY;
	}
	return 0;
}
static int omap3epfb_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct fb_info *info = platform_get_drvdata(pdev);
	struct omap3epfb_par *par = info->par;

	/* check again if DSP is running */
	if(unlikely(par->subfq.shwr.p->wq.dsp_running_flag)) {
		return -EBUSY;
	} else {
		pmic_pm_sleep(par->pwr_sess);
		return 0;
	}
}

static int omap3epfb_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct fb_info *info = platform_get_drvdata(pdev);
	struct omap3epfb_par *par = info->par;

	pmic_pm_resume(par->pwr_sess);

	return 0;
}

static const struct dev_pm_ops omap3epfb_pm_ops = {
	.prepare	= omap3epfb_prepare,
	.suspend	= omap3epfb_suspend,
	.resume		= omap3epfb_resume,
};

#endif /* CONFIG_PM */

static struct platform_driver omap3epfb_driver = {
	.probe	= omap3epfb_probe,
	.remove = omap3epfb_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name	= "omap3epfb",
#ifdef CONFIG_PM
		.pm	= &omap3epfb_pm_ops,
#endif
	},
};

static struct platform_device *omap3epfb_device;
static u64 dmamask = DMA_BIT_MASK(32);

static int __init omap3epfb_init(void)
{
	int ret = 0;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("omap3epfb", &option))
		return -ENODEV;
	omap3epfb_setup(option);
#endif

	ret = platform_driver_register(&omap3epfb_driver);

	if (!ret) {
		omap3epfb_device = platform_device_alloc("omap3epfb", 0);

		if (omap3epfb_device) {
			/* TODO - too ugly (platform device must provide it)! */
			omap3epfb_device->dev.dma_mask = &dmamask;
			omap3epfb_device->dev.coherent_dma_mask = DMA_BIT_MASK(32);

			ret = platform_device_add(omap3epfb_device);
		} else
			ret = -ENOMEM;

		if (ret) {
			platform_device_put(omap3epfb_device);
			platform_driver_unregister(&omap3epfb_driver);
		}
	}

	return ret;
}

module_init(omap3epfb_init);

#ifdef MODULE
static void __exit omap3epfb_exit(void)
{
	platform_device_unregister(omap3epfb_device);
	platform_driver_unregister(&omap3epfb_driver);
}

module_exit(omap3epfb_exit);

MODULE_AUTHOR("Dimitar Dimitrov <dddimitrov@mm-sol.com>");
MODULE_DESCRIPTION("OMAP3 Electronic Paper Framebuffer driver.");
MODULE_LICENSE("GPL");
#endif				/* MODULE */
