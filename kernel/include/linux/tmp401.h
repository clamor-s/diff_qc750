/*
 * include/linux/TMP401.h
 *
 * NCT1008, temperature monitoring device from ON Semiconductors
 *
 * Copyright (c) 2010-2012, NVIDIA Corporation.
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

#ifndef _LINUX_TMP401_H
#define _LINUX_TMP401_H

#include <linux/types.h>
#include <linux/workqueue.h>

#include <mach/edp.h>

#define MAX_ZONES	16

struct tmp401_data;
enum tmp_chips { tmp401, tmp411 };

struct tmp401_platform_data {
	bool supported_hwrev;
	bool ext_range;
	u8 conv_rate;
	u8 offset;
	u8 hysteresis;
	s8 shutdown_ext_limit;
	s8 shutdown_local_limit;
	s8 throttling_ext_limit;
	s8 thermal_zones[MAX_ZONES];
	u8 thermal_zones_sz;
	void (*alarm_fn)(bool raised);
	void (*probe_callback)(struct tmp401_data *);
};

struct tmp401_data {
	struct device *hwmon_dev;
	struct workqueue_struct *workqueue;
	struct work_struct work;
	struct i2c_client *client;
	struct tmp401_platform_data plat_data;
	struct mutex update_lock;
	char valid; /* zero until following fields are valid */
	unsigned long last_updated; /* in jiffies */
	enum tmp_chips kind;

	/* register values */
	u8 status;
	u8 config;
	u16 temp[2];
	u16 temp_low[2];
	u16 temp_high[2];
	u8 temp_crit[2];
	u8 temp_crit_hyst;
	u16 temp_lowest[2];
	u16 temp_highest[2];

	long current_lo_limit;
	long current_hi_limit;
	void (*alert_func)(void *);
	void *alert_data;
};




#ifdef CONFIG_SENSORS_TMP401
int tmp401_thermal_get_temp(struct tmp401_data *data, long *temp);
int tmp401_thermal_get_temps(struct tmp401_data *data, long *etemp,
				long *itemp);
int tmp401_thermal_get_temp_low(struct tmp401_data *data, long *temp);
int tmp401_thermal_set_limits(struct tmp401_data *data,
				long lo_limit_milli,
				long hi_limit_milli);
int tmp401_thermal_set_alert(struct tmp401_data *data,
				void (*alert_func)(void *),
				void *alert_data);
int tmp401_thermal_set_shutdown_temp(struct tmp401_data *data,
					long shutdown_temp);
#else
static inline int tmp401_thermal_get_temp(struct tmp401_data *data,
						long *temp)
{ return -EINVAL; }
static inline int tmp401_thermal_get_temps(struct tmp401_data *data,
						long *etemp, long *itemp)
{ return -EINVAL; }
static inline int tmp401_thermal_get_temp_low(struct tmp401_data *data,
						long *temp)
{ return -EINVAL; }
static inline int tmp401_thermal_set_limits(struct tmp401_data *data,
				long lo_limit_milli,
				long hi_limit_milli)
{ return -EINVAL; }
static inline int tmp401_thermal_set_alert(struct tmp401_data *data,
				void (*alert_func)(void *),
				void *alert_data)
{ return -EINVAL; }
static inline int tmp401_thermal_set_shutdown_temp(struct tmp401_data *data,
					long shutdown_temp)
{ return -EINVAL; }
#endif

#endif /* _LINUX_TMP401_H */
