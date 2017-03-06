/*
 * drivers/misc/bcm4329_rfkill.c
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <linux/err.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/rfkill.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/delay.h>

static int mrvlsd8787_bt_rfkill_set_power(void *data, bool blocked)
{
	if (blocked) {
		printk("----%s--1----\n",__FUNCTION__);
	} else {
		printk("----%s--2----\n",__FUNCTION__);
	}

	return 0;
}

static const struct rfkill_ops mrvlsd8787_bt_rfkill_ops = {
	.set_block = mrvlsd8787_bt_rfkill_set_power,
};

static int mrvlsd8787_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill *bt_rfkill;	
	int ret;
	bool enable = false;  /* off */
	bool default_sw_block_state;

	bt_rfkill = rfkill_alloc("mrvlsd8787 Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &mrvlsd8787_bt_rfkill_ops,
				NULL);

	if (unlikely(!bt_rfkill))
		return -ENODEV;

	default_sw_block_state = !enable;
	rfkill_set_states(bt_rfkill, default_sw_block_state, false);	
	ret = rfkill_register(bt_rfkill);

	if (unlikely(ret)) {
		rfkill_destroy(bt_rfkill);
		return -ENODEV;
	}
	return 0;
	
}

static int mrvlsd8787_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill *bt_rfkill = platform_get_drvdata(pdev);
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);
	return 0;
}


static struct platform_driver mrvlsd8787_rfkill_driver = {
	.probe = mrvlsd8787_rfkill_probe,
	.remove = mrvlsd8787_rfkill_remove,
	.driver = {
		   .name = "mrvlsd8787_rfkill",
		   .owner = THIS_MODULE,
	},
};

static int __init mrvlsd8787_rfkill_init(void)
{
	return platform_driver_register(&mrvlsd8787_rfkill_driver);
}

static void __exit mrvlsd8787_rfkill_exit(void)
{
	platform_driver_unregister(&mrvlsd8787_rfkill_driver);
}

module_init(mrvlsd8787_rfkill_init);
module_exit(mrvlsd8787_rfkill_exit);

MODULE_DESCRIPTION("MRVLSD8787 rfkill");
MODULE_AUTHOR("KEENHI");
MODULE_LICENSE("GPL");
