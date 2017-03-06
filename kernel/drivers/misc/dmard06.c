/*
 * @file drivers/i2c/dmard06.c
 * @brief DMARD06 g-sensor Linux device driver
 * @author Domintech Technology Co., Ltd (http://www.domintech.com.tw)
 * @version 1.22
 * @date 2011/12/01
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
 *
 */
#include <linux/dmard06.h>
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
//****Add by Steve Huang*********2011-11-18********
#include <linux/syscalls.h>
void gsensor_write_offset_to_file(void);
void gsensor_read_offset_from_file(void);
char OffsetFileName[] = "/data/misc/dmt/offset.txt";
static int Device_First_Time_Opened_flag=1;
//*************************************************
static char const *const ACCELEMETER_CLASS_NAME = "accelemeter";
static char const *const GSENSOR_DEVICE_NAME = "dmard06";

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
void device_i2c_read_xyz(struct i2c_client *client, s8 *xyz);

typedef union {
	struct {
		s8	x;
		s8	y;
		s8	z;
	} u;
	s8	v[SENSOR_DATA_SIZE];
} raw_data;
static raw_data offset;

struct dev_data 
{
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

void gsensor_read_accel_avg(raw_data *avg_p )
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
}
/* calc delta offset */
int gsensor_calculate_offset(int gAxis,raw_data avg)
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

void gsensor_calibrate(int side)
{	
	raw_data avg;
	IN_FUNC_MSG;
	// get acceleration average reading
	gsensor_read_accel_avg(&avg);
	// calculate and set the offset
	gsensor_calculate_offset(side, avg); 
}

int gsensor_reset(struct i2c_client *client)
{
	char cAddress = 0 , cData = 0;
	
	cAddress = SW_RESET;
	i2c_master_send( client, (char*)&cAddress, 1);
	i2c_master_recv( client, (char*)&cData, 1);
	printk(KERN_INFO "i2c Read SW_RESET = %x \n", cData);
	
	//read WHO_AM_I, it must equal 05
	cAddress = WHO_AM_I;
	i2c_master_send( client, (char*)&cAddress, 1);
	i2c_master_recv( client, (char*)&cData, 1);	
	printk(KERN_INFO "i2c Read WHO_AM_I = %d \n", cData);

	if(( cData&0x00FF) == WHO_AM_I_VALUE) //read 0Fh should be 05, else some err there
	{
		printk(KERN_INFO "@@@ %s gsensor registered I2C driver!\n",__FUNCTION__);
		dev.client = client;
	}
	else
	{
		printk(KERN_INFO "@@@ %s gsensor I2C err = %d!\n",__FUNCTION__,cData);
		dev.client = NULL;
		return -1;
	}
	return 0;
}

void gsensor_set_offset(int val[3])
{
	int i;
	IN_FUNC_MSG;
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

static const struct i2c_device_id device_i2c_ids[] = 
{
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
	IN_FUNC_MSG;
	//Add by Steve Huang 2011-11-30
		if(Device_First_Time_Opened_flag)
	{
          Device_First_Time_Opened_flag=0;
	      gsensor_read_offset_from_file();
	}
	printk("device_open: open_from_Android\n");
	return 0; 
}

static ssize_t device_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	IN_FUNC_MSG;
	
	return 0;
}

static ssize_t device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s8 xyz[SENSOR_DATA_SIZE];
	int i;
	short tmpBuffer[SENSOR_DATA_SIZE];

	IN_FUNC_MSG;
	device_i2c_read_xyz(dev.client, (s8 *)&xyz);
	//offset compensation 
	for(i = 0; i < SENSOR_DATA_SIZE; i++)
		xyz[i] -= offset.v[i];

	for(i = 0; i < SENSOR_DATA_SIZE; i++)
		tmpBuffer[i] = xyz[i];
	
	if(copy_to_user(buf, &tmpBuffer, count)) 
		return -EFAULT;
	PRINT_X_Y_Z(tmpBuffer[0], tmpBuffer[1], tmpBuffer[2]);
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
			//get orientation info
			if(copy_from_user(&intBuf, (int*)arg, sizeof(intBuf))) return -EFAULT;
			gsensor_calibrate(intBuf[0]);
			// save file 2011-11-30	
			gsensor_write_offset_to_file();
			
			//return the offset
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;
		
		case SENSOR_GET_OFFSET:
			// get data from file 2011-11-30
			gsensor_read_offset_from_file();
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;

		case SENSOR_SET_OFFSET:
			ret = copy_from_user(&intBuf, (int *)arg, sizeof(intBuf));
			gsensor_set_offset(intBuf);
			// write in to file 2011-11-30
			gsensor_write_offset_to_file();
			return ret;
		
		case SENSOR_READ_ACCEL_XYZ:
			device_i2c_read_xyz(dev.client, (s8 *)&xyz);
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = xyz[i] - offset.v[i];
			
		  	ret = copy_to_user((int*)arg, &intBuf, sizeof(intBuf));
			return ret;
		
		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	
	return 0;
}
	
static int device_close(struct inode *inode, struct file *filp)
{
	IN_FUNC_MSG;
	
	return 0;
}

static int device_i2c_xyz_read_reg(struct i2c_client *client,u8 *buffer, int length)
{
	
	struct i2c_msg msg[] = 
	{
		{.addr = client->addr, .flags = 0, .len = 1, .buf = buffer,}, 
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = buffer,},
	};
	IN_FUNC_MSG;
	return i2c_transfer(client->adapter, msg, 2);
}

void device_i2c_read_xyz(struct i2c_client *client, s8 *xyz_p)
{
	s8 xyzTmp[SENSOR_DATA_SIZE];
	int i, j;
	
	u8 buffer[3];
	buffer[0] = X_OUT;
	IN_FUNC_MSG;   
	device_i2c_xyz_read_reg(client, buffer, 3); 
	
	//merge to 11-bits value
	for(i = 0; i < SENSOR_DATA_SIZE; ++i){
		xyzTmp[i] = buffer[i];
	}
	//transfer to the default layout
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
	{
		xyz_p[i] = 0;
		for(j = 0; j < SENSOR_DATA_SIZE; ++j)
			xyz_p[i] += sensorlayout[i][j] * xyzTmp[j];
	}
	PRINT_X_Y_Z(xyz_p[0], xyz_p[1], xyz_p[2]);
}

static int device_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	IN_FUNC_MSG;
	
	return 0;
}

static int __devinit device_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int i;
	
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		offset.v[i] = 0;

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
  		printk("%s, I2C_FUNC_I2C not support\n", __func__);
    		return -1;
  	}

	gsensor_reset(client);
	
	return 0;
}

static int __devexit device_i2c_remove(struct i2c_client *client)
{
	IN_FUNC_MSG;
	
	return 0;
}

static int device_i2c_resume(struct i2c_client *client)
{
	IN_FUNC_MSG;
	
	return 0;
}

static int __init device_init(void)
{
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
  	device_create(dev.class, NULL, dev.devno, NULL, GSENSOR_DEVICE_NAME);
	return i2c_add_driver(&device_i2c_driver);
}

static void __exit device_exit(void)
{
	IN_FUNC_MSG;
	
	cdev_del(&dev.cdev);
	unregister_chrdev_region(dev.devno, 1);
	device_destroy(dev.class, dev.devno);
	class_destroy(dev.class);
	i2c_del_driver(&device_i2c_driver);
}

//*********************************************************************************************************
// 2011-11-30
// Add by Steve Huang 
// function definition
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

 return;
}

void gsensor_read_offset_from_file(void)
{
	unsigned int orgfs;
	char data[18];
	long lfile=-1;
	orgfs = get_fs();
// Set segment descriptor associated to kernel space
	set_fs(KERNEL_DS);

	lfile=sys_open(OffsetFileName, O_RDONLY, 0);
	if (lfile < 0)
	{
 	 printk("sys_open %s error!!. %ld\n",OffsetFileName,lfile);
	 if(lfile==-2)
	 {
           lfile=sys_open(OffsetFileName,O_WRONLY|O_CREAT, 0777);
	   if(lfile >=0)
	   {
	    strcpy(data,"00000 00000 00000");
 	    printk("sys_open %s OK!!. %ld\n",OffsetFileName,lfile);
	    sys_write(lfile,data,18);
	    sys_read(lfile, data, 18);
	    sys_close(lfile);
	   }
	  else
 	   printk("sys_open %s error!!. %ld\n",OffsetFileName,lfile);
	 }  
	 
	}
	else
	{
	 sys_read(lfile, data, 18);
	 sys_close(lfile);
	}
	sscanf(data,"%hd %hd %hd",&offset.u.x,&offset.u.y,&offset.u.z);
	set_fs(orgfs);


}
//*********************************************************************************************************
MODULE_AUTHOR("DMT_RD");
MODULE_DESCRIPTION("DMARD06 g-sensor Driver");
MODULE_LICENSE("GPL");

module_init(device_init);
module_exit(device_exit);

