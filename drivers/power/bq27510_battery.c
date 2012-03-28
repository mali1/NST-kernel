/*
 * bq27510 battery driver
 *
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 * Copyright (C) 2010 Konstantin Motov <kmotov@mm-sol.com>
 * Copyright (C) 2010 Dimitar Dimitrov <dddimitrov@mm-sol.com>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <linux/i2c/twl4030-madc.h>
#include <asm/unaligned.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DRIVER_VERSION			"1.0.1"

/*
 * Polling interval, in milliseconds.
 *
 * It is expected that each power supply will call power_supply_changed(),
 * thus alleviating user-space from the need to poll. But BQ27510 cannot
 * raise an interrupt so we're forced to issue change events regularly.
 */
#define T_POLL_MS			30000
/*
 * Because the battery may not report accurate status on the first
 * poll we check every 500ms for the first 5 seconds. 5 seconds
 * was empirically determined to be an ok interval for keeping
 * the charge status accurate-ish
 */
#define T_POLL_PLUG_MS        500 // ms
#define T_POLL_PLUG_MAX        10 // iterations

#define USB_CURRENT_LIMIT_LOW		100000   /* microAmp */
#define USB_CURRENT_LIMIT_HIGH		500000   /* microAmp */
#define AC_CURRENT_LIMIT		1500000  /* microAmp */

#define BQ72520_REG_CONTROL     (0x00)
#define BQ72520_CONTROL_DEVICE_TYPE   (0x0001)
#define BQ72520_CONTROL_FW_VERSION   (0x0002)
#define BQ72520_CONTROL_HW_VERSION   (0x0003)


#define BQ27510_REG_TEMP		0x06
#define BQ27510_REG_VOLT		0x08
#define BQ27510_REG_RSOC		0x2C /* Relative State-of-Charge */
#define BQ27510_REG_AI			0x14
#define BQ27510_REG_FLAGS		0x0A
#define BQ27510_REG_TTE			0x16
#define BQ27510_REG_TTF			0x18
#define BQ27510_REG_FCC			0x12
#if defined(CONFIG_BATTERY_BQ27520)
#define BQ27510_REG_SOH			0x28
#define BQ27510_REG_DATALOG_INDEX	0x32
#define BQ27510_REG_DATALOG_BUFFER	0x34
#define BQ27510_REG_NOMINAL_CAPACITY	0x0C
#endif /* CONFIG_BATTERY_BQ27520 */
#define BAT_DET_FLAG_BIT		3
#define OFFSET_KELVIN_CELSIUS		273
#define OFFSET_KELVIN_CELSIUS_DECI	2731
#define KELVIN_SCALE_RANGE		10
#define CURRENT_OVF_THRESHOLD		((1 << 15) - 1)

#define FLAG_BIT_DSG			0
#define FLAG_BIT_SOCF			1
#define FLAG_BIT_SOC1			2
#define FLAG_BIT_BAT_DET		3
#define FLAG_BIT_WAIT_ID		4
#define FLAG_BIT_OCV_GD			5
#define FLAG_BIT_CHG			8
#define FLAG_BIT_FC				9
#define FLAG_BIT_XCHG			10
#define FLAG_BIT_CHG_INH		11
#define FLAG_BIT_OTD			14
#define FLAG_BIT_OTC			15

#define to_bq27510_device_info(x) container_of((x), \
				struct bq27510_device_info, bat);
#define to_bq27510_device_usb_info(x) container_of((x), \
				struct bq27510_device_info, usb);
#define to_bq27510_device_wall_info(x) container_of((x), \
				struct bq27510_device_info, wall);
				
/* If the system has several batteries we need a different name for each
 * of them...
 */
static DEFINE_IDR(battery_id);
static DEFINE_MUTEX(battery_mutex);

struct bq27510_device_info;
struct bq27510_access_methods {
	int (*read)(u8 reg, int *rt_value, int b_single,
		struct bq27510_device_info *di);
};

struct bq27510_device_info {
	struct device 			*dev;
	int				id;
	int				voltage_uV;
	int				current_uA;
	int				temp_C;
	int				charge_rsoc;
	int                     	time_to_empty;
	int                     	time_to_full;
    int rapid_poll_cycle;
	struct bq27510_access_methods	*bus;
	struct power_supply		bat;
	struct power_supply		usb;
	struct power_supply		wall;

	struct i2c_client		*client;
};
static int bq27200_read_control(u16 reg, int *rt_value, struct bq27510_device_info *di);
static int bq27510_battery_supply_prop_present(struct bq27510_device_info *di);

static struct delayed_work bat_work;

#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
enum {
	BATTERY_MCNAIR= 0,
	BATTERY_LICO = 1,
	BATTERY_ABSENT = 2,
	BATTERY_UNKNOWN = 3,
	BATTERY_NUM = 4
};

static const char * const manufacturer_name[BATTERY_NUM] = {
	"McNair",
	"Lico",
	"Absent",
	"Unknown"
};

/* ID voltages in uV */
static const int battery_id_min[BATTERY_NUM-1] = {
	290000,
	690000,
	1425000
};

static const int battery_id_max[BATTERY_NUM-1] = {
	400000,
	810000,
	1575000
};

static int manufacturer_id;
#endif

static enum power_supply_property bq27510_battery_props[] = {
	/* Battery status - see POWER_SUPPLY_STATUS_* */
	POWER_SUPPLY_PROP_STATUS,
	/* Battery health - see POWER_SUPPLY_HEALTH_* */
	POWER_SUPPLY_PROP_HEALTH,
	/* Battery technology - see POWER_SUPPLY_TECHNOLOGY_* */
	POWER_SUPPLY_PROP_TECHNOLOGY,
	/* Boolean 1 -> battery detected, 0 battery not inserted. */
	POWER_SUPPLY_PROP_PRESENT,
	/* Measured Voltage cell pack in mV. */
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	/* 
	 * Signed measured average current (1 sec) in mA.
	 * Negative means discharging, positive means charging.
	 */
#ifdef CONFIG_BATTERY_BQ27520
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_STATE_OF_HEALTH,
	POWER_SUPPLY_PROP_DATALOG_INDEX,
	POWER_SUPPLY_PROP_DATALOG_BUFFER,
	POWER_SUPPLY_PROP_NOMINAL_CAPACITY,
#endif /* CONFIG_BATTERY_BQ27520 */
	POWER_SUPPLY_PROP_CURRENT_NOW,
	/*
	 * Predicted remaining battery capacity expressed 
	 * as a percentage 0 - 100%.
	 */
	POWER_SUPPLY_PROP_CAPACITY,
	/* Battery temperature converted in Celsius. */
	POWER_SUPPLY_PROP_TEMP,
	/* 
	 * Time to discharge the battery in minutes based on the 
	 * average current. 65535 indicates charging cycle.
	 */
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	/* Time to recharge the battery in minutes based on the average current. 65535 indicates discharging cycle. */
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	/* Maximum battery charge */
	POWER_SUPPLY_PROP_CHARGE_FULL
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
	,
	POWER_SUPPLY_PROP_MANUFACTURER
#endif
};

static enum power_supply_property bq27510_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_AVG,
};

static enum power_supply_property bq27510_wall_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_AVG,
};

static int  bq27x10_type = POWER_SUPPLY_TYPE_BATTERY;
static struct bq27510_device_info *local_di = NULL;

void bq27x10_charger_type(int limit)
{
	bq27x10_type = POWER_SUPPLY_TYPE_BATTERY;
	if (limit == USB_CURRENT_LIMIT_HIGH)
		bq27x10_type = POWER_SUPPLY_TYPE_USB;
	if (limit == AC_CURRENT_LIMIT)
		bq27x10_type = POWER_SUPPLY_TYPE_MAINS;

	if (local_di)
	{
    	cancel_delayed_work_sync(&bat_work);
        local_di->rapid_poll_cycle = 0;
		schedule_delayed_work(&bat_work, msecs_to_jiffies(T_POLL_PLUG_MS));
	}
}
EXPORT_SYMBOL(bq27x10_charger_type);

static void bq27x10_bat_work(struct work_struct *work)
{
    int polling_interval = T_POLL_MS;

	if (local_di)
	{
		power_supply_changed(&local_di->bat);
		power_supply_changed(&local_di->usb);
		power_supply_changed(&local_di->wall);

        if (local_di->rapid_poll_cycle < T_POLL_PLUG_MAX) {
            polling_interval = T_POLL_PLUG_MS;
            ++local_di->rapid_poll_cycle;
        }
    }
	schedule_delayed_work(&bat_work, msecs_to_jiffies(polling_interval));
}

#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
static unsigned int g_pause_i2c = 1;
static ssize_t pause_i2c_show(struct device *dev, struct device_attribute *attr,
							  char *buf) {
	return sprintf(buf, "%u\n",  g_pause_i2c);
}

static ssize_t pause_i2c_store(struct device *dev,
							   struct device_attribute *attr,
							   const char *buf, size_t count) {
	if (count > 1) {
		if (buf[0] == '0')
			g_pause_i2c = 0;
		else
			g_pause_i2c = 1;
	}
	return count;
}

static struct device_attribute dev_attr_pause_i2c = {
	.attr	= {
		.name = "pause_i2c",
		.mode = 0777
		},
	.show = pause_i2c_show,
	.store = pause_i2c_store

};
#endif
/*
 * Common code for bq27x10 devices
 */
static int bq27x10_read(u8 reg, int *rt_value, int b_single,
			struct bq27510_device_info *di)
{
	int ret;
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
	if (g_pause_i2c == 0)
		return -1;
#endif
	ret = di->bus->read(reg, rt_value, b_single, di);
	return ret;
}

/*
 * Return the battery temperature in 0.1 Kelvin degrees
 * Or < 0 if something fails.
 */
static int bq27510_battery_temperature(struct bq27510_device_info *di)
{
	int ret;
	int temp = 0;

	ret = bq27x10_read(BQ27510_REG_TEMP, &temp, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading temperature\n");
		return ret;
	}

	dev_dbg(di->dev, "temperature: %d [0.1K]\n", temp);
	return temp;
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
static int bq27510_battery_voltage(struct bq27510_device_info *di)
{
	int ret;
	int volt = 0;
	ret = bq27x10_read(BQ27510_REG_VOLT, &volt, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	}

	return volt;
}

/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
static int bq27510_battery_current(struct bq27510_device_info *di)
{
	int ret;
	int curr = 0;
	/*int flags = 0;*/

	ret = bq27x10_read(BQ27510_REG_AI, &curr, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading current\n");
		return 0;
	}

	/* In the BQ27510 convention, charging current is positive while discharging current is negative */
	return ((curr > CURRENT_OVF_THRESHOLD) ? (curr-((1 << 16) -1)):curr);
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
static int bq27510_battery_rsoc(struct bq27510_device_info *di)
{
	int ret;
	int rsoc = 0;

	ret = bq27x10_read(BQ27510_REG_RSOC, &rsoc, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading relative State-of-Charge\n");
		printk(KERN_INFO "gauge: %s read RSOC error ret=%d  SOC=%d\n", __FUNCTION__,ret,rsoc);
		return 100;
	}

    if (rsoc == 0 ) {
        if (!bq27510_battery_supply_prop_present(di)) {
            rsoc=100;
        }
    }
	if (rsoc < 0){
		rsoc = 1;
	}
	else if (rsoc > 100)
		rsoc = 100;

	return rsoc;
}

/*
 * Battery detected.
 * Rerturn true when batery is present
 */
static int bq27510_battery_supply_prop_present(struct bq27510_device_info *di)
{
	int ret;
	int bat_det_flag = 0;

	ret = bq27x10_read(BQ27510_REG_FLAGS, &bat_det_flag, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	} 
	return (bat_det_flag & (1 << FLAG_BIT_BAT_DET)) ? 1 : 0;
}

/*
 * Return predicted remaining battery life at the present rate of discharge,
 * in minutes.
 */
static int bq27510_battery_time_to_empty_now(struct bq27510_device_info *di)
{
	int ret;
	int tte = 0;

	ret = bq27x10_read(BQ27510_REG_TTE, &tte, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	}
	return tte;
}

/*
 * Return predicted remaining time until the battery reaches full charge,
 * in minutes
  */
static int bq27510_battery_time_to_full_now(struct bq27510_device_info *di)
{
	int ret;
	int ttf = 0;

	ret = bq27x10_read(BQ27510_REG_TTF, &ttf, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	}
	return ttf;
}

/*
 * Returns the compensated capacity of the battery when fully charged.
 * Units are mAh
  */
static int bq27510_battery_max_level(struct bq27510_device_info *di)
{
	int ret;
	int bml = 0;

	ret = bq27x10_read(BQ27510_REG_FCC, &bml, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading voltage\n");
		return ret;
	}
	return bml;
}

/*
 * Returns the state of health for battery
 * Units are %
  */
#ifdef CONFIG_BATTERY_BQ27520
static int bq27510_battery_health_percent(struct bq27510_device_info *di)
{
	int ret;
	int bml = 0;

	ret = bq27x10_read(BQ27510_REG_SOH, &bml, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading state of health\n");
		return ret;
	}

	return bml;
}

static int bq27510_battery_datalog_index(struct bq27510_device_info *di)
{
	int ret;
	int bml = 0;

	ret = bq27x10_read(BQ27510_REG_DATALOG_INDEX, &bml, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading datalog index\n");
		return ret;
	}
	return bml;
}

static int bq27510_battery_datalog_buffer(struct bq27510_device_info *di)
{
	int ret;
	int bml = 0;

	ret = bq27x10_read(BQ27510_REG_DATALOG_BUFFER, &bml, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading datalog buffer\n");
		return ret;
	}
	return bml;
}

static int bq27510_battery_nominal_capacity(struct bq27510_device_info *di)
{
	int ret;
	int bml = 0;

	ret = bq27x10_read(BQ27510_REG_NOMINAL_CAPACITY, &bml, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading nominal capacity\n");
		return ret;
	}
	return bml;
}
#endif /* CONFIG_BATTERY_BQ27520 */

static int bq27510_battery_status(struct bq27510_device_info *di)
{
	int ret, curr;
	int flags = 0;

	ret = bq27x10_read(BQ27510_REG_FLAGS, &flags, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading status flags (%d)\n", ret);
		return ret;
	}

	curr = bq27510_battery_current(di);

	dev_dbg(di->dev, "Flags=%04x\n", flags);

	if (flags & (1u << FLAG_BIT_FC))
		ret = POWER_SUPPLY_STATUS_FULL;
	else if ((flags & (1u << FLAG_BIT_DSG)) && (curr < 0))
		ret = POWER_SUPPLY_STATUS_DISCHARGING;
	else if ((flags & (1u << FLAG_BIT_CHG)) && (curr > 0))
		ret = POWER_SUPPLY_STATUS_CHARGING;
	else
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;

	return ret;
}

static int bq27510_battery_health(struct bq27510_device_info *di)
{
	int ret;
	int flags = 0;

	ret = bq27x10_read(BQ27510_REG_FLAGS, &flags, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading health flags (%d)\n", ret);
		return ret;
	}

	if (flags & (1u << FLAG_BIT_OTC))
		ret = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (flags & (1u << FLAG_BIT_OTD))
		ret = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (flags & ((1u << FLAG_BIT_XCHG) | (1u << FLAG_BIT_CHG_INH))) {
		int t = bq27510_battery_temperature(di);
		if (t < OFFSET_KELVIN_CELSIUS_DECI)
			ret = POWER_SUPPLY_HEALTH_COLD;
		else
			ret = POWER_SUPPLY_HEALTH_OVERHEAT;
	} else
		ret = POWER_SUPPLY_HEALTH_GOOD;

	return ret;
}

/*
 * Return reuired battery property or error.
  */
static int bq27510_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = -EINVAL;

	struct bq27510_device_info *di = to_bq27510_device_info(psy);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = bq27510_battery_status(di);
		ret = 0;
		if (val->intval < 0)
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = bq27510_battery_health(di);
		ret = 0;
		if (val->intval < 0)
			val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		ret = val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* return voltage in uV */
		ret = val->intval = bq27510_battery_voltage(di) * 1000;
		break;
#ifdef CONFIG_BATTERY_BQ27520
	case POWER_SUPPLY_PROP_DATALOG_INDEX:
		val->intval = bq27510_battery_datalog_index(di);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_DATALOG_BUFFER:
		val->intval = bq27510_battery_datalog_buffer(di);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_STATE_OF_HEALTH:
		val->intval = bq27510_battery_health_percent(di);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_NOMINAL_CAPACITY:
		val->intval = bq27510_battery_nominal_capacity(di);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
#endif /* CONFIG_BATTERY_BQ27520 */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		/* return positive current in uA */
		val->intval = bq27510_battery_current(di) * 1000;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = val->intval = bq27510_battery_rsoc(di);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = val->intval = bq27510_battery_temperature(di);
		/* convert from 0.1K to C */
		val->intval = ((val->intval/KELVIN_SCALE_RANGE) - OFFSET_KELVIN_CELSIUS);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = bq27510_battery_supply_prop_present(di);
		/* Report an absent battery if we can't reach the BQ chip. */
		if (val->intval < 0)
			val->intval = 0;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = val->intval = bq27510_battery_time_to_empty_now(di);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:		
		ret = val->intval = bq27510_battery_time_to_full_now(di);
		break;	
	case POWER_SUPPLY_PROP_CHARGE_FULL:		
		/* present capacity in uAh */
		ret = val->intval = bq27510_battery_max_level(di) * 1000;
		break;	
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = manufacturer_name[manufacturer_id];
		ret = 0;
		break;
#endif
	default:
		return -EINVAL;
	}

	ret = (ret < 0) ? ret : 0;

	return ret;
}
/*
 * Return reuired battery property or error.
  */
static int bq27510_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (bq27x10_type == POWER_SUPPLY_TYPE_USB);
		ret = 0;
		break;
    case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = USB_CURRENT_LIMIT_HIGH;
		ret = 0;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

/*
 * Return reuired battery property or error.
  */
static int bq27510_wall_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (bq27x10_type == POWER_SUPPLY_TYPE_MAINS);
		ret = 0;
		break;
    case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = AC_CURRENT_LIMIT;
		ret = 0;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
static int get_gossamer_battery_manufacturer(void)
{
	struct twl4030_madc_request req;
	int temp, i, type;

	req.channels = 1;
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	twl4030_madc_conversion(&req);
	temp = (u16)req.rbuf[0];
	temp = temp*1500000/1023; /* Accurate value in uV */

	type = BATTERY_UNKNOWN;
	for (i = 0; i < BATTERY_NUM-1; i++) {
		if ((temp > battery_id_min[i]) &&
		    (temp < battery_id_max[i])) {
			type = i;
			break;
		}
	}
	return type;
}
#endif

static int bq27200_read_control(u16 reg, int *rt_value, struct bq27510_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[4];
	int err;

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = data;

	data[0] = BQ72520_REG_CONTROL;
    data[1] = reg & 0xff;
    data[2] = (reg >> 8) & 0xff;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
        msleep(2);
        msg->addr = client->addr;
        msg->flags = 0;
        msg->len = 1;
        msg->buf = data;

        data[0] = BQ72520_REG_CONTROL;
        err = i2c_transfer(client->adapter, msg, 1);

        if (err >= 0) {
            msg->len = 2;

            msg->flags = I2C_M_RD;
            err = i2c_transfer(client->adapter, msg, 1);
            if (err >= 0) {
                    *rt_value = data[0] | (data[1]<<8);
                return 0;
            }
        }
	}
	return err;
}

static ssize_t bq275200_get_device_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct bq27510_device_info *di = dev_get_drvdata(dev->parent);
    int rt=0;
    bq27200_read_control(BQ72520_CONTROL_DEVICE_TYPE, &rt, di);
    return sprintf(buf, "0x%04x\n", rt);
}
static DEVICE_ATTR(device_type, S_IRUGO, bq275200_get_device_type, NULL);

static ssize_t bq275200_get_fw_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct bq27510_device_info *di = dev_get_drvdata(dev->parent);
    int rt=0;
    bq27200_read_control(BQ72520_CONTROL_FW_VERSION, &rt, di);
    return sprintf(buf, "0x%04x\n", rt);
}
static DEVICE_ATTR(fw_version, S_IRUGO, bq275200_get_fw_version, NULL);

static ssize_t bq275200_get_hw_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct bq27510_device_info *di = dev_get_drvdata(dev->parent);
    int rt=0;
    bq27200_read_control(BQ72520_CONTROL_HW_VERSION, &rt, di);
    return sprintf(buf, "0x%04x\n", rt);
}

static DEVICE_ATTR(hw_version, S_IRUGO, bq275200_get_hw_version, NULL);




/*
 * init batery descriptor.
  */
static void bq27510_powersupply_init(struct bq27510_device_info *di)
{
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = bq27510_battery_props;
	di->bat.num_properties = ARRAY_SIZE(bq27510_battery_props);
	di->bat.get_property = bq27510_battery_get_property;
	di->bat.external_power_changed = NULL;
}

/*
 * init usb descriptor.
  */
static void bq27510_powersupply_usb_init(struct bq27510_device_info *di)
{
	di->usb.type = POWER_SUPPLY_TYPE_USB;
	di->usb.properties = bq27510_usb_props;
	di->usb.num_properties = ARRAY_SIZE(bq27510_usb_props);
	di->usb.get_property = bq27510_usb_get_property;
	di->usb.external_power_changed = NULL;
}

/*
 * init wall descriptor.
  */
static void bq27510_powersupply_wall_init(struct bq27510_device_info *di)
{
	di->wall.type = POWER_SUPPLY_TYPE_MAINS;
	di->wall.properties = bq27510_wall_props;
	di->wall.num_properties = ARRAY_SIZE(bq27510_wall_props);
	di->wall.get_property = bq27510_wall_get_property;
	di->wall.external_power_changed = NULL;
}

/*
 * Read by I2C.
 */
static int bq27510_read(u8 reg, int *rt_value, int b_single,
			struct bq27510_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[2];
	unsigned char data[2];
	int err;
	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    // Not the cleanest way for sure
    msg[1].len = (b_single) ? 1 : 2;
    msg[1].buf = data;

    // do this as single transfer in case
    // someone else is also trying to use the bus
    err = i2c_transfer(client->adapter, msg, 2);	

    // err is the number of messages executed,
    // we sent 2 and expect 2 back
    if (err == 2) {
        if(b_single) {
            *rt_value = data[0];
        } else {
		    *rt_value = data[0]+(data[1]<<8);
		}

        err = 0;
	}

	return err;
}

/*
 *
 */
static int bq27510_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	char *name;
	struct bq27510_device_info *di;
	struct bq27510_access_methods *bus;
	int num;
	int retval = 0;

	/* Get new ID for the new battery device */
	retval = idr_pre_get(&battery_id, GFP_KERNEL);
	if (retval == 0)
		return -ENOMEM;
	mutex_lock(&battery_mutex);
	retval = idr_get_new(&battery_id, client, &num);
	mutex_unlock(&battery_mutex);
	if (retval < 0)
		return retval;
	name = kasprintf(GFP_KERNEL, "bq27510-%d", num);
	if (!name) {
		dev_err(&client->dev, "failed to allocate device name\n");
		retval = -ENOMEM;
		goto batt_failed_1;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}
	di->id = num;

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate access method "
					"data\n");
		retval = -ENOMEM;
		goto batt_failed_3;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;

	di->bat.name = name;
	di->usb.name = "bq27510-usb";
	di->wall.name = "bq27510-wall";

	bus->read = bq27510_read;
	di->bus = bus;
	di->client = client;
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
	manufacturer_id = get_gossamer_battery_manufacturer();
#endif
	bq27510_powersupply_init(di);
	bq27510_powersupply_usb_init(di);
	bq27510_powersupply_wall_init(di);

	retval = power_supply_register(&client->dev, &di->bat);
	if (retval) {
		dev_err(&client->dev, "failed to register battery\n");
		goto batt_failed_4;
	}
	retval = power_supply_register(&client->dev, &di->usb);
	if (retval) {
		dev_err(&client->dev, "failed to register battery(usb)\n");
		power_supply_unregister(&di->bat);
		goto batt_failed_4;
	}
	retval = power_supply_register(&client->dev, &di->wall);
	if (retval) {
		dev_err(&client->dev, "failed to register battery(wall)\n");
		power_supply_unregister(&di->bat);
		power_supply_unregister(&di->usb);
		goto batt_failed_4;
	}

	local_di = di;

	INIT_DELAYED_WORK_DEFERRABLE(&bat_work, bq27x10_bat_work);
	schedule_delayed_work(&bat_work, msecs_to_jiffies(T_POLL_MS));

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
	retval = device_create_file(&client->dev, &dev_attr_pause_i2c);
	if (retval)
		printk(KERN_ERR "Failed to create pause_i2c sysfs entry\n");
#endif
    retval = device_create_file(di->bat.dev, &dev_attr_device_type);
    if (retval)
        printk(KERN_ERR "Failed to create evice_type sysfs entry\n");
    retval = device_create_file(di->bat.dev, &dev_attr_hw_version);
    if (retval)
        printk(KERN_ERR "Failed to create hw_version sysfs entry\n");
    retval = device_create_file(di->bat.dev, &dev_attr_fw_version);
    if (retval)
        printk(KERN_ERR "Failed to create fw_version sysfs entry\n");

    return 0;
batt_failed_4:
	kfree(bus);
batt_failed_3:
	kfree(di);
batt_failed_2:
	kfree(name);
batt_failed_1:
	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, num);
	mutex_unlock(&battery_mutex);

	return retval;
}

/*
 *
 */
static int bq27510_battery_remove(struct i2c_client *client)
{
	struct bq27510_device_info *di = i2c_get_clientdata(client);

    device_remove_file(di->bat.dev, &dev_attr_fw_version);
    device_remove_file(di->bat.dev, &dev_attr_hw_version);
    device_remove_file(di->bat.dev, &dev_attr_device_type);
#if defined(CONFIG_MACH_OMAP3621_GOSSAMER)
    device_remove_file(&client->dev, &dev_attr_pause_i2c);
#endif
	power_supply_unregister(&di->bat);
	power_supply_unregister(&di->usb);
	power_supply_unregister(&di->wall);

	kfree(di->bat.name);

	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, di->id);
	mutex_unlock(&battery_mutex);

	local_di = NULL;
	kfree(di);

	return 0;
}

/*
 * Module stuff
 */

static const struct i2c_device_id bq27510_id[] = {
	{ "bq27510", 0 },
	{},
};

static void bq27510_battery_shutdown(struct i2c_client *client)
{
    dev_dbg(&client->dev, "shutting down");
    cancel_delayed_work_sync(&bat_work);
}


static int bq27510_battery_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct bq27510_device_info *di = i2c_get_clientdata(client);
    int volts,temp,cap,curr;
#ifdef CONFIG_BATTERY_BQ27520
    int ncap;
    ncap = bq27510_battery_nominal_capacity(di);
#endif /* CONFIG_BATTERY_BQ27520 */

    volts = bq27510_battery_voltage(di);
    curr = bq27510_battery_current(di);

    temp= bq27510_battery_temperature(di)-2730;
    cap = bq27510_battery_rsoc(di);
    printk("batsuspend %d %d %d %d %d\n",volts,curr,cap,ncap, temp/10);

	return 0;
}

static int bq27510_battery_resume(struct i2c_client *client)
{
    struct bq27510_device_info *di = i2c_get_clientdata(client);
    int volts,temp,cap,curr;
#ifdef CONFIG_BATTERY_BQ27520
    int ncap;
    ncap = bq27510_battery_nominal_capacity(di);
#endif /* CONFIG_BATTERY_BQ27520 */

    volts = bq27510_battery_voltage(di);
    curr = bq27510_battery_current(di);

    temp= bq27510_battery_temperature(di)-2730;
    cap = bq27510_battery_rsoc(di);
    printk("batresume  %d %d %d %d %d\n",volts,curr,cap,ncap, temp/10);
	return 0;
}



static struct i2c_driver bq27510_battery_driver = {
	.driver = {
		.name = "bq27510-battery",
	},
	.probe = bq27510_battery_probe,
	.remove = bq27510_battery_remove,

    .suspend  = bq27510_battery_suspend,
    .resume	  = bq27510_battery_resume,

    .shutdown = bq27510_battery_shutdown,
	.id_table = bq27510_id,
};

static int __init bq27510_battery_init(void)
{
	int ret;

	ret = i2c_add_driver(&bq27510_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register BQ27510 driver\n");

	return ret;
}
module_init(bq27510_battery_init);

static void __exit bq27510_battery_exit(void)
{
	i2c_del_driver(&bq27510_battery_driver);
}
module_exit(bq27510_battery_exit);

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("BQ27510 battery monitor driver");
MODULE_LICENSE("GPL");
