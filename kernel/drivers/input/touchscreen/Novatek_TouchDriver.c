/* drivers/input/touchscreen/novatek_touchdriver.c
 *
 * Copyright (C) 2010 - 2011 novatek, Inc.
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
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
//#include <plat/gpio-cfg.h>
//#include <plat/gpio-bank-l.h>
//#include <plat/gpio-bank-f.h>
#include <linux/irq.h>

//#include <linux/syscalls.h>

//#include <linux/reboot.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/Novatek_TouchDriver.h>
#include<linux/novatek_ts_data.h>
#include <linux/nvctp_BinaryFile_edt.h>
#include <linux/nvctp_BinaryFile.h>
#include <linux/nvctp_BinaryFile_CntIto.h>
#include <linux/nvctp_BinaryFile_cmi.h>
#include <linux/nvctp_BinaryFile_jin.h>
#include <linux/nvctp_BinaryFile_rtr.h>
#include <linux/nvctp_BinaryFile_soe.h>
#include <linux/nvctp_BinaryFile_rtr_n710.h>
#include <linux/nvctp_BinaryFile_rtr_n750.h>
#include <linux/nvctp_BinaryFile_rtr_xd.h>
#include <linux/nvctp_BinaryFile_edt_2s.h>
#include <linux/nvctp_BinaryFile_rtr_wikipad.h>
#include <linux/nvctp_BinaryFile_edt_wikipad.h>
#include<linux/nvctp_BinaryFile_edt_nabi2_xd.h>
#include<linux/nvctp_BinaryFile_edt101_n1010.h>
#include<linux/nvctp_BinaryFile_edt_n750.h>
#include<linux/nvctp_BinaryFile_edt_birch.h>
#include<linux/nvctp_BinaryFile_edt_birch_055.h>
#include<linux/nvctp_BinaryFile_rtr_birch.h>
#include <asm/mach-types.h>
#if 0
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <asm/uaccess.h>
#endif

//#define DEBUG
#ifdef DEBUG
#define NOVADBG(fmt, arg...)	printk(KERN_ERR "%s: %s() " fmt, "NOVATEK-nt1103", __FUNCTION__, ##arg)
#else
#define NOVADBG(fmt, arg...)
#endif
#define SOCINF(fmt, args...)	printk(KERN_INFO "%s: " fmt, "NOVATEK-nt1103",  ##args)
#define SOCERR(fmt, args...)	printk(KERN_ERR "%s: " fmt, "NOVATEK-nt1103",  ##args)

void hw_reset(void)
{
#if 0
	gpio_set_value(BABBAGE_NT11003_TS_RST1, 0);
	msleep(500);
	//gpio_set_value(BABBAGE_NT11003_TS_RST1, 1);
	gpio_direction_output(BABBAGE_NT11003_TS_RST1, 1);
#endif
    Novatek_HWRST_LowLeval();
	msleep(50);
	Nvoatek_HWRST_HighLeval();
	msleep(100);

}
#ifdef __TOUCH_RESET__
void Calibration_Init(void)
{
 
	FlagInCtpIsr = 0;
	TouchCaliState = 0;	
	TouchCount = 0;
	TimerCount1 = MaxTimerCount1;
	TimerCount2 = MaxTimerCount2;

}
#endif
/*******************************************************	
Description:
	Read data from the i2c slave device;
	This operation consisted of 2 i2c_msgs,the first msg used
	to write the operate address,the second msg used to read data.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer
*********************************************************/
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	int retries = 0;

	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=client->addr;
	msgs[0].len=1;
	msgs[0].buf=&buf[0];

	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=client->addr;
	msgs[1].len=len-1;
	msgs[1].buf=&buf[1];

	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,msgs, 2);
		if(ret == 2)break;
		retries++;
	}
	return ret;
}

/*******************************************************	
Description:
	write data to the i2c slave device.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer.
*********************************************************/
static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	int retries = 0;

	msg.flags=!I2C_M_RD;
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;		
	
	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,&msg, 1);
		if(ret == 1)break;
		retries++;
	}
	return ret;
}

#ifdef  NVT_TOUCH_CTRL_DRIVER
#define DEVICE_NAME		"NVTflash"
static int nvt_flash_major = 121;
static int nvt_flash_minor = 0;
static struct cdev nvt_flash_cdev;
static struct class *nvt_flash_class = NULL;
static dev_t nvt_flash_dev;
static struct nvt_flash_data *flash_priv;

/*******************************************************
Description:
	Novatek touchscreen control driver initialize function.

Parameter:
	priv:	i2c client private struct.
	
return:
	Executive outcomes.0---succeed.
*******************************************************/
int nvt_flash_write(struct file *file, const char __user *buff, size_t count, loff_t *offp)
{
 struct i2c_msg msgs[2];	
 char *str;
 //struct nvt_flash_data *dev = file->private_data;
 int ret=-1;
 int retries = 0;
 file->private_data = (uint8_t *)kmalloc(64, GFP_KERNEL);
 str = file->private_data;
 ret=copy_from_user(str, buff, count);
 if(str[0] ==0x33 && str[1] == 0x44){
	hw_reset();
}
 else{
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = str[0];
	msgs[0].len   = str[1];
	msgs[0].buf   = &str[2];

	while(retries < 20)
	{
		ret = i2c_transfer(flash_priv->client->adapter, msgs, 1);
		if(ret == 1)	break;
		else
			printk("write error %d\n", retries);
		retries++;
	}
 }
 return ret;
}

int nvt_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
 struct i2c_msg msgs[2];	 
 char *str;
 //struct nvt_flash_data *dev = file->private_data;
 int ret = -1;
 int retries = 0;

 file->private_data = (uint8_t *)kmalloc(64, GFP_KERNEL);
 str = file->private_data;
 if(copy_from_user(str, buff, count))
	return -EFAULT;
if(str[0] ==0x11 && str[1] == 0x22){
	str[0] = touch_type;
}
else{
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = str[0];
	msgs[0].len   = 1;
	msgs[0].buf   = &str[2];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = str[0];
	msgs[1].len   = str[1]-1;
	msgs[1].buf   = &str[3];

	while(retries < 20)
	{
		ret = i2c_transfer(flash_priv->client->adapter, msgs, 2);
		if(ret == 2)	break;
		else
			printk("read error %d\n", retries);
		retries++;
	}
}
 	ret=copy_to_user(buff, str, count);


 return ret;
}


int nvt_flash_open(struct inode *inode, struct file *file)
{
	int i;
	struct nvt_flash_data *dev;

	dev = kmalloc(sizeof(struct nvt_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		return -ENOMEM;
	}

	/* initialize members */
	rwlock_init(&dev->lock);
	for(i = 0; i < MAX_BUFFER_SIZE; i++) {
		dev->buffer[i] = 0xFF;
	}
//	dev->client = flash_priv->client;
//	printk("%d\n", dev->client->addr);
	file->private_data = dev;


	return 0;   /* success */
}

int nvt_flash_close(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev = file->private_data;

	if (dev) {
		kfree(dev);
	}
	return 0;   /* success */
}

struct file_operations nvt_flash_fops = {
	.owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.write = nvt_flash_write,
	.read = nvt_flash_read,
};

static int nvt_flash_init(struct novatek_ts_data *ts)
{		
	dev_t dev = MKDEV(nvt_flash_major, 0);
	int alloc_ret = 0;
	int major;
	int cdev_err = 0;
	int input_err = 0;
	struct device *class_dev = NULL;

	// dynamic allocate driver handle
	alloc_ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if(alloc_ret) {
		goto error;
	}
	major = MAJOR(dev);
	nvt_flash_major = major;

	cdev_init(&nvt_flash_cdev, &nvt_flash_fops);
	nvt_flash_cdev.owner = THIS_MODULE;
	nvt_flash_cdev.ops = &nvt_flash_fops;
	cdev_err = cdev_add(&nvt_flash_cdev, MKDEV(nvt_flash_major, nvt_flash_minor), 1);
	if(cdev_err) {
		goto error;
	}

	// register class
	nvt_flash_class = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(nvt_flash_class)) {
		goto error;
	}
	nvt_flash_dev = MKDEV(nvt_flash_major, nvt_flash_minor);
	class_dev = device_create(nvt_flash_class, NULL, nvt_flash_dev, NULL, DEVICE_NAME);

	printk("============================================================\n");
	printk("nvt_flash driver loaded\n");
	printk("============================================================\n");	
	flash_priv=kzalloc(sizeof(*flash_priv),GFP_KERNEL);	
	if (ts == NULL) {
		//dev_set_drvdata(&client->dev, NULL);
                input_err = -ENOMEM;
                goto error;
	}
	flash_priv->client = ts->client;
	return 0;

error:
	if(cdev_err == 0) {
		cdev_del(&nvt_flash_cdev);
	}

	if(alloc_ret == 0) {
		unregister_chrdev_region(dev, 1);
	}

	return -1;
}

#endif
static int novatek_init_panel(struct novatek_ts_data *ts)
{
	//int ret=-1;
	
	if(machine_is_nabi2_3d()|| machine_is_nabi_2s() ||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700() || machine_is_itq701() || machine_is_mm3201() || machine_is_n750() || machine_is_birch() || machine_is_wikipad())
	{
		ts->abs_x_max = SCREEN_MAX_WIDTH_3D;
		ts->abs_y_max = SCREEN_MAX_HEIGHT_3D;
	} 
	else if(machine_is_nabi2_xd() || machine_is_n1010()|| machine_is_n1020()||machine_is_ns_14t004()||machine_is_itq1000()||
			machine_is_n1011() || machine_is_n1050())
	{
		ts->abs_x_max = SCREEN_MAX_WIDTH_XD;
		ts->abs_y_max = SCREEN_MAX_HEIGHT_XD;
	} 
	else {
	       ts->abs_x_max = SCREEN_MAX_WIDTH;
       	ts->abs_y_max = SCREEN_MAX_HEIGHT;
	}
	ts->max_touch_num = MAX_FINGER_NUM;
	ts->int_trigger_type = 1;
	
	return 0;

}
#ifdef __TOUCH_RESET__
static void novatek_ts_reset_work_func(struct work_struct *work)
{
		struct novatek_ts_data *ts = container_of(to_delayed_work(work), struct novatek_ts_data, reset_work);


		 if(FlagInCtpIsr){
		 	
				 if(TouchCount<TIMERCOUNT){
				 	TouchCount++;
					#ifdef CONFIG_TOUCHSCREEN_SSL_DEBUG
					NOVADBG("calibration3 *******************************\n");
					#endif
					if(!TouchCaliState && TouchCount>TIMERCOUNT-1){
						TouchCaliState = 1;
						#ifdef CONFIG_TOUCHSCREEN_SSL_DEBUG
						NOVADBG("calibration4 *******************************\n");
						#endif
				 		hw_reset();
					 }
				 }
				 else{
				 	TimerCount1 = 0;
				 	hrtimer_cancel(&ts->timer);
				 	}
			
			 
	   	 }
	

}
#endif
/*******************************************************
Description:
	Read novatek touchscreen version function.

Parameter:
	ts:	i2c client private struct.
	
return:
	Executive outcomes.0---succeed.
*******************************************************/

/*******************************************************
Description:
	novatek touchscreen work function.

Parameter:
	ts:	i2c client private struct.
	
return:
	Executive outcomes.0---succeed.
*******************************************************/
static void novatek_ts_work_func(struct work_struct *work)
{	
	int ret=-1;
	int tmp = 0;
	uint8_t  point_data[1+ IIC_BYTENUM*MAX_FINGER_NUM]={0};//[(1-READ_COOR_ADDR)+1+2+5*MAX_FINGER_NUM+1]={ 0 };  //read address(1byte)+key index(1byte)+point mask(2bytes)+5bytes*MAX_FINGER_NUM+coor checksum(1byte)
	uint8_t  check_sum = 0;
	uint16_t  finger_current = 0;
	uint16_t  finger_bit = 0;
	unsigned int  count = 0, point_count = 0;
	unsigned int position = 0;	
	uint8_t track_id[MAX_FINGER_NUM] = {0};
	unsigned int input_x = 0;
	unsigned int input_y = 0;
	unsigned int input_w = 0;
	unsigned char index = 0;
	unsigned char touch_num = 0;
	unsigned char touch_check = 0;
	
	struct novatek_ts_data *ts = container_of(work, struct novatek_ts_data, work);

#ifdef __TOUCH_RESET__
	if(TouchCount<TIMERCOUNT)
	{	
		#ifdef CONFIG_TOUCHSCREEN_SSL_DEBUG
		NOVADBG("calibration5 *******************************\n");
		#endif
		//i2c_smbus_write_byte_data(ts->client,0x87, 0x01);	
		TouchCount = 0;
		TouchCaliState = 0;
	}
//	FlagInCtpIsr = 1;    //Mark we are in CTP_ISR()
#endif
	point_data[0] = READ_COOR_ADDR;		//read coor address
	ret=i2c_read_bytes(ts->client, point_data,  sizeof(point_data)/sizeof(point_data[0]));
	/////////////////////////////////////////////////////////////////////////////////////////////
	#ifdef HAVE_TOUCH_KEY
	touch_check = point_data[1]>>3;  
	if( touch_check > 20 )
		goto handle_key;
	#endif
	////////////////////////////////////////////////////////////////////////////////////////////
  	touch_num = MAX_FINGER_NUM;
  	for(index = 0; index < MAX_FINGER_NUM; index++)
  	{
  		position = 1 + IIC_BYTENUM*index;
		if( ( point_data[position]&0x03 ) == 0x03 )
  		   touch_num--;	
  	}
  	
	if(touch_num)
	{
		for(index=0; index<MAX_FINGER_NUM; index++)
		{
			position = 1 + IIC_BYTENUM*index;
		  #if IC_DEFINE == NT11003
			track_id[index] = (unsigned int)(point_data[position]>>3)-1;
		  #elif IC_DEFINE == NT11002
		  	track_id[index] = (unsigned int)(point_data[position]>>4)-1;
		  #endif
			input_x = (unsigned int)(point_data[position+1]<<4) + (unsigned int)( point_data[position+3]>>4);
			input_y = (unsigned int)(point_data[position+2]<<4) + (unsigned int) (point_data[position+3]&0x0f); 
		  #if IC_DEFINE == NT11003		 
			input_w =(unsigned int) (point_data[position+4])+127;
		  #elif IC_DEFINE == NT11002
		  	input_w =(unsigned int) 127;
		  #endif
		if((machine_is_wikipad() && touch_type == EDT_TOUCH)||(touch_type==EDT_TOUCH_2S && machine_is_nabi_2s())) {
			if(input_x == 0 && input_y == 0)
				goto XFER_ERROR;
		}
		  switch(touch_type)
		  {
		  	case CNT_TOUCH:
			case CNT_TOUCH_ITO:
				input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_CNT);	
				input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_CNT);
				break;
			case EDT_TOUCH_BIRCH_055:
				input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_EDT_2S);	
				input_y =SCREEN_MAX_WIDTH_3D- input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_EDT_2S);
				tmp = input_x;
				input_x =  input_y;
				input_y =  tmp;
				break;
			case EDT_TOUCH:
				if(machine_is_wikipad()) {
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_EDT_2S);	
					input_y = 800 - input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_EDT_2S);
					tmp = input_x;
					input_x = input_y;
					input_y = tmp;
				}
				else if(machine_is_nabi2_xd()||machine_is_n1010()||machine_is_ns_14t004()||machine_is_itq1000()||machine_is_n1050()/*|| machine_is_limen()*/){
					input_x = input_x *SCREEN_MAX_WIDTH_XD/(TOUCH_MAX_WIDTH_EDT_XD);	
					input_y = input_y *SCREEN_MAX_HEIGHT_XD/(TOUCH_MAX_HEIGHT_EDT_XD);
					}
				else if(machine_is_n750() || machine_is_birch())
					{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_EDT_2S);	
					input_y =SCREEN_MAX_WIDTH_3D- input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_EDT_2S);
					tmp = input_x;
					input_x =  input_y;
					input_y =  tmp;
					}
				else
				{
					input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_EDT);	
					input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_EDT);
				}
				break;
			case CMI_TOUCH:
				input_x =1024- input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_CMI);	
				input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_CMI);
				break;
			case JIN_TOUCH :
				input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_JIN);	
				input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_JIN);
				break;
			case RTR_TOUCH:
				if(machine_is_nabi2_3d() || machine_is_qc750() ||  machine_is_n710() /*|| machine_is_cybtt10_bk()*/)
				{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);
					input_y = input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = input_y;
					input_y = 1280 - tmp;
				}
				else if(machine_is_itq700() || machine_is_itq701() || machine_is_mm3201())
				{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);
					input_y = 800 - input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = 800 - input_y;
					input_y = 1280 - tmp;

				}
				else if(machine_is_cybtt10_bk())
				{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);	
					input_y = 800 - input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = 800 - input_y;
					input_y = 1280 - tmp;
				}
				else if(machine_is_n750())
				{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_GFT);	
					input_y = 800 - input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_GFT);
					tmp = input_x;
					input_x = input_y;
					input_y = tmp;
				}
				else if(machine_is_birch())
				{
					//input_x =SCREEN_MAX_HEIGHT_3D- input_x*SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);
					//input_y =800-input_y*SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = input_y;
					input_y =tmp;
				}
				else if(machine_is_wikipad())
				{
					input_x = input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);
					input_y = 800 - input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = input_y;
					input_y = tmp;
				}
				else {
					input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_JIN);	
					input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_JIN);
				}
				break;
		       case SOE_TOUCH:
			   	input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_SOE);	
				input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_SOE);
				break;
			case WIDTH_TOUCH:
				input_x = input_x *SCREEN_MAX_WIDTH_XD/(TOUCH_MAX_WIDTH_XD);	
				input_y = input_y *SCREEN_MAX_HEIGHT_XD/(TOUCH_MAX_HEIGHT_XD);
				break;
			case EDT_TOUCH_2S:
				 if(machine_is_nabi_2s()){
					input_x = 1280-input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_EDT_2S);	
					input_y =  input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_EDT_2S);
					tmp = input_x;
					input_x = input_y;
					input_y = tmp;
				}
				break;
			case RTR_TOUCH_2S:
				 if(machine_is_nabi_2s())
				{
					input_x = 1280-input_x *SCREEN_MAX_HEIGHT_3D/(TOUCH_MAX_WIDTH_JIN);	
					input_y =  input_y *SCREEN_MAX_WIDTH_3D/(TOUCH_MAX_HEIGHT_JIN);
					tmp = input_x;
					input_x = input_y;
					input_y = tmp;
				}
				 break;
			default:
				input_x = input_x *SCREEN_MAX_WIDTH/(TOUCH_MAX_WIDTH_CNT);	
				input_y = input_y *SCREEN_MAX_HEIGHT/(TOUCH_MAX_HEIGHT_CNT);
				break;
		  }
			if((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))continue;
			//printk("input_x = %d,input_y = %d, input_w = %d\n", input_x, input_y, input_w);

			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, track_id[index]);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);			
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);	
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, TOOL_PRESSURE);
			input_mt_sync(ts->input_dev);
#ifdef __TOUCH_RESET__
		FlagInCtpIsr = 1;    //Mark we are in CTP_ISR()
#endif
		}
	}
	else
	{

		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_sync(ts->input_dev);
	}

	input_sync(ts->input_dev);

#if defined(INT_PORT)
	if(ts->int_trigger_type> 1)
	{
		msleep(POLL_TIME);
		goto XFER_ERROR;
	}
#endif
	goto END_WORK_FUNC;
handle_key:
#ifdef HAVE_TOUCH_KEY
	NOVADBG("HAVE KEY DOWN!0x%x\n",point_data[1]);
        value =(unsigned int)(point_data[1]>>3);
	switch (value)
			{
			case MENU:
			    	input_report_key(ts->input_dev, KEY_MENU, point_data[1]&0x01);
				break;
			case HOME:
			    	input_report_key(ts->input_dev, KEY_HOME, point_data[1]&0x01);
				break;			
			case BACK:
			    	input_report_key(ts->input_dev, KEY_BACK, point_data[1]&0x01);
				break;
			case VOLUMEDOWN:
			    	input_report_key(ts->input_dev, KEY_VOLUMEDOWN, point_data[1]&0x01);
				break;
			case VOLUMEUP:
			    	input_report_key(ts->input_dev, KEY_VOLUMEUP, point_data[1]&0x01);
				break;
			default:
				break;
			}	   
#endif
	input_sync(ts->input_dev);
END_WORK_FUNC:
XFER_ERROR:
	if(ts->use_irq)
		enable_irq(ts->client->irq);

}

/*******************************************************
Description:
	Timer interrupt service routine.

Parameter:
	timer:	timer struct pointer.
	
return:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart novatek_ts_timer_func(struct hrtimer *timer)
{
	struct novatek_ts_data *ts = container_of(timer, struct novatek_ts_data, timer);
#ifdef __TOUCH_RESET__
   if(touch_type >0 &&  touch_type!=EDT_TOUCH){
		queue_work(novatek_wq, &ts->reset_work);
		hrtimer_start(&ts->timer, ktime_set(0.1, MicroTimeTInterupt), HRTIMER_MODE_REL);
   	}
#else
	queue_work(novatek_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
#endif
	
	return HRTIMER_NORESTART;
}

/*******************************************************
Description:
	External interrupt service routine.

Parameter:
	irq:	interrupt number.
	dev_id: private data pointer.
	
return:
	irq execute status.
*******************************************************/
static irqreturn_t novatek_ts_irq_handler(int irq, void *dev_id)
{
	struct novatek_ts_data *ts = dev_id;
	disable_irq_nosync(ts->client->irq);
	queue_work(novatek_wq, &ts->work);
	return IRQ_HANDLED;
}

/*******************************************************
Description:
	novatek touchscreen power manage function.

Parameter:
	on:	power status.0---suspend;1---resume.
	
return:
	Executive outcomes.-1---i2c transfer error;0---succeed.
*******************************************************/
static int novatek_ts_power(struct novatek_ts_data * ts, int on)
{
	int ret = -1;
	unsigned char i2c_control_buf[2] = {80,  1};		//suspend cmd
	int retry = 0;
	if(on != 0 && on !=1)
	{
		NOVADBG(KERN_DEBUG "%s: Cant't support this command.", novatek_ts_name);
		return -EINVAL;
	}
	
	if(ts != NULL && !ts->use_irq)
		return -2;
	
	if(on == 0)		//suspend
	{ 
		if(ts->green_wake_mode)
		{
			disable_irq(ts->client->irq);
			gpio_direction_output(INT_PORT, 0);
			msleep(5);
			//s3c_gpio_cfgpin(INT_PORT, INT_CFG);
			enable_irq(ts->client->irq);
		}
		while(retry<5)
		{
			ret = i2c_write_bytes(ts->client, i2c_control_buf, 2);
			if(ret == 1)
			{
				NOVADBG("Send suspend cmd\n");
				break;
			}
			NOVADBG("Send cmd failed!\n");
			retry++;
			msleep(10);
		}
		if(ret > 0)
			ret = 0;
	}
	else if(on == 1)		//resume
	{
		NOVADBG(KERN_INFO"Int resume\n");
		gpio_direction_output(INT_PORT, 0); 
		msleep(20);
		if(ts->use_irq) 
			gpio_direction_input(INT_PORT);
			//s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port as interrupt port	
		else 
			gpio_direction_input(INT_PORT);
		//msleep(260);

		ret = 0;
	}	 
	return ret;
}

/*******************************************************
Description:
	novatek debug sysfs cat version function.

Parameter:
	standard sysfs show param.
	
return:
	Executive outcomes. 0---failed.
*******************************************************/

/*******************************************************
Description:
	novatek debug sysfs cat resolution function.

Parameter:
	standard sysfs show param.
	
return:
	Executive outcomes. 0---failed.
*******************************************************/
static ssize_t novatek_debug_resolution_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct novatek_ts_data *ts;
	ts = i2c_get_clientdata(i2c_connect_client_novatek);
	dev_info(&ts->client->dev,"ABS_X_MAX = %d,ABS_Y_MAX = %d\n",ts->abs_x_max,ts->abs_y_max);
	sprintf(buf,"ABS_X_MAX = %d,ABS_Y_MAX = %d\n",ts->abs_x_max,ts->abs_y_max);

	return strlen(buf);
}
/*******************************************************
Description:
	novatek debug sysfs cat version function.

Parameter:
	standard sysfs show param.
	
return:
	Executive outcomes. 0---failed.
*******************************************************/
static ssize_t novatek_debug_diffdata_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	//char diff_data[300];
	unsigned char diff_data[2241] = {00,};
	int ret = -1;
	char diff_data_cmd[2] = {80, 202};
	int i;
	int short_tmp;
	struct novatek_ts_data *ts;

	disable_irq(TS_INT);
	
	ts = i2c_get_clientdata(i2c_connect_client_novatek);
	//memset(diff_data, 0, sizeof(diff_data));
	if(ts->green_wake_mode)
	{
		//disable_irq(client->irq);
		gpio_direction_output(INT_PORT, 0);
		msleep(5);
		//zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
		//enable_irq(client->irq);
	}
	ret = i2c_write_bytes(ts->client, diff_data_cmd, 2);
	if(ret != 1)
	{
		dev_info(&ts->client->dev, "Write diff data cmd failed!\n");
		enable_irq(TS_INT);
		return 0;
	}

	while(gpio_get_value(INT_PORT));
	ret = i2c_read_bytes(ts->client, diff_data, sizeof(diff_data));
	if(ret != 2)
	{
		dev_info(&ts->client->dev, "Read diff data failed!\n");
		enable_irq(TS_INT);
		return 0;
	}
	for(i=1; i<sizeof(diff_data); i+=2)
	{
		short_tmp = diff_data[i] + (diff_data[i+1]<<8);
		if(short_tmp&0x8000)
			short_tmp -= 65535;
		if(short_tmp == 512)continue;
		sprintf(buf+strlen(buf)," %d",short_tmp);
		//NOVADBG(" %d\n", short_tmp);
	}
	
	diff_data_cmd[1] = 0;
	ret = i2c_write_bytes(ts->client, diff_data_cmd, 2);
	if(ret != 1)
	{
		dev_info(&ts->client->dev, "Write diff data cmd failed!\n");
		enable_irq(TS_INT);
		return 0;
	}
	enable_irq(TS_INT);
	/*for (i=0; i<1024; i++)
	{
 		sprintf(buf+strlen(buf)," %d",i);
	}*/
	
	return strlen(buf);
}


/*******************************************************
Description:
	novatek debug sysfs echo calibration function.

Parameter:
	standard sysfs store param.
	
return:
	Executive outcomes..
*******************************************************/
static ssize_t novatek_debug_calibration_store(struct device *dev,
			struct device_attribute *attr, const char *buf, ssize_t count)
{
	int ret = -1;
	char cal_cmd_buf[] = {110,1};
	struct novatek_ts_data *ts;

	ts = i2c_get_clientdata(i2c_connect_client_novatek);
	dev_info(&ts->client->dev,"Begin calibration......\n");
	if((*buf == 10)||(*buf == 49))
	{
		if(ts->green_wake_mode)
		{
			disable_irq(ts->client->irq);
			gpio_direction_output(INT_PORT, 0);
			msleep(5);
			//zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
			enable_irq(ts->client->irq);
		}
		ret = i2c_write_bytes(ts->client,cal_cmd_buf,2);
		if(ret!=1)
		{
			dev_info(&ts->client->dev,"Calibration failed!\n");
			return count;
		}
		else
		{
			dev_info(&ts->client->dev,"Calibration succeed!\n");
		}
	}
	return count;
}


static int	nvctp_CheckIsBootloader(struct i2c_client *client);
/*******************************************************
Description:
	novatek touchscreen probe function.

Parameter:
	client:	i2c device struct.
	id:device id.
	
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int novatek_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	int retry=0;
	struct novatek_ts_data *ts;
	char *version_info = NULL;
	//zy---char test_data = 1;
	uint8_t test_data[7] = {0x00,};
	const char irq_table[4] = {IRQ_TYPE_EDGE_RISING,
							   IRQ_TYPE_EDGE_FALLING,
							   IRQ_TYPE_LEVEL_LOW,
							   IRQ_TYPE_LEVEL_HIGH};

	struct novatek_i2c_rmi_platform_data *pdata;
	NOVADBG("******************************************Install touch driver.***********************************\n");
	hw_reset();
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		printk( "Must have I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
  
#ifdef INT_PORT	
    ts->int_trigger_type = 1;
	client->irq=TS_INT;
	if (client->irq)
	{
		//zy--ret = gpio_request(INT_PORT, "TS_INT");
		//gpio_direction_input(INT_PORT);//zy++
		if (ret < 0) 
		{
			printk("Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
			goto err_gpio_request_failed;
		}
		//zy--s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_UP);
		//zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
		//printk("ts->int_trigger_type=%d\n",ts->int_trigger_type);
		ret  = request_irq(client->irq, novatek_ts_irq_handler ,  irq_table[ts->int_trigger_type],
			client->name, ts);
		if (ret != 0) {
			printk("Cannot allocate ts INT!ERRNO:%d\n", ret);
			gpio_direction_input(INT_PORT);
			gpio_free(INT_PORT);
			goto err_gpio_request_failed;
		}
		else 
		{	
			disable_irq(client->irq);
			ts->use_irq = 1;
			//printk("Reques EIRQ %d succesd on GPIO:%d\n",TS_INT,INT_PORT);
		}

	}
#endif

err_gpio_request_failed:

	for(retry=0;retry < 10; retry++)
	{
		disable_irq(client->irq);
		//zy--gpio_direction_output(INT_PORT, 0);
		msleep(5);
		//zy--s3c_gpio_cfgpin(INT_PORT, INT_CFG);
		enable_irq(client->irq);
		ret =i2c_read_bytes(client, test_data, 5);
        //printk("test_data[1]=%d,test_data[2]=%d,test_data[3]=%d,test_data[4]=%d,test_data[5]=%d\n",test_data[1],test_data[2],test_data[3],test_data[4],test_data[5]);
		if (ret > 0){
			break;
		}
		printk("novatek i2c test failed!\n");
	}
	if(ret <= 0)
	{
		printk( "I2C communication ERROR!novatek touchscreen driver become invalid\n");
		goto err_i2c_failed;
	}

	i2c_connect_client_novatek = client;
		
	
	INIT_WORK(&ts->work, novatek_ts_work_func);
#ifdef __TOUCH_RESET__

	INIT_WORK(&ts->reset_work,novatek_ts_reset_work_func);

#endif
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;
	
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk("Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
#if 1	
	for(retry=0; retry<3; retry++)
	{
		ret=novatek_init_panel(ts);
		msleep(2);
		if(ret != 0)
			continue;
		else
			break;
	}
	if(ret != 0) {
		ts->bad_data=1;
		goto err_init_godix_ts;
	}
#endif
	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE); 						// absolute coor (x,y)
#ifdef HAVE_TOUCH_KEY
	for(retry = 0; retry < MAX_KEY_NUM; retry++)
	{
		input_set_capability(ts->input_dev,EV_KEY,touch_key_array[retry]);	
	}
#endif

//	input_set_abs_params(ts->input_dev, ABS_X, 0, ts->abs_x_max-1, 0, 0);
//	input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->abs_y_max-1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);

#ifdef NOVATEK_MULTI_TOUCH	
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, ts->max_touch_num, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0,TOOL_PRESSURE, 0, 0);	
#endif	

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = novatek_ts_name;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 111028;	//screen firmware version
	
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk("Probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	ts->bad_data = 0;
#ifdef __TOUCH_RESET__
	Calibration_Init();
#endif		
	if (!ts->use_irq) 
	{
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = novatek_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	hw_reset();
	msleep(500);
/****************************************************************/
	// BootLoader Function...
	#ifdef _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_
	nvctp_CheckIsBootloader(ts->client);
	#endif
	/****************************************************************/

#ifdef __TOUCH_RESET__
	 if(touch_type >0 &&  touch_type!=EDT_TOUCH){
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = novatek_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(0.1, MicroTimeTInterupt), HRTIMER_MODE_REL);
	 	}
#endif
	if(ts->use_irq)
		enable_irq(client->irq);

	#ifdef NVT_TOUCH_CTRL_DRIVER
		nvt_flash_init(ts);
	#endif
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = novatek_ts_early_suspend;
	ts->early_suspend.resume = novatek_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	printk("Start %s in %s mode\n", 
		ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
	printk( "Driver Modify Date:2011-06-27\n");
	return 0;

err_init_godix_ts:
	if(ts->use_irq)
	{
		ts->use_irq = 0;
		free_irq(client->irq,ts);
	#ifdef INT_PORT	
		gpio_direction_input(INT_PORT);
		gpio_free(INT_PORT);
	#endif	
	}
	else 
		hrtimer_cancel(&ts->timer);
#ifdef __TOUCH_RESET__
    if(touch_type >0 &&  touch_type!=EDT_TOUCH){
	hrtimer_cancel(&ts->timer);
    	}
#endif	
err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
err_i2c_failed:	
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
err_create_proc_entry:
	return ret;
}


/*******************************************************
Description:
	novatek touchscreen driver release function.

Parameter:
	client:	i2c device struct.
	
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int novatek_ts_remove(struct i2c_client *client)
{
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
    uint8_t serialinterface_data[] ={0xff, 0x8f, 0xff};
    uint8_t suspend_data[]= {0x00, 0xAE};
    uint8_t ret;
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
#ifdef CONFIG_TOUCHSCREEN_NT11003_IAP
	remove_proc_entry("novatek-update", NULL);
#endif
	//nt11003_debug_sysfs_deinit();
	if (ts && ts->use_irq) 
	{
	#ifdef INT_PORT
		gpio_direction_input(INT_PORT);
		gpio_free(INT_PORT);
	#endif	
		free_irq(client->irq, ts);
	}	
	else if(ts)
		hrtimer_cancel(&ts->timer);

   NOVADBG(KERN_INFO "******************novatek_ts_remove******************\n");
    ret = i2c_write_bytes(ts->client, serialinterface_data, (sizeof(serialinterface_data)/sizeof(serialinterface_data[0])));
    if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
	ret = i2c_write_bytes(ts->client, suspend_data, (sizeof(suspend_data)/sizeof(suspend_data[0])));
    if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
	mdelay(50);
#ifdef __TOUCH_RESET__
	 if(touch_type >0 &&  touch_type!=EDT_TOUCH){
		hrtimer_cancel(&ts->timer);
	 	}
#endif
	dev_notice(&client->dev,"The driver is removing...\n");
	i2c_set_clientdata(client, NULL);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int novatek_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
   struct novatek_ts_data *ts = i2c_get_clientdata(client);
    uint8_t ret;
  #if 1
   uint8_t serialinterface_data[] ={0xff, 0x8f, 0xff};
    uint8_t suspend_data[]= {0x00, 0xaf};
    ret = i2c_write_bytes(ts->client, serialinterface_data, (sizeof(serialinterface_data)/sizeof(serialinterface_data[0])));
    if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}

	ret = i2c_write_bytes(ts->client, suspend_data, (sizeof(suspend_data)/sizeof(suspend_data[0])));
   	
	if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
	 msleep(50);
#else
	  Novatek_HWRST_LowLeval();
#endif
	NOVADBG(KERN_ERR " novatek_ts_suspend   ts->use_irq = %d\n",ts->use_irq);
	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);
#ifdef __TOUCH_RESET__
	 if(touch_type >0 &&  touch_type!=EDT_TOUCH){
		hrtimer_cancel(&ts->timer);
 	}
#endif
	//ret = cancel_work_sync(&ts->work);
	//if(ret && ts->use_irq)	
		//enable_irq(client->irq);
	if (ts->power) {
		ret = ts->power(ts, 0);
		if (ret < 0)
			printk(KERN_ERR "nt11003_ts_resume power off failed\n");
	}
	return 0;
}

static void novatek_reset_int()
{
	TS_GPIO;
	Novatek_HWINT_LowLeval();
	msleep(5);
	Nvoatek_HWINT_HighLeval();
	TS_INT;
	
}
static int novatek_ts_resume(struct i2c_client *client)
{
	int ret;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
     NOVADBG(KERN_INFO "novatek_ts_resume\n");
	hw_reset();
#ifdef __TOUCH_RESET__
	Calibration_Init();
#endif
	if (ts->power) {
		ret = ts->power(ts, 1);
		if (ret < 0)
			printk(KERN_ERR "novatek_ts_resume power on failed\n");
	}
	NOVADBG(KERN_INFO "novatek_ts_resume   ts->use_irq = %d\n",ts->use_irq);
	if (ts->use_irq)
		enable_irq(client->irq);
	else
		hrtimer_start(&ts->timer,ktime_set(1,0), HRTIMER_MODE_REL);
#ifdef __TOUCH_RESET__
	 if(touch_type >0 &&  touch_type!=EDT_TOUCH){
		hrtimer_start(&ts->timer, ktime_set(0.1, MicroTimeTInterupt), HRTIMER_MODE_REL);
	}
#endif
	if(machine_is_nabi2_xd() || machine_is_nabi_2s() || machine_is_n1010()|| machine_is_n1020()||machine_is_ns_14t004()||machine_is_itq1000()||
		machine_is_n1011() || machine_is_n1050()/*|| machine_is_limen()*/){
		novatek_reset_int();
	}
	return 0;
}
static int novatek_ts_shutdown(struct i2c_client *client)
{

   struct novatek_ts_data *ts = i2c_get_clientdata(client);
  #if 1
   uint8_t serialinterface_data[] ={0xff, 0x8f, 0xff};
    uint8_t suspend_data[]= {0x00, 0xaf};
   uint8_t ret;
    ret = i2c_write_bytes(ts->client, serialinterface_data, (sizeof(serialinterface_data)/sizeof(serialinterface_data[0])));
    if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}

	ret = i2c_write_bytes(ts->client, suspend_data, (sizeof(suspend_data)/sizeof(suspend_data[0])));
   	 msleep(20);
	if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
    	msleep(50);
  #else
	 Novatek_HWRST_LowLeval();
  #endif
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void novatek_ts_early_suspend(struct early_suspend *h)
{
	struct novatek_ts_data *ts;
	ts = container_of(h, struct novatek_ts_data, early_suspend);
	novatek_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void novatek_ts_late_resume(struct early_suspend *h)
{
	struct novatek_ts_data *ts;
	ts = container_of(h, struct novatek_ts_data, early_suspend);
	novatek_ts_resume(ts->client);
}
#endif

#ifdef _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_

#define  FW_DATASIZE      (1024*32)
#define  FLASHSECTORSIZE  (FW_DATASIZE/128)
#define  FW_CHECKSUM_ADDR     (FW_DATASIZE - 8)

#define  FW_DATASIZE_NEW      (1024*26)
#define  FLASHSECTORSIZE_NEW  (FW_DATASIZE_NEW/128)
#define  FW_CHECKSUM_ADDR_NEW     (FW_DATASIZE - 8)
/*******************************************************	
Description:
	Read data from the flash slave device;
	This operation consisted of 2 i2c_msgs,the first msg used
	to write the operate address,the second msg used to read data.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer
*********************************************************/

static int BootLoader_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	int retries = 0;

	msgs[0].flags=client->flags;
	msgs[0].addr=0x7F;//client->addr;
	msgs[0].len=1;
	msgs[0].buf=&buf[0];
//	msgs[0].scl_rate = NOVATEK_I2C_SCL;
	
	msgs[1].flags=client->flags | I2C_M_RD;;
	msgs[1].addr=0x7F;//client->addr;
	msgs[1].len=len-1;
	msgs[1].buf=&buf[1];
//	msgs[1].scl_rate = NOVATEK_I2C_SCL;
	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,msgs, 2);
		if(ret == 2)break;
		retries++;
	}
	return ret;
}

/*******************************************************	
Description:
	write data to the flash slave device.

Parameter:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
return:
	numbers of i2c_msgs to transfer.
*********************************************************/
static int BootLoader_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	int retries = 0;

	msg.flags=client->flags;
	msg.addr=0x7F;//client->addr;
	msg.len=len;
	msg.buf=data;		
//	msg.scl_rate = NOVATEK_I2C_SCL;
	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,&msg, 1);
		if(ret == 1)break;
		retries++;
	}
	return ret;
}

/*******************************************************	
Description:
	Driver Bootloader  function.
return:
	Executive Outcomes. 0---succeed.
********************************************************/


void nvctp_DelayMs(unsigned long wtime)
{
	msleep(wtime);
}
/*************************************************************************************
*Description:
*			Flash Mass Erase Command(7FH --> 30H --> 00H)
*
*Return:
*			Executive Outcomes: 0 -- succeed
*************************************************************************************/
static int novatek_FlashEraseSector(struct i2c_client *client, uint16_t wAddress);
static int novatek_FlashEraseMass(struct i2c_client *client)
{
	uint8_t i;
	uint16_t wAddress;//,wAddress1;
	uint8_t ret = RS_ERAS_ER;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
			
	printk("--%s--\n",__func__);


	wAddress = 0;
	for(i=0;i<208;i++)
	{	
	    
		ret=novatek_FlashEraseSector(client,  wAddress);
		if(ret != RS_OK)
		{
			printk(" %d sector nvctp_EraseSector error \n",i);
			return ret;
		}
		wAddress = wAddress+128;			
	}
	
	wAddress = 32*1024-128;
	
	ret=novatek_FlashEraseSector( client,  wAddress);
		if(ret != RS_OK)
		{
			printk(" end nvctp_EraseSector error \n");
			return ret;
		}
		
	return ret;
}
#if 0
{
	uint8_t i,status;
	uint8_t Buffer[4]={0};
	int ret= RS_ERAS_ER;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
	
    Buffer[0] = 0x00;
    Buffer[1] = 0x33;
	for(i = 5; i > 0; i--)
	{
		Buffer[2] = 0x00;
		
		ret = BootLoader_write_bytes(ts->client, Buffer, 3);
		if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
		nvctp_DelayMs(25);

		/*Read status*/
   		ret = BootLoader_read_bytes(ts->client, Buffer, 2);
		if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
   		if(Buffer[1] == 0xAA)
   		{
	   		ret = RS_OK;
			break;
   		}
		
		nvctp_DelayMs(1);
	}
	return ret;


}
#endif

/*************************************************************************************
*
*
*
*
*
*************************************************************************************/
static int novatek_FlashEraseSector(struct i2c_client *client, uint16_t wAddress)
{
	uint8_t i,status;
	uint8_t Buffer[4]={0};
	int ret= RS_ERAS_ER;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
	
    Buffer[0] = 0x00;
    Buffer[1] = 0x30;
	for(i = 5; i > 0; i--)
	{
		Buffer[2] = (uint8_t)(wAddress >> 8);
		Buffer[3] = (uint8_t)(wAddress)&0x00FF;

		ret = BootLoader_write_bytes(ts->client, Buffer, 4);
		if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
		nvctp_DelayMs(10);

		/*Read status*/
   		ret = BootLoader_read_bytes(ts->client, Buffer, 2);
		if (ret <= 0)
    	{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
    	}
   		if(Buffer[1] == 0xAA)
   		{
	   		ret = RS_OK;
			break;
   		}
		
		nvctp_DelayMs(2);
	}
	return ret;


}

/*************************************************************************************
*
*
*
*
*
*************************************************************************************/
static int novatek_Initbootloader(uint8_t bType, struct i2c_client *client)
{
	u8 ret = RS_OK;
    u8 status;
	u8 iic_buffer[13];
	u8 write_cmd1[3] = {0xff,0xf1,0x91};
  	u8 write_cmd2[2] = {0x00,0x01};
	
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

	
	//ts->client->Addr = 0x7F;
	iic_buffer[0] = 0x00;
	if(bType == HW_RST)
	 {
		iic_buffer[1] = 0x00;
		BootLoader_write_bytes(ts->client, (uint8_t *)iic_buffer, 2);
		nvctp_DelayMs(2);
		//nvctp_GobalRset(1,2);				
	 }
	else if(bType == SW_RST)
	{
	        iic_buffer[1] = 0xA5;
		BootLoader_write_bytes(ts->client, iic_buffer, 2);
                nvctp_DelayMs(10);
		iic_buffer[1] = 0x00;
		BootLoader_write_bytes(ts->client, (uint8_t *)iic_buffer, 2);
		nvctp_DelayMs(2);
	}

  /*Read status*/
   BootLoader_read_bytes(ts->client,iic_buffer,2);
   	printk("iic_buffer[1] %d\n",iic_buffer[1]);
   if(iic_buffer[1] != 0xAA)
   	{
	   ret = RS_INIT_ER;
   	}
   	printk("ret %d\n",ret);
	
  	//ts->client->Addr = 0x01;
    //i2c_write_bytes(ts->client, write_cmd1, 3); 
	//i2c_write_bytes(ts->client, write_cmd2, 2);
	//ts->client->Addr = 0x7F;
   return ret;
}


#define CALIBRATION_ADDR 0x6C70
static  int nvctp_calibration_data_in(struct i2c_client *client)
{
	int ret = 0;
	u8 buf[16] = {0};
		
	if(novatek_Initbootloader(SW_RST, client) == RS_OK)
	{
		buf[0] = 0x00;
		buf[1] = 0x99;
		buf[2] = (u8)(CALIBRATION_ADDR>>8);
		buf[3] = (u8)(CALIBRATION_ADDR&0xFF);;
		buf[4] = 8;

		BootLoader_write_bytes(client, buf, 5);
		msleep(2);
		
		BootLoader_read_bytes(client, buf, 14);

		printk("buf 0 -13: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13]);

		if((buf[12]==0x55)&&(buf[13]==0xAA))
		//if((buf[12]==0x0)&&(buf[13]==0x0))
		{
			ret = 1;
		}
	}

	hw_reset();
	msleep(500);
	return ret;
}

/*******************************************************************************************




********************************************************************************************/
static  int nvctp_ReadDynamicchecksum(struct i2c_client *client)
{
	unsigned char iic_data_buffer[14]={0};
	unsigned int wValue,DynChecksum;
  	unsigned char ret = RS_OK;
	
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
	u8 write_cmd1[3] = {0xff,0x8F,0xFF};
	u8 write_cmd2[2] = {0x00,0xE1};
	u8 write_cmd3[3] = {0xff,0x8E,0x0E};

	//////////////////////////////////////////
	i2c_write_bytes(ts->client, write_cmd1, 3);
	i2c_write_bytes(ts->client, write_cmd2, 2);
	msleep(1000);
	i2c_write_bytes(ts->client, write_cmd3, 3);
	iic_data_buffer[0] = 0;
	i2c_read_bytes(ts->client,iic_data_buffer,3);
	DynChecksum =  (unsigned long)iic_data_buffer[1]<<8|iic_data_buffer[2];
	return DynChecksum;

}
unsigned char nvctp_ReadFinalChecksum(unsigned long flash_addr, unsigned long final_checksum,struct i2c_client *client)
{
	unsigned char iic_data_buffer[14]={0};
	unsigned long wValue,DynChecksum;
  	unsigned char ret = RS_OK;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
#if 0
	u8 write_cmd1[3] = {0xff,0x8E,0x0E};
  	//u8 write_cmd2[2] = {0x00,0x01};
	
	i2c_write_bytes(ts->client, write_cmd1, 3);
	iic_data_buffer[0] = 0;
	i2c_read_bytes(ts->client,iic_data_buffer,3);
	DynChecksum = (unsigned long)iic_data_buffer[1]<<8|iic_data_buffer[2];
	
#endif
     DynChecksum=   nvctp_ReadDynamicchecksum(ts->client);
       printk("%s, DynChecksum checksum = %x \n",__func__,DynChecksum);
	
	/*inital bootloader \BD\F8\C8\EBbootloader ģʽ\A3\AC\B8\C3ģʽ\CF\C2i2c\B6\C1д\B5\D8ַ\BB\E1\B8ı\E4*/
	ret = novatek_Initbootloader(SW_RST,ts->client);
	if(ret != RS_OK)
	 {
		return ret;
	 }
	iic_data_buffer[0]=0x00;		
	iic_data_buffer[1] = 0x99;
	iic_data_buffer[2] = (unsigned char)(flash_addr >> 8);
	iic_data_buffer[3] = flash_addr&0xFF;
	iic_data_buffer[4] = 8;
	BootLoader_write_bytes(ts->client,iic_data_buffer,5);
	nvctp_DelayMs(2);
	BootLoader_read_bytes(ts->client,iic_data_buffer,14);
	wValue = (unsigned long)iic_data_buffer[12]<<8 |iic_data_buffer[13];

	printk("%s,  old version checksum = %x \n",__func__,wValue);

	if((wValue != final_checksum)||(DynChecksum != final_checksum))
	{
		ret = RS_FLCS_ER;
	}

	return ret;
	
}


/*************************************************************************************
*
*
*
*
*
*************************************************************************************/
static int nvctp_WriteDatatoFlash( unsigned char *fw_BinaryData, unsigned long BinaryDataLen,struct i2c_client *client)
{
	uint8_t ret = RS_OK;
  	uint8_t iic_data_buffer[14];
	uint8_t j,k;
	uint8_t iic_buffer[16] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t Checksum[16] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    	uint16_t flash_addr,DynChecksum,FW_CHECKSUM;
	uint16_t sector;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

	           printk("--%s--\n",__func__);
    WFStart:
				iic_data_buffer[0]=0x00;
				sector = 0;
				flash_addr = 0;

				for(sector = 0; sector < FLASHSECTORSIZE_NEW; sector++)
				{
					printk("data writing ....... %d \n",sector);
   WFRepeat:	    	 
     	 			flash_addr = 128*sector;  
     	 			for (j = 0; j < 16; j++)
     	 			{
        			/* Write Data to flash*/
                    	iic_data_buffer[1] = 0x55;
	            		iic_data_buffer[2] = (unsigned char)(flash_addr >> 8);
                    	iic_data_buffer[3] = (unsigned char)flash_addr;
		    			iic_data_buffer[4] = 8;
		    
		    			iic_data_buffer[6] = fw_BinaryData[flash_addr + 0];
		    			iic_data_buffer[7] = fw_BinaryData[flash_addr + 1];
		    			iic_data_buffer[8] = fw_BinaryData[flash_addr + 2];
		    			iic_data_buffer[9] = fw_BinaryData[flash_addr + 3];
		    			iic_data_buffer[10]= fw_BinaryData[flash_addr + 4];
		    			iic_data_buffer[11]= fw_BinaryData[flash_addr + 5];
		    			iic_data_buffer[12]= fw_BinaryData[flash_addr + 6];
		    			iic_data_buffer[13]= fw_BinaryData[flash_addr + 7];
        
		    			Checksum[j] = ~(iic_data_buffer[2]+iic_data_buffer[3]+iic_data_buffer[4]+iic_data_buffer[6]+\
		    	        iic_data_buffer[7]+iic_data_buffer[8]+iic_data_buffer[9]+\
		    	  		iic_data_buffer[10]+iic_data_buffer[11]+iic_data_buffer[12]+iic_data_buffer[13]) + 1;
		    			iic_data_buffer[5] = Checksum[j];
	    
		    			BootLoader_write_bytes(ts->client, iic_data_buffer, 14);

						flash_addr += 8;	
     	 			}
					nvctp_DelayMs(20);
					/*Setup force genrate Check sum */
				}

				iic_data_buffer[0]=0x00;				
				flash_addr = 32*1024-128;
				printk("data writing ....... %d \n",sector);
     	 		for (j = 0; j < 16; j++)
     	 		{
        			/* Write Data to flash*/
                    	iic_data_buffer[1] = 0x55;
	            		iic_data_buffer[2] = (unsigned char)(flash_addr >> 8);
                    	iic_data_buffer[3] = (unsigned char)flash_addr;
		    			iic_data_buffer[4] = 8;
		    
		    			iic_data_buffer[6] = fw_BinaryData[flash_addr + 0];
		    			iic_data_buffer[7] = fw_BinaryData[flash_addr + 1];
		    			iic_data_buffer[8] = fw_BinaryData[flash_addr + 2];
		    			iic_data_buffer[9] = fw_BinaryData[flash_addr + 3];
		    			iic_data_buffer[10]= fw_BinaryData[flash_addr + 4];
		    			iic_data_buffer[11]= fw_BinaryData[flash_addr + 5];
		    			iic_data_buffer[12]= fw_BinaryData[flash_addr + 6];
		    			iic_data_buffer[13]= fw_BinaryData[flash_addr + 7];
        
		    			Checksum[j] = ~(iic_data_buffer[2]+iic_data_buffer[3]+iic_data_buffer[4]+iic_data_buffer[6]+\
		    	        iic_data_buffer[7]+iic_data_buffer[8]+iic_data_buffer[9]+\
		    	  		iic_data_buffer[10]+iic_data_buffer[11]+iic_data_buffer[12]+iic_data_buffer[13]) + 1;
		    			iic_data_buffer[5] = Checksum[j];
	    
		    			BootLoader_write_bytes(ts->client, iic_data_buffer, 14);

						flash_addr += 8;	
     	 		}
					nvctp_DelayMs(20);
#if 0
    				flash_addr = 128*sector;  
    				for (j = 0; j < 16; j++)
    				{
                			iic_data_buffer[0] = 0x00;
					iic_data_buffer[1] = 0x99;
					iic_data_buffer[2] = (unsigned char)(flash_addr >> 8);
      	        			iic_data_buffer[3] = (unsigned char)flash_addr & 0xFF;
		        		iic_data_buffer[4] = 8;
					BootLoader_write_bytes(ts->client, iic_data_buffer,5);
					nvctp_DelayMs(2);
					BootLoader_read_bytes(ts->client, iic_data_buffer, 14);
	                  
					if(iic_data_buffer[5] != Checksum[j] )
					{
							printk("checksum error iic_data_buffer[5] = %x,  Checksum[%d] = %x\n", iic_data_buffer[5],j,Checksum[j]);
			  				ret = RS_WD_ER;

                        				//novatek_FlashEraseMass(ts->client);
                        				//goto WFStart;
                        				novatek_FlashEraseSector(ts->client,(unsigned int)(sector*128));
                        				goto WFRepeat;
							
					}
						flash_addr += 8;	
	                    nvctp_DelayMs(2);
	  				}
#endif

   
/////////////////////////////////////////////////////////////////
   hw_reset();
   msleep(500);

 switch(touch_type)
 {
	case EDT_TOUCH:
		if(machine_is_wikipad())
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_wikipad[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_wikipad[FW_DATASIZE-1]);
		else if(machine_is_nabi2_xd()){
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_nabi2_xd[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_nabi2_xd[FW_DATASIZE-1]);
			}		
		else if(machine_is_n1010()||machine_is_ns_14t004()||machine_is_itq1000()||machine_is_n1050()/*|| machine_is_limen()*/){
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt101_n1010[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt101_n1010[FW_DATASIZE-1]);
			}
		else if(machine_is_n750()){
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_n750[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_n750[FW_DATASIZE-1]);
		}
		else if(machine_is_birch()){
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_birch[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_birch[FW_DATASIZE-1]);
			}
		else
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt[FW_DATASIZE-1]);
		break;
	case EDT_TOUCH_BIRCH_055:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_birch_055[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_birch_055[FW_DATASIZE-1]);
		break;
	case CNT_TOUCH:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile[FW_DATASIZE-1]);
		break;
	case CNT_TOUCH_ITO:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_ITO[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_ITO[FW_DATASIZE-1]);
		break;
	case CMI_TOUCH:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_cmi[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_cmi[FW_DATASIZE-1]);
		break;
	case JIN_TOUCH :
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_jin[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_jin[FW_DATASIZE-1]);
		break;
	case RTR_TOUCH:
		if(machine_is_nabi_2s() || machine_is_qc750() ||  machine_is_n710() ||  machine_is_itq700() || machine_is_itq701() || machine_is_mm3201() || machine_is_cybtt10_bk())
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n710[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n710[FW_DATASIZE-1]);
		else if(machine_is_n750() )
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n750[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n750[FW_DATASIZE-1]);
		else if(machine_is_wikipad())
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_wikipad[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_wikipad[FW_DATASIZE-1]);
		else if(machine_is_birch())
			FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_birch[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_birch[FW_DATASIZE-1]);
		else
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr[FW_DATASIZE-1]);
		break;
	case SOE_TOUCH:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_soe[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_soe[FW_DATASIZE-1]);
		break;
	case WIDTH_TOUCH:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_xd[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_xd[FW_DATASIZE-1]);
		break;
	case EDT_TOUCH_2S:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_2s[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_2s[FW_DATASIZE-1]);
		break;
	case RTR_TOUCH_2S:
		FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n710[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n710[FW_DATASIZE-1]);
		break;
	default:
		break;
}

   DynChecksum = nvctp_ReadDynamicchecksum(ts->client);
   printk("%s, DynChecksum checksum = %x \n",__func__,DynChecksum);
	   if(FW_CHECKSUM != DynChecksum)
   	{
		ret = novatek_Initbootloader(SW_RST,ts->client);
		if(ret != RS_OK)
		 {
			return ret;
		 }
	       novatek_FlashEraseMass(ts->client);
		goto WFStart;
   	}
   
   return ret;		        
}

/*************************************************************************************
*
*
*
*
*
*************************************************************************************/
static int novatek_Bootloader(unsigned char *nvctp_binaryfile , unsigned long nvctp_binaryfilelength,struct i2c_client *client)
{
	uint8_t i;
	uint8_t ret = RS_OK;
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
        
	printk("--%s--\n",__func__);
	
	/*Erase Sector*/
	#if 1
	ret = novatek_FlashEraseMass(ts->client);
			
	if(ret != RS_OK)
	{
		printk("nvctp_EraseSector error \n");
		return ret;
	}
	#endif
	#if 0
	wAddress = 0;
	for(i=0;i<208;i++)
	{				
		ret=novatek_FlashEraseSector(struct i2c_client *client, uint16_t wAddress);
		if(ret != RS_OK)
		{
			printk("nvctp_EraseSector error \n");
			return ret;
		}
		wAddress = wAddress+128;			
	}
	wAddress = 32*1024-128;
	ret=novatek_FlashEraseSector(struct i2c_client *client, uint16_t wAddress);
		if(ret != RS_OK)
		{
			printk("nvctp_EraseSector error \n");
			return ret;
		}
	#endif
	/*Write binary data to flash*/
	ret = nvctp_WriteDatatoFlash(nvctp_binaryfile,nvctp_binaryfilelength,ts->client);
	if(ret != RS_OK)
	{
		return ret;
	}

		
    printk("--%s-- ret = %d\n",__func__,ret);
	return ret;
}

/*************************************************************************************
*
*
*
*
*
*************************************************************************************/

static int	nvctp_CheckIsBootloader(struct i2c_client *client)
{
	struct novatek_ts_data *ts = i2c_get_clientdata(client);
	struct novatek_i2c_platform_data *pdata = client->dev.platform_data;
	uint16_t FW_CHECKSUM ;
	uint8_t iic_buffer[3];
	printk("nvctp_CheckIsBootloader\n");
	uint8_t version_address []= {0x3f};
	uint8_t i;
	u8 write_cmd1[3] = {0xff,0x3f,0xff};

	u8 iic_data_buffer[100]={ 0};
	unsigned long wValue;
	int detect_touch,detect_touch_old;
	i2c_write_bytes(ts->client, write_cmd1, 3);
	iic_data_buffer[0] = 0;
	i2c_read_bytes(ts->client,iic_data_buffer,2);
	NOVADBG("nvctp_CheckIsBootloader version = %x,%x,%x,%x\n",iic_data_buffer[1],iic_data_buffer[0],iic_data_buffer[2],write_cmd1[1]);
{
		u8 write_cmd1[3] = {0xff,0x3f,0xf3};
		u8 write_cmd2[3] = {0xff,0x3f,0xf4};
		u8 write_cmd3[3] = {0xff,0x3f,0xf5};
		u8 iic_data_buffer[100]={ 0};
		unsigned long wValue;

		i2c_write_bytes(client, write_cmd1, 3);
		iic_data_buffer[0] = 0;
		i2c_read_bytes(client,iic_data_buffer,4);
		printk("nvctp_CheckIsBootloader version = %x,%x,%x,%x,%x\n",iic_data_buffer[0],iic_data_buffer[1],iic_data_buffer[2],iic_data_buffer[3],write_cmd1[1]);
}	
		if(pdata->detect)
		{
			detect_touch = pdata->detect();
			detect_touch_old=detect_touch>>1;
		}
		printk("detect touch= %d\n",detect_touch);
		printk("detect touch old= %d\n",detect_touch_old);

		if(machine_is_nabi2_3d()||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700()  || machine_is_itq701() || machine_is_mm3201() || machine_is_cybtt10_bk())
			touch_type = RTR_TOUCH;
		if(machine_is_wikipad()) {
			if(5 == detect_touch_old)
				touch_type = RTR_TOUCH;
			else
				touch_type = EDT_TOUCH;
		}
		if(machine_is_n750()) {
			if(6 == detect_touch_old)
				touch_type = EDT_TOUCH;
			else
				touch_type = RTR_TOUCH;
		}
		if(machine_is_birch()) {
			if(8 == detect_touch)
			{
			if(nvctp_calibration_data_in(ts->client))
				touch_type = EDT_TOUCH_BIRCH_055;//0.55
			else
				touch_type = EDT_TOUCH;//0.7
			}
			else if(0x0f == detect_touch)
				touch_type = RTR_TOUCH;
			else return;
		}
		if(machine_is_n1020()||machine_is_n1011()){
			touch_type = WIDTH_TOUCH;
		}
		if(machine_is_n1010()||machine_is_ns_14t004()||machine_is_itq1000()|| machine_is_n1050()/*|| machine_is_limen()*/)

		{
			if(0x0c==detect_touch)
				touch_type=EDT_TOUCH;
			else
				touch_type=WIDTH_TOUCH;
		}
		if(machine_is_nabi2_xd()){
			if(0x0c==detect_touch)
				touch_type=EDT_TOUCH;
			else
				touch_type=WIDTH_TOUCH;
		}
		if(machine_is_nabi_2s()){
			if(detect_touch_old == 0x07)
				touch_type = EDT_TOUCH_2S;
			else if(detect_touch_old == 0x06)
				touch_type = RTR_TOUCH_2S;
			return ;
		}
			
		printk("touch type = %d\n",touch_type);
		
		switch(touch_type)
		{

			case EDT_TOUCH:
				if(machine_is_wikipad())
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_wikipad[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_wikipad[FW_DATASIZE-1]);
				else if(machine_is_nabi2_xd()){
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_nabi2_xd[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_nabi2_xd[FW_DATASIZE-1]);
					}
				else if(machine_is_n1010()||machine_is_ns_14t004()||machine_is_itq1000()||machine_is_n1050()/*|| machine_is_limen()*/){
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt101_n1010[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt101_n1010[FW_DATASIZE-1]);
					}
				else	if(machine_is_n750()){
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_n750[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_n750[FW_DATASIZE-1]);
				}
				else	if(machine_is_birch()){
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_birch[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_birch[FW_DATASIZE-1]);
					}
				else
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt[FW_DATASIZE-1]);
				break;
			case EDT_TOUCH_BIRCH_055:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_birch_055[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_birch_055[FW_DATASIZE-1]);
				break;
			case CNT_TOUCH:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile[FW_DATASIZE-1]);
				break;
			case CNT_TOUCH_ITO:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_ITO[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_ITO[FW_DATASIZE-1]);
				break;
			case CMI_TOUCH:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_cmi[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_cmi[FW_DATASIZE-1]);
				break;
			case JIN_TOUCH :
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_jin[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_jin[FW_DATASIZE-1]);
				break;
			case RTR_TOUCH:
				if(machine_is_qc750() ||  machine_is_n710() ||  machine_is_itq700() || machine_is_itq701() || machine_is_mm3201() || machine_is_cybtt10_bk())
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n710[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n710[FW_DATASIZE-1]);
				else if(machine_is_n750() ) {
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n750[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n750[FW_DATASIZE-1]);
				}
				else if(machine_is_wikipad()) {
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_wikipad[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_wikipad[FW_DATASIZE-1]);
				}
				else if(machine_is_birch()) {
					FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_birch[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_birch[FW_DATASIZE-1]);
				}
				else
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr[FW_DATASIZE-1]);
				break;
			case SOE_TOUCH:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_soe[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_soe[FW_DATASIZE-1]);
				break;
			case WIDTH_TOUCH:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_xd[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_xd[FW_DATASIZE-1]);
				break;
			case EDT_TOUCH_2S:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_edt_2s[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_edt_2s[FW_DATASIZE-1]);
				break;
			case RTR_TOUCH_2S:
				FW_CHECKSUM = (unsigned int)(nvctp_BinaryFile_rtr_n710[FW_DATASIZE-2]<< 8 |nvctp_BinaryFile_rtr_n710[FW_DATASIZE-1]);
				break;
				break;
			default:
				break;
		}
	

	NOVADBG("%s,  new version checksum = %x \n",__func__,FW_CHECKSUM );
	if(nvctp_ReadFinalChecksum(FW_CHECKSUM_ADDR,FW_CHECKSUM,ts->client))
	{
		NOVADBG("nvctp_CheckIsBootloader1\n");

		switch(touch_type)
		{
			case EDT_TOUCH:
				if(machine_is_wikipad())
					novatek_Bootloader(nvctp_BinaryFile_edt_wikipad,sizeof(nvctp_BinaryFile_edt_wikipad),ts->client);
				else if(machine_is_nabi2_xd()){
					novatek_Bootloader(nvctp_BinaryFile_edt_nabi2_xd,sizeof(nvctp_BinaryFile_edt_nabi2_xd),ts->client);
					}
				else if(machine_is_n1010()||machine_is_ns_14t004()||machine_is_itq1000()||machine_is_n1050()/*|| machine_is_limen()*/){
					novatek_Bootloader(nvctp_BinaryFile_edt101_n1010,sizeof(nvctp_BinaryFile_edt101_n1010),ts->client);
					}
				else	if(machine_is_n750()){
					novatek_Bootloader(nvctp_BinaryFile_edt_n750,sizeof(nvctp_BinaryFile_edt_n750),ts->client);
				}
				else	if(machine_is_birch()){
					novatek_Bootloader(nvctp_BinaryFile_edt_birch,sizeof(nvctp_BinaryFile_edt_birch),ts->client);
					}
				else
					novatek_Bootloader(nvctp_BinaryFile_edt,sizeof(nvctp_BinaryFile_edt),ts->client);
				break;
			case EDT_TOUCH_BIRCH_055:
				novatek_Bootloader(nvctp_BinaryFile_edt_birch_055,sizeof(nvctp_BinaryFile_edt_birch_055),ts->client);
				break;
			case CNT_TOUCH:
				novatek_Bootloader(nvctp_BinaryFile,sizeof(nvctp_BinaryFile),ts->client);
				break;
			case CNT_TOUCH_ITO:
				novatek_Bootloader(nvctp_BinaryFile_ITO,sizeof(nvctp_BinaryFile_ITO),ts->client);
				break;
			case CMI_TOUCH:
				novatek_Bootloader(nvctp_BinaryFile_cmi,sizeof(nvctp_BinaryFile_cmi),ts->client);
				break;
			case JIN_TOUCH :
				novatek_Bootloader(nvctp_BinaryFile_jin,sizeof(nvctp_BinaryFile_jin),ts->client);
				break;
			case RTR_TOUCH:
				if( machine_is_qc750() ||  machine_is_n710() ||  machine_is_itq700() || machine_is_itq701() || machine_is_mm3201() || machine_is_cybtt10_bk())
					novatek_Bootloader(nvctp_BinaryFile_rtr_n710,sizeof(nvctp_BinaryFile_rtr_n710),ts->client);
				else if(machine_is_n750() ) {
					novatek_Bootloader(nvctp_BinaryFile_rtr_n750,sizeof(nvctp_BinaryFile_rtr_n750),ts->client);
				}
				else if(machine_is_wikipad()) {
					novatek_Bootloader(nvctp_BinaryFile_rtr_wikipad,sizeof(nvctp_BinaryFile_rtr_wikipad),ts->client);
				}
				else if(machine_is_birch()) {
					novatek_Bootloader(nvctp_BinaryFile_rtr_birch,sizeof(nvctp_BinaryFile_rtr_birch),ts->client);
				}
				else
					novatek_Bootloader(nvctp_BinaryFile_rtr,sizeof(nvctp_BinaryFile_rtr),ts->client);
				break;
			case SOE_TOUCH:
				novatek_Bootloader(nvctp_BinaryFile_soe,sizeof(nvctp_BinaryFile_soe),ts->client);
				break;
			case WIDTH_TOUCH:
				novatek_Bootloader(nvctp_BinaryFile_rtr_xd,sizeof(nvctp_BinaryFile_rtr_xd),ts->client);
				break;
			case EDT_TOUCH_2S:
				if(machine_is_nabi_2s())
					novatek_Bootloader(nvctp_BinaryFile_edt_2s,sizeof(nvctp_BinaryFile_edt_2s),ts->client);
				break;
			case RTR_TOUCH_2S:
				if(machine_is_nabi_2s())
					novatek_Bootloader(nvctp_BinaryFile_rtr_n710,sizeof(nvctp_BinaryFile_rtr_n710),ts->client);
				break;	
			default:
				break;
		}
			

			   
	}
	else
	{
		NOVADBG("--%s--, tp version is no change\n",__func__);
		iic_buffer[0] = 0x00;
		iic_buffer[1] = 0xA5;
		BootLoader_write_bytes(ts->client, iic_buffer, 2);										
	}
	
	/*****Hareware reset command******/
//	pdata->platform_wakeup();
	hw_reset();
	
}


/*************************************************************************/

#endif

static const struct i2c_device_id novatek_ts_id[] = {
	{ NOVATEK_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver novatek_ts_driver = {
	.probe		= novatek_ts_probe,
	.remove		= novatek_ts_remove,
	.shutdown 	= novatek_ts_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= novatek_ts_suspend,
	.resume		= novatek_ts_resume,
#endif
	.id_table	= novatek_ts_id,
	.driver = {
		.name	= NOVATEK_I2C_NAME,
		.owner = THIS_MODULE,
	},
};

/*******************************************************	
Description:
	Driver Install function.
return:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit novatek_ts_init(void)
{
	int ret;
	
	novatek_wq = create_workqueue("novatek_wq");		//create a work queue and worker thread
	if (!novatek_wq) {
		printk(KERN_ALERT "creat workqueue faiked\n");
		return -ENOMEM;
		
	}
	ret=i2c_add_driver(&novatek_ts_driver);
	return ret; 
}

/*******************************************************	
Description:
	Driver uninstall function.
return:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit novatek_ts_exit(void)
{
	printk(KERN_INFO "Touchscreen driver of guitar exited.\n");
	i2c_del_driver(&novatek_ts_driver);
	if (novatek_wq)
		destroy_workqueue(novatek_wq);		//release our work queue
}

late_initcall(novatek_ts_init);
module_exit(novatek_ts_exit);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
