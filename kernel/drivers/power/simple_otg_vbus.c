/*
 * drivers/power/smb349-charger.c
 *
 * Battery charger driver for smb349 from summit microelectronics
 *
 * Copyright (c) 2012, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/usb/otg.h>
#include <linux/simple_otg_vbus.h>

typedef void (*callback_t)(enum usb_otg_state to,
		enum usb_otg_state from, void *args);
extern int register_otg_callback(callback_t cb, void *args);

static void sov_otg_status(enum usb_otg_state to, enum usb_otg_state from, void *data)
{
	int vbusPin =((struct simple_otg_vbus* )data)->mVbusPin;
	int ret;

	printk("OTG status change from=%d to to %d\n", from, to);

	if ((from == OTG_STATE_A_SUSPEND) && (to == OTG_STATE_A_HOST)) {

		gpio_set_value(vbusPin, 1);

	} else if ((from == OTG_STATE_A_HOST) && (to == OTG_STATE_A_SUSPEND)) {

		/* Disable OTG */
		gpio_set_value(vbusPin, 0);
	}
}

static int __devinit sov_probe(struct platform_device *pdev)
{
	struct simple_otg_vbus* data;
	int ret;
	
	data =(struct simple_otg_vbus*)(pdev->dev.platform_data);

	if(data==NULL){
		dev_err(&pdev->dev, "%s() data is empty\n", __func__);
		return -1;
	}

	ret =gpio_request(data->mVbusPin, "sov_vbus");
	if(ret){
		dev_err(&pdev->dev, "%s() request pin failed\n", __func__);
	}

	gpio_direction_output(data->mVbusPin, 0);
	
	ret = register_otg_callback(sov_otg_status, data);
	if (ret < 0)
		goto error;

	printk("sov_probe ok\n");

	return 0;
error:
	gpio_free(data->mVbusPin);
	return ret;
}

static int __devexit sov_remove(struct platform_device *pdev)
{
	struct simple_otg_vbus* data;
	int ret;
	
	data =(struct simple_otg_vbus*)(pdev->dev.platform_data);

	gpio_free(data->mVbusPin);

	return 0;
}

static struct platform_driver tegra_sov_driver = {
	.driver = {
		.name  = "simple_otg_vbus",
	},
	.remove  = __exit_p(sov_remove),
	.probe   = sov_probe,
};

static int __init sov_init(void)
{
	return platform_driver_register(&tegra_sov_driver);
}
module_init(sov_init);

static void __exit sov_exit(void)
{
	platform_driver_unregister(&tegra_sov_driver);
}
module_exit(sov_exit);

MODULE_AUTHOR("densy.lai <densy.lai@keenhi.com>");
MODULE_DESCRIPTION("simple otg vbus");
MODULE_LICENSE("GPL");
