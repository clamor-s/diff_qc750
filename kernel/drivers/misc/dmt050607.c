/*
 * @file drivers/misc/dmt050607.c
 * @brief DMT g-sensor Linux device driver
 * @author Domintech Technology Co., Ltd (http://www.domintech.com.tw)
 * @version 1.33a
 * @date 2012/6/2
 *
 * @section LICENSE
 *
 *  Copyright 2011 Domintech Technology Co., Ltd
 *
 * 	This software is licensed under the terms of the GNU General Public
 * 	License version 2, as published by the Free Software Foundation, and
 * 	may be copied, distributed, and modified under those terms.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 *  V1.32	ADD AUTO_CALIBRATION FUNCTION
 *  V1.33	1. ADD device_i2c_txdata FUNCTION
 *			2. SET BANDWIDTH_REG to No filter
 *  V1.33a	1. Default Open AUTO_CALIBRATION FUNCTION
 *			2. Check the file "offset.txt". If not exist,immediate implementation of the automatic calibration.
 */
#include "dmt.h"
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/serio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
//****FILE offset.txt*******************************
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

wait_queue_head_t	open_wq;
atomic_t		active;
static unsigned int interval;
struct mutex DMT_mutex;
struct delayed_work work;
struct work_struct irq_work;
atomic_t delay;
atomic_t enable;

static int mDeviceSleep;

void gsensor_write_offset_to_file(void);
void gsensor_read_offset_from_file(void);
char OffsetFileName[] = "/forever/misc/gsensor/offset.txt";
//**************************************************
static char const *const ACCELEMETER_CLASS_NAME = "accelemeter";

#if (defined(CONFIG_SENSORS_DMARD05) || defined(CONFIG_SENSORS_DMARD05_MODULE))
static char const *const GSENSOR_DEVICE_NAME = "dmard05";
#elif (defined(CONFIG_SENSORS_DMARD06) || defined(CONFIG_SENSORS_DMARD06_MODULE))
static char const *const GSENSOR_DEVICE_NAME = "dmard06";
#elif (defined(CONFIG_SENSORS_DMARD07) || defined(CONFIG_SENSORS_DMARD07_MODULE))
static char const *const GSENSOR_DEVICE_NAME = "dmard07";
#endif

static int device_init(void);
static void device_exit(void);

static int device_open(struct inode*, struct file*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int device_close(struct inode*, struct file*);

static int device_i2c_suspend(struct i2c_client *client, pm_message_t mesg);
static int device_i2c_resume(struct i2c_client *client);
static int __devinit device_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __devexit device_i2c_remove(struct i2c_client *client);
static void device_i2c_read_xyz(struct i2c_client *client, s8 *xyz);
static int device_i2c_rxdata(struct i2c_client *client, unsigned char *buffer, int length);
static int device_i2c_txdata(struct i2c_client *client, unsigned char *txData, int length);
struct input_dev 	*input;

static int DMT_GetOpenStatus(void)
{	
#if DMT_DEBUG_DATA
	printk("start active=%d\n",active.counter);
#endif
	wait_event_interruptible(open_wq, (atomic_read(&active) != 0));
	return 0;
}

static int DMT_GetCloseStatus(void)
{
#if DMT_DEBUG_DATA
	printk("start active=%d\n",active.counter);
#endif
	wait_event_interruptible(open_wq, (atomic_read(&active) <= 0));
	return 0;
}

static void DMT_sysfs_update_active_status(int en)
{
	unsigned long dmt_delay;
	if(en)
	{
		dmt_delay=msecs_to_jiffies(atomic_read(&delay))/1000;
		if(dmt_delay<1)
			dmt_delay=1;
#if DMT_DEBUG_DATA
		printk("schedule_delayed_work start with delay time=%lu\n",dmt_delay);
#endif
		cancel_delayed_work_sync(&work); //cancel last one ;densy
		schedule_delayed_work(&work,dmt_delay);
	}
	else 
		cancel_delayed_work_sync(&work);
}

static bool get_value_as_int(char const *buf, size_t size, int *value)
{
	long tmp;

	if (size == 0)
		return false;

	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtol(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtol(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtol(buf, 10, &tmp))
			return false;
	}

	if (tmp > INT_MAX)
		return false;

	*value = tmp;

	return true;
}
static bool get_value_as_int64(char const *buf, size_t size, long long *value)
{
	long long tmp;

	if (size == 0)
		return false;

	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtoll(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtoll(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtoll(buf, 10, &tmp))
			return false;
	}

	if (tmp > LLONG_MAX)
		return false;

	*value = tmp;

	return true;
}
static ssize_t DMT_enable_acc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char str[2][16]={"ACC enable OFF","ACC enable ON"};
	int flag;
	flag=atomic_read(&enable); 
	
	return sprintf(buf, "%s\n", str[flag]);
}

static ssize_t DMT_enable_acc_store( struct device *dev, struct device_attribute *attr, char const *buf, size_t count)
{
	int en;
#if DMT_DEBUG_DATA
	printk("%s:buf=%x %x\n",__func__,buf[0],buf[1]);
#endif	
	if (false == get_value_as_int(buf, count, &en))
		return -EINVAL;

	mutex_lock(&DMT_mutex);

	en = en ? 1 : 0;
	atomic_set(&enable,en);
	DMT_sysfs_update_active_status(en);

	mutex_unlock(&DMT_mutex);
	return 1;
}

static ssize_t DMT_delay_acc_show( struct device *dev, struct device_attribute *attr, char *buf)
{
	
	return sprintf(buf, "%d\n", atomic_read(&delay));
}

static ssize_t DMT_delay_acc_store( struct device *dev, struct device_attribute *attr,char const *buf, size_t count)
{
	long long data;
	if (false == get_value_as_int64(buf, count, &data))
		return -EINVAL;
	//mutex_lock(&DMT_mutex);
	//interval=(unsigned int)data;       
	//mutex_unlock(&DMT_mutex);
	atomic_set(&delay, (unsigned int) data);
#if DMT_DEBUG_DATA
	printk("Driver attribute set delay =%lu\n",data);
#endif
	return count;
}

static struct device_attribute DMT_attributes[] = {
	__ATTR(enable_acc, 0660, DMT_enable_acc_show, DMT_enable_acc_store),
	__ATTR(delay_acc,  0660, DMT_delay_acc_show,  DMT_delay_acc_store),
	__ATTR_NULL,
};

static int create_device_attributes(struct device *dev,	struct device_attribute *attrs)
{
	int i;
	int err = 0;
	for (i = 0 ; NULL != attrs[i].attr.name ; ++i) 
	{
		err = device_create_file(dev, &attrs[i]);
		if (0 != err)
			break;
	}

	if (0 != err) 
	{
		for (; i >= 0 ; --i)
			device_remove_file(dev, &attrs[i]);
	}
	return err;
}

int input_init(void)
{
	int err=0;
	input=input_allocate_device();
	if (!input)
		return -ENOMEM;
	else
#if DMT_DEBUG_DATA
	   printk("input device allocate Success !!\n");
#endif
	
	input->id.bustype = BUS_I2C;
	/* Setup input device */
	set_bit(EV_ABS, input->evbit);
	/* Accelerometer [-78.5, 78.5]m/s2 in Q16*/
	input_set_abs_params(input, ABS_X, -5144576, 5144576, 0, 0);
	input_set_abs_params(input, ABS_Y, -5144576, 5144576, 0, 0);
	input_set_abs_params(input, ABS_Z, -5144576, 5144576, 0, 0);

	/* Set InputDevice Name */
	//input->name = INPUT_NAME_ACC;
	input->name = GSENSOR_DEVICE_NAME;
	/* Register */
	err = input_register_device(input);
	if (err) {
		input_free_device(input);
		return err;
	}
	atomic_set(&active, 0);
	printk("in driver ,active=%d\n",active.counter);

	init_waitqueue_head(&open_wq);

	return err;
}

typedef union {
	struct {
		s8	x;
		s8	y;
		s8	z;
	} u;
	s8	v[SENSOR_DATA_SIZE];
} raw_data;
static raw_data offset;

struct dev_data {
	dev_t devno;
	struct cdev cdev;
  	struct class *class;
	struct i2c_client *client;
};
static struct dev_data dev;

s8 sensorlayout[3][3] = {
#if defined(CONFIG_GSEN_LAYOUT_PAT_1)
    { 1, 0, 0},	{ 0, 1,	0}, { 0, 0, 1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_2)
    { 0, 1, 0}, {-1, 0,	0}, { 0, 0, 1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_3)
    {-1, 0, 0},	{ 0,-1,	0}, { 0, 0, 1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_4)
    { 0,-1, 0},	{ 1, 0,	0}, { 0, 0, 1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_5)
    {-1, 0, 0},	{ 0, 1,	0}, { 0, 0,-1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_6)
    { 0,-1, 0}, {-1, 0,	0}, { 0, 0,-1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_7)
    { 1, 0, 0},	{ 0,-1,	0}, { 0, 0,-1},
#elif defined(CONFIG_GSEN_LAYOUT_PAT_8)
    { 0, 1, 0},	{ 1, 0,	0}, { 0, 0,-1},
#endif
};

static int gsensor_read_accel_avg(raw_data *avg_p )
{
   	long xyz_acc[SENSOR_DATA_SIZE];   
  	s8 xyz[SENSOR_DATA_SIZE];
  	int i, j;
	
	//initialize the accumulation buffer
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i) 
		xyz_acc[i] = 0;

	for(i = 0; i < AVG_NUM; i++) 
	{      
		device_i2c_read_xyz(dev.client, (s8 *)&xyz);
		for(j = 0; j < SENSOR_DATA_SIZE; ++j) 
			xyz_acc[j] += xyz[j];
  	}

	// calculate averages
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i) 
		avg_p->v[i] = (s8) (xyz_acc[i] / AVG_NUM);
		
	if(avg_p->v[2] < 0)
		return CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_NEGATIVE;
	else
		return CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_POSITIVE;
}
/* calc delta offset */
static int gsensor_calculate_offset(int gAxis,raw_data avg)
{
	switch(gAxis)
	{
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_NEGATIVE:  
			offset.u.x =  avg.u.x ;    
			offset.u.y =  avg.u.y ;
			offset.u.z =  avg.u.z + DEFAULT_SENSITIVITY;
		  break;
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_X_POSITIVE:  
			offset.u.x =  avg.u.x + DEFAULT_SENSITIVITY;    
			offset.u.y =  avg.u.y ;
			offset.u.z =  avg.u.z ;
		  break;
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_POSITIVE:  
			offset.u.x =  avg.u.x ;    
			offset.u.y =  avg.u.y ;
			offset.u.z =  avg.u.z - DEFAULT_SENSITIVITY;
		  break;
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_X_NEGATIVE:  
			offset.u.x =  avg.u.x - DEFAULT_SENSITIVITY;    
			offset.u.y =  avg.u.y ;
			offset.u.z =  avg.u.z ;
		  break;
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Y_NEGATIVE:
			offset.u.x =  avg.u.x ;    
			offset.u.y =  avg.u.y + DEFAULT_SENSITIVITY;
			offset.u.z =  avg.u.z ;
		  break;
		case CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Y_POSITIVE: 
			offset.u.x =  avg.u.x ;    
			offset.u.y =  avg.u.y - DEFAULT_SENSITIVITY;
			offset.u.z =  avg.u.z ;
		  break;
		default:  
			return -ENOTTY;
	}
	return 0;
}

static void gsensor_calibrate(int side)
{	
	raw_data avg;
	// get acceleration average reading
	gsensor_read_accel_avg(&avg);
	// calculate and set the offset
	gsensor_calculate_offset(side, avg); 
}

static int gsensor_reset(struct i2c_client *client)
{
	unsigned char buffer[2];
	buffer[0] = SW_RESET_REG;
	device_i2c_rxdata(client, buffer, 1);
#if DMT_DEBUG_DATA
	IN_FUNC_MSG;
	printk(KERN_INFO "i2c Read SW_RESET = %x \n", buffer[0]);
#endif
	buffer[0] = WHO_AM_I_REG;
	device_i2c_rxdata(client, buffer, 1);
#if DMT_DEBUG_DATA
		printk(KERN_INFO "i2c Read WHO_AM_I = %d \n", buffer[0]);
#endif
	if( ( buffer[0]&0x00FF) == WHO_AM_I_VALUE)
	{
		printk(KERN_INFO "@@@ %s DMT_DEVICE_NAME registered I2C driver!\n",__FUNCTION__);
		dev.client = client;
	}
	else
	{
		printk(KERN_INFO "@@@ %s gsensor I2C err = %d!\n",__FUNCTION__,buffer[1]);
		dev.client = NULL;
		return -1;
	}
	//set Low-pass Filter to No filter
#if DMT_DEBUG_DATA
	buffer[0] = CONTROL_REGISTER_2_REG;
	device_i2c_rxdata(client, buffer, 1);
	printk(KERN_INFO " Default Control Register 2 = %d \n", buffer[0]);
#endif
	buffer[0] = CONTROL_REGISTER_2_REG;
	buffer[1] = CONTROL_REGISTER_2_VALUE;
	device_i2c_txdata(client, buffer, 2);	

	return 0;
}

static void gsensor_set_offset(int val[3])
{
	int i;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		offset.v[i] = (s8) val[i];
}

struct file_operations dmt_g_sensor_fops = 
{
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_close,
};

static const struct i2c_device_id device_i2c_ids[] = {
	{DEVICE_I2C_NAME, 0},
	{}   
};

MODULE_DEVICE_TABLE(i2c, device_i2c_ids);

static struct i2c_driver device_i2c_driver = 
{
	.driver	= {
		.owner = THIS_MODULE,
		.name = DEVICE_I2C_NAME,
		},
	.class = I2C_CLASS_HWMON,
	.probe = device_i2c_probe,
	.remove	= __devexit_p(device_i2c_remove),
	.suspend = device_i2c_suspend,
	.resume	= device_i2c_resume,
	.id_table = device_i2c_ids,
};

static int device_open(struct inode *inode, struct file *filp)
{
	return 0; 
}

static ssize_t device_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static ssize_t device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s8 xyz[SENSOR_DATA_SIZE];
	int i;

	device_i2c_read_xyz(dev.client, (s8 *)&xyz);
	//offset compensation 
	for(i = 0; i < SENSOR_DATA_SIZE; i++)
		xyz[i] -= offset.v[i];
	
	if(copy_to_user(buf, &xyz, count)) 
		return -EFAULT;
	return count;
}


static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, ret = 0, i;
	int intBuf[SENSOR_DATA_SIZE];
	s8 xyz[SENSOR_DATA_SIZE];
	//check type and number
	if (_IOC_TYPE(cmd) != IOCTL_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SENSOR_MAXNR) return -ENOTTY;

	//check user space pointer is valid
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	
	switch(cmd) 
	{
		case SENSOR_RESET:
			gsensor_reset(dev.client);
			return ret;

		case SENSOR_CALIBRATION:
			// get orientation info
			if(copy_from_user(&intBuf, (int*)arg, sizeof(intBuf))) return -EFAULT;
			gsensor_calibrate(intBuf[0]);
			printk("intBuf[0] %d\n",intBuf[0]);
			// save file
			gsensor_write_offset_to_file();
			
			// return the offset
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;
		
		case SENSOR_GET_OFFSET:
			// get data from file 
			gsensor_read_offset_from_file();
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;

		case SENSOR_SET_OFFSET:
			ret = copy_from_user(&intBuf, (int *)arg, sizeof(intBuf));
			gsensor_set_offset(intBuf);
			// write in to file
			gsensor_write_offset_to_file();
			return ret;
		
		case SENSOR_READ_ACCEL_XYZ:

			device_i2c_read_xyz(dev.client, (s8 *)&xyz);
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = xyz[i] - offset.v[i];
			
		  	ret = copy_to_user((int*)arg, &intBuf, sizeof(intBuf));
#if DMT_DEBUG_DATA
	IN_FUNC_MSG;
	PRINT_X_Y_Z(intBuf[0], intBuf[1], intBuf[2]);
#endif
			return ret;
		case SENSOR_SETYPR:
			if(copy_from_user(&intBuf, (int*)arg, sizeof(intBuf))) 
			{
				printk("%s:copy_from_user(&intBuf, (int*)arg, sizeof(intBuf)) ERROR, -EFAULT\n",__func__);			
				return -EFAULT;
			}
			input_report_abs(input, ABS_X, intBuf[0]);
			input_report_abs(input, ABS_Y, intBuf[1]);
			input_report_abs(input, ABS_Z, intBuf[2]);
			input_sync(input);
			//printk("%s:SENSOR_SETYPR OK! x=%d,y=%d,z=%d\n",__func__,intBuf[0],intBuf[1],intBuf[2]);
			return 1;
		case SENSOR_GET_OPEN_STATUS:
			//printk("%s:Going into DMT_GetOpenStatus()\n",__func__);
			DMT_GetOpenStatus();
			//printk("%s:DMT_GetOpenStatus() finished\n",__func__);
			return 1;
			break;
		case SENSOR_GET_CLOSE_STATUS:
			//printk("%s:Going into DMT_GetCloseStatus()\n",__func__);
			DMT_GetCloseStatus();	
			//printk("%s:DMT_GetCloseStatus() finished\n",__func__);
			return 1;
			break;		
		case SENSOR_GET_DELAY:
		  	ret = copy_to_user((int*)arg, &interval, sizeof(interval));
			return 1;
			break;
		
		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	
	return 0;
}
	
static int device_close(struct inode *inode, struct file *filp)
{
	return 0;
}

/***** I2C I/O function ***********************************************/
static int device_i2c_rxdata( struct i2c_client *client, unsigned char *rxData, int length)
{
	struct i2c_msg msgs[] = 
	{
		{.addr = client->addr, .flags = 0, .len = 1, .buf = rxData,}, 
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = rxData,},
	};
#if DMT_DEBUG_DATA
	unsigned char addr = rxData[0];
#endif
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}
	DMT_DATA(&client->dev, "RxData: len=%02x, addr=%02x, data=%02x\n",
		length, addr, rxData[0]);

	return 0;
}

static int device_i2c_txdata( struct i2c_client *client, unsigned char *txData, int length)
{
	struct i2c_msg msg[] = 
	{
		{.addr = client->addr, .flags = 0, .len = length, .buf = txData,}, 
	};

	if (i2c_transfer(client->adapter, msg, 1) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}
	DMT_DATA(&client->dev, "TxData: len=%02x, addr=%02x data=%02x\n",
		length, txData[0], txData[1]);
	return 0;
}

static inline void device_i2c_correct_accel_sign(s8 *val)
{
	*val>>= (sizeof(s8) * BITS_PER_BYTE - 7);  
}

void device_i2c_read_xyz(struct i2c_client *client, s8 *xyz_p)
{
	s8 xyzTmp[SENSOR_DATA_SIZE];
	int i, j;
	u8 buffer[3];
	buffer[0] = X_OUT_REG;

	if(client==NULL){
		printk("dmt06 initialization fail??????\n");
		return;
	}
	
	device_i2c_rxdata(client, buffer, 3); 
	
#if (defined(CONFIG_SENSORS_DMARD06) || defined(CONFIG_SENSORS_DMARD06_MODULE))
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
	{
		xyzTmp[i] = buffer[i];
		device_i2c_correct_accel_sign(xyzTmp+i);		
	}
#else
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
	{
		xyzTmp[i] = buffer[i];
	}
#endif
	//transfer to the default layout
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
	{
		xyz_p[i] = 0;
		for(j = 0; j < SENSOR_DATA_SIZE; ++j)
			xyz_p[i] += sensorlayout[i][j] * xyzTmp[j];
	}
}

static int device_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	mDeviceSleep =1;
	printk("DMT06 suspend\n");

	cancel_delayed_work_sync(&work);
	
	return 0;
}

static void DMT_work_func(struct work_struct *fakework)
{
	int i;
	static int firsttime=0;
	s8 xyz[SENSOR_DATA_SIZE];
	unsigned long t=atomic_read(&delay);
  	unsigned long dmt_delay = msecs_to_jiffies(t);
	//printk("***********DMT_work_func***************\n",__func__);
	if(mDeviceSleep==1)//sleepping
	{
		return;
	}
	
	if(!firsttime)
	{
		gsensor_read_offset_from_file();	
	 	firsttime=1;
	}
	
	dmt_delay/=1000;
#if DMT_DEBUG_DATA
	printk("t=%lu , dmt_delay=%lu\n", t, dmt_delay);
#endif
  	device_i2c_read_xyz(dev.client, (s8 *)&xyz);
#if DMT_DEBUG_DATA
	printk("@@@ raw data @@@\n");
	PRINT_X_Y_Z(xyz[0], xyz[1], xyz[2]);
#endif
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
     		xyz[i] -= offset.v[i];

#if DMT_DEBUG_DATA
	printk("@@@ offset @@@\n");
	printk("%5d %5d %5d\n",offset.u.x,offset.u.y,offset.u.z);
	printk("@@@ xyz - offset @@@\n");
	PRINT_X_Y_Z(xyz[0], xyz[1], xyz[2]);
#endif
	input_report_abs(input, ABS_X, xyz[0]);
	input_report_abs(input, ABS_Y, xyz[1]);
	input_report_abs(input, ABS_Z, xyz[2]);
	input_sync(input);
		
	if(dmt_delay<1)
		dmt_delay=1;
	schedule_delayed_work(&work, dmt_delay);

}
static device_init_dev()
{
	int err=-1;	
	struct device *device;
	int ret = 0;
	IN_FUNC_MSG;
	ret = alloc_chrdev_region(&dev.devno, 0, 1, GSENSOR_DEVICE_NAME);
  	if(ret)
	{
		printk("%s, can't allocate chrdev\n", __func__);
		return ret;
	}
	printk("%s, register chrdev(%d, %d)\n", __func__, MAJOR(dev.devno), MINOR(dev.devno));
	
	cdev_init(&dev.cdev, &dmt_g_sensor_fops);  
	dev.cdev.owner = THIS_MODULE;
  	ret = cdev_add(&dev.cdev, dev.devno, 1);
  	if(ret < 0)
	{
		printk("%s, add character device error, ret %d\n", __func__, ret);
		return ret;
	}
	dev.class = class_create(THIS_MODULE, ACCELEMETER_CLASS_NAME);
	if(IS_ERR(dev.class))
	{
   		printk("%s, create class, error\n", __func__);
		return ret;
  	}
	device=device_create(dev.class, NULL, dev.devno, NULL, GSENSOR_DEVICE_NAME);

	mutex_init(&DMT_mutex);

	INIT_DELAYED_WORK(&work, DMT_work_func);
	printk("DMT: INIT_DELAYED_WORK\n");

	err=input_init();
	if(err)
		printk("%s:input_init fail, error code= %d\n",__func__,err);

	err = create_device_attributes(device,DMT_attributes);
	
}
static int __devinit device_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int i;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		offset.v[i] = 0;
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
  		printk("%s, functionality check failed\n", __func__);
		return -1;
  	}

	mDeviceSleep =0;

	dev.client=NULL;

	int tryCount =10;

	while(dev.client==NULL){

		if(tryCount-- <=0){
			printk("init gsensor fail...give up!!!!!!!!!!!!!!!!!!!\n");
			return -1;
		}
		
		gsensor_reset(client);

		if(dev.client==NULL){
			mdelay(1000);

			printk("gsensor init fail , try again left try=%d\n",tryCount);
		}
	}
	device_init_dev();
	return 0;
}

static int __devexit device_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int device_i2c_resume(struct i2c_client *client)
{
	mDeviceSleep =0;

	printk("DMT06 resume\n");

	int flag;

	mutex_lock(&DMT_mutex); //densy added
	
	flag=atomic_read(&enable);

	if(flag!=0){
		cancel_delayed_work_sync(&work);
		
		int dmt_delay=msecs_to_jiffies(atomic_read(&delay))/1000;
		if(dmt_delay<1)
			dmt_delay=1;

		schedule_delayed_work(&work,dmt_delay);
	}
	mutex_unlock(&DMT_mutex);
	
	return 0;
}
static int __init device_init(void)
{
	
	return i2c_add_driver(&device_i2c_driver);
}


static void __exit device_exit(void)
{
	IN_FUNC_MSG;
	input_unregister_device(input);
	input_free_device(input);
	cdev_del(&dev.cdev);
	unregister_chrdev_region(dev.devno, 1);
	device_destroy(dev.class, dev.devno);
	class_destroy(dev.class);
	i2c_del_driver(&device_i2c_driver);
}

void gsensor_write_offset_to_file(void)
{
	char data[18];
	unsigned int orgfs;
	long lfile=-1;
	sprintf(data,"%5d %5d %5d",offset.u.x,offset.u.y,offset.u.z);

	orgfs = get_fs();
// Set segment descriptor associated to kernel space
	set_fs(KERNEL_DS);
	
    	lfile=sys_open(OffsetFileName,O_WRONLY|O_CREAT, 0777);
	if (lfile < 0)
	{
		printk("sys_open %s error!!. %ld\n",OffsetFileName,lfile);
	}
	else
	{
		sys_write(lfile, data,18);
		sys_close(lfile);
	}
	
	set_fs(orgfs);
}

void gsensor_read_offset_from_file(void)
{
	unsigned int orgfs;
	char data[18];
	long lfile=-1;
	raw_data avg;
	int ret = 0;
	int ux,uy,uz;
	orgfs = get_fs();
// Set segment descriptor associated to kernel space
	set_fs(KERNEL_DS);

	lfile = sys_open(OffsetFileName, O_RDONLY, 0);
	if (lfile < 0)
	{
		offset.v[0]=0;offset.v[1]=0;offset.v[2]=0;
#if AUTO_CALIBRATION
		// get acceleration average reading
		ret = gsensor_read_accel_avg(&avg);
		printk("@@@ cheak GRAVITY @@@ ret = %d\n",ret);
		
		if( ret == CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_NEGATIVE || ret == CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_POSITIVE )
		{	
			gsensor_calibrate(ret);
			// write in to file	
			gsensor_write_offset_to_file();
		}
		else{
			offset.v[0]=0;offset.v[1]=0;offset.v[2]=0;
		}
#endif
	}
	else{
		sys_read(lfile, data, 18);
		
		sscanf(data,"%d %d %d",&ux,&uy,&uz);
		offset.u.x=ux;
		offset.u.y=uy;
		offset.u.z=uz;
		
		sys_close(lfile);
	}
	printk("%5d %5d %5d",offset.u.x,offset.u.y,offset.u.z);
	set_fs(orgfs);
}
//*********************************************************************************************************
MODULE_AUTHOR("DMT_RD");
MODULE_DESCRIPTION("DMT Gsensor Driver");
MODULE_LICENSE("GPL");

module_init(device_init);
module_exit(device_exit);
