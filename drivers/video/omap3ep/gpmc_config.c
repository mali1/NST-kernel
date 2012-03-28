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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <plat/omap-pm.h>
#include <plat/gpmc.h>
#include "gpmc_config.h"

#define GPMC_SYSCONFIG	0x10
#define AUTO_IDLE		(1 << 0)
#define IDLE_MODE(val)		((val & 3) << 3)
enum {FORCE_IDLE=0,NO_IDLE,SMART_IDLE};

//#define GPMC_CONFIG		0x50 /*defined  in plat/gpmc.h*/
#define NANDFORCEPOSTEDWRITE	(1 << 0)
#define LIMITEDADDRESS		(1 << 1)
#define WRITEPROTECT		(1 << 4)
#define WAIT0PINPOLARITY(val)	((val & 1) << 8)
#define WAIT1PINPOLARITY(val)	((val & 1) << 9)
#define WAIT2PINPOLARITY(val)	((val & 1) << 10)
#define WAIT3PINPOLARITY(val)	((val & 1) << 11)
enum {LOW,HIGH};

#define OMAP3EPFB_GPMC_CS 7

int gpmc_probe(struct gpmc_sess **sess)
{
	struct gpmc_timings t;
	int st = 0;
	unsigned long l;

	*sess = kzalloc(sizeof(**sess), GFP_KERNEL);
	if (!*sess)
		return -ENOMEM;


	(*sess)->cs = OMAP3EPFB_GPMC_CS;
	(*sess)->size = SZ_4K;

//	(*sess)->baseaddr;
//	(*sess)->gpmc_fck = clk_get(NULL, "gpmc_fck");
//	printk("gpmc_fck = %ld\n",clk_get_rate((*sess)->gpmc_fck));

//	st = clk_set_rate((*sess)->gpmc_fck,166000000);
//	printk("after clk_set_rate = %d\n",st);

	st = gpmc_cs_request((*sess)->cs, (*sess)->size, &(*sess)->phys_base);
	if (st < 0) {
		pr_err("Cannot request GPMC CS\n");
		goto out_free_sess;
	}

	if (!request_mem_region((*sess)->phys_base, (*sess)->size,"EPAPER_GPMC")) {
		st = -EBUSY;
		goto out_free_cs;
	}

	gpmc_write_reg(GPMC_SYSCONFIG, IDLE_MODE(SMART_IDLE)|AUTO_IDLE);
	gpmc_write_reg(GPMC_CONFIG, WAIT1PINPOLARITY(LOW)|LIMITEDADDRESS);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG1,
					GPMC_CONFIG1_DEVICETYPE_NOR
					);

	memset(&t, 0, sizeof(t));
#if 0
	t.sync_clk 		= 0;
	t.cs_on 		= 0;		/* Assertion time */
	t.cs_rd_off		= 26*6;		/* Read deassertion time */
	t.cs_wr_off		= 7*6;		/* Write deassertion time */

	t.adv_on		= 1*6;		/* Assertion time */
	t.adv_rd_off		= 2*6;		/* Read deassertion time */
	t.adv_wr_off		= 2*6;		/* Write deassertion time */

	t.we_on			= 4*6;		/* WE assertion time */
	t.we_off		= 7*6;		/* WE deassertion time */

	t.oe_on			= 0*6;		/* OE assertion time */
	t.oe_off		= 6*6;		/* OE deassertion time */

	t.page_burst_access	= 0*6;		/* Multiple access word delay */
	t.access		= 8*6;		/* Start-cycle to first data valid delay */
	t.rd_cycle		= 30*6;		/* Total read cycle time */
	t.wr_cycle		= 7*6;		/* Total write cycle time */

	t.wr_access		= 0*6;		/* WRACCESSTIME */
	t.wr_data_mux_bus	= 3*6;		/* WRDATAONADMUXBUS */


	st = gpmc_cs_set_timings((*sess)->cs,&t);
	if(st){
		pr_err("Cannot set GPMC timings\n");
		goto out_free_cs;
	}

#else
	t.sync_clk 		= 0;
	t.cs_on 		= 0;		/* Assertion time */
	t.cs_rd_off		= 26;		/* Read deassertion time */
	t.cs_wr_off		= 7;		/* Write deassertion time */

	t.adv_on		= 1;		/* Assertion time */
	t.adv_rd_off		= 2;		/* Read deassertion time */
	t.adv_wr_off		= 2;		/* Write deassertion time */

	t.we_on			= 4;		/* WE assertion time */
	t.we_off		= 7;		/* WE deassertion time */

	t.oe_on			= 0;		/* OE assertion time */
	t.oe_off		= 6;		/* OE deassertion time */

	t.page_burst_access	= 0;		/* Multiple access word delay */
	t.access		= 8;		/* Start-cycle to first data valid delay */
	t.rd_cycle		= 30;		/* Total read cycle time */
	t.wr_cycle		= 7;		/* Total write cycle time */

	t.wr_access		= 0;		/* WRACCESSTIME */
	t.wr_data_mux_bus	= 3;		/* WRDATAONADMUXBUS */

	l = (t.cs_on & 0x0F) | ((t.cs_rd_off & 0x1F) << 8) | ((t.cs_wr_off & 0x0F) << 16);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG2,l);

	l = (t.adv_on & 0x0F) | ((t.adv_rd_off & 0x1F) << 8) | ((t.adv_wr_off & 0x0F) << 16);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG3,l);

	l = (t.oe_on & 0x0F) | ((t.oe_off & 0x1F) << 8) | ((t.we_on & 0x0F) << 16) | ((t.we_off & 0x1F) << 24);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG4,l);

	l = (t.rd_cycle & 0x1F) | ((t.wr_cycle & 0x1F) << 8) | ((t.access & 0x1F) << 16) | ((t.page_burst_access & 0x0F) << 24);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG5,l);

	l = ((t.wr_data_mux_bus & 0x0F) << 16) | ((t.wr_access & 0x1F) << 24) | (1 << 31);
	gpmc_cs_write_reg((*sess)->cs, GPMC_CS_CONFIG6,l);
#endif

#if 0
	printk(KERN_INFO "GPMC settings configured \n");

	printk(KERN_INFO "GPMC_SYSCONFIG	= %x\n", gpmc_read_reg(GPMC_SYSCONFIG));
	printk(KERN_INFO "GMPC_CONFIG		= %x\n", gpmc_read_reg(GPMC_CONFIG));
	printk(KERN_INFO "GMPC_CONFIG1		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG1));
	printk(KERN_INFO "GMPC_CONFIG2		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG2));
	printk(KERN_INFO "GMPC_CONFIG3		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG3));
	printk(KERN_INFO "GMPC_CONFIG4		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG4));
	printk(KERN_INFO "GMPC_CONFIG5		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG5));
	printk(KERN_INFO "GMPC_CONFIG6		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG6));
	printk(KERN_INFO "GMPC_CONFIG7		= %x\n", gpmc_cs_read_reg((*sess)->cs, GPMC_CS_CONFIG7));

//    printf("GPMC_CS = %x",  *(uint32 *)gpmc_cs_addr);
#endif
	printk(KERN_INFO "gpmc config : phys_base= %lx\n", (*sess)->phys_base);
	return 0;

out_free_cs:
	gpmc_cs_free((*sess)->cs);
out_free_sess:
	kfree(*sess);

	return st;
}

void gpmc_remove(struct gpmc_sess **sess)
{
	gpmc_cs_free((*sess)->cs);
	kfree(*sess);
	(*sess) = NULL;
}

void gpmc_clock_request(struct fb_info *info)
{
	omap_pm_set_min_bus_tput(info->device, OCP_INITIATOR_AGENT, 166 * 1000 * 4); // 166MHz / 32bit bus
}

void gpmc_clock_release(struct fb_info *info)
{
	omap_pm_set_min_bus_tput(info->device, OCP_INITIATOR_AGENT, 0);
}
