/*
 * AUO  power control HAL
 *
 *      Copyright (C) 2010 Vladimir Krushkin, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#include <asm/gpio.h>
#else
#include <linux/gpio.h>
#endif

#include "pmic.h"

struct auo_sess
{
	struct i2c_adapter *adap;
	int dvcom_state;
	bool pwup;
	/* Custom power up/down sequence settings */
	struct
	{
		unsigned int dly[4];
	} seq;
};

#define VCOM0_GPIO				(38)
#define VCOM1_GPIO				(39)

#define RST_N					(41)    /* active high */
#define SHD_N					(40)    /* active high */

#if defined(CONFIG_MACH_OMAP3621_EDP1)
#define AUO_I2C_BUS_NUM				(2)
#else
#define AUO_I2C_BUS_NUM				(3)
#endif

#define AUO_I2C_ADDRESS				0x20

static int auo_hw_setreg(struct auo_sess *sess, uint8_t regaddr, uint8_t val)
{
	int stat;
	uint8_t txbuf[2] = { regaddr, val };
	struct i2c_msg msgs[] = { { .addr = AUO_I2C_ADDRESS, .flags = 0, .len = 2,
			.buf = txbuf, } };

	stat = i2c_transfer(sess->adap, msgs, ARRAY_SIZE(msgs));

	if (stat < 0)
		pr_err("auo: I2C send error: %d\n", stat);
	else if (stat != ARRAY_SIZE(msgs)) {
		pr_err("auo: I2C send N mismatch: %d\n", stat);
		stat = -EIO;
	} else
		stat = 0;

	return stat;
}

static void auo_hw_send_powerup(struct auo_sess *sess)
{
	int stat;

	stat = auo_hw_setreg(sess, 0x01, 0x01);
	mdelay(sess->seq.dly[0]);
	gpio_direction_output(SHD_N, 1);
	mdelay(sess->seq.dly[1]);
	stat |= auo_hw_setreg(sess, 0x01, 0x03);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0x07);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0x0F);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0x1F);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0x3F);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0x7F);
	mdelay(sess->seq.dly[2]);
	stat |= auo_hw_setreg(sess, 0x01, 0xFF);
	mdelay(sess->seq.dly[3]);

	if (stat)
		printk("auo: I2C error: %d\n", stat);
}

static void auo_hw_send_powerdown(struct auo_sess *sess)
{
	int stat;

	mdelay(20);
	gpio_direction_output(SHD_N, 0);
	mdelay(2);
	stat = auo_hw_setreg(sess, 0x01, 0x3F);
	mdelay(4);
	stat |= auo_hw_setreg(sess, 0x01, 0x37);
	mdelay(6);
	stat |= auo_hw_setreg(sess, 0x01, 0x07);
	mdelay(24);
	stat |= auo_hw_setreg(sess, 0x01, 0x03);
	mdelay(12);
	stat |= auo_hw_setreg(sess, 0x01, 0x01);
	mdelay(40);
	stat |= auo_hw_setreg(sess, 0x01, 0x00);

	if (stat)
		pr_err("auo: I2C error: %d\n", stat);
}

static int auo_hw_init(struct auo_sess *sess, const char *chip_id)
{
	int stat = 0;

	stat |= gpio_request(RST_N, "auo-pwr");
	stat |= gpio_request(SHD_N, "auo-pwr");

	stat |= gpio_request(VCOM0_GPIO, "dss-vcom");
	stat |= gpio_request(VCOM1_GPIO, "dss-vcom");

	if (stat) {
		printk(KERN_ERR "dss: cannot reserve GPIOs\n");
		stat = -ENODEV;
		return stat;
	}
	gpio_direction_output(RST_N, 0);
	gpio_direction_output(SHD_N, 0);

	gpio_direction_output(VCOM0_GPIO, 0);
	gpio_direction_output(VCOM1_GPIO, 0);

	/* reset I2C */

	msleep(20);
	gpio_direction_output(RST_N, 1);
	msleep(20);

	sess->adap = i2c_get_adapter(AUO_I2C_BUS_NUM);
	if (!sess->adap) {
		printk("auo: cannot get I2C adapter %u\n", AUO_I2C_BUS_NUM);
		stat = -ENODEV;
		goto free_gpios;
	}

//	gpio_direction_output(RST_N, 0);

	stat = auo_hw_setreg(sess, 0x01, 0x00);
	stat |= auo_hw_setreg(sess, 0x03, 0x00);

	stat = 0;
	sess->pwup = false;
	return stat;

free_gpios:
	gpio_direction_output(RST_N, 0);
	gpio_free(RST_N);
	gpio_free(SHD_N);
	return stat;

}

static void auo_hw_power_req(struct pmic_sess *pmsess, bool up)
{
	struct auo_sess *sess = pmsess->drvpar;

	pr_debug("auo: i2c pwr req: %d\n", up);
	if (up)
		auo_hw_send_powerup(sess);
	else
		auo_hw_send_powerdown(sess);


	sess->pwup = up;
}

static int auo_vcom_switch(struct pmic_sess *pmsess, bool state)
{
	return 0;
}
static bool auo_hw_power_ack(struct pmic_sess *pmsess)
{
	struct auo_sess *sess = pmsess->drvpar;
	return sess->pwup;
}



static void auo_hw_cleanup(struct auo_sess *sess)
{
	gpio_direction_output(RST_N, 0);
	gpio_free(VCOM0_GPIO);
	gpio_free(VCOM1_GPIO);

	gpio_free(RST_N);
	gpio_free(SHD_N);

	i2c_put_adapter(sess->adap);
}

static void auo_hw_setdvcom(int state)
{
	gpio_direction_output(VCOM0_GPIO, state & 0x00000001);
	state >>= 1;
	gpio_direction_output(VCOM1_GPIO, state & 0x00000001);
}

/* -------------------------------------------------------------------------*/

/*
 * Expect four integer delays in the range [0..15]. See HW manual for
 * more information. Syntax is: <DLY0>x<DLY1>x<DLY2>x<DLY3>
 */
static void auo_parse_options(struct auo_sess *sess, char *opt)
{
	int i;

	for (i = 0; opt && (i < 4); i++) {
		const char *dly = strsep(&opt, "x");
		sess->seq.dly[i] = simple_strtoul(dly, NULL, 0) & 0xf;
	}

	if (i != 4)
		goto err;

	pr_info("auo: using timing %u/%u/%u/%u ms.\n", sess->seq.dly[0],
			sess->seq.dly[1], sess->seq.dly[2], sess->seq.dly[3]);

	return;

	err: pr_err("auo: Invalid options string!\n");
}

static int auo_probe(struct pmic_sess *pmsess, char *opt)
{
	struct auo_sess *sess;
	int stat;

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess)
		return -ENOMEM;

	sess->seq.dly[0] = 600; //after power up
	sess->seq.dly[1] = 50;	//after reset release
	sess->seq.dly[2] = 10;  //after all others next
	sess->seq.dly[3] = 20; //after last power set

	if (opt && strlen(opt))
		auo_parse_options(sess, opt);

	stat = auo_hw_init(sess, pmsess->drv->id);
	if (stat)
		goto free_sess;

	pmsess->drvpar = sess;

	return 0;

	free_sess: kfree(sess);

	return stat;
}

static void auo_remove(struct pmic_sess *pmsess)
{
	struct auo_sess *sess = pmsess->drvpar;

	auo_hw_cleanup(sess);

	kfree(sess);
	pmsess->drvpar = 0;
}

static int auo_set_dvcom(struct pmic_sess *pmsess, int state)
{
//	struct auo_sess *sess = pmsess->drvpar;

	auo_hw_setdvcom(state);

	return 0;
}

const struct pmic_driver pmic_driver_auo_i2c = {
	.id = "auo-i2c",
	.hw_power_req = auo_hw_power_req,
	.hw_power_ack = auo_hw_power_ack,
	.hw_vcom_switch = auo_vcom_switch,
	.hw_init = auo_probe,
	.hw_cleanup = auo_remove,
	.hw_set_dvcom = auo_set_dvcom,
};

