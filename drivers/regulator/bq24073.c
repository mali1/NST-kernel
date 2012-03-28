/*
 * Support for TI bq24073 (bqTINY-II) Dual Input (USB/AC Adpater)
 * 1-Cell Li-Ion Charger connected via GPIOs.
 *
 * Copyright (c) 2008 Philipp Zabel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/ctype.h>

/* Power regulator thresholds. */
#define CURRENT_TRH_100MA 		(100000)
#define CURRENT_TRH_500MA 		(500000)

/* Power regulator modes. */
#define CURRENT_MAX_100MA 		(0x00)
#define CURRENT_MAX_500MA 		(0x01)
#define CURRENT_MAX_SET_EXTERNAL	(0x02)
#define CURRENT_MAX_SUSPEND		(0x03)

#define EN1_MASK	(1 << 0)
#define EN2_MASK	(1 << 1)


static int bq24073_set_current_limit(struct regulator_dev *rdev,
					int min_uA, int max_uA)
{
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);

    if (pdata->force) {
        min_uA = pdata->force_current;
    }
    if (min_uA == 0) {
        pdata->gpio_en1_state = !!(CURRENT_MAX_SUSPEND & EN1_MASK);
        pdata->gpio_en2_state = !!(CURRENT_MAX_SUSPEND & EN2_MASK);
    }
	else if (min_uA <= CURRENT_TRH_100MA) {
		pdata->gpio_en1_state = !!(CURRENT_MAX_100MA & EN1_MASK);
		pdata->gpio_en2_state = !!(CURRENT_MAX_100MA & EN2_MASK);
	} else if (min_uA <= CURRENT_TRH_500MA) {
		pdata->gpio_en1_state = !!(CURRENT_MAX_500MA & EN1_MASK);
		pdata->gpio_en2_state = !!(CURRENT_MAX_500MA & EN2_MASK);
	} else {
		pdata->gpio_en1_state = !!(CURRENT_MAX_SET_EXTERNAL & EN1_MASK);
		pdata->gpio_en2_state = !!(CURRENT_MAX_SET_EXTERNAL & EN2_MASK);
	}

	if (!pdata->gpio_en1_state) gpio_set_value(pdata->gpio_en1, pdata->gpio_en1_state);
	gpio_set_value(pdata->gpio_en2, pdata->gpio_en2_state);
    gpio_set_value(pdata->gpio_en1, pdata->gpio_en1_state);

    if (!pdata->init_en) {
        pdata->init_en = 1;
        gpio_direction_output(pdata->gpio_en1, pdata->gpio_en1_state);
        gpio_direction_output(pdata->gpio_en2, pdata->gpio_en2_state);
    }
    if (!pdata->init_nce) {
        pdata->init_nce = 1;
        gpio_direction_output(pdata->gpio_nce, pdata->gpio_nce_state);
    }


	return 0;
}

static int bq24073_get_current_limit(struct regulator_dev *rdev)
{
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);
	u8 curr_lim;
	int ret;

	curr_lim = pdata->gpio_en2_state;
	curr_lim = curr_lim << 1;
	curr_lim |= pdata->gpio_en1_state;

	switch (curr_lim) {
	case CURRENT_MAX_100MA:
		ret = 100000;
		break;
	case CURRENT_MAX_500MA:
		ret = 500000;
		break;
	case CURRENT_MAX_SET_EXTERNAL:
		ret = 1500000;
		break;
	case CURRENT_MAX_SUSPEND:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int bq24073_enable(struct regulator_dev *rdev)
{
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);

	dev_dbg(rdev_get_dev(rdev), "enabling charger\n");
	pdata->gpio_nce_state = 0;
	gpio_set_value(pdata->gpio_nce, 0);

    if (!pdata->init_nce) {
        pdata->init_nce = 1;
        gpio_direction_output(pdata->gpio_nce, pdata->gpio_nce_state);
    }

	return 0;
}

static int bq24073_disable(struct regulator_dev *rdev)
{
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);

    if (!pdata->init_nce) {
        return 0;
    }

	dev_dbg(rdev_get_dev(rdev), "disabling charger\n");

    pdata->gpio_nce_state = 1;
	gpio_set_value(pdata->gpio_nce, 1);
	return 0;
}

static int bq24073_is_enabled(struct regulator_dev *rdev)
{
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);

	return !(pdata->gpio_nce_state);
}

static int bq24073_set_suspend_enable(struct regulator_dev *rdev)
{
	int ret = 0;
	struct bq24073_mach_info *pdata = rdev_get_drvdata(rdev);

	ret |= gpio_direction_output(pdata->gpio_nce, pdata->gpio_nce_state);
	ret |= gpio_direction_output(pdata->gpio_en1, pdata->gpio_en1_state);
	ret |= gpio_direction_output(pdata->gpio_en2, pdata->gpio_en2_state);

	return ret;
}

static int bq24073_set_suspend_disable(struct regulator_dev *rdev)
{
	return 0;
}

static ssize_t bq24073_force_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct regulator_dev *bq24073 = dev_get_drvdata(dev);
    struct bq24073_mach_info *pdata = rdev_get_drvdata(bq24073);
    if (pdata->force) {
        return sprintf(buf, "%d\n", pdata->force_current);
    }
    return sprintf(buf, "auto\n");
}

static ssize_t bq24073_force_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count) {
    struct regulator_dev *bq24073 = dev_get_drvdata(dev);
    struct bq24073_mach_info *pdata = rdev_get_drvdata(bq24073);
    if (count>1) { 
        pdata->force_current = simple_strtol(buf,NULL,0);
        pdata->force = (pdata->force_current < 0 || (pdata->force_current==0 && isalpha(buf[0])) ) ? 0:1;
        if (pdata->force) {
            bq24073_set_current_limit(bq24073,pdata->force_current,0); 
        }
    }
	return count;
}

static DEVICE_ATTR(force_current, S_IRUGO|S_IWUSR, bq24073_force_show, bq24073_force_store);


static struct regulator_ops bq24073_ops = {
	.set_current_limit = bq24073_set_current_limit,
	.get_current_limit = bq24073_get_current_limit,
	.enable            = bq24073_enable,
	.disable           = bq24073_disable,
	.is_enabled        = bq24073_is_enabled,
	.set_suspend_enable = bq24073_set_suspend_enable,
	.set_suspend_disable = bq24073_set_suspend_disable,
};

static struct regulator_desc bq24073_desc = {
	.name  = "bq24073",
	.ops   = &bq24073_ops,
	.type  = REGULATOR_CURRENT,
};

static int __init bq24073_probe(struct platform_device *pdev)
{
	struct regulator_init_data *i_pdata = pdev->dev.platform_data;
	struct bq24073_mach_info *pdata = (struct bq24073_mach_info *)(i_pdata->driver_data);
	struct regulator_dev *bq24073;
	int ret;

	if (!pdata || !pdata->gpio_nce || !pdata->gpio_en1 || !pdata->gpio_en2)
		return -EINVAL;

	ret = gpio_request(pdata->gpio_nce, "ncharge_en");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request nCE GPIO: %d\n",
			pdata->gpio_nce);
		goto err_ce;
	}
	ret = gpio_request(pdata->gpio_en1, "charge_mode_in1");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request EN1 GPIO: %d\n",
			pdata->gpio_en1);
		goto err_en1;
	}
	ret = gpio_request(pdata->gpio_en2, "charge_mode_in2");
	if (ret) {
		dev_dbg(&pdev->dev, "couldn't request EN2 GPIO: %d\n",
			pdata->gpio_en2);
		goto err_en2;
	}
    pdata->init_nce = 0;
    pdata->init_en = 0;

	bq24073 = regulator_register(&bq24073_desc, &pdev->dev,
					pdata);
	if (IS_ERR(bq24073)) {
		dev_dbg(&pdev->dev, "couldn't register regulator\n");
		ret = PTR_ERR(bq24073);
		goto err_reg;
	}

    platform_set_drvdata(pdev, bq24073);
	dev_dbg(&pdev->dev, "registered regulator\n");

    ret = device_create_file(&pdev->dev, &dev_attr_force_current);
    if (ret) {
        printk(KERN_ERR "Failed to create force_current sysfs entry\n");
        goto err_attr;
    }

	return 0;
err_attr:
	regulator_unregister(bq24073);
err_reg:
	gpio_free(pdata->gpio_en2);
err_en2:
	gpio_free(pdata->gpio_en1);
err_en1:
	gpio_free(pdata->gpio_nce);
err_ce:
	return ret;
}

static int __devexit bq24073_remove(struct platform_device *pdev)
{
	struct bq24073_mach_info *pdata = pdev->dev.platform_data;
	struct regulator_dev *bq24073 = platform_get_drvdata(pdev);
    device_remove_file(&pdev->dev, &dev_attr_force_current);


	regulator_unregister(bq24073);
	gpio_free(pdata->gpio_en1);
	gpio_free(pdata->gpio_en2);
	gpio_free(pdata->gpio_nce);

	return 0;
}

static struct platform_driver bq24073_driver = {
	.driver = {
		.name = "bq24073",
	},
	.remove = __devexit_p(bq24073_remove),
};

static int __init bq24073_init(void)
{
	return platform_driver_probe(&bq24073_driver, bq24073_probe);
}

static void __exit bq24073_exit(void)
{
	platform_driver_unregister(&bq24073_driver);
}

module_init(bq24073_init);
module_exit(bq24073_exit);

MODULE_AUTHOR("Philipp Zabel");
MODULE_DESCRIPTION("TI bq24073 Li-Ion Charger driver");
MODULE_LICENSE("GPL");
