/*
 * PMIC management for epaper power control HAL
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

#ifndef PMIC_H
#define PMIC_H

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/workqueue.h>

struct pmic_sess;

#define PMIC_DEFAULT_DWELL_TIME_MS	1111
#define PMIC_DEFAULT_VCOMOFF_TIME_MS	20

#if defined(FB_OMAP3EP_PAPYRUS_PM_VZERO)
  #define PAPYRUS_STANDBY_DWELL_TIME	4 /*sec*/
#else
  #define PAPYRUS_STANDBY_DWELL_TIME	0
#endif

#define WAKEUP_GPIO		(87)	/* active high */
#define EN_CPLD_POW_GPIO	(85)	/* active high */
#define CPLD_V_DET1_GPIO	(78)
#define CPLD_V_DET2_GPIO	(79)

struct pmic_driver {
	const char *id;

	int vcom_min;
	int vcom_max;
	int vcom_step;

	int (*hw_read_temperature)(struct pmic_sess *sess, int *t);
	bool (*hw_power_ack)(struct pmic_sess *sess);
	void (*hw_power_req)(struct pmic_sess *sess, bool up);
	int (*set_vcom_voltage)(struct pmic_sess *sess, int vcom_mv);

	int (*hw_vcom_switch)(struct pmic_sess *sess, bool state);

	int (*hw_set_dvcom)(struct pmic_sess *sess, int state);

	int (*hw_init)(struct pmic_sess *sess, char *opt);
	void (*hw_cleanup)(struct pmic_sess *sess);

	bool (*hw_standby_dwell_time_ready)(struct pmic_sess *sess);
	void (*hw_pm_sleep)(struct pmic_sess *sess);
	void (*hw_pm_resume)(struct pmic_sess *sess);


};

struct pmic_sess {
	bool powered;
	struct delayed_work powerdown_work;
	unsigned int dwell_time_ms;
	struct delayed_work vcomoff_work;
	unsigned int vcomoff_time_ms;
	const struct pmic_driver *drv;
	void *drvpar;
};

extern int pmic_probe(struct pmic_sess **sess, const char *id, char *opt,
					unsigned int dwell_time_ms,
					unsigned int vcomoff_time_ms);

extern void pmic_remove(struct pmic_sess **sess);

/*
 * Set VCOM voltage, in millivolts.
 * NB! Change will take effect with the next pmic power up!
 */
extern int pmic_set_vcom(struct pmic_sess *sess, int vcom_mv);

/*Runtame Change VCOM state */
extern int pmic_set_dvcom(struct pmic_sess *sess,int state);

/* Request asynchronous power up. */
extern int pmic_req_powerup(struct pmic_sess *sess);

/* Ensure that power is OK. Called after a pmic_req_powerup(). */
extern int pmic_sync_powerup(struct pmic_sess *sess);

/* Get pmic temperature sensor measurement. */
extern int pmic_get_temperature(struct pmic_sess *sess, int *t);

/*
 * Asynchronously release the power up requirement. Power may go down
 * up to a few seconds after this call in order to avoid power up/down
 * cycles with frequent screen updates.
 */
extern void pmic_release_powerup_req(struct pmic_sess *sess);

/* Get that enough delay between standby and sleep*/
extern bool pmic_standby_dwell_time_ready(struct pmic_sess *sess);

extern void pmic_pm_sleep(struct pmic_sess *sess);

extern void pmic_pm_resume(struct pmic_sess *sess);


#endif	/* PMIC_H */

