/*
 * arch/arm/mach-tegra/board-kai-sensors.c
 *
 * Copyright (c) 2012, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#ifndef CONFIG_SENSORS_TMP401
#include <linux/nct1008.h>
#endif
#include <linux/err.h>
#include <linux/mpu.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <mach/thermal.h>
#include "board.h"
#include "board-nabi2.h"
#include "cpu-tegra.h"
#include <linux/interrupt_bq27x00.h>
#include <linux/bq24160_charger.h>
#if defined(CONFIG_INPUT_KIONIX_ACCEL)
#include <linux/input/kionix_accel.h>
#endif
#if defined(CONFIG_LIS3DH_ACC)
#include <linux/input/lis3dh.h>
#endif
#if defined(CONFIG_LIGHTSENSOR_EPL6814)
#include <linux/input/elan_interface.h>
#endif

#ifdef CONFIG_SENSORS_TMP401
#include <linux/tmp401.h>
#endif

#ifdef CONFIG_GPIO_SWITCH_OTG
#include <linux/gpio_switch.h>
#endif
#define BMA250_IRQ_GPIO		TEGRA_GPIO_PX1
#define DMARD06_IRQ_GPIO		TEGRA_GPIO_PX1
#if defined(CONFIG_INPUT_KIONIX_ACCEL)
#define KIONIX_IRQ_GPIO			TEGRA_GPIO_PW3
#endif
#define GC0308_POWER_RST_PIN TEGRA_GPIO_PBB0
#define GC0308_POWER_DWN_PIN TEGRA_GPIO_PBB5
#define HM2057_POWER_RST_PIN TEGRA_GPIO_PBB0
#define HM2057_POWER_DWN_PIN TEGRA_GPIO_PBB5

#define T8EV5_POWER_RST_PIN TEGRA_GPIO_PBB4
#define T8EV5_POWER_DWN_PIN TEGRA_GPIO_PBB6

static struct regulator *kai_1v8_cam1;
static struct regulator *kai_vdd_cam1;

#if (MPU_GYRO_TYPE == MPU_TYPE_MPU3050)
#define MPU_GYRO_NAME		"mpu3050"
#endif
#if (MPU_GYRO_TYPE == MPU_TYPE_MPU6050)
#define MPU_GYRO_NAME		"mpu6050"
#endif

static struct mpu_platform_data keenhi_mpu_gyro_data = {
	.int_config	= 0x10,
	.level_shifter	= 0,
	.orientation	= MPU_GYRO_ORIENTATION,	/* Located in board_[platformname].h	*/
};

static struct mpu_platform_data keenhi_mpu_gyro_data_bestbuy = {
	.int_config	= 0x10,
	.level_shifter	= 0,
	.orientation	= MPU_GYRO_ORIENTATION_BESYBUY,	/* Located in board_[platformname].h	*/
};

#if (MPU_GYRO_TYPE == MPU_TYPE_MPU3050)
static struct ext_slave_platform_data keenhi_mpu_accel_data = {
	.address	= MPU_ACCEL_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_ACCEL_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_SECONDARY,
	.orientation	= MPU_ACCEL_ORIENTATION,	/* Located in board_[platformname].h	*/
};
#endif

#if (MPU_GYRO_TYPE == MPU_TYPE_MPU3050)
static struct ext_slave_platform_data keenhi_mpu_accel_data_bestbuy = {
	.address	= MPU_ACCEL_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_ACCEL_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_SECONDARY,
	.orientation	= MPU_ACCEL_ORIENTATION_BESTBUY,	/* Located in board_[platformname].h	*/
};
#endif

static struct ext_slave_platform_data keenhi_mpu_compass_data = {
	.address	= MPU_COMPASS_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_COMPASS_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation	= MPU_COMPASS_ORIENTATION,	/* Located in board_[platformname].h	*/
};
static struct ext_slave_platform_data keenhi_mpu_compass_data_bestbuy = {
	.address	= MPU_COMPASS_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_COMPASS_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation	= MPU_COMPASS_ORIENTATION_BESTBUY,	/* Located in board_[platformname].h	*/
};

static struct i2c_board_info __initdata keenhi_inv_mpu_i2c0_board_info[] = {
	{
		I2C_BOARD_INFO(MPU_GYRO_NAME, MPU_GYRO_ADDR),
		.irq = TEGRA_GPIO_TO_IRQ(MPU_GYRO_IRQ_GPIO),
		.platform_data = &keenhi_mpu_gyro_data,
	},
#if (MPU_GYRO_TYPE == MPU_TYPE_MPU3050)
	{
		I2C_BOARD_INFO(MPU_ACCEL_NAME, MPU_ACCEL_ADDR),
#if	MPU_ACCEL_IRQ_GPIO
		.irq = TEGRA_GPIO_TO_IRQ(MPU_ACCEL_IRQ_GPIO),
#endif
		.platform_data = &keenhi_mpu_accel_data,
	},
#endif
	{
		I2C_BOARD_INFO(MPU_COMPASS_NAME, MPU_COMPASS_ADDR),
#if	MPU_COMPASS_IRQ_GPIO
		.irq = TEGRA_GPIO_TO_IRQ(MPU_COMPASS_IRQ_GPIO),
#endif
		.platform_data = &keenhi_mpu_compass_data,
	},
};


#ifndef CONFIG_SENSORS_TMP401
#ifndef CONFIG_TEGRA_INTERNAL_TSENSOR_EDP_SUPPORT
static int nct_get_temp(void *_data, long *temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_get_temp(data, temp);
}

static int nct_get_temp_low(void *_data, long *temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_get_temp_low(data, temp);
}

static int nct_set_limits(void *_data,
			long lo_limit_milli,
			long hi_limit_milli)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_limits(data,
					lo_limit_milli,
					hi_limit_milli);
}

static int nct_set_alert(void *_data,
				void (*alert_func)(void *),
				void *alert_data)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_alert(data, alert_func, alert_data);
}

static int nct_set_shutdown_temp(void *_data, long shutdown_temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_shutdown_temp(data, shutdown_temp);
}

static void nct1008_probe_callback(struct nct1008_data *data)
{
	struct tegra_thermal_device *thermal_device;

	thermal_device = kzalloc(sizeof(struct tegra_thermal_device),
					GFP_KERNEL);
	if (!thermal_device) {
		pr_err("unable to allocate thermal device\n");
		return;
	}

	thermal_device->name = "nct72";
	thermal_device->data = data;
	thermal_device->id = THERMAL_DEVICE_ID_NCT_EXT;
	thermal_device->offset = TDIODE_OFFSET;
	thermal_device->get_temp = nct_get_temp;
	thermal_device->get_temp_low = nct_get_temp_low;
	thermal_device->set_limits = nct_set_limits;
	thermal_device->set_alert = nct_set_alert;
	thermal_device->set_shutdown_temp = nct_set_shutdown_temp;

	tegra_thermal_device_register(thermal_device);
}
#endif

static struct nct1008_platform_data kai_nct1008_pdata = {
	.supported_hwrev = true,
	.ext_range = true,
	.conv_rate = 0x09, /* 0x09 corresponds to 32Hz conversion rate */
	.offset = 8, /* 4 * 2C. 1C for device accuracies */
#ifndef CONFIG_TEGRA_INTERNAL_TSENSOR_EDP_SUPPORT
	.probe_callback = nct1008_probe_callback,
#endif
};

static struct i2c_board_info kai_i2c4_nct1008_board_info[] = {
	{
		I2C_BOARD_INFO("nct72", 0x4C),
		.platform_data = &kai_nct1008_pdata,
		.irq = -1,
	}
};

static int kai_nct1008_init(void)
{
	int ret = 0;

	/* FIXME: enable irq when throttling is supported */
	kai_i2c4_nct1008_board_info[0].irq =
		TEGRA_GPIO_TO_IRQ(KAI_TEMP_ALERT_GPIO);

	ret = gpio_request(KAI_TEMP_ALERT_GPIO, "temp_alert");
	if (ret < 0) {
		pr_err("%s: gpio_request failed\n", __func__);
		return ret;
	}

	ret = gpio_direction_input(KAI_TEMP_ALERT_GPIO);
	if (ret < 0) {
		pr_err("%s: set gpio to input failed\n", __func__);
		gpio_free(KAI_TEMP_ALERT_GPIO);
	}
	return ret;
}
#else
static int tmp_get_temp(void *_data, long *temp)
{
	struct tmp401_data *data = _data;
	return tmp401_thermal_get_temp(data, temp);
}

static int tmp_get_temp_low(void *_data, long *temp)
{
	struct tmp401_data *data = _data;
	return tmp401_thermal_get_temp_low(data, temp);
}

static int tmp_set_limits(void *_data,
			long lo_limit_milli,
			long hi_limit_milli)
{
	struct tmp401_data *data = _data;
	return tmp401_thermal_set_limits(data,
					lo_limit_milli,
					hi_limit_milli);
}

static int tmp_set_alert(void *_data,
				void (*alert_func)(void *),
				void *alert_data)
{
	struct tmp401_data *data = _data;
	return tmp401_thermal_set_alert(data, alert_func, alert_data);
}

static int tmp_set_shutdown_temp(void *_data, long shutdown_temp)
{
	struct tmp401_data *data = _data;
	return tmp401_thermal_set_shutdown_temp(data,
						shutdown_temp);
}

static void tmp401_probe_callback(struct tmp401_data *data)
{
	struct tegra_thermal_device *thermal_device;

	thermal_device = kzalloc(sizeof(struct tegra_thermal_device),
					GFP_KERNEL);
	//printk("tmp401_probe_callback 1\n");
	if (!thermal_device) {
		pr_err("unable to allocate thermal device\n");
		return;
	}
	//printk("tmp401_probe_callback 2\n");
	thermal_device->name = "tmp401";//tmp401
	thermal_device->data = data;
	thermal_device->id = THERMAL_DEVICE_ID_NCT_EXT;
	thermal_device->offset = TDIODE_OFFSET;
	thermal_device->get_temp = tmp_get_temp;
	thermal_device->get_temp_low = tmp_get_temp_low;
	thermal_device->set_limits = tmp_set_limits;
	thermal_device->set_alert = tmp_set_alert;
	thermal_device->set_shutdown_temp = tmp_set_shutdown_temp;

	tegra_thermal_device_register(thermal_device);
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
        {
               struct tegra_thermal_device *int_nct;
               int_nct = kzalloc(sizeof(struct tegra_thermal_device),
                               GFP_KERNEL);
               if (!int_nct) {
                       kfree(int_nct);
                       pr_err("unable to allocate thermal device\n");
                       return;
               }

               int_nct->name = "nct_int";
               int_nct->id = THERMAL_DEVICE_ID_NCT_INT;
               int_nct->data = data;
               int_nct->get_temp = tmp_get_temp;

               tegra_thermal_device_register(int_nct);
       }
#endif	
}

static struct tmp401_platform_data enterprise_tmp401_pdata = {
	.supported_hwrev = true,
	.ext_range = true,
	.conv_rate = 0x08,
	.offset = 8, /* 4 * 2C. Bug 844025 - 1C for device accuracies */
	.probe_callback = tmp401_probe_callback,
};
static struct i2c_board_info enterprise_i2c4_tmp401_board_info[] = {
	{
		I2C_BOARD_INFO("tmp401", 0x4C),
	//	.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_TEMP_ALERT_N),
		.irq = TEGRA_GPIO_TO_IRQ(KAI_TEMP_ALERT_GPIO),
		.platform_data = &enterprise_tmp401_pdata,
	}
};

static int enterprise_tmp401_init(void)
{
#if 0
	int ret;

	//ret = gpio_request(TEGRA_GPIO_TEMP_ALERT_N, "temp_alert");
	//if (ret < 0) {
	//	pr_err("%s: gpio_request failed %d\n", __func__, ret);
	//	return;
	//}

	ret = gpio_direction_input(TEGRA_GPIO_TEMP_ALERT_N);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(TEGRA_GPIO_TEMP_ALERT_N);
		return;
	}

	i2c_register_board_info(4, enterprise_i2c4_tmp401_board_info,
				ARRAY_SIZE(enterprise_i2c4_tmp401_board_info));

#else
	int ret = 0;

	/* FIXME: enable irq when throttling is supported */
//	enterprise_i2c4_tmp401_board_info[0].irq =
//		TEGRA_GPIO_TO_IRQ(KAI_TEMP_ALERT_GPIO);

	ret = gpio_request(KAI_TEMP_ALERT_GPIO, "temp_alert");
	if (ret < 0) {
		pr_err("%s: gpio_request failed\n", __func__);
		return ret;
	}

	ret = gpio_direction_input(KAI_TEMP_ALERT_GPIO);
	if (ret < 0) {
		pr_err("%s: set gpio to input failed\n", __func__);
		gpio_free(KAI_TEMP_ALERT_GPIO);
	}
	return ret;

#endif
}

#endif

struct yuv_sensor_platform_data {
	int (*power_on)(void);
	int (*power_off)(void);

};
#if defined(CONFIG_VIDEO_GC0308)

static int kai_gc0308_power_on(void)
{
	pr_err("%s:==========>EXE\n",__func__);

       //first pull up 1.8 power
	if (kai_1v8_cam1 == NULL) {
		kai_1v8_cam1 = regulator_get(NULL, "vdd_1v8_cam1");
		if (WARN_ON(IS_ERR(kai_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1 %d\n",	__func__, (int)PTR_ERR(kai_1v8_cam1));
			goto reg_get_vdd_1v8_cam3_fail;
		}
	}
	regulator_enable(kai_1v8_cam1);
	//second pull up 2.8 power
	if (kai_vdd_cam1 == NULL) {
		kai_vdd_cam1 = regulator_get(NULL, "vdd_cam1");//2.8v
		if (WARN_ON(IS_ERR(kai_vdd_cam1))) {
			pr_err("%s: couldn't get regulator vdd_cam1: %d\n",	__func__, (int)PTR_ERR(kai_vdd_cam1));
			goto reg_get_vdd_cam3_fail;
		}
	}
	regulator_enable(kai_vdd_cam1);
	mdelay(5);

	gpio_direction_output(GC0308_POWER_DWN_PIN, 0);
	mdelay(10);
	gpio_direction_output(GC0308_POWER_RST_PIN, 1);
	mdelay(10);
	return 0;

reg_get_vdd_1v8_cam3_fail:
	regulator_put(kai_1v8_cam1);
	kai_1v8_cam1 = NULL;
reg_get_vdd_cam3_fail:
	regulator_put(kai_vdd_cam1);
	kai_vdd_cam1 = NULL;
	
	return -ENODEV;
}

static int kai_gc0308_power_off(void)
{
	pr_err("%s:==========>EXE\n",__func__);
	gpio_direction_output(GC0308_POWER_DWN_PIN, 1);//old 1
	gpio_direction_output(GC0308_POWER_RST_PIN, 0);
	
	if (kai_1v8_cam1){
		regulator_disable(kai_1v8_cam1);
		regulator_put(kai_1v8_cam1);
		kai_1v8_cam1 =NULL;
	}
	if (kai_vdd_cam1){
		regulator_disable(kai_vdd_cam1);
		regulator_put(kai_vdd_cam1);
		kai_vdd_cam1 =NULL;
	}
	return 0;
}



struct yuv_sensor_platform_data kai_gc0308_data = {
	.power_on = kai_gc0308_power_on,
	.power_off = kai_gc0308_power_off,
};

#endif

#if defined(CONFIG_VIDEO_HM2057)
static int kai_hm2057_power_on(void)
{

	pr_err("%s:==========>EXE\n",__func__);

	if (kai_1v8_cam1 == NULL) {
		kai_1v8_cam1 = regulator_get(NULL, "vdd_1v8_cam1");
		if (WARN_ON(IS_ERR(kai_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1 %d\n",	__func__, (int)PTR_ERR(kai_1v8_cam1));
			goto reg_get_vdd_1v8_cam3_fail;
		}
	}
	regulator_enable(kai_1v8_cam1);

	if (kai_vdd_cam1 == NULL) {
		kai_vdd_cam1 = regulator_get(NULL, "vdd_cam1");
		if (WARN_ON(IS_ERR(kai_vdd_cam1))) {
			pr_err("%s: couldn't get regulator vdd_cam1: %d\n",	__func__, (int)PTR_ERR(kai_vdd_cam1));
			goto reg_get_vdd_cam3_fail;
		}
	}
	regulator_enable(kai_vdd_cam1);
	mdelay(5);

	gpio_direction_output(HM2057_POWER_DWN_PIN, 0);
	mdelay(10);
	
	gpio_direction_output(HM2057_POWER_RST_PIN, 1);
	mdelay(5);
	gpio_direction_output(HM2057_POWER_RST_PIN, 0);
	mdelay(5);
	gpio_direction_output(HM2057_POWER_RST_PIN, 1);
	mdelay(10);

	return 0;

reg_get_vdd_1v8_cam3_fail:
	regulator_put(kai_1v8_cam1);
	kai_1v8_cam1 = NULL;
reg_get_vdd_cam3_fail:
	regulator_put(kai_vdd_cam1);
	kai_vdd_cam1 = NULL;
	
	return -ENODEV;
}

static int kai_hm2057_power_off(void)
{
	pr_err("%s:==========>EXE\n",__func__);
	gpio_direction_output(HM2057_POWER_DWN_PIN, 1);//old 1
	gpio_direction_output(HM2057_POWER_RST_PIN, 0);

	if (kai_vdd_cam1){
		regulator_disable(kai_vdd_cam1);
		regulator_put(kai_vdd_cam1);
		kai_vdd_cam1 =NULL;
	}
	
	if (kai_1v8_cam1){
		regulator_disable(kai_1v8_cam1);
		regulator_put(kai_1v8_cam1);
		kai_1v8_cam1 =NULL;
	}
	
	return 0;
}



struct yuv_sensor_platform_data kai_hm2057_data = {
	.power_on = kai_hm2057_power_on,
	.power_off = kai_hm2057_power_off,
};

#endif
#if defined(CONFIG_VIDEO_T8EV5)
static int kai_t8ev5_power_on(void)
{

	pr_err("%s:==========>EXE\n",__func__);

	if (kai_1v8_cam1 == NULL) {
		kai_1v8_cam1 = regulator_get(NULL, "vdd_1v8_cam1");
		if (WARN_ON(IS_ERR(kai_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1 %d\n",	__func__, (int)PTR_ERR(kai_1v8_cam1));
			goto reg_get_vdd_1v8_cam3_fail;
		}
	}
	regulator_enable(kai_1v8_cam1);
	
	if (kai_vdd_cam1 == NULL) {
		kai_vdd_cam1 = regulator_get(NULL, "vdd_cam1");
		if (WARN_ON(IS_ERR(kai_vdd_cam1))) {
			pr_err("%s: couldn't get regulator vdd_cam1: %d\n",	__func__, (int)PTR_ERR(kai_vdd_cam1));
			goto reg_get_vdd_cam3_fail;
		}
	}
	regulator_enable(kai_vdd_cam1);
	mdelay(5);

	gpio_direction_output(T8EV5_POWER_DWN_PIN, 1);
	mdelay(10);
	gpio_direction_output(T8EV5_POWER_RST_PIN, 1);
	mdelay(10);
	
	return 0;

reg_get_vdd_1v8_cam3_fail:
	regulator_put(kai_1v8_cam1);
	kai_1v8_cam1 = NULL;
reg_get_vdd_cam3_fail:
	regulator_put(kai_vdd_cam1);
	kai_vdd_cam1 = NULL;
	
	return -ENODEV;
}

static int kai_t8ev5_power_off(void)
{
	pr_err("%s:==========>EXE\n",__func__);	
	gpio_direction_output(T8EV5_POWER_DWN_PIN, 0);//old 1
	gpio_direction_output(T8EV5_POWER_RST_PIN, 0);
	
	if (kai_1v8_cam1){
		regulator_disable(kai_1v8_cam1);
		regulator_put(kai_1v8_cam1);
		kai_1v8_cam1 =NULL;
	}
	if (kai_vdd_cam1){
		regulator_disable(kai_vdd_cam1);
		regulator_put(kai_vdd_cam1);
		kai_vdd_cam1 =NULL;
	}

	return 0;
}



struct yuv_sensor_platform_data kai_t8ev5_data = {
	.power_on = kai_t8ev5_power_on,
	.power_off = kai_t8ev5_power_off,
};

#endif

static struct i2c_board_info nabi2_gc0308_board_info[] = {
	{
		I2C_BOARD_INFO("gc0308", 0x21),
		.platform_data = &kai_gc0308_data,
	},
};

static struct i2c_board_info nabi2_t8ev5_i2c9_board_info[] = {
	{
		I2C_BOARD_INFO("t8ev5", 0x3c),
		.platform_data = &kai_t8ev5_data,
	},
};

static struct i2c_board_info nabi2_xd_i2c9_board_info[] = {
	{
		I2C_BOARD_INFO("hm2057", 0x24),
		.platform_data = &kai_hm2057_data,
	},
	{
		I2C_BOARD_INFO("t8ev5", 0x3c),
		.platform_data = &kai_t8ev5_data,
	},
};
static struct i2c_board_info ventana_i2c9_board_info[] = {
	{
		I2C_BOARD_INFO("hm2057", 0x24),
		.platform_data = &kai_hm2057_data,
	},
};
static int kai_camera_init(void)
{
	int ret;
	if(machine_is_qc750() || machine_is_n750() || machine_is_birch()){
		//tegra_gpio_enable(GC0308_POWER_DWN_PIN);
		ret = gpio_request(GC0308_POWER_DWN_PIN, "gc0308_pwdn");
		if (ret < 0) {
			pr_err("%s: gpio_request failed for gpio %s\n",
				__func__, "GC0308_PWDN_GPIO");
		}
		gpio_direction_output(GC0308_POWER_DWN_PIN, 1);//old 1

		//tegra_gpio_enable(GC0308_POWER_RST_PIN);
		ret = gpio_request(GC0308_POWER_RST_PIN, "gc0308_reset");
		if (ret < 0) {
			pr_err("%s: gpio_request failed for gpio %s\n",
				__func__, "GC0308_RESET_GPIO");
		}
		gpio_direction_output(GC0308_POWER_RST_PIN, 0);
	}
if(machine_is_nabi2_xd() ||machine_is_qc750() ||machine_is_n1010() 
|| machine_is_n750() || machine_is_birch() || machine_is_ns_14t004()){

	//tegra_gpio_enable(T8EV5_POWER_DWN_PIN);
	ret = gpio_request(T8EV5_POWER_DWN_PIN, "t8ev5_pwdn");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "T8EV5_PWDN_GPIO");
	}
	gpio_direction_output(T8EV5_POWER_DWN_PIN, 0);//old 1

	//tegra_gpio_enable(T8EV5_POWER_RST_PIN);
	ret = gpio_request(T8EV5_POWER_RST_PIN, "t8ev5_reset");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "T8EV5_RESET_GPIO");
	}
	gpio_direction_output(T8EV5_POWER_RST_PIN, 0);

}

if(machine_is_nabi2_xd()||machine_is_nabi2() || machine_is_nabi2_3d() 
||  machine_is_n710() || machine_is_itq700() || machine_is_itq701() 
|| machine_is_mm3201() ||machine_is_nabi_2s() || machine_is_n1010() 
|| machine_is_wikipad() || machine_is_ns_14t004()){
	//tegra_gpio_enable(HM2057_POWER_DWN_PIN);
	ret = gpio_request(HM2057_POWER_DWN_PIN, "hm2057_pwdn");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "HM2057_PWDN_GPIO");
	}
	gpio_direction_output(HM2057_POWER_DWN_PIN, 1);//old 1

	//tegra_gpio_enable(HM2057_POWER_RST_PIN);
	ret = gpio_request(HM2057_POWER_RST_PIN, "hm2057_reset");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "HM2057_RESET_GPIO");
	}
	gpio_direction_output(HM2057_POWER_RST_PIN, 0);
}
	return 0;
}

//add by vin for usi_3g_module

static int kai_usi_3g_module_init(void)
{
	int ret;

	if(machine_is_qc750() || machine_is_n1050() || machine_is_n750())
	{
	    ret = gpio_request(USI_3G_PWR_KEY_GPIO, "usi_3g_pwr_key");
	    if (ret < 0) {
	        pr_err("%s: gpio_request failed for gpio %s\n",
			    __func__, "USI_3G_PWR_KEY_GPIO");
	    }
	    gpio_direction_output(USI_3G_PWR_KEY_GPIO, 1);

	    ret = gpio_request(USI_3G_PWR_EN_GPIO, "usi_3g_pwr_en");
	    if (ret < 0) {
	        pr_err("%s: gpio_request failed for gpio %s\n",
			    __func__, "USI_3G_PWR_EN_GPIO");
	    }
	    gpio_direction_output(USI_3G_PWR_EN_GPIO, 1);
	}
	return 0;
}

void kai_usi_3g_usb_en_value(int value)
{
    //printk("======================set gpio to %d \n",value);
    if(machine_is_qc750() || machine_is_n1050() || machine_is_n750())
    {
        gpio_direction_output(USI_3G_USB_EN_GPIO, value);
        msleep(10);
    }
}

EXPORT_SYMBOL_GPL(kai_usi_3g_usb_en_value);


#if defined(CONFIG_BATTERY_BQ27x00_2)

static struct interrupt_plug_ac ac_ok_int[] = {

	{					
		.irq = TEGRA_MAX77663_TO_IRQ(MAX77663_IRQ_ONOFF_ACOK_FALLING),			
		.active_low = 1,		
	},
	{					
		.irq = TEGRA_MAX77663_TO_IRQ(MAX77663_IRQ_ONOFF_ACOK_RISING),			
		.active_low = 0,		
	},
};
struct interrupt_bq27x00_platform_data kai_bq27x00_data = {
	.int_plug_ac = ac_ok_int,
	.nIrqs = ARRAY_SIZE(ac_ok_int),
};

static const struct i2c_board_info ventana_i2c1_board_info[] = {
	{
		I2C_BOARD_INFO("bq27500", 0x55),
		.platform_data = &kai_bq27x00_data,
	},
};
#endif

#ifdef CONFIG_INPUT_KIONIX_ACCEL
struct kionix_accel_platform_data kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 1,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_xd_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 2,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_qc750_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 8,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_n710_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 2,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_n1010_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 6,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_ns_14t004_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 6,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2_n750_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 8,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
struct kionix_accel_platform_data nabi2s_kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 4,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};
#endif /* CONFIG_INPUT_KIONIX_ACCEL */
#ifdef CONFIG_LIS3DH_ACC
struct lis3dh_acc_platform_data  lsm303dlc_acc_plt_dat = {
         .poll_interval = 20,          //Driver polling interval as 50ms
         .min_interval = 10,    //Driver polling interval minimum 10ms
         .g_range = LIS3DH_ACC_G_2G,    //Full Scale of LSM303DLH Accelerometer  
         .axis_map_x = 0,      //x = x 
         .axis_map_y = 1,      //y = y 
         .axis_map_z = 2,      //z = z 
         .negate_x = 0,      //x = +x 
         .negate_y = 0,      //y = +y 
          .negate_z = 0,      //z = +z
          .gpio_int1=-1,
	.gpio_int2=-1,  

};
#endif
static struct i2c_board_info __initdata inv_mpu_i2c0_board_info[] = {
#ifdef CONFIG_SENSORS_BMA250
	{
		I2C_BOARD_INFO("bma250", 0x18),
		.irq = TEGRA_GPIO_TO_IRQ(BMA250_IRQ_GPIO),
	},
#endif
#ifdef CONFIG_SENSORS_DMARD06
	{
		I2C_BOARD_INFO("dmard06", 0x1C),
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO),
	},
#endif

#ifdef CONFIG_LIS3DH_ACC
	{
		I2C_BOARD_INFO("lis3dh_acc", 0x18),
		.platform_data = &lsm303dlc_acc_plt_dat,
	},
#endif

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif

};

static struct i2c_board_info __initdata nabi_2s_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2s_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};
static struct i2c_board_info __initdata nabi2_xd_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_xd_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata nabi2_qc750_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_qc750_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata nabi2_n710_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_n710_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata nabi2_n1010_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_n1010_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata nabi2_n750_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_n750_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata nabi2_ns_14t004_inv_mpu_i2c0_board_info[] = {

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	{ I2C_BOARD_INFO("kionix_accel", KIONIX_ACCEL_I2C_ADDR),
		.platform_data = &nabi2_ns_14t004_kionix_accel_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(DMARD06_IRQ_GPIO), // Replace with appropriate GPIO setup
	},
#endif
};

static struct i2c_board_info __initdata keenhi_i2c0_birch_copmass[] = {
		{
			I2C_BOARD_INFO("akm8975", 0x0d),
			.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PW0),
		},
};

static struct i2c_board_info __initdata keenhi_i2c0_board_info_copmass[] = {
#ifdef CONFIG_SENSORS_AK8975
		{
			I2C_BOARD_INFO("akm8975", 0x0c),
			.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PW0),
		},
#endif


};


#ifdef CONFIG_CHARGER_BQ24160

static struct regulator_consumer_supply bq24160_otg_vbus_supply[] = {
	REGULATOR_SUPPLY("usb_vbus_otg", NULL),
};

static struct bq24160_charger_platform_data bq24160_charger_pdata = {
       .vbus_gpio=TEGRA_GPIO_PH1,
	.otg_consumer_supplies = bq24160_otg_vbus_supply,
	.num_otg_consumer_supplies = ARRAY_SIZE(bq24160_otg_vbus_supply),
    	.bq24160_reg3=0x8C,// set charge regulation voltage 4.2v  Input Limit for   1.5A
	.bq24160_reg5=0x6E, //set changer current 0.1A , 1.5A	
	.bq24160_reg5_susp=0x6A
      	
};

static struct i2c_board_info charger_board_info[] = {
	{
		I2C_BOARD_INFO("bq24160", 0x6b),
		.irq		= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PK2),	
		.platform_data = &bq24160_charger_pdata,
		
	},
};


static struct bq24160_charger_platform_data bq24160_charger_pdata_xd = {
       .vbus_gpio=TEGRA_GPIO_PH1,
	.otg_consumer_supplies = bq24160_otg_vbus_supply,
	.num_otg_consumer_supplies = ARRAY_SIZE(bq24160_otg_vbus_supply),
    	.bq24160_reg3=0x8E, // set charge regulation voltage 4.2v  Input Limit for   2.5A
	.bq24160_reg5=0xEE,//set changer current 0.1A , 2.7A  
	.bq24160_reg5_susp=0xEA
      	
};

static struct i2c_board_info charger_board_info_xd[] = {
	{
		I2C_BOARD_INFO("bq24160", 0x6b),
		.irq		= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PK2),	
		.platform_data = &bq24160_charger_pdata_xd,
		
	},
};
static struct bq24160_charger_platform_data bq24160_charger_ns_14t004 = {
       .vbus_gpio=TEGRA_GPIO_PH1,
	.otg_consumer_supplies = bq24160_otg_vbus_supply,
	.num_otg_consumer_supplies = ARRAY_SIZE(bq24160_otg_vbus_supply),
    	.bq24160_reg3=0x8C,// set charge regulation voltage 4.2v  Input Limit for   1.5A
	.bq24160_reg5=0x9E, //set changer current 0.1A , 1.5A	
	.bq24160_reg5_susp=0x9A
      	
};
static struct i2c_board_info charger_board_info_ns_14t004[] = {
	{
		I2C_BOARD_INFO("bq24160", 0x6b),
		.irq		= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PK2),	
		.platform_data = &bq24160_charger_ns_14t004,
		
	},
};
static struct bq24160_charger_platform_data bq24160_charger_birch = {
       .vbus_gpio=TEGRA_GPIO_PH1,
	.otg_consumer_supplies = bq24160_otg_vbus_supply,
	.num_otg_consumer_supplies = ARRAY_SIZE(bq24160_otg_vbus_supply),
    	.bq24160_reg3=0x8C,// set charge regulation voltage 4.2v  Input Limit for   1.5A
	.bq24160_reg5=0x9E, //set changer current 0.1A , 1.5A	
	.bq24160_reg5_susp=0x9A
      	
};
static struct i2c_board_info charger_board_info_birch[] = {
	{
		I2C_BOARD_INFO("bq24160", 0x6b),
		.irq		= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PK2),	
		.platform_data = &bq24160_charger_birch,
		
	},
};
#endif

#ifdef  CONFIG_LIGHTSENSOR_EPL6814
static struct i2c_board_info __initdata i2c0_board_info_lightsensor[] = {
		{
		        I2C_BOARD_INFO(ELAN_LS_6814, 0x92 >> 1),
    		},
};
#endif
#ifdef CONFIG_GPIO_SWITCH_OTG
static struct gpio_switch_platform_data gpio_switch_birch_pdata = {
     .gpio = TEGRA_GPIO_OTG_ENABLE,
      	
};

static struct platform_device gpio_switch_birch = {
		.name	= "gpio_switch",
		.id	= 0,
		.dev	= {
			.platform_data  = &gpio_switch_birch_pdata,
		},
};
#endif
static void keenhi_camera_init(void)
{
	kai_camera_init();
	if(machine_is_nabi2_xd() || machine_is_n1010()||machine_is_ns_14t004()){
		i2c_register_board_info(2, nabi2_xd_i2c9_board_info,
			ARRAY_SIZE(nabi2_xd_i2c9_board_info));
	}
	else if(machine_is_qc750() || machine_is_n750() || machine_is_birch()){
		i2c_register_board_info(2, nabi2_gc0308_board_info,
			ARRAY_SIZE(nabi2_gc0308_board_info));
		i2c_register_board_info(2, nabi2_t8ev5_i2c9_board_info,
			ARRAY_SIZE(nabi2_t8ev5_i2c9_board_info));
	}
	else if(machine_is_nabi_2s()){
		i2c_register_board_info(2,ventana_i2c9_board_info,
			ARRAY_SIZE(ventana_i2c9_board_info));
		}
	else{
	       i2c_register_board_info(2, ventana_i2c9_board_info,
			ARRAY_SIZE(ventana_i2c9_board_info));
	}
}



static void keenhi_charge_init(void)
{
	int rc;
#if defined(CONFIG_BATTERY_BQ27x00_2)
	i2c_register_board_info(1, ventana_i2c1_board_info,
		ARRAY_SIZE(ventana_i2c1_board_info));
#endif
#ifdef CONFIG_CHARGER_BQ24160
	rc = gpio_request(TEGRA_GPIO_PK2, "charge-bq24160");
	if (rc)
		pr_err("charge-bq24160 gpio request failed:%d\n", rc);
	
	rc = gpio_direction_input(TEGRA_GPIO_PK2);
	if (rc)
		pr_err("harge-bq24160 gpio direction configuration failed:%d\n", rc);

	if(machine_is_nabi2_xd() ||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700() || machine_is_itq701() || machine_is_mm3201() ||machine_is_nabi_2s() 
		|| machine_is_n1010() || machine_is_n750()  || machine_is_wikipad()){
	i2c_register_board_info(4, charger_board_info_xd,
		ARRAY_SIZE(charger_board_info_xd));
		}
	else if(machine_is_ns_14t004() )
	{
		i2c_register_board_info(4, charger_board_info_ns_14t004,
		ARRAY_SIZE(charger_board_info_ns_14t004));
	}
	else if (machine_is_birch()){
		i2c_register_board_info(4, charger_board_info_birch,
		ARRAY_SIZE(charger_board_info_birch));
	}
	else{
	i2c_register_board_info(4, charger_board_info,
		ARRAY_SIZE(charger_board_info));
		}
#endif

}

static void keenhi_gsensor_init(void)
{

	pr_info("*** sensor_init...\n");
	
	if(machine_is_nabi2_xd()){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_xd_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_xd_inv_mpu_i2c0_board_info));
	}
	else if( machine_is_n710() || machine_is_itq700() || machine_is_itq701() || machine_is_mm3201()){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_n710_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_n710_inv_mpu_i2c0_board_info));
	}
	else if(machine_is_n1010()){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_n1010_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_n1010_inv_mpu_i2c0_board_info));
	}
	else if(machine_is_n750() || machine_is_birch())
	{
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_n750_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_n750_inv_mpu_i2c0_board_info));
	}
	else if(machine_is_ns_14t004()){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_ns_14t004_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_ns_14t004_inv_mpu_i2c0_board_info));
	}
	else if(machine_is_qc750() ){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi2_qc750_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi2_qc750_inv_mpu_i2c0_board_info));
	}else if(machine_is_nabi_2s()){
		i2c_register_board_info(MPU_GYRO_BUS_NUM, nabi_2s_inv_mpu_i2c0_board_info,
			ARRAY_SIZE(nabi_2s_inv_mpu_i2c0_board_info));
		}
	else{
		i2c_register_board_info(MPU_GYRO_BUS_NUM, inv_mpu_i2c0_board_info,
			ARRAY_SIZE(inv_mpu_i2c0_board_info));
	}
}

static void keenhi_compass_init(void)
{
	pr_info("** compass init **\n");
	
	if(machine_is_nabi2_3d() || machine_is_nabi2_xd() ||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700() || machine_is_itq701() || machine_is_mm3201()
		|| machine_is_n1010() || machine_is_n750() || machine_is_wikipad()||machine_is_ns_14t004()) {
		i2c_register_board_info(KEENHI_COMPASS_BUS_NUM, keenhi_i2c0_board_info_copmass,
				ARRAY_SIZE(keenhi_i2c0_board_info_copmass));
	}
	else if(machine_is_birch())
	{
		i2c_register_board_info(KEENHI_COMPASS_BUS_NUM, keenhi_i2c0_birch_copmass,
			ARRAY_SIZE(keenhi_i2c0_birch_copmass));
	}
	
}
static void keenhi_lightsensor_init(void)
{
	if( machine_is_nabi2_xd() ||machine_is_nabi_2s() ||machine_is_n1010()||machine_is_ns_14t004()) {
		i2c_register_board_info(KEENHI_LIGHTSENSOR_I2C_BUS, i2c0_board_info_lightsensor,
			ARRAY_SIZE(i2c0_board_info_lightsensor));
	}

}

static void keenhi_mpuirq_init(void)
{
	int ret = 0;

	pr_info("*** MPU START *** keenhi_mpuirq_init...\n");

#if (MPU_GYRO_TYPE == MPU_TYPE_MPU3050)
#if	MPU_ACCEL_IRQ_GPIO
	/* ACCEL-IRQ assignment */
	ret = gpio_request(MPU_ACCEL_IRQ_GPIO, MPU_ACCEL_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_ACCEL_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_ACCEL_IRQ_GPIO);
		return;
	}
#endif
#endif

	/* MPU-IRQ assignment */
	ret = gpio_request(MPU_GYRO_IRQ_GPIO, MPU_GYRO_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_GYRO_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_GYRO_IRQ_GPIO);
		return;
	}
	pr_info("*** MPU END *** keenhi_mpuirq_init...\n");

	i2c_register_board_info(MPU_GYRO_BUS_NUM, keenhi_inv_mpu_i2c0_board_info,
		ARRAY_SIZE(keenhi_inv_mpu_i2c0_board_info));
}

int __init kai_sensors_init(void)
{
	int err;
#ifndef CONFIG_SENSORS_TMP401
	err = kai_nct1008_init();
	if (err)
		pr_err("%s: nct1008 init failed\n", __func__);
	else
		i2c_register_board_info(4, kai_i2c4_nct1008_board_info,
			ARRAY_SIZE(kai_i2c4_nct1008_board_info));
#else
	err = enterprise_tmp401_init();
	if (err)
		pr_err("%s: nct1008 init failed\n", __func__);
	else
		i2c_register_board_info(4, enterprise_i2c4_tmp401_board_info,
			ARRAY_SIZE(enterprise_i2c4_tmp401_board_info));
	
#endif

	kai_usi_3g_module_init();

#ifdef CONFIG_GPIO_SWITCH_OTG
	if(machine_is_birch())
		platform_device_register(&gpio_switch_birch);
#endif
	keenhi_camera_init();
	keenhi_charge_init();
	if(machine_is_wikipad() || machine_is_ns_14t004() ) {
			if(machine_is_ns_14t004()) {
			keenhi_inv_mpu_i2c0_board_info[0].platform_data = &keenhi_mpu_gyro_data_bestbuy;
			keenhi_inv_mpu_i2c0_board_info[1].platform_data = &keenhi_mpu_accel_data_bestbuy;
			keenhi_inv_mpu_i2c0_board_info[2].platform_data = &keenhi_mpu_compass_data_bestbuy;
#ifdef  CONFIG_LIGHTSENSOR_EPL6814
			keenhi_lightsensor_init();
#endif
		}
		keenhi_mpuirq_init();
		return 0;
	}
	keenhi_gsensor_init();
	keenhi_compass_init();
	keenhi_lightsensor_init();
	return 0;
}
