/* tmp401.c
 *
 * Copyright (C) 2007,2008 Hans de Goede <hdegoede@redhat.com>
 * Preliminary tmp411 support by:
 * Gabriel Konat, Sander Leget, Wouter Willems
 * Copyright (C) 2009 Andre Prendel <andre.prendel@gmx.de>
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

/*
 * Driver for the Texas Instruments TMP401 SMBUS temperature sensor IC.
 *
 * Note this IC is in some aspect similar to the LM90, but it has quite a
 * few differences too, for example the local temp has a higher resolution
 * and thus has 16 bits registers for its value and limit instead of 8 bits.
 */
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/tmp401.h>
#include <linux/device.h>
//#include <linux/sched.h>

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x4c, I2C_CLIENT_END };
#define CELSIUS_TO_MILLICELSIUS(x) ((x)*1000)
#define MILLICELSIUS_TO_CELSIUS(x) ((x)/1000)

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static int is_suspended;
#endif

/*
 * The TMP401 registers, note some registers have different addresses for
 * reading and writing
 */
#define TMP401_STATUS				0x02
#define TMP401_CONFIG_READ			0x03
#define TMP401_CONFIG_WRITE			0x09
#define TMP401_CONVERSION_RATE_READ		0x04
#define TMP401_CONVERSION_RATE_WRITE		0x0A
#define TMP401_ONE_SHOT					0x0F

#define TMP401_TEMP_CRIT_HYST			0x21
#define TMP401_CONSECUTIVE_ALERT		0x22
#define TMP401_MANUFACTURER_ID_REG		0xFE
#define TMP401_DEVICE_ID_REG			0xFF
#define TMP411_N_FACTOR_REG			0x18

static const u8 TMP401_TEMP_MSB[2]			= { 0x00, 0x01 };
static const u8 TMP401_TEMP_LSB[2]			= { 0x15, 0x10 };
static const u8 TMP401_TEMP_LOW_LIMIT_MSB_READ[2]	= { 0x06, 0x08 };
static const u8 TMP401_TEMP_LOW_LIMIT_MSB_WRITE[2]	= { 0x0C, 0x0E };
static const u8 TMP401_TEMP_LOW_LIMIT_LSB[2]		= { 0x17, 0x14 };
static const u8 TMP401_TEMP_HIGH_LIMIT_MSB_READ[2]	= { 0x05, 0x07 };
static const u8 TMP401_TEMP_HIGH_LIMIT_MSB_WRITE[2]	= { 0x0B, 0x0D };
static const u8 TMP401_TEMP_HIGH_LIMIT_LSB[2]		= { 0x16, 0x13 };
/* These are called the THERM limit / hysteresis / mask in the datasheet */
static const u8 TMP401_TEMP_CRIT_LIMIT[2]		= { 0x20, 0x19 };

static const u8 TMP411_TEMP_LOWEST_MSB[2]		= { 0x30, 0x34 };
static const u8 TMP411_TEMP_LOWEST_LSB[2]		= { 0x31, 0x35 };
static const u8 TMP411_TEMP_HIGHEST_MSB[2]		= { 0x32, 0x36 };
static const u8 TMP411_TEMP_HIGHEST_LSB[2]		= { 0x33, 0x37 };

/* Flags */
#define TMP401_CONFIG_RANGE		0x04
#define TMP401_CONFIG_SHUTDOWN		0x40
#define TMP401_STATUS_LOCAL_CRIT		0x01
#define TMP401_STATUS_REMOTE_CRIT		0x02
#define TMP401_STATUS_REMOTE_OPEN		0x04
#define TMP401_STATUS_REMOTE_LOW		0x08
#define TMP401_STATUS_REMOTE_HIGH		0x10
#define TMP401_STATUS_LOCAL_LOW		0x20
#define TMP401_STATUS_LOCAL_HIGH		0x40

/* Manufacturer / Device ID's */
#define TMP401_MANUFACTURER_ID			0x55
#define TMP401_DEVICE_ID			0x11
#define TMP411_DEVICE_ID			0x12

#define MAX_CONV_TIME_ONESHOT_MS (125)

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static int is_suspended;
#endif

/*
 * Driver data (common to all clients)
 */

static const struct i2c_device_id tmp401_id[] = {
	{ "tmp401", tmp401 },
	{ "tmp411", tmp411 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tmp401_id);

/*
 * Client data (each client gets its own)
 */


/*
 * Sysfs attr show / store functions
 */

static int tmp401_register_to_temp(u16 reg, u8 config)
{
	int temp = reg;

	if (config & TMP401_CONFIG_RANGE)
		temp -= 64 * 256;

	return (temp * 625 + 80) / 160;
}

static u16 tmp401_temp_to_register(long temp, u8 config)
{
	if (config & TMP401_CONFIG_RANGE) {
		temp = SENSORS_LIMIT(temp, -64000, 191000);
		temp += 64000;
	} else
		temp = SENSORS_LIMIT(temp, 0, 127000);

	return (temp * 160 + 312) / 625;
}

static int tmp401_crit_register_to_temp(u8 reg, u8 config)
{
	int temp = reg;

	if (config & TMP401_CONFIG_RANGE)
		temp -= 64;

	return temp * 1000;
}

static u8 tmp401_crit_temp_to_register(long temp, u8 config)
{
	//printk("crit_temp_to_register temp = %ld config = %d\n",temp,config);
	if (config & TMP401_CONFIG_RANGE) {
		temp = SENSORS_LIMIT(temp, -64000, 191000);
		temp += 64000;
		//printk("tmp401_crit_temp_to_register 1 temp = %ld\n",temp);
	} else
		{
		temp = SENSORS_LIMIT(temp, 0, 127000);
		//printk("tmp401_crit_temp_to_register 2 temp = %ld\n",temp);
	}
	
	return (temp + 500) / 1000;
}

static struct tmp401_data *tmp401_update_device_reg16(
	struct i2c_client *client, struct tmp401_data *data)
{
	int i;

	for (i = 0; i < 2; i++) {
		/*
		 * High byte must be read first immediately followed
		 * by the low byte
		 */
		data->temp[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_MSB[i]) << 8;
		data->temp[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LSB[i]);
		data->temp_low[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LOW_LIMIT_MSB_READ[i]) << 8;
		data->temp_low[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LOW_LIMIT_LSB[i]);
		data->temp_high[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_HIGH_LIMIT_MSB_READ[i]) << 8;
		data->temp_high[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_HIGH_LIMIT_LSB[i]);
		data->temp_crit[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_CRIT_LIMIT[i]);

		if (data->kind == tmp411) {
			data->temp_lowest[i] = i2c_smbus_read_byte_data(client,
				TMP411_TEMP_LOWEST_MSB[i]) << 8;
			data->temp_lowest[i] |= i2c_smbus_read_byte_data(
				client, TMP411_TEMP_LOWEST_LSB[i]);

			data->temp_highest[i] = i2c_smbus_read_byte_data(
				client, TMP411_TEMP_HIGHEST_MSB[i]) << 8;
			data->temp_highest[i] |= i2c_smbus_read_byte_data(
				client, TMP411_TEMP_HIGHEST_LSB[i]);
		}
	}
	return data;
}

static struct tmp401_data *tmp401_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tmp401_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ) || !data->valid) {
		data->status = i2c_smbus_read_byte_data(client, TMP401_STATUS);
		data->config = i2c_smbus_read_byte_data(client,
						TMP401_CONFIG_READ);
		tmp401_update_device_reg16(client, data);

		data->temp_crit_hyst = i2c_smbus_read_byte_data(client,
						TMP401_TEMP_CRIT_HYST);

		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}
static int tmp401_get_temp(struct device *dev, long *etemp, long *itemp)
{
	struct tmp401_data *data = tmp401_update_device(dev);
	long temp_ext_milli;
	long temp_local_milli;

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	if ( is_suspended )
		return -16;
#endif				
	/* Read Local Temp */
	if (itemp) {
		temp_local_milli = tmp401_register_to_temp(data->temp[0], data->config);
		*itemp = temp_local_milli;
		//printk("tmp401_thermal_get_temp temp_local_milli=%ld\n",temp_local_milli);
	}

	/* Read External Temp */
	if (etemp) {
		temp_ext_milli = tmp401_register_to_temp(data->temp[1], data->config);
		*etemp = temp_ext_milli;
		//printk("tmp401_thermal_get_temp temp_ext_milli=%ld\n",temp_ext_milli);
	}

	return 0;
}
int tmp401_thermal_get_temp(struct tmp401_data *data, long *temp)
{
	int err=0;

	err=tmp401_get_temp(&data->client->dev, temp, NULL);
	//printk("tmp401_thermal_get_temp temp=%ld\n",*temp);
	return err;
}

int tmp401_thermal_get_temps(struct tmp401_data *data, long *etemp, long *itemp)
{
	//printk("tmp401_thermal_get_temps etemp= %ld itemp=%ld\n",*etemp,*itemp);
	return tmp401_get_temp(&data->client->dev, etemp, itemp);
}

int tmp401_thermal_get_temp_low(struct tmp401_data *data, long *temp)
{
	//printk("tmp401_thermal_get_temp_low \n");
	*temp = 0;
	return 0;
}
int tmp401_thermal_set_limits(struct tmp401_data *data,
				long lo_limit_milli,
				long hi_limit_milli)
{
	//struct tmp401_data *data = tmp401_update_device(&data->client->dev);
	long lo_limit = MILLICELSIUS_TO_CELSIUS(lo_limit_milli);
	long hi_limit = MILLICELSIUS_TO_CELSIUS(hi_limit_milli);
	u16 reg;
	int err;
	//show_stack(NULL,NULL);
	//printk("tmp401_thermal_set_limits lo_limit=%ld hi_limit=%ld \n",lo_limit,hi_limit);
	if (lo_limit >= hi_limit)
		return -EINVAL;

	if (data->current_lo_limit != lo_limit) {
		
		reg = tmp401_temp_to_register(lo_limit_milli, data->config);
		//printk("tmp401_thermal_set_limits lo reg=%d \n",reg);
		//mutex_lock(&data->update_lock);
		
		err = i2c_smbus_write_byte_data(data->client,
			TMP401_TEMP_LOW_LIMIT_MSB_WRITE[1], reg >> 8);
		if (err)
			return err;
		err = i2c_smbus_write_byte_data(data->client,
			TMP401_TEMP_LOW_LIMIT_LSB[1], reg & 0xFF);
		if (err)
			return err;
		data->temp_low[1] = reg;
		
		//mutex_unlock(&data->update_lock);

		data->current_lo_limit = lo_limit;
	}

	if (data->current_hi_limit != hi_limit) {
		
		reg = tmp401_temp_to_register(hi_limit_milli, data->config);
		//printk("tmp401_thermal_set_limits hi reg=%d \n",reg);
		//mutex_lock(&data->update_lock);
		
		err = i2c_smbus_write_byte_data(data->client,
			TMP401_TEMP_HIGH_LIMIT_MSB_WRITE[1], reg >> 8);
		if (err)
			return err;
		err = i2c_smbus_write_byte_data(data->client,
			TMP401_TEMP_HIGH_LIMIT_LSB[1], reg & 0xFF);
		if (err)
			return err;
		data->temp_high[1] = reg;
		
		//mutex_unlock(&data->update_lock);

		data->current_hi_limit = hi_limit;
	}

	return 0;
}

int tmp401_thermal_set_alert(struct tmp401_data *data,
				void (*alert_func)(void *),
				void *alert_data)
{
	//printk("tmp401_thermal_set_alert  \n");
	data->alert_data = alert_data;
	data->alert_func = alert_func;

	return 0;
}

int tmp401_thermal_set_shutdown_temp(struct tmp401_data *data,
					long shutdown_temp_milli)
{

	//struct tmp401_data *data = tmp401_update_device(&data->client->dev);
	//long shutdown_temp;
	u8 reg;
	int err;
	//shutdown_temp = MILLICELSIUS_TO_CELSIUS(shutdown_temp_milli);
	reg = tmp401_crit_temp_to_register(shutdown_temp_milli, data->config);
	//printk("tmp401_thermal_set_shutdown_temp shutdown_temp=%ld reg=%d\n",shutdown_temp_milli,reg);

	//mutex_lock(&data->update_lock);
	
	/* External temperature h/w shutdown limit */
	err =i2c_smbus_write_byte_data(data->client,
		TMP401_TEMP_CRIT_LIMIT[1], reg);
	if (err)
		return err;
	
	/* Local temperature h/w shutdown limit */
	err =i2c_smbus_write_byte_data(data->client,
		TMP401_TEMP_CRIT_LIMIT[0], reg);
	if (err)
		return err;
	
	data->temp_crit[1] = reg;
	data->temp_crit[0] = reg;
	//printk(" data->temp_crit=%d\n",reg);
	//mutex_unlock(&data->update_lock);

	return 0;
}
static int tmp401_enable(struct i2c_client *client)
{
	struct tmp401_data *data = i2c_get_clientdata(client);
	int err;

	err = i2c_smbus_write_byte_data(client, TMP401_CONFIG_WRITE,
				  data->config & ~TMP401_CONFIG_SHUTDOWN);
	if (err < 0)
		dev_err(&client->dev, "%s, line=%d, i2c write error=%d\n",
		__func__, __LINE__, err);
	return err;
}

static int tmp401_disable(struct i2c_client *client)
{
	struct tmp401_data *data = i2c_get_clientdata(client);
	int err;

	err = i2c_smbus_write_byte_data(client, TMP401_CONFIG_WRITE,
				  data->config | TMP401_CONFIG_SHUTDOWN);
	if (err < 0)
		dev_err(&client->dev, "%s, line=%d, i2c write error=%d\n",
		__func__, __LINE__, err);
	return err;
}

static void tmp401_work_func(struct work_struct *work)
{
	struct tmp401_data *data = container_of(work, struct tmp401_data,
						work);
	//int intr_status;
	struct timespec ts;

	tmp401_disable(data->client);

	if (data->alert_func)
	{
		//if (!nct1008_within_limits(data))
			data->alert_func(data->alert_data);
	}

	/* Initiate one-shot conversion */
	i2c_smbus_write_byte_data(data->client, TMP401_ONE_SHOT, 0x1);

	/* Give hardware necessary time to finish conversion */
	ts = ns_to_timespec(MAX_CONV_TIME_ONESHOT_MS * 1000 * 1000);
	hrtimer_nanosleep(&ts, NULL, HRTIMER_MODE_REL, CLOCK_MONOTONIC);

	//intr_status = i2c_smbus_read_byte_data(data->client, STATUS_RD);

	tmp401_enable(data->client);

	enable_irq(data->client->irq);
}

static irqreturn_t tmp401_irq(int irq, void *dev_id)
{
	struct tmp401_data *data = dev_id;

	disable_irq_nosync(irq);
	queue_work(data->workqueue, &data->work);
	return IRQ_HANDLED;
}

static int __devinit tmp401_configure_irq(struct tmp401_data *data)
{
	data->workqueue = create_singlethread_workqueue((data->kind == tmp401) \
							? "tmp401" : "tmp411");

	INIT_WORK(&data->work, tmp401_work_func);

	if (data->client->irq < 0)
		return 0;
	else
		return request_irq(data->client->irq, tmp401_irq,
			IRQF_TRIGGER_LOW,
			(data->kind == tmp401) ? "tmp401" : "tmp411",
			data);
}

static ssize_t show_temp_value(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp[index], data->config));
}
static ssize_t show_temperature_value(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	//int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d %d\n",
		tmp401_register_to_temp(data->temp[0],data->config),tmp401_register_to_temp(data->temp[1], data->config));
}

static ssize_t show_temp_min(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_low[index], data->config));
}

static ssize_t show_temp_max(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_high[index], data->config));
}

static ssize_t show_temp_crit(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
			tmp401_crit_register_to_temp(data->temp_crit[index],
							data->config));
}

static ssize_t show_temp_crit_hyst(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int temp, index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	mutex_lock(&data->update_lock);
	temp = tmp401_crit_register_to_temp(data->temp_crit[index],
						data->config);
	temp -= data->temp_crit_hyst * 1000;
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", temp);
}

static ssize_t show_temp_lowest(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_lowest[index],
					data->config));
}

static ssize_t show_temp_highest(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_highest[index],
					data->config));
}

static ssize_t show_status(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int mask = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	if (data->status & mask)
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static ssize_t store_temp_min(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u16 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_LOW_LIMIT_MSB_WRITE[index], reg >> 8);
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_LOW_LIMIT_LSB[index], reg & 0xFF);

	data->temp_low[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_max(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u16 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_HIGH_LIMIT_MSB_WRITE[index], reg >> 8);
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_HIGH_LIMIT_LSB[index], reg & 0xFF);

	data->temp_high[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_crit(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u8 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_crit_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_CRIT_LIMIT[index], reg);

	data->temp_crit[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_crit_hyst(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int temp, index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u8 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (data->config & TMP401_CONFIG_RANGE)
		val = SENSORS_LIMIT(val, -64000, 191000);
	else
		val = SENSORS_LIMIT(val, 0, 127000);

	mutex_lock(&data->update_lock);
	temp = tmp401_crit_register_to_temp(data->temp_crit[index],
						data->config);
	val = SENSORS_LIMIT(val, temp - 255000, temp);
	reg = ((temp - val) + 500) / 1000;

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_CRIT_HYST, reg);

	data->temp_crit_hyst = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

/*
 * Resets the historical measurements of minimum and maximum temperatures.
 * This is done by writing any value to any of the minimum/maximum registers
 * (0x30-0x37).
 */
static ssize_t reset_temp_history(struct device *dev,
	struct device_attribute	*devattr, const char *buf, size_t count)
{
	long val;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (val != 1) {
		dev_err(dev, "temp_reset_history value %ld not"
			" supported. Use 1 to reset the history!\n", val);
		return -EINVAL;
	}
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP411_TEMP_LOWEST_MSB[0], val);

	return count;
}

static struct sensor_device_attribute tmp401_attr[] = {
	SENSOR_ATTR(temp1_input, S_IRUGO, show_temp_value, NULL, 0),
	SENSOR_ATTR(temp1_min, S_IWUSR | S_IRUGO, show_temp_min,
		    store_temp_min, 0),
	SENSOR_ATTR(temp1_max, S_IWUSR | S_IRUGO, show_temp_max,
		    store_temp_max, 0),
	SENSOR_ATTR(temp1_crit, S_IWUSR | S_IRUGO, show_temp_crit,
		    store_temp_crit, 0),
	SENSOR_ATTR(temp1_crit_hyst, S_IWUSR | S_IRUGO, show_temp_crit_hyst,
		    store_temp_crit_hyst, 0),
	SENSOR_ATTR(temp1_min_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_LOW),
	SENSOR_ATTR(temp1_max_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_HIGH),
	SENSOR_ATTR(temp1_crit_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_CRIT),
	SENSOR_ATTR(temp2_input, S_IRUGO, show_temp_value, NULL, 1),
	SENSOR_ATTR(temp2_min, S_IWUSR | S_IRUGO, show_temp_min,
		    store_temp_min, 1),
	SENSOR_ATTR(temp2_max, S_IWUSR | S_IRUGO, show_temp_max,
		    store_temp_max, 1),
	SENSOR_ATTR(temp2_crit, S_IWUSR | S_IRUGO, show_temp_crit,
		    store_temp_crit, 1),
	SENSOR_ATTR(temp2_crit_hyst, S_IRUGO, show_temp_crit_hyst, NULL, 1),
	SENSOR_ATTR(temp2_fault, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_OPEN),
	SENSOR_ATTR(temp2_min_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_LOW),
	SENSOR_ATTR(temp2_max_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_HIGH),
	SENSOR_ATTR(temp2_crit_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_CRIT),
	SENSOR_ATTR(temperature, S_IRUGO, show_temperature_value, NULL, 0),
};

/*
 * Additional features of the TMP411 chip.
 * The TMP411 stores the minimum and maximum
 * temperature measured since power-on, chip-reset, or
 * minimum and maximum register reset for both the local
 * and remote channels.
 */
static struct sensor_device_attribute tmp411_attr[] = {
	SENSOR_ATTR(temp1_highest, S_IRUGO, show_temp_highest, NULL, 0),
	SENSOR_ATTR(temp1_lowest, S_IRUGO, show_temp_lowest, NULL, 0),
	SENSOR_ATTR(temp2_highest, S_IRUGO, show_temp_highest, NULL, 1),
	SENSOR_ATTR(temp2_lowest, S_IRUGO, show_temp_lowest, NULL, 1),
	SENSOR_ATTR(temp_reset_history, S_IWUSR, NULL, reset_temp_history, 0),
};

/*
 * Begin non sysfs callback code (aka Real code)
 */

static void tmp401_init_client(struct i2c_client *client)
{
	int config, config_orig;

	/* Set the conversion rate to 2 Hz */
	i2c_smbus_write_byte_data(client, TMP401_CONVERSION_RATE_WRITE, 5);

	/* Start conversions (disable shutdown if necessary) */
	config = i2c_smbus_read_byte_data(client, TMP401_CONFIG_READ);
	if (config < 0) {
		dev_warn(&client->dev, "Initialization failed!\n");
		return;
	}

	config_orig = config;
	config &= ~TMP401_CONFIG_SHUTDOWN;

	if (config != config_orig)
		i2c_smbus_write_byte_data(client, TMP401_CONFIG_WRITE, config);
}

static int tmp401_detect(struct i2c_client *client,
			 struct i2c_board_info *info)
{
	enum tmp_chips kind;
	struct i2c_adapter *adapter = client->adapter;
	u8 reg;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/* Detect and identify the chip */
	reg = i2c_smbus_read_byte_data(client, TMP401_MANUFACTURER_ID_REG);
	if (reg != TMP401_MANUFACTURER_ID)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, TMP401_DEVICE_ID_REG);
	//printk("tmp401 reg = 0x%x\n",reg);
	switch (reg) {
	case TMP401_DEVICE_ID:
		kind = tmp401;
		break;
	case TMP411_DEVICE_ID:
		kind = tmp411;
		break;
	default:
		return -ENODEV;
	}

	reg = i2c_smbus_read_byte_data(client, TMP401_CONFIG_READ);
	if (reg & 0x1b)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, TMP401_CONVERSION_RATE_READ);
	/* Datasheet says: 0x1-0x6 */
	if (reg > 15)
		return -ENODEV;

	strlcpy(info->type, tmp401_id[kind].name, I2C_NAME_SIZE);

	return 0;
}

static int tmp401_remove(struct i2c_client *client)
{
	struct tmp401_data *data = i2c_get_clientdata(client);
	int i;

	if (data->hwmon_dev)
		hwmon_device_unregister(data->hwmon_dev);

	for (i = 0; i < ARRAY_SIZE(tmp401_attr); i++)
		device_remove_file(&client->dev, &tmp401_attr[i].dev_attr);

	if (data->kind == tmp411) {
		for (i = 0; i < ARRAY_SIZE(tmp411_attr); i++)
			device_remove_file(&client->dev,
					   &tmp411_attr[i].dev_attr);
	}

	kfree(data);
	return 0;
}

static int tmp401_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int i, err = 0;
	struct tmp401_data *data;
	const char *names[] = { "TMP401", "TMP411" };

	data = kzalloc(sizeof(struct tmp401_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	
	data->client = client;
	memcpy(&data->plat_data, client->dev.platform_data,
		sizeof(struct tmp401_platform_data));
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->kind = id->driver_data;

	/* Initialize the TMP401 chip */
	tmp401_init_client(client);

	/* Register sysfs hooks */
	for (i = 0; i < ARRAY_SIZE(tmp401_attr); i++) {
		err = device_create_file(&client->dev,
					 &tmp401_attr[i].dev_attr);
		if (err)
			goto exit_remove;
	}

	/* Register aditional tmp411 sysfs hooks */
	if (data->kind == tmp411) {
		for (i = 0; i < ARRAY_SIZE(tmp411_attr); i++) {
			err = device_create_file(&client->dev,
						 &tmp411_attr[i].dev_attr);
			if (err)
				goto exit_remove;
		}
	}
	tmp401_configure_irq(data);
	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		data->hwmon_dev = NULL;
		goto exit_remove;
	}
	
	/* notify callback that probe is done */
	if (data->plat_data.probe_callback)
		data->plat_data.probe_callback(data);

	dev_info(&client->dev, "Detected TI %s chip\n", names[data->kind]);

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	is_suspended = false;
	printk("%s() success\n", __func__);
#endif

	return 0;

exit_remove:
	tmp401_remove(client); /* will also free data for us */
	return err;
}
#ifdef CONFIG_PM
static int tmp401_suspend(struct i2c_client *client, pm_message_t state)
{
	int err;

	disable_irq(client->irq);
	err = tmp401_disable(client);
#ifdef CONFIG_TEGRA_SKIN_THROTTLE	
	is_suspended = true;
#endif	
	return err;
}

static int tmp401_resume(struct i2c_client *client)
{
	int err;

	err = tmp401_enable(client);
	if (err < 0) {
		dev_err(&client->dev, "Error: %s, error=%d\n",
			__func__, err);
#ifdef CONFIG_TEGRA_SKIN_THROTTLE			
		is_suspended = false;
#endif		
		return err;
	}
	enable_irq(client->irq);
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	is_suspended = false;
#endif	
	return 0;
}
#endif

static struct i2c_driver tmp401_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "tmp401",
	},
	.probe		= tmp401_probe,
	.remove		= tmp401_remove,
	.id_table	= tmp401_id,
	.detect		= tmp401_detect,
	.address_list	= normal_i2c,
#ifdef CONFIG_PM
	.suspend	= tmp401_suspend,
	.resume 	= tmp401_resume,
#endif
};

static int __init tmp401_init(void)
{
	return i2c_add_driver(&tmp401_driver);
}

static void __exit tmp401_exit(void)
{
	i2c_del_driver(&tmp401_driver);
}

MODULE_AUTHOR("Hans de Goede <hdegoede@redhat.com>");
MODULE_DESCRIPTION("Texas Instruments TMP401 temperature sensor driver");
MODULE_LICENSE("GPL");

module_init(tmp401_init);
module_exit(tmp401_exit);
