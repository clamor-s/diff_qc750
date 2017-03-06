/*
 * arch/arm/mach-tegra/board-kai-kbc.c
 * Keys configuration for Nvidia tegra3 kai platform.
 *
 * Copyright (C) 2012 NVIDIA, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mfd/max77663-core.h>
#include <linux/gpio_scrollwheel.h>
#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include <asm/mach-types.h>
#include "board.h"
#include "board-nabi2.h"

#include "gpio-names.h"
#include "devices.h"

#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}

#define GPIO_IKEY(_id, _irq, _iswake, _deb)	\
	{					\
		.code = _id,			\
		.gpio = -1,			\
		.irq = _irq,			\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = _deb,	\
	}

static struct gpio_keys_button kai_keys[] = {
	[0] = GPIO_KEY(KEY_VOLUMEUP, PR0, 0),
	[1] = GPIO_KEY(KEY_VOLUMEDOWN, PR1, 0),
	[2] = GPIO_IKEY(KEY_POWER, MAX77663_IRQ_BASE + MAX77663_IRQ_ONOFF_EN0_FALLING, 0, 100),
	[3] = GPIO_IKEY(KEY_POWER, MAX77663_IRQ_BASE + MAX77663_IRQ_ONOFF_EN0_1SEC, 0, 3000),
};

static struct gpio_keys_platform_data kai_keys_platform_data = {
	.buttons	= kai_keys,
	.nbuttons	= ARRAY_SIZE(kai_keys),
};

static struct platform_device kai_keys_device = {
	.name   = "gpio-keys",
	.id     = 0,
	.dev    = {
		.platform_data  = &kai_keys_platform_data,
	},
};
static struct gpio_keys_button kai_keys_max7663[] = {
	[0] = GPIO_KEY(KEY_VOLUMEUP, PR0, 0),
	[1] = GPIO_KEY(KEY_VOLUMEDOWN, PR1, 0),
	[2] = GPIO_IKEY(KEY_POWER, MAX77663_IRQ_BASE + MAX77663_IRQ_ONOFF_EN0_RISING, 0, 1),
	[3] = GPIO_IKEY(KEY_POWER, MAX77663_IRQ_BASE + MAX77663_IRQ_ONOFF_EN0_FALLING, 0, 100),
	[4] = GPIO_IKEY(KEY_POWER, MAX77663_IRQ_BASE + MAX77663_IRQ_ONOFF_EN0_1SEC, 0, 3000),
};

static struct gpio_keys_platform_data kai_keys_max7663_platform_data = {
	.buttons	= kai_keys_max7663,
	.nbuttons	= ARRAY_SIZE(kai_keys_max7663),
};

static struct platform_device kai_keys_max7663_device = {
	.name   = "gpio-keys",
	.id     = 0,
	.dev    = {
		.platform_data  = &kai_keys_max7663_platform_data,
	},
};
int __init kai_keys_init(void)
{
	pr_info("Registering gpio keys\n");
	if(machine_is_nabi2_xd()||machine_is_n1010()||machine_is_n1020()||machine_is_ns_14t004()||machine_is_itq1000()||machine_is_n1011()
		||machine_is_n1050()||machine_is_w1011a() || machine_is_birch() || machine_is_qc750() || machine_is_wikipad()){
		platform_device_register(&kai_keys_max7663_device);
	}else
	platform_device_register(&kai_keys_device);

	return 0;
}
