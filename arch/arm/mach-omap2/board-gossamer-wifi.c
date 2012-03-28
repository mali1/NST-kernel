/* linux/arch/arm/mach-omap2/board-gossamer-wifi.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/wifi_tiwlan.h>

#define GOSSAMER_WIFI_PMENA_GPIO	22
#define GOSSAMER_WIFI_IRQ_GPIO		15
#define GOSSAMER_WIFI_EN_POW		16

/* WIFI virtual 'card detect' status */
static int gossamer_wifi_cd = 0;

static int gossamer_wifi_power_state;
static int gossamer_wifi_reset_state;

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int omap_wifi_status_register(void (*callback)(int card_present, void *dev_id),
			      void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;

	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;

	return 0;
}

int gossamer_wifi_status(int irq)
{
	return gossamer_wifi_cd;
}

int gossamer_wifi_set_carddetect(int val)
{
	gossamer_wifi_cd = val;
	if (wifi_status_cb) {
		wifi_status_cb(val, wifi_status_cb_devid);
	} else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);

	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(gossamer_wifi_set_carddetect);
#endif

int gossamer_wifi_power(int on)
{
	/* VSYS-WLAN also enabled */
	gpio_direction_output(GOSSAMER_WIFI_EN_POW, on);

	/* only WLAN_EN is driven during power-up/down */
	gpio_direction_output(GOSSAMER_WIFI_PMENA_GPIO, on);

	gossamer_wifi_power_state = on;

	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(gossamer_wifi_power);
#endif

int gossamer_wifi_reset(int on)
{
	gossamer_wifi_reset_state = on;

	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(gossamer_wifi_reset);
#endif

struct wifi_platform_data gossamer_wifi_control = {
	.set_power	= gossamer_wifi_power,
	.set_reset	= gossamer_wifi_reset,
	.set_carddetect	= gossamer_wifi_set_carddetect,
};

#ifdef CONFIG_WIFI_CONTROL_FUNC
static struct resource gossamer_wifi_resources[] = {
	[0] = {
		.name		= "device_wifi_irq",
		.start		= OMAP_GPIO_IRQ(GOSSAMER_WIFI_IRQ_GPIO),
		.end		= OMAP_GPIO_IRQ(GOSSAMER_WIFI_IRQ_GPIO),
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct platform_device gossamer_wifi_device = {
        .name           = "device_wifi",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(gossamer_wifi_resources),
        .resource       = gossamer_wifi_resources,
        .dev            = {
                .platform_data = &gossamer_wifi_control,
        },
};
#endif

static int __init gossamer_wifi_init(void)
{
	int ret;

	ret = gpio_request(GOSSAMER_WIFI_IRQ_GPIO, "wifi_irq");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			GOSSAMER_WIFI_IRQ_GPIO);
		goto out;
	}

	ret = gpio_request(GOSSAMER_WIFI_PMENA_GPIO, "wifi_pmena");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			GOSSAMER_WIFI_PMENA_GPIO);
		goto out1;
	}

	ret = gpio_request(GOSSAMER_WIFI_EN_POW, "wifi_en_pow");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			GOSSAMER_WIFI_EN_POW);
		goto out2;
	}

	gpio_direction_input(GOSSAMER_WIFI_IRQ_GPIO);
#ifdef CONFIG_WIFI_CONTROL_FUNC
	ret = platform_device_register(&gossamer_wifi_device);
#endif
	return ret;
out2:
	gpio_free(GOSSAMER_WIFI_PMENA_GPIO);
out1:
	gpio_free(GOSSAMER_WIFI_IRQ_GPIO);
out:
        return ret;
}

device_initcall(gossamer_wifi_init);
