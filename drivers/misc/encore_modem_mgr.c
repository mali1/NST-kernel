/*
 * Encore Modem Manager library
 *
 * Copyright (C) 2010 Intrinsyc Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board-boxer.h>


/*****************************************************************************
 * Configuration
 *****************************************************************************/

#define EMM_GPIO_MODEM_RST             50   /* active low  */
#define EMM_GPIO_MODEM_ON              34   /* active high */
#define EMM_GPIO_MODEM_NDISABLE        21   /* active high */

#define EMM_DEFAULT_DIR_CHANGE_DELAY    1   /* Delay in ms */


/*****************************************************************************
 * Logging/Debugging Configuration
 *****************************************************************************/

#define EMM_GPIO_DEBUG                  0   /* SYSFS for GPIOs? */
#define ENCORE_MODEM_MGR_DEBUG          1   /* Regular debug    */
#define ENCORE_MODEM_MGR_DEBUG_VERBOSE  1   /* Extra debug      */

#if ENCORE_MODEM_MGR_DEBUG
#define DEBUGPRINT(x...) printk(x)
#else  /* ENCORE_MODEM_MGR_DEBUG */
#define DEBUGPRINT(x...)
#endif /* ENCORE_MODEM_MGR_DEBUG */


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

static unsigned long int g_pulse_width_ON  = 0;
static unsigned long int g_pulse_width_RST = 0;


/*****************************************************************************
 * Logging/Debugging Helpers
 *****************************************************************************/

static char * getGpioLabel(unsigned int GpioID)
{
    switch(GpioID)
    {
        case EMM_GPIO_MODEM_ON:
            return "Modem ON";

        case EMM_GPIO_MODEM_NDISABLE:
            return "Modem nDISABLE";

        case EMM_GPIO_MODEM_RST:
            return "Modem RST";

        default:
            return "Unknown";
    }
}


/*****************************************************************************
 * Helpers - GPIO
 *****************************************************************************/

/*
 * Returns 0 if the given GPIO line is Low.
 * Returns 1 if the given GPIO line is High.
 */
static int GetGpioValue(unsigned int GpioID)
{
    int value = gpio_get_value(GpioID);

#if ENCORE_MODEM_MGR_DEBUG_VERBOSE
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: GPIO %02d (%s) state: %s\n", GpioID, getGpioLabel(GpioID), ((0 == value) ? "LOW" : "HIGH") );
#endif /* ENCORE_MODEM_MGR_DEBUG_VERBOSE */

    return ((0 == value) ? 0 : 1);
}

/*
 * Sets a GPIO Line to the given value.
 */
static void SetGpioValue(unsigned int GpioID, int newValue, bool restoreToInput)
{
    int actualNewValue = ((0 == newValue) ? 0 : 1);

#if ENCORE_MODEM_MGR_DEBUG_VERBOSE
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Setting GPIO %02d (%s) state to %s...\n", GpioID, getGpioLabel(GpioID), ((0 == actualNewValue) ? "LOW" : "HIGH") );
#endif /* ENCORE_MODEM_MGR_DEBUG_VERBOSE */

    gpio_direction_output(GpioID, actualNewValue);
    gpio_set_value(GpioID, actualNewValue);

    if (restoreToInput)
    {
        msleep(EMM_DEFAULT_DIR_CHANGE_DELAY);
        gpio_direction_input(GpioID);
    }
}

/*
 * Toggles the value of the given GPIO for the specified amount of time.
 */
static void PulseGpioValue(unsigned int GpioID, int pulseValue, unsigned long int numMillisecs, bool restoreToInput)
{
    int gpioValue = ((0 == pulseValue) ? 0 : 1);

    if (0 == numMillisecs)
    {
        DEBUGPRINT(KERN_INFO "encore_modem_mgr: Ignoring pulse request for GPIO %02d (%s) since the given duration is zero milliseconds.\n", GpioID, getGpioLabel(GpioID) );
    }
    else
    {
#if ENCORE_MODEM_MGR_DEBUG_VERBOSE
        DEBUGPRINT(KERN_INFO "encore_modem_mgr: Pulsing GPIO %02d (%s) %s for %ld milliseconds...\n", GpioID, getGpioLabel(GpioID), ((0 == gpioValue) ? "LOW" : "HIGH"), numMillisecs );
#endif /* ENCORE_MODEM_MGR_DEBUG_VERBOSE */

        SetGpioValue(GpioID, gpioValue, false);
        msleep(numMillisecs);
        gpioValue = ((0 == gpioValue) ? 1 : 0);
        SetGpioValue(GpioID, gpioValue, restoreToInput);
    }
}


/*****************************************************************************
 * SysFS Handlers
 *****************************************************************************/

/****** modem_on ******/

static ssize_t modem_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = GetGpioValue(EMM_GPIO_MODEM_ON);

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem ON) is currently %s\n", EMM_GPIO_MODEM_ON, ((0 == val) ? "LOW" : "HIGH") );

	return snprintf(buf, PAGE_SIZE, "%d\n", ((0 == val) ? 0 : 1));
}

static ssize_t modem_on_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem ON).\n", EMM_GPIO_MODEM_ON);
        retval = -EINVAL;
    }
    else
    {
        if (0 == strcmp(buf, "0\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem ON) state to OUTPUT LOW...\n", EMM_GPIO_MODEM_ON);
            SetGpioValue(EMM_GPIO_MODEM_ON, 0, false);
        }
        else if (0 == strcmp(buf, "1\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem ON) state to OUTPUT HIGH...\n", EMM_GPIO_MODEM_ON);
            SetGpioValue(EMM_GPIO_MODEM_ON, 1, false);
        }
        else
        {
            printk(KERN_ERR "encore_modem_mgr: Invalid value specified for GPIO %d (Modem ON): \"%s\"\n", EMM_GPIO_MODEM_ON, buf);
            retval = -EINVAL;
        }
    }

	return retval;
}

static DEVICE_ATTR(modem_on, S_IWUGO | S_IRUGO, modem_on_show, modem_on_store);


/****** modem_ndisable ******/

static ssize_t modem_ndisable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = GetGpioValue(EMM_GPIO_MODEM_NDISABLE);

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem nDISABLE) is currently %s\n", EMM_GPIO_MODEM_NDISABLE, ((0 == val) ? "LOW" : "HIGH") );

	return snprintf(buf, PAGE_SIZE, "%d\n", ((0 == val) ? 0 : 1));
}

static ssize_t modem_ndisable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem nDISABLE).\n", EMM_GPIO_MODEM_NDISABLE);
        retval = -EINVAL;
    }
    else
    {
        if (0 == strcmp(buf, "0\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem nDISABLE) state to OUTPUT LOW...\n", EMM_GPIO_MODEM_NDISABLE);

            gpio_direction_output(EMM_GPIO_MODEM_NDISABLE, 0);
            SetGpioValue(EMM_GPIO_MODEM_NDISABLE, 0, true);
        }
        else if (0 == strcmp(buf, "1\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem nDISABLE) state to INPUT...\n", EMM_GPIO_MODEM_NDISABLE);
            gpio_direction_input(EMM_GPIO_MODEM_NDISABLE);
        }
        else
        {
            printk(KERN_ERR "encore_modem_mgr: Invalid value specified for GPIO %d (Modem nDISABLE): \"%s\"\n", EMM_GPIO_MODEM_NDISABLE, buf);
            retval = -EINVAL;
        }
    }

	return retval;
}

static DEVICE_ATTR(modem_ndisable, S_IWUGO | S_IRUGO, modem_ndisable_show, modem_ndisable_store);


/****** modem_rst ******/

static ssize_t modem_rst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = GetGpioValue(EMM_GPIO_MODEM_RST);

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem RST) is currently %s\n", EMM_GPIO_MODEM_RST, ((0 == val) ? "LOW" : "HIGH") );

	return snprintf(buf, PAGE_SIZE, "%d\n", ((0 == val) ? 0 : 1));
}

static ssize_t modem_rst_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem RST).\n", EMM_GPIO_MODEM_RST);
        retval = -EINVAL;
    }
    else
    {
        if (0 == strcmp(buf, "0\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem RST) state to OUTPUT LOW...\n", EMM_GPIO_MODEM_RST);
            gpio_direction_output(EMM_GPIO_MODEM_RST, 0);
            SetGpioValue(EMM_GPIO_MODEM_RST, 0, true);
        }
        else if (0 == strcmp(buf, "1\n"))
        {
            DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - Setting GPIO %d (Modem RST) state to INPUT...\n", EMM_GPIO_MODEM_RST);
            gpio_direction_input(EMM_GPIO_MODEM_RST);
        }
        else
        {
            printk(KERN_ERR "encore_modem_mgr: Invalid value specified for GPIO %d (Modem RST): \"%s\"\n", EMM_GPIO_MODEM_RST, buf);
            retval = -EINVAL;
        }
    }

	return retval;
}

static DEVICE_ATTR(modem_rst, S_IWUGO | S_IRUGO, modem_rst_show, modem_rst_store);


/****** modem_on_pulse_width ******/

static ssize_t modem_on_pulse_width_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem ON) Pulse Width is currently %ld ms.\n", EMM_GPIO_MODEM_ON, g_pulse_width_ON);

	return snprintf(buf, PAGE_SIZE, "%ld\n", g_pulse_width_ON);
}

static ssize_t modem_on_pulse_width_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem ON).\n", EMM_GPIO_MODEM_ON);
        retval = -EINVAL;
    }
    else
    {
        /* We assume simple_strtoul gives us something useable */
        g_pulse_width_ON = simple_strtoul(buf, NULL, 10);

        DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem ON) Pulse Width is now %ld.\n", EMM_GPIO_MODEM_ON, g_pulse_width_ON);
    }

	return retval;
}

static DEVICE_ATTR(modem_on_pulse_width, S_IWUGO | S_IRUGO, modem_on_pulse_width_show, modem_on_pulse_width_store);


/****** modem_on_pulse ******/

static ssize_t modem_on_pulse_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem ON) Pulse - Nothing to show.\n", EMM_GPIO_MODEM_ON );

	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t modem_on_pulse_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem ON) Pulse.\n", EMM_GPIO_MODEM_ON);
        retval = -EINVAL;
    }
    else if (0 == strcmp(buf, "1\n"))
    {
        PulseGpioValue(EMM_GPIO_MODEM_ON, 1, g_pulse_width_ON, false);
    }
    else
    {
        printk(KERN_ERR "encore_modem_mgr: Invalid value specified for GPIO %d (Modem ON) Pulse: \"%s\"\n", EMM_GPIO_MODEM_ON, buf);
        retval = -EINVAL;
    }

	return retval;
}

static DEVICE_ATTR(modem_on_pulse, S_IWUGO | S_IRUGO, modem_on_pulse_show, modem_on_pulse_store);


/****** modem_rst_pulse_width ******/

static ssize_t modem_rst_pulse_width_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem RST) Pulse Width is currently %ld ms.\n", EMM_GPIO_MODEM_RST, g_pulse_width_RST);

	return snprintf(buf, PAGE_SIZE, "%ld\n", g_pulse_width_RST);
}

static ssize_t modem_rst_pulse_width_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem RST).\n", EMM_GPIO_MODEM_RST);
        retval = -EINVAL;
    }
    else
    {
        /* We assume simple_strtoul gives us something useable */
        g_pulse_width_RST = simple_strtoul(buf, NULL, 10);

        DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem RST) Pulse Width is now %ld.\n", EMM_GPIO_MODEM_RST, g_pulse_width_RST);
    }

	return retval;
}

static DEVICE_ATTR(modem_rst_pulse_width, S_IWUGO | S_IRUGO, modem_rst_pulse_width_show, modem_rst_pulse_width_store);


/****** modem_rst_pulse ******/

static ssize_t modem_rst_pulse_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: SYSFS - GPIO %d (Modem RST) Pulse - Nothing to show.\n", EMM_GPIO_MODEM_RST );

	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t modem_rst_pulse_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t retval = count;

    if (NULL == buf)
    {
        printk(KERN_ERR "encore_modem_mgr: SYSFS - No value provided for GPIO %d (Modem RST) Pulse.\n", EMM_GPIO_MODEM_RST);
        retval = -EINVAL;
    }
    else if (0 == strcmp(buf, "1\n"))
    {
        PulseGpioValue(EMM_GPIO_MODEM_RST, 0, g_pulse_width_RST, true);
    }
    else
    {
        printk(KERN_ERR "encore_modem_mgr: Invalid value specified for GPIO %d (Modem RST) Pulse: \"%s\"\n", EMM_GPIO_MODEM_RST, buf);
        retval = -EINVAL;
    }

	return retval;
}

static DEVICE_ATTR(modem_rst_pulse, S_IWUGO | S_IRUGO, modem_rst_pulse_show, modem_rst_pulse_store);


/****** SysFS table entries ******/

static struct device_attribute* const g_encore_modem_mgr_attributes[] =
{
    &dev_attr_modem_on,
    &dev_attr_modem_ndisable,
    &dev_attr_modem_rst,
    &dev_attr_modem_on_pulse_width,
    &dev_attr_modem_on_pulse,
    &dev_attr_modem_rst_pulse_width,
    &dev_attr_modem_rst_pulse,
}; 


/*****************************************************************************
 * Power Management
 *****************************************************************************/

#ifdef CONFIG_PM
static int encore_modem_mgr_suspend(struct platform_device *pdev, pm_message_t state)
{
    /* Do nothing, for now */
    return 0;
}

static int encore_modem_mgr_resume(struct platform_device *pdef)
{
    /* Do nothing, for now */
    return 0;
}
#else  /* CONFIG_PM */
#define encore_modem_mgr_suspend NULL
#define encore_modem_mgr_resume  NULL
#endif /* CONFIG_PM */


/*****************************************************************************
 * Module initialization/deinitialization
 *****************************************************************************/

static int __init encore_modem_mgr_probe(struct platform_device *pdev)
{
    int err = 0;
    int i;

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Configuring Encore Modem Manager\n");

    /* Reserve the necessary GPIOs */

    err = gpio_request(EMM_GPIO_MODEM_RST, "encore_modem_mgr");
    if (0 > err)
    {
        printk(KERN_ERR "encore_modem_mgr: ERROR - Request for GPIO %d (Modem RST) FAILED.\n", EMM_GPIO_MODEM_RST);
        goto ERROR;
    }

    err = gpio_request(EMM_GPIO_MODEM_ON, "encore_modem_mgr");
    if (0 > err)
    {
        printk(KERN_ERR "encore_modem_mgr: ERROR - Request for GPIO %d (Modem ON) FAILED.\n", EMM_GPIO_MODEM_ON);
        goto ERROR;
    }

    err = gpio_request(EMM_GPIO_MODEM_NDISABLE, "encore_modem_mgr");
    if (0 > err)
    {
        printk(KERN_ERR "encore_modem_mgr: ERROR - Request for GPIO %d (Modem nDISABLE) FAILED.\n", EMM_GPIO_MODEM_NDISABLE);
        goto ERROR;
    }

    /* Configure the necessary GPIOs */

    gpio_direction_input(EMM_GPIO_MODEM_RST);
    gpio_direction_input(EMM_GPIO_MODEM_NDISABLE);

    gpio_direction_output(EMM_GPIO_MODEM_ON, 0);
    SetGpioValue(EMM_GPIO_MODEM_ON, 0, false);

#if EMM_GPIO_DEBUG
    /* Make it so that we can see these GPIOs in /sys/class/gpio */

    gpio_export(EMM_GPIO_MODEM_RST,      true);
    gpio_export(EMM_GPIO_MODEM_ON,       true);
    gpio_export(EMM_GPIO_MODEM_NDISABLE, true);
#endif /* EMM_GPIO_DEBUG */

    /* Configure SysFS entries */
    for (i = 0; i < ARRAY_SIZE(g_encore_modem_mgr_attributes); i++)
    {
        err = device_create_file(&(pdev->dev), g_encore_modem_mgr_attributes[i]);
        if (0 > err)
        {
            while (i--)
            {
                device_remove_file(&(pdev->dev), g_encore_modem_mgr_attributes[i]);
            }
            printk(KERN_ERR "encore_modem_mgr: ERROR - failed to register SYSFS\n");
            goto ERROR;
        }
    }

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Encore Modem Manager configuration complete\n");

    return 0;

ERROR:
    gpio_free(EMM_GPIO_MODEM_RST);
    gpio_free(EMM_GPIO_MODEM_ON);
    gpio_free(EMM_GPIO_MODEM_NDISABLE);

    return err;
}

static void encore_modem_mgr_shutdown(struct platform_device *pdef)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Shutting down Encore Modem Manager\n");

    /* Nothing to do, for now */

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Encore Modem Manager shut down complete\n");
}

static int __devexit encore_modem_mgr_remove(struct platform_device *pdev)
{
    int i;

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Removing Encore Modem Manager\n");

    encore_modem_mgr_shutdown(pdev);

    /* Remove SysFS entries */
    for (i = 0; i < ARRAY_SIZE(g_encore_modem_mgr_attributes); i++)
    {
        device_remove_file(&(pdev->dev), g_encore_modem_mgr_attributes[i]);
	}

    /* Release the requested GPIOs */

    gpio_free(EMM_GPIO_MODEM_RST);
    gpio_free(EMM_GPIO_MODEM_ON);
    gpio_free(EMM_GPIO_MODEM_NDISABLE);

    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Encore Modem Manager removal complete\n");
    return 0;
}

static struct platform_driver g_encore_modem_mgr_driver =
{
    .probe    = encore_modem_mgr_probe,
    .remove   = __devexit_p(encore_modem_mgr_remove),
    .shutdown = encore_modem_mgr_shutdown,
    .suspend  = encore_modem_mgr_suspend,
    .resume   = encore_modem_mgr_resume,
    .driver   =
                {
                    .name   = "encore_modem_mgr",
                    .bus    = &platform_bus_type,
                    .owner  = THIS_MODULE,
                },
};


/*****************************************************************************
 * Module init/cleanup
 *****************************************************************************/

static int __init encore_modem_mgr_init(void)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Initializing Encore Modem Manager\n");

    return platform_driver_register(&g_encore_modem_mgr_driver);
}

static void __exit encore_modem_mgr_cleanup(void)
{
    DEBUGPRINT(KERN_INFO "encore_modem_mgr: Cleaning Up Encore Modem Manager\n");

    platform_driver_unregister(&g_encore_modem_mgr_driver);
}

module_init(encore_modem_mgr_init);
module_exit(encore_modem_mgr_cleanup);


/*****************************************************************************
 * Final Administrivia
 *****************************************************************************/

MODULE_AUTHOR("Intrinsyc Software Inc., <support@intrinsyc.com>");
MODULE_DESCRIPTION("Encore Modem Manager");
MODULE_LICENSE("GPL");

