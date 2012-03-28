/*
 * Papyrus epaper power control HAL
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 * TPS6518x power control is facilitated using I2C control and WAKEUP GPIO
 * pin control. The other VCC GPIO Papyrus' signals must be tied to ground.
 *
 * TODO:
 * 	- Instead of polling, use interrupts to signal power up/down
 * 	  acknowledge.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/i2c/pmic-tps65185-i2c.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
  #include <asm/gpio.h>
#else
  #include <linux/gpio.h>
#endif

#include "pmic.h"

#define PAPYRUS_VCOM_MAX_MV		0
#define PAPYRUS_VCOM_MIN_MV		-5110

/* After waking up from sleep, Papyrus
   waits for VN to be discharged and all
   voltage ref to startup before loading
   the default EEPROM settings. So accessing
   registers too early after WAKEUP could
   cause the register to be overridden by
   default values */
#define PAPYRUS_EEPROM_DELAY_MS 50
/* Papyrus WAKEUP pin must stay low for
   a minimum time */
#define PAPYRUS_SLEEP_MINIMUM_MS 110
/* Temp sensor might take a little time to
   settle eventhough the status bit in TMST1
   state conversion is done - if read too early
   0C will be returned instead of the right temp */
#define PAPYRUS_TEMP_READ_TIME_MS 10

/* Powerup sequence takes at least 24 ms - no need to poll too frequently */
#define HW_GET_STATE_INTERVAL_MS 24

struct papyrus_sess {
	struct i2c_adapter *adap;
	struct i2c_client *client;
	int vcom_voltage;
	uint8_t enable_reg_shadow;

	/* Custom power up/down sequence settings */
	struct {
		/* If options are not valid we will rely on HW defaults. */
		bool valid;
		unsigned int dly[8];
	} seq;
	struct timeval standby_tv;

	/* True if a high WAKEUP brings Papyrus out of reset. */
	int wakeup_active_high;
};


#define WAKEUP_GPIO		(87)	/* active high */
#define CPLD_RESET_GPIO		(88)	/* active low */
#define EN_CPLD_POW_GPIO	(85)	/* active high */

#if defined(CONFIG_MACH_OMAP3621_EDP1) ||\
    defined(CONFIG_MACH_OMAP3621_GOSSAMER)
#define PAPYRUS_I2C_BUS_NUM	(2)
#else
#define PAPYRUS_I2C_BUS_NUM	(3)
#endif

#define PAPYRUS_ADDR_TMST_VALUE		0x00
#define PAPYRUS_ADDR_ENABLE		0x01
#define PAPYRUS_ADDR_VADJ		0x02
#define PAPYRUS_ADDR_VCOM1_ADJUST	0x03
#define PAPYRUS_ADDR_VCOM2_ADJUST	0x04
#define PAPYRUS_ADDR_INT_ENABLE1	0x05
#define PAPYRUS_ADDR_INT_ENABLE2	0x06
#define PAPYRUS_ADDR_INT_STATUS1	0x07
#define PAPYRUS_ADDR_INT_STATUS2	0x08
#define PAPYRUS_ADDR_UPSEQ0		0x09
#define PAPYRUS_ADDR_UPSEQ1		0x0a
#define PAPYRUS_ADDR_DWNSEQ0		0x0b
#define PAPYRUS_ADDR_DWNSEQ1		0x0c
#define PAPYRUS_ADDR_TMST1		0x0d
#define PAPYRUS_ADDR_TMST2		0x0e
#define PAPYRUS_ADDR_PG_STATUS		0x0f
#define PAPYRUS_ADDR_REVID		0x10


#define PAPYRUS_MV_TO_VCOMREG(MV)	((MV) / 10)

#define V3P3_EN_MASK	0x20
#define PAPYRUS_HIGH_VOL_PWRDN_DELAY_MS 100

struct papyrus_hw_state {
	uint8_t tmst_value;
	uint8_t int_status1;
	uint8_t int_status2;
	uint8_t pg_status;
};

static uint8_t papyrus2_i2c_addr = PAPYRUS2_1P0_I2C_ADDRESS;

static int papyrus_hw_setreg(struct papyrus_sess *sess, uint8_t regaddr, uint8_t val)
{
	int stat;
	uint8_t txbuf[2] = { regaddr, val };
	struct i2c_msg msgs[] = {
		{
			.addr = papyrus2_i2c_addr,
			.flags = 0,
			.len = 2,
			.buf = txbuf,
		}
	};

	stat = i2c_transfer(sess->adap, msgs, ARRAY_SIZE(msgs));

	if (stat < 0)
		pr_err("papyrus: I2C send error: %d\n", stat);
	else if (stat != ARRAY_SIZE(msgs)) {
		pr_err("papyrus: I2C send N mismatch: %d\n", stat);
		stat = -EIO;
	} else
		stat = 0;

	return stat;
}


static int papyrus_hw_getreg(struct papyrus_sess *sess, uint8_t regaddr, uint8_t *val)
{
	int stat;
	struct i2c_msg msgs[] = {
		{
			.addr = papyrus2_i2c_addr,
			.flags = 0,
			.len = 1,
			.buf = &regaddr,
		},
		{
			.addr = papyrus2_i2c_addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = val,
		}
	};

	stat = i2c_transfer(sess->adap, msgs, ARRAY_SIZE(msgs));

	if (stat < 0)
    {
		pr_err("papyrus: I2C read error: %d\n", stat);
        pr_err("Papyrus i2c addr %x %s\n",papyrus2_i2c_addr,__FILE__); 
    }
	else if (stat != ARRAY_SIZE(msgs)) {
		pr_err("papyrus: I2C read N mismatch: %d\n", stat);
		stat = -EIO;
	} else
		stat = 0;

	return stat;
}


static void papyrus_hw_get_pg(struct papyrus_sess *sess,
							  struct papyrus_hw_state *hwst)
{
	int stat;

	stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_PG_STATUS, &hwst->pg_status);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
}

/*
static void papyrus_hw_get_state(struct papyrus_sess *sess, struct papyrus_hw_state *hwst)
{
	int stat;

	stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_TMST_VALUE, &hwst->tmst_value);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_INT_STATUS1, &hwst->int_status1);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_INT_STATUS2, &hwst->int_status2);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_PG_STATUS, &hwst->pg_status);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
}
*/

static void papyrus_hw_send_powerup(struct papyrus_sess *sess)
{
	int stat = 0;
	uint16_t vcom1_val;
	uint8_t vcom2_val;

	/* enable CPLD */
	gpio_direction_output(CPLD_RESET_GPIO, 0);
	gpio_direction_output(EN_CPLD_POW_GPIO, 1);
	gpio_direction_output(CPLD_RESET_GPIO, 1);

	/* Keep the default Papyrus2 power sequences, since they 
	   match with E Ink recommendation - uncomment and modify 
	   the following lines if this needs to change in the future */
	/* stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_UPSEQ0,0xE4);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_DWNSEQ0,0x1E); */

	if (sess->seq.valid) {
		/* change UP default delays */
		stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_UPSEQ1,
					((sess->seq.dly[0] & 0x3) << 0) |
					((sess->seq.dly[1] & 0x3) << 2) |
					((sess->seq.dly[2] & 0x3) << 4) |
					((sess->seq.dly[3] & 0x3) << 6));
		/* change DOWN default delays */
		stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_DWNSEQ1,
					((sess->seq.dly[4] & 0x1) << 1) |
					((sess->seq.dly[5] & 0x3) << 2) |
					((sess->seq.dly[6] & 0x3) << 4) |
					((sess->seq.dly[7] & 0x3) << 6));
					/* DFCTR = 0 ? (dflt) */
	
	} 

	/* set VCOM1 */
	vcom1_val = PAPYRUS_MV_TO_VCOMREG(-sess->vcom_voltage);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_VCOM1_ADJUST,
							  vcom1_val & 0x00ff);

	/* set VCOM2 */
	vcom2_val = (vcom1_val & 0x0100) >> 8;
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_VCOM2_ADJUST, vcom2_val);

	/* Enable 3.3V switch to the panel */
	sess->enable_reg_shadow |= V3P3_EN_MASK;
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);

	/* switch to active mode, VCOM buffer disabled */
	sess->enable_reg_shadow = 0xaf;
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
}


static void papyrus_hw_send_powerdown(struct papyrus_sess *sess)
{
	int stat;

	/* keep XXX_PWR_EN signals enabled and activate STANDBY */
	sess->enable_reg_shadow = 0x6f;
	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);

	msleep(PAPYRUS_HIGH_VOL_PWRDN_DELAY_MS);

	/* 3.3V switch must be turned off last */
	sess->enable_reg_shadow &= ~V3P3_EN_MASK;
	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);

	/* disable CPLD */
	gpio_direction_output(CPLD_RESET_GPIO, 0);
	gpio_direction_output(EN_CPLD_POW_GPIO, 0);

	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);

	do_gettimeofday(&sess->standby_tv);
}

static int papyrus_hw_get_revid(struct papyrus_sess *sess)
{
	int stat;
	uint8_t revid;

	stat = papyrus_hw_getreg(sess, PAPYRUS_ADDR_REVID, &revid);

	if (stat) {
		pr_err("papyrus: I2C error: %d\n", stat);
		return stat;
	} else
		return revid;
}

void papyrus_set_i2c_address(int address)
{
    if    (address == PAPYRUS2_1P0_I2C_ADDRESS)
    {
        papyrus2_i2c_addr = PAPYRUS2_1P0_I2C_ADDRESS;
    }
    else if (address == PAPYRUS2_1P1_I2C_ADDRESS)
    {
        papyrus2_i2c_addr = PAPYRUS2_1P1_I2C_ADDRESS;
    } else {
        pr_err("papyrus: Invalid i2c address: %d\n", address);
    }
    printk("papyrus i2c addr set to %x\n",papyrus2_i2c_addr);
    

}

static int papyrus_hw_init(struct papyrus_sess *sess, const char *chip_id)
{
	int stat = 0;

	stat |= gpio_request(WAKEUP_GPIO, "papyrus-pwr");
	stat |= gpio_request(EN_CPLD_POW_GPIO, "cpld-pwr");
	stat |= gpio_request(CPLD_RESET_GPIO, "cpld-reset");

	if (stat) {
		pr_err("papyrus: cannot reserve GPIOs\n");
		stat = -ENODEV;
		return stat;
	}

	gpio_direction_output(EN_CPLD_POW_GPIO, 0);

	sess->wakeup_active_high = 1;

	gpio_direction_output(WAKEUP_GPIO, !sess->wakeup_active_high);
	/* wait to reset papyrus */
	msleep(PAPYRUS_SLEEP_MINIMUM_MS);
	gpio_direction_output(WAKEUP_GPIO, sess->wakeup_active_high);
	msleep(PAPYRUS_EEPROM_DELAY_MS);

	sess->adap = i2c_get_adapter(PAPYRUS_I2C_BUS_NUM);
	if (!sess->adap) {
		pr_err("cannot get I2C adapter %u\n",
		       PAPYRUS_I2C_BUS_NUM);
		stat = -ENODEV;
		goto free_gpios;
	}

	stat = papyrus_hw_get_revid(sess);
	if (stat < 0)
		goto cleanup_i2c_adapter;
	pr_info("papyrus: detected device with ID=%02x (TPS6518%dr%dp%d)\n",
			stat, stat & 0xF, (stat & 0xC0) >> 6, (stat & 0x30) >> 4);
	stat = 0;

	return stat;

cleanup_i2c_adapter:
	i2c_put_adapter(sess->adap);
free_gpios:
	gpio_free(WAKEUP_GPIO);
	gpio_free(EN_CPLD_POW_GPIO);
	gpio_free(CPLD_RESET_GPIO);

	pr_err("papyrus: ERROR: could not initialize I2C papyrus!\n");
	return stat;
}

static int papyrus_hw_read_temperature(struct pmic_sess *pmsess, int *t)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	int stat;
	int ntries = 50;
	uint8_t tb;

	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_TMST1, 0x80);

	do {
		stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_TMST1, &tb);
	} while (!stat && ntries-- && (((tb & 0x20) == 0) || (tb & 0x80)));

	if (stat)
		return stat;

	msleep(PAPYRUS_TEMP_READ_TIME_MS);
	stat = papyrus_hw_getreg(sess, PAPYRUS_ADDR_TMST_VALUE, &tb);
	*t = (int)(int8_t)tb;

	return stat;
}


static void papyrus_hw_power_req(struct pmic_sess *pmsess, bool up)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	pr_debug("papyrus: i2c pwr req: %d\n", up);
	if (up)
		papyrus_hw_send_powerup(sess);
	else
		papyrus_hw_send_powerdown(sess);
}


static bool papyrus_hw_power_ack(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	struct papyrus_hw_state hwst;
	int st;
	int retries_left = 10;

	do {
		papyrus_hw_get_pg(sess, &hwst);

		pr_debug("hwst: tmst_val=%d, ist1=%02x, ist2=%02x, pg=%02x\n",
				hwst.tmst_value, hwst.int_status1,
				hwst.int_status2, hwst.pg_status);
		hwst.pg_status &= 0xfa;
		if (hwst.pg_status == 0xfa)
			st = 1;
		else if (hwst.pg_status == 0x00)
			st = 0;
		else {
			st = -1;	/* not settled yet */
			msleep(HW_GET_STATE_INTERVAL_MS);
		}
		retries_left--;
	} while ((st == -1) && retries_left);

	if ((st == -1) && !retries_left)
		pr_err("papyrus: power up/down settle error (PG = %02x)\n", hwst.pg_status);

	return !!st;
}


static void papyrus_hw_cleanup(struct papyrus_sess *sess)
{
	gpio_free(WAKEUP_GPIO);
	gpio_free(EN_CPLD_POW_GPIO);
	gpio_free(CPLD_RESET_GPIO);

	i2c_put_adapter(sess->adap);
}


/* -------------------------------------------------------------------------*/
/*
 * Expect integer delays in the ranges:
 * UP_DLY [0..12]
 * DWN_DLY1 [3-6]
 * DWN_DLY2-4 [0-48]
 *
 * See HW manual for more information.
 *
 * Syntax is:
 * <UP_DLY1>x<UP_DLY2>x<UP_DLY3>x<UP_DLY4>x<D_DLY1>x<D_DLY2>x<D_DLY3>x<D_DLY4>
 */
static int upms_to_regvalue(int ms)
{
	switch (ms) {
		case 3:
			return 0;
		case 6:
			return 1;
		case 9:
			return 2;
		case 12:
			return 3;
		default:
			pr_err("papyrus: illegal upseq value: %d (only 3, 6, 9 and 12 allowed). Defaulting to 6 ms\n", ms);
			return 1;
	}
}

static int dnms_to_regvalue(int ms)
{
	switch (ms) {
		case 6:
			return 0;
		case 12:
			return 1;
		case 24:
			return 2;
		case 48:
			return 3;
		default:
			pr_err("papyrus: illegal downseq value: %d (only 6, 12, 24 and 48 allowed). Defaulting to 6 ms\n", ms);
			return 0;
	}
}

static void papyrus_parse_options(struct papyrus_sess *sess, char *opt)
{
	int i;

	for (i = 0; opt && (i < 4); i++) {
		const char *dly = strsep(&opt, "x");
		sess->seq.dly[i] = upms_to_regvalue(simple_strtoul(dly, NULL, 0));
	}

	if (opt) {
		const char *dly = strsep(&opt, "x");
		int d = simple_strtoul(dly, NULL, 0);
		switch (d) {
			case 3:
				sess->seq.dly[4] = 0;
				break;
			case 6:
				sess->seq.dly[4] = 1;
				break;
			default:
				pr_err("papyrus: illegal downseq0 value: %d (only 3 and 6 allowed). Defaulting to 3 ms\n", d);
				sess->seq.dly[4] = 0;
				break;
		}
	}

	for (i=5; opt && (i < 8); i++) {
		const char *dly = strsep(&opt, "x");
		sess->seq.dly[i] = dnms_to_regvalue(simple_strtoul(dly, NULL, 0));
	}

	if (i != 8)
		goto err;

	pr_info("papyrus: using UP timings %u/%u/%u/%u ms.\n",
					sess->seq.dly[0], sess->seq.dly[1],
					sess->seq.dly[2], sess->seq.dly[3]);

	pr_info("papyrus: using DOWN timings %u/%u/%u/%u ms.\n",
					sess->seq.dly[4], sess->seq.dly[5],
					sess->seq.dly[6], sess->seq.dly[7]);
	sess->seq.valid = true;
	return;

err:
	pr_err("papyrus: Invalid options string!\n");
}

static int papyrus_probe(struct pmic_sess *pmsess, char *opt)
{
	struct papyrus_sess *sess;
	int stat;

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess)
		return -ENOMEM;

#warning "-----> FIXME? <-----"
	sess->vcom_voltage = -2550;

	do_gettimeofday(&sess->standby_tv);

	if (opt && strlen(opt))
		papyrus_parse_options(sess, opt);

	stat = papyrus_hw_init(sess, pmsess->drv->id);
	if (stat)
		goto free_sess;

	pmsess->drvpar = sess;

	return 0;

free_sess:
	kfree(sess);

	return stat;
}


static void papyrus_remove(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	papyrus_hw_cleanup(sess);

	kfree(sess);
	pmsess->drvpar = 0;
}


static int papyrus_set_vcom(struct pmic_sess *pmsess, int vcom_mv)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->vcom_voltage = vcom_mv;

	return 0;
}

static int papyrus_vcom_switch(struct pmic_sess *pmsess, bool state)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	int stat;

	sess->enable_reg_shadow &= ~((1u << 4) | (1u << 6) | (1u << 7));
	sess->enable_reg_shadow |= (state ? 1u : 0) << 4;

	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);

	return stat;
}

static bool papyrus_standby_dwell_time_ready(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	struct timeval current_tv;
	long total_secs;

	do_gettimeofday(&current_tv);
mb();
	total_secs = current_tv.tv_sec - sess->standby_tv.tv_sec;

	if (total_secs < PAPYRUS_STANDBY_DWELL_TIME)
		return false;

	return true;
}

static void papyrus_pm_sleep(struct pmic_sess *sess)
{
#if !defined(FB_OMAP3EP_PAPYRUS_PM_STANDBY)
	struct papyrus_sess *s = sess->drvpar;
	gpio_direction_output(WAKEUP_GPIO, !s->wakeup_active_high);
	msleep(PAPYRUS_SLEEP_MINIMUM_MS);
#endif
	pr_debug("%s\n", __func__);
}

static void papyrus_pm_resume(struct pmic_sess *sess)
{
#if !defined(FB_OMAP3EP_PAPYRUS_PM_STANDBY)
	struct papyrus_sess *s = sess->drvpar;
	gpio_direction_output(WAKEUP_GPIO, s->wakeup_active_high);
	msleep(PAPYRUS_EEPROM_DELAY_MS);
#endif
	pr_debug("%s\n", __func__);
}

const struct pmic_driver pmic_driver_tps65185_i2c = {
	.id = "tps65185-i2c",

	.vcom_min = PAPYRUS_VCOM_MIN_MV,
	.vcom_max = PAPYRUS_VCOM_MAX_MV,
	.vcom_step = 10,

	.hw_read_temperature = papyrus_hw_read_temperature,
	.hw_power_ack = papyrus_hw_power_ack,
	.hw_power_req = papyrus_hw_power_req,
	.set_vcom_voltage = papyrus_set_vcom,

	.hw_vcom_switch = papyrus_vcom_switch,

	.hw_init = papyrus_probe,
	.hw_cleanup = papyrus_remove,

	.hw_standby_dwell_time_ready = papyrus_standby_dwell_time_ready,
	.hw_pm_sleep = papyrus_pm_sleep,
	.hw_pm_resume = papyrus_pm_resume,
};
