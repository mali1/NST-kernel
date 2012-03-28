/*
 * Papyrus epaper power control HAL
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, MM Solutions
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

#include "pmic.h"


/*
 * How many milliseconds to wait after pmic_sync_powerup() with power up
 * not acknowledged before reporting an error.
 */
#define PMIC_POWERUP_TIMEOUT_MS		100

/*
 * How many milliseconds to wait for power ACK to go down after a
 * power down request, before giving up and reporting error.
 */
#define PMIC_POWERDOWN_TIMEOUT_MS	100


static int null_hw_init(struct pmic_sess *sess, char *opt)
{
	(void)opt;
	sess->drvpar = 0;
	return 0;
}

static void null_hw_cleanup(struct pmic_sess *sess)
{
	(void)sess;
}

static bool null_hw_power_ack(struct pmic_sess *sess)
{
	return !!sess->drvpar;
}

static void null_hw_power_req(struct pmic_sess *sess, bool up)
{
	sess->drvpar = (void *)up;
}

static const struct pmic_driver pmic_driver_null = {
	.id = "null",

	.vcom_min = -20000,
	.vcom_max = 20000,
	.vcom_step = 10,

	.hw_power_ack = null_hw_power_ack,
	.hw_power_req = null_hw_power_req,
	.hw_init = null_hw_init,
	.hw_cleanup = null_hw_cleanup,
};


extern const struct pmic_driver pmic_driver_tps65180_1p1_i2c;
extern const struct pmic_driver pmic_driver_tps65180_1p2_i2c;
extern const struct pmic_driver pmic_driver_tps65185_i2c;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
extern const struct pmic_driver pmic_driver_auo_i2c;
#endif
static const struct pmic_driver *pmic_drivers[] = {
	&pmic_driver_null,
	&pmic_driver_tps65180_1p1_i2c,
	&pmic_driver_tps65180_1p2_i2c,
	&pmic_driver_tps65185_i2c,
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	&pmic_driver_auo_i2c,
#endif
	NULL
};


static void pmic_powerdown_execute(struct work_struct *work)
{
	struct pmic_sess *sess;

	sess = container_of(work, struct pmic_sess, powerdown_work.work);

	BUG_ON(!sess->powered);

	if (sess->drv->hw_power_req)
		sess->drv->hw_power_req(sess, false);
	sess->powered = false;
}

static void pmic_vcomoff_execute(struct work_struct *work)
{
	struct pmic_sess *sess;
	int st = 0;

	sess = container_of(work, struct pmic_sess, vcomoff_work.work);

	if (sess->drv->hw_vcom_switch)
		st = sess->drv->hw_vcom_switch(sess, false);
}


int pmic_probe(struct pmic_sess **sess, const char *id, char *opt,
					unsigned int dwell_time_ms,
					unsigned int vcomoff_time_ms)
{
	int stat;
	int i;

	*sess = kzalloc(sizeof(**sess), GFP_KERNEL);
	if (!*sess)
		return -ENOMEM;

	if ((dwell_time_ms < 100) || (dwell_time_ms > 10*1000)) {
		pr_warning("pmic: Invalid dwell time %u ms given, ignoring.\n",
							dwell_time_ms);
		dwell_time_ms = PMIC_DEFAULT_DWELL_TIME_MS;
	}

	(*sess)->powered = false;
	(*sess)->dwell_time_ms = dwell_time_ms;
	(*sess)->vcomoff_time_ms = vcomoff_time_ms;

	INIT_DELAYED_WORK(&(*sess)->powerdown_work, pmic_powerdown_execute);
	INIT_DELAYED_WORK(&(*sess)->vcomoff_work, pmic_vcomoff_execute);

	/* find the proper driver */
	i = 0;
	do {
		(*sess)->drv = pmic_drivers[i++];
		if ((*sess)->drv && !strcmp((*sess)->drv->id, id))
			break;
	} while ((*sess)->drv);

	if (!(*sess)->drv) {
		pr_err("pmic: Could not find driver %s\n", id);
		stat = -EINVAL;
		goto free_sess;
	}

	stat = (*sess)->drv->hw_init(*sess, opt);

	if (stat)
		goto free_sess;

	return 0;

free_sess:
	kfree(*sess);

	return stat;
}


void pmic_remove(struct pmic_sess **sess)
{
	cancel_delayed_work_sync(&(*sess)->powerdown_work);
	cancel_delayed_work_sync(&(*sess)->vcomoff_work);

	(*sess)->drv->hw_cleanup(*sess);

	kfree(*sess);
	*sess = 0;
}


int pmic_set_vcom(struct pmic_sess *sess, int vcom_mv)
{
	if ((vcom_mv < sess->drv->vcom_min) || (vcom_mv > sess->drv->vcom_max))
		return -EINVAL;

	if (sess->drv->set_vcom_voltage)
		return sess->drv->set_vcom_voltage(sess, vcom_mv);
	else
		return -ENOSYS;
}


static int pmic_vcom_switch(struct pmic_sess *sess, bool state)
{
	int st = 0;

	if (sess->drv->hw_vcom_switch)
		st = sess->drv->hw_vcom_switch(sess, state);

	return st;
}

int pmic_set_dvcom(struct pmic_sess *sess,int state)
{
        int st = 0;
        if (sess->drv->hw_set_dvcom)
                st = sess->drv->hw_set_dvcom(sess, state);

        return st;
}

int pmic_req_powerup(struct pmic_sess *sess)
{
	int stat = 0;

	/* we don't want the timer running while power is up */
	cancel_delayed_work_sync(&sess->powerdown_work);

	cancel_delayed_work_sync(&sess->vcomoff_work);

	mb();

	if (sess->powered)
		return 0;

	if (sess->drv->hw_power_ack(sess)) {
		int i;

		/* hardware is still switching power off, so wait it */
		for (i = 0; i < PMIC_POWERDOWN_TIMEOUT_MS; i += 10) {
			msleep(10);
			if (!sess->drv->hw_power_ack(sess))
				break;
		}
		if (sess->drv->hw_power_ack(sess))
			stat = -EAGAIN;
	}

	if (stat)
		return stat;

	sess->drv->hw_power_req(sess, true);

	return 0;
}


int pmic_sync_powerup(struct pmic_sess *sess)
{
	int stat = 0;

	if (!sess->drv->hw_power_ack(sess)) {
		int i;

		for (i = 0; i < PMIC_POWERUP_TIMEOUT_MS; i += 10) {
			msleep(10);
			if (sess->drv->hw_power_ack(sess))
				break;
		}

		if (!sess->drv->hw_power_ack(sess))
			stat = -EAGAIN;
	}

	if (stat)
		return stat;

	stat = pmic_vcom_switch(sess, true);

	sess->powered = true;

	return stat;
}

int pmic_get_temperature(struct pmic_sess *sess, int *t)
{
	if (sess->drv->hw_read_temperature)
		return sess->drv->hw_read_temperature(sess, t);
	else
		return -ENOSYS;
}

void pmic_release_powerup_req(struct pmic_sess *sess)
{
	long timeout_jif;

	if (sess->vcomoff_time_ms) {
		timeout_jif = sess->vcomoff_time_ms * HZ / 1000;
		schedule_delayed_work(&sess->vcomoff_work, timeout_jif);
	} else {//
		pmic_vcom_switch(sess, false);
	}

	timeout_jif = sess->dwell_time_ms * HZ / 1000;
	schedule_delayed_work(&sess->powerdown_work, timeout_jif);
}


bool pmic_standby_dwell_time_ready(struct pmic_sess *sess)
{
	bool d = true;
	if (sess->drv->hw_standby_dwell_time_ready)
		d = sess->drv->hw_standby_dwell_time_ready(sess);

	return d;
}

void pmic_pm_sleep(struct pmic_sess *sess)
{
	if (sess->drv->hw_pm_sleep)
		sess->drv->hw_pm_sleep(sess);
}

void pmic_pm_resume(struct pmic_sess *sess)
{
	if (sess->drv->hw_pm_resume)
		sess->drv->hw_pm_resume(sess);
}
