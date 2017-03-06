/*
 * arch/arm/mach-tegra/board-kai-sdhci.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2012 NVIDIA Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mmc/host.h>
#include <linux/wl12xx.h>

#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>

#include "gpio-names.h"
#include "board.h"
#include "board-cm9000.h"
#include <linux/wlan_plat.h>

#define KAI_SD_CD	TEGRA_GPIO_PI5
#define KAI_SD_POWER	TEGRA_GPIO_PC6
#define KAI_WLAN_EN	TEGRA_GPIO_PD3
#define KAI_WLAN_IRQ	TEGRA_GPIO_PV1

#define WLAN_RESET TEGRA_GPIO_PO5
#define WLAN_PDN	TEGRA_GPIO_PU0

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static int kai_wifi_power(int power_on);
static int kai_wifi_set_carddetect(int val);
static int kai_wifi_reset(int val);

static int kai_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}


static struct wl12xx_platform_data kai_wlan_data __initdata = {
	.irq = TEGRA_GPIO_TO_IRQ(KAI_WLAN_IRQ),
	.board_ref_clock = WL12XX_REFCLOCK_26,
	.board_tcxo_clock = 1,
	.set_power = kai_wifi_power,
	.set_carddetect = kai_wifi_set_carddetect,
};

static struct wifi_platform_data kai_wifi_control = {
	.set_power	= kai_wifi_power,	
	.set_carddetect = kai_wifi_set_carddetect,
	.set_reset = kai_wifi_reset,
};

static struct platform_device kai_wifi_device = {
	.name		= "wlan-sd8787",
	.id		= 1,
	.dev		= {
		.platform_data = &kai_wifi_control,
	},
};

static struct resource sdhci_resource0[] = {
	[0] = {
		.start	= INT_SDMMC1,
		.end	= INT_SDMMC1,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC1_BASE,
		.end	= TEGRA_SDMMC1_BASE + TEGRA_SDMMC1_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource sdhci_resource2[] = {
	[0] = {
		.start	= INT_SDMMC3,
		.end	= INT_SDMMC3,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC3_BASE,
		.end	= TEGRA_SDMMC3_BASE + TEGRA_SDMMC3_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource sdhci_resource3[] = {
	[0] = {
		.start	= INT_SDMMC4,
		.end	= INT_SDMMC4,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC4_BASE,
		.end	= TEGRA_SDMMC4_BASE + TEGRA_SDMMC4_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct embedded_sdio_data embedded_sdio_data2 = {
	.cccr   = {
		.sdio_vsn       = 3,
		.multi_block    = 1,
		.low_speed      = 0,
		.wide_bus       = 0,
		.high_power     = 1,
		.high_speed     = 1,
	},
	.cis  = {
		.vendor         = 0x02df,
		.device         = 0x9119,
	},
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data2 = {
	.mmc_data = {
		//.register_status_notify	= kai_wifi_status_register,
		.embedded_sdio = &embedded_sdio_data2,
		.built_in = 1,
	},
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
/*	.tap_delay = 6,
	.is_voltage_switch_supported = false,
	.vdd_rail_name = NULL,
	.slot_rail_name = NULL,
	.vdd_max_uv = -1,
	.vdd_min_uv = -1,
	.max_clk = 0,
	.is_8bit_supported = false, */
	/* .max_clk = 25000000, */
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data0 = {
	.cd_gpio = KAI_SD_CD,
	.wp_gpio = -1,
	.power_gpio = -1,
/*	.tap_delay = 6,
	.is_voltage_switch_supported = true,
	.vdd_rail_name = "vddio_sdmmc1",
	.slot_rail_name = "vddio_sd_slot",
	.vdd_max_uv = 3320000,
	.vdd_min_uv = 3280000,
	.max_clk = 208000000,
	.is_8bit_supported = false, */
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data3 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = 1,
	.tap_delay = 0x0F,
	.mmc_data = {
		.built_in = 1,
	}
/*	.tap_delay = 6,
	.is_voltage_switch_supported = false,
	.vdd_rail_name = NULL,
	.slot_rail_name = NULL,
	.vdd_max_uv = -1,
	.vdd_min_uv = -1,
	.max_clk = 48000000,
	.is_8bit_supported = true, */
};

static struct platform_device tegra_sdhci_device0 = {
	.name		= "sdhci-tegra",
	.id		= 0,
	.resource	= sdhci_resource0,
	.num_resources	= ARRAY_SIZE(sdhci_resource0),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data0,
	},
};

static struct platform_device tegra_sdhci_device2 = {
	.name		= "sdhci-tegra",
	.id		= 2,
	.resource	= sdhci_resource2,
	.num_resources	= ARRAY_SIZE(sdhci_resource2),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data2,
	},
};

static struct platform_device tegra_sdhci_device3 = {
	.name		= "sdhci-tegra",
	.id		= 3,
	.resource	= sdhci_resource3,
	.num_resources	= ARRAY_SIZE(sdhci_resource3),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data3,
	},
};

static int kai_wifi_set_carddetect(int val)
{
	pr_debug("%s: %d\n", __func__, val);
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
	pr_warning("%s: Nobody to notify\n", __func__);
	return 0;
}

static int kai_wifi_power(int power_on)
{
	pr_err("Powering %s wifi\n", (power_on ? "on" : "off"));

	if (power_on) {
		gpio_set_value(WLAN_PDN, 1);
		gpio_set_value(WLAN_RESET, 0);
		mdelay(50);
		gpio_set_value(WLAN_RESET, 1);
	} else {
		gpio_set_value(WLAN_PDN, 0);
		gpio_set_value(WLAN_RESET, 0);
	}

	return 0;
}

static int kai_wifi_reset(int power_on)
{
	printk("--------------wifi reset-------------\n");
	gpio_set_value(WLAN_RESET, 0);
	mdelay(50);
	gpio_set_value(WLAN_RESET, 1);
	return 0;
}

static int __init kai_wifi_init(void)
{
	int rc;

	printk("----------------------------init wifi 1-------------------------------------\n");
	rc = gpio_request(WLAN_RESET, "8787-reset");
	if (rc)
		pr_err("-----------------WLAN_RESET gpio request failed:%d\n", rc);

	rc = gpio_request(WLAN_PDN, "8787-pd");
	if (rc)
		pr_err("-----------------WLAN_RESET gpio request failed:%d\n", rc);

	tegra_gpio_enable(WLAN_RESET);
	tegra_gpio_enable(WLAN_PDN);

	rc = gpio_direction_output(WLAN_PDN, 0);
	if (rc)
		pr_err("-----------------WLAN_PDN gpio direction configuration failed:%d\n", rc);

	rc = gpio_direction_output(WLAN_RESET, 0);
	if (rc)
		pr_err("-----------------WLAN_PDN gpio direction configuration failed:%d\n", rc);
#if 1	
	rc = gpio_request(KAI_SD_POWER,"sd-power");
	if (rc)
		pr_err("-----------------WLAN_RESET gpio request failed:%d\n", rc);
	tegra_gpio_enable(KAI_SD_POWER);
	rc = gpio_direction_output(KAI_SD_POWER, 0);
	if (rc)
		pr_err("-----------------KAI_SD_POWER gpio direction configuration failed:%d\n", rc);
	gpio_set_value(KAI_SD_POWER, 1);
#endif

	gpio_set_value(WLAN_PDN, 1);
	gpio_set_value(WLAN_RESET, 0);
	mdelay(50);
	gpio_set_value(WLAN_RESET, 1);
	//platform_device_register(&kai_wifi_device);
	return 0;
}

int __init kai_sdhci_init(void)
{
	platform_device_register(&tegra_sdhci_device3);
	platform_device_register(&tegra_sdhci_device2);
	platform_device_register(&tegra_sdhci_device0);

	kai_wifi_init();
	return 0;
}
