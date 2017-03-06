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
#include <mach/io_dpd.h>

#include "gpio-names.h"
#include "board.h"
#include "board-nabi2.h"
#include <linux/wlan_plat.h>

#define BCM_WLAN_RST	TEGRA_GPIO_PD4
#define BCM_WLAN_WOW	TEGRA_GPIO_PU5

#define TI_WLAN_EN	TEGRA_GPIO_PD4
#define TI_WLAN_IRQ	TEGRA_GPIO_PV1

#define NABI2_SD_CD TEGRA_GPIO_PI5
#define NABI2_SD_POWER TEGRA_GPIO_PC6

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static int nabi2_wifi_status_register(void (*callback)(int , void *), void *);

static int nabi2_wifi_reset(int on);
static int nabi2_wifi_power(int on);
static int nabi2_wifi_set_carddetect(int val);

static int nabi2_wifi_status = 0;

static struct wifi_platform_data bcm_wifi_control = {
	.set_power	= nabi2_wifi_power,
	.set_reset	= nabi2_wifi_reset,
	.set_carddetect	= nabi2_wifi_set_carddetect,
};

static struct resource wifi_resource[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
		.start	= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PU5),
		.end	= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PU5),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct platform_device bcm_wifi_device = {
	.name		= "bcm4329_wlan",
	.id		= 1,
	.num_resources	= 1,
	.resource	= wifi_resource,
	.dev		= {
		.platform_data = &bcm_wifi_control,
	},
};

static struct wl12xx_platform_data ti_wlan_data __initdata = {
	.irq = TEGRA_GPIO_TO_IRQ(TI_WLAN_IRQ),
	.board_ref_clock = WL12XX_REFCLOCK_26,
	.board_tcxo_clock = 1,
	.set_power = nabi2_wifi_power,
	.set_carddetect = nabi2_wifi_set_carddetect,
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

#ifdef CONFIG_MMC_EMBEDDED_SDIO
static struct embedded_sdio_data embedded_sdio_data2 = {
	.cccr   = {
		.sdio_vsn       = 2,
		.multi_block    = 1,
		.low_speed      = 0,
		.wide_bus       = 0,
		.high_power     = 1,
		.high_speed     = 1,
	},
	.cis  = {
		.vendor         = 0x02d0,
		.device         = 0x4329,
	},
};
#endif

static struct tegra_sdhci_platform_data bcm_sdhci_platform_data2 = {
	.mmc_data = {
		.register_status_notify	= nabi2_wifi_status_register,
#ifdef CONFIG_MMC_EMBEDDED_SDIO
		.embedded_sdio = &embedded_sdio_data2,
#endif
		.built_in = 0,
		.ocr_mask = MMC_OCR_1V8_MASK,
	},
#ifndef CONFIG_MMC_EMBEDDED_SDIO
	.pm_flags = MMC_PM_KEEP_POWER,
#endif
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x0F,
	.ddr_clk_limit = 41000000,
/*	.is_voltage_switch_supported = false,
	.vdd_rail_name = NULL,
	.slot_rail_name = NULL,
	.vdd_max_uv = -1,
	.vdd_min_uv = -1,
	.max_clk = 0,
	.is_8bit_supported = false, */
	/* .max_clk = 25000000, */
};

static struct tegra_sdhci_platform_data ti_sdhci_platform_data2 = {
	.mmc_data = {
		.register_status_notify	= nabi2_wifi_status_register,
		.built_in = 0,
		.ocr_mask = MMC_OCR_1V8_MASK,
	},
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x0F,
	.ddr_clk_limit = 41000000,
/*	.is_voltage_switch_supported = false,
	.vdd_rail_name = NULL,
	.slot_rail_name = NULL,
	.vdd_max_uv = -1,
	.vdd_min_uv = -1,
	.max_clk = 0,
	.is_8bit_supported = false, */
	/* .max_clk = 25000000, */
};


static struct tegra_sdhci_platform_data tegra_sdhci_platform_data0 = {
	.cd_gpio = NABI2_SD_CD,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x0F,
	.ddr_clk_limit = 41000000,
/*	.is_voltage_switch_supported = true,
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
	.ddr_clk_limit = 41000000,
	.mmc_data = {
		.built_in = 1,
	}
/*	.is_voltage_switch_supported = false,
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

static struct platform_device bcm_sdhci_device2 = {
	.name		= "sdhci-tegra",
	.id		= 2,
	.resource	= sdhci_resource2,
	.num_resources	= ARRAY_SIZE(sdhci_resource2),
	.dev = {
		.platform_data = &bcm_sdhci_platform_data2,
	},
};

static struct platform_device ti_sdhci_device2 = {
	.name		= "sdhci-tegra",
	.id		= 2,
	.resource	= sdhci_resource2,
	.num_resources	= ARRAY_SIZE(sdhci_resource2),
	.dev = {
		.platform_data = &ti_sdhci_platform_data2,
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
int nabi2_get_wifi_status(void)
{
	//printk("nabi2_wifi_status = %d\n",nabi2_wifi_status);
	return nabi2_wifi_status;
}

static int nabi2_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int nabi2_wifi_set_carddetect(int val)
{
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		pr_warning("%s: Nobody to notify\n", __func__);
	return 0;
}
static int nabi2_wifi_power(int on)
{

	if(machine_is_nabi2_3d() || machine_is_nabi2() || machine_is_nabi2_xd()|| machine_is_nabi_2s()|| machine_is_wikipad())
	{
		gpio_set_value(BCM_WLAN_RST, on);
		mdelay(100);
	}
	else
	{
		/*
	 * FIXME : we need to revisit IO DPD code
	 * on how should multiple pins under DPD get controlled
	 *
	 * cardhu GPIO WLAN enable is part of SDMMC3 pin group
	 */
	 	struct tegra_io_dpd *sd_dpd;
	 	sd_dpd = tegra_io_dpd_get(&ti_sdhci_device2.dev);
		if (sd_dpd) {
			mutex_lock(&sd_dpd->delay_lock);
			tegra_io_dpd_disable(sd_dpd);
			mutex_unlock(&sd_dpd->delay_lock);
		}
		if (on) {
			gpio_set_value(TI_WLAN_EN, 1);
			mdelay(15);
			gpio_set_value(TI_WLAN_EN, 0);
			mdelay(1);
			gpio_set_value(TI_WLAN_EN, 1);
			mdelay(70);
		} else {
			gpio_set_value(TI_WLAN_EN, 0);
		}

		if (sd_dpd) {
			mutex_lock(&sd_dpd->delay_lock);
			tegra_io_dpd_enable(sd_dpd);
			mutex_unlock(&sd_dpd->delay_lock);
		}
	}

	return 0;
}

#ifdef CONFIG_TEGRA_PREPOWER_WIFI
static int __init kai_wifi_prepower(void)
{
	//nabi2_wifi_power(1);
	return 0;
}

subsys_initcall_sync(kai_wifi_prepower);
#endif


static int nabi2_wifi_reset(int on)
{
	pr_debug("%s: do nothing\n", __func__);
	return 0;
}
static void bcm_wifi_init(void)
{
	int rc;

	rc = gpio_request(BCM_WLAN_RST, "wlan_rst");
	if (rc)
		pr_err("BCM_WLAN_RST gpio request failed:%d\n", rc);
	
	
	rc = gpio_request(BCM_WLAN_WOW, "bcmsdh_sdmmc");
	if (rc)
		pr_err("WLAN_WOW gpio request failed:%d\n", rc);	


	rc = gpio_direction_output(BCM_WLAN_RST, 0);
	if (rc)
		pr_err("BCM_WLAN_RST gpio direction configuration failed:%d\n", rc);
	
	rc = gpio_direction_input(BCM_WLAN_WOW);
	if (rc)
		pr_err("BCM_WLAN_WOW gpio direction configuration failed:%d\n", rc);

	platform_device_register(&bcm_wifi_device);
}

static void ti_wifi_init(void)
{
	int rc;

	rc = gpio_request(TI_WLAN_EN, "wl12xx-power");
	if (rc)
		pr_err("WLAN_EN gpio request failed:%d\n", rc);

	rc = gpio_request(TI_WLAN_IRQ, "wl12xx");
	if (rc)
		pr_err("WLAN_IRQ gpio request failed:%d\n", rc);

	rc = gpio_direction_output(TI_WLAN_EN, 0);
	if (rc)
		pr_err("WLAN_EN gpio direction configuration failed:%d\n", rc);

	rc = gpio_direction_input(TI_WLAN_IRQ);
	if (rc)
		pr_err("WLAN_IRQ gpio direction configuration failed:%d\n", rc);

	if (wl12xx_set_platform_data(&ti_wlan_data))
		pr_err("Error setting wl12xx data\n");
}

int __init kai_sdhci_init(void)
{
	platform_device_register(&tegra_sdhci_device3);
	if(machine_is_nabi2_3d() || machine_is_nabi2() || machine_is_nabi2_xd() || machine_is_nabi_2s() || machine_is_wikipad())
	{
		platform_device_register(&bcm_sdhci_device2);
		bcm_wifi_init();
	} else {
		platform_device_register(&ti_sdhci_device2);
		ti_wifi_init();
	}
	platform_device_register(&tegra_sdhci_device0);	
	return 0;
}
