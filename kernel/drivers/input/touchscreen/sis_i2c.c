/*
 * drivers/input/touchscreen/panjit_i2c.c
 *
 * Touchscreen class input driver for Panjit touch panel using I2C bus
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/i2c/sis_ts.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/linkage.h>
#include <asm/uaccess.h>
//#include "nvodm_services.h"

#if 0
#define CSR				0x00
#define CSR_SCAN_EN		(1 << 3)
#define CSR_SLEEP_EN	(1 << 7)
#define C_FLAG			0x01
#define X1_H				0x03
#endif

#define DRIVER_VERSION	"1.2.4"

#if defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION) || defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_10INCH)  || \
		defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_SWAP_XY)
#define RESOLUTION_MODE	1
#else
#define RESOLUTION_MODE	0
#endif

#if RESOLUTION_MODE
#define SIS_MAX_X		4095
#define SIS_MAX_Y		4095
#else 
#define SIS_MAX_X		39*64 //38*64//(2496) //keeli_h
#define SIS_MAX_Y		23*64 //22*64//(1472)	//keeli_h
#endif
#define SIS_MIN_X		(0)
#define SIS_MIN_Y		(0)

//NvOdmServicesI2cHandle mHOdmI2c;
static volatile int mFinished;

#define DRIVER_NAME	"sis_touch"

#define MAX_FINGERS		5
#define TOOL_WIDTH_FINGER	1
#define TOOL_WIDTH_MAX		8
#define TOOL_PRESSURE	100

// Package Information
#define MAX_READ_BYTE_COUNT					16
#define REAL_TOUCH_MODE						0x80
#define PKT_HAS_CRC							   	0x10
#define PKT_RTM_TOUCHNUM						0x0f
#define PS_TPDOWN							   	0x0000
#define PS_TPUP							           	0x0001

#define BYTECNT									0
#define FRAMEID									1
#define PKTINFO									2
#define P1TSTATE								3
#define X1_H									   	4
#define X1_L									   	5
#define Y1_H									   	6
#define Y1_L									   	7
#define P2TSTATE								8
#define X2_H									   	9
#define X2_L									   	10
#define Y2_H									   	11
#define Y2_L									   	12
#define MCRC_H									13
#define MCRC_L									14
#define SCRC_H									8
#define SCRC_L									9
#define BYTECNT_NOTOUCH						0x02
#define BYTECNT_SINGLE							0x09
#define BYTECNT_MULTI							0x0e

#define M_IS_RESUME                                                 0x0001 //jimmy add
#define M_IS_SUSPEND                                                0x0010//jimmy add

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sis_early_suspend(struct early_suspend *h);
static void sis_late_resume(struct early_suspend *h);
#endif

static int driver_paused = 0;

struct sis_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	int			gpio_reset;
	int 			gpio_shutdown;
	int 		isResume;
	struct early_suspend	early_suspend;
	struct work_struct work;
};

struct sis_event {
	__be16	coord[2][2];
	__u8	fingers;
	__u8	gesture;
};

union sis_buff {
	struct sis_event	data;
	unsigned char	buff[sizeof(struct sis_data)];
};

struct sis_data *ts_bak = 0;

static u8 sis_processdata (struct sis_data* handle ,u8* data);

static void sis_reset(struct sis_data *touch)
{
	if (touch->gpio_reset < 0)
		return;

	//gpio_set_value(touch->gpio_reset, 0);
	//msleep(10);
	gpio_set_value(touch->gpio_reset, 1);
	msleep(15);
}

//jimmy add start
static void sis_chips_shutdown(struct sis_data *touch)
{
	if (touch->gpio_shutdown< 0)
		return;

	gpio_set_value(touch->gpio_shutdown, 0);
}
static void sis_chips_open(struct sis_data *touch)
{
	if (touch->gpio_shutdown< 0)
		return;

	gpio_set_value(touch->gpio_shutdown, 1);
	msleep(15);
}



static void sis_work(struct work_struct *works)
{
	struct sis_data* touch =container_of(works, struct sis_data, work);
	if(touch->isResume == M_IS_RESUME)
	{
		//pr_err("%s:m is resume\n",__func__);
		sis_chips_open(touch);//jimmy add
		sis_reset(touch);//jimmy emp add 
		enable_irq(touch->client->irq);
	}else if(touch->isResume == M_IS_SUSPEND){
		//pr_err("%s:m is suspend\n",__func__);
		gpio_set_value(touch->gpio_reset, 0);
		msleep(15);
		sis_chips_shutdown(touch);//jimmy add
	}
}
//jimmy add end
static irqreturn_t sis_irq(int irq, void *dev_id)
{
	struct sis_data *touch = dev_id;
	struct i2c_client *client = touch->client;

	//struct i2c_msg msg[2];
	//u8 w_data[1]={0};
	u8 t_data[16]={0};
	int ret = 0;
	int port =irq_to_gpio(touch->client->irq);

	//printk("sis_irq client->addr=%d\n",client->addr);
	if ( driver_paused )
    {
       	return IRQ_HANDLED;
    }
	while(mFinished==0){

		//gpio_direction_input(button->gpio);
		
		int state =gpio_get_value(port);

		//printk("sis_irq state=%d,port=%d\n",state,port);

		if(state==1){
			//input_report_abs(touch->input_dev, 
			//	ABS_MT_PRESSURE, 0);
			//input_report_abs(touch->input_dev, 
			//	ABS_MT_TRACKING_ID, 1);
			//input_mt_sync(touch->input_dev);
			//input_sync(touch->input_dev);
			
			printk("sis_irq touch up3\n");
			break;
		}
#if 0

		if(mHOdmI2c ==NULL){
			printk("mHOdmI2c==NULL!!!!!\n");
			return IRQ_NONE;
		}

		NvOdmI2cStatus Error;
		NvOdmI2cTransactionInfo TransactionInfo[2];

		NvU8 REG = 0x0;

		TransactionInfo[0].Address = 0x0a;
		TransactionInfo[0].Buf = (char *)&REG;;
		TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
		TransactionInfo[0].NumBytes = 1;

		TransactionInfo[1].Address = 0x0a;
		TransactionInfo[1].Buf = t_data; 
		TransactionInfo[1].Flags = 0;
		TransactionInfo[1].NumBytes = 15;

		int dotry =0;
		do {
		    Error = NvOdmI2cTransaction(mHOdmI2c,
		                                TransactionInfo,
		                                2,
		                                100,
		                                500);
			dotry++;
			if(dotry>10){
				printk("timeout!!!!!........");
				break;
			}
		} while (Error == NvOdmI2cStatus_Timeout);
		//printk("Error=%d........",Error);
#elif 0
		msg[0].addr = client->addr;
		msg[0].flags = 0;
		msg[0].len = 1;
		msg[0].buf = w_data;

		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 15;
		msg[1].buf = t_data;

		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0) {
			dev_err(&client->dev, ".............Read from device fails.\n");
			return false;
		}
#else
		//ret = i2c_smbus_write_byte_data(client, 0, 0);
		//if (WARN_ON(ret < 0)) {
		//	dev_err(&client->dev, "error %d clearing interrupt\n", ret);
		//	return IRQ_NONE;
		//}

		ret = i2c_smbus_read_i2c_block_data(client, 0,15, t_data);
		if (WARN_ON(ret < 0)) {
			dev_err(&client->dev, "error %d reading event data\n", ret);
			return IRQ_NONE;
		}
#endif

		//printk("t_data = %x %x %x %x %x %x %x %x %x %x %x\n", t_data[0], t_data[1], t_data[2], t_data[3], 
		//	t_data[4], t_data[5], t_data[6], t_data[7], t_data[8], t_data[9], t_data[10]);

		sis_processdata(touch,t_data);
		break;
		//msleep(0.5);
		
	}

	return IRQ_HANDLED;
}
#if 0
static irqreturn_t sis_irq2(int irq, void *dev_id)
{
	

	struct sis_data *touch = dev_id;
	struct i2c_client *client = touch->client;

	struct i2c_msg msg[2];
	u8 w_data[1]={0};
	u8 t_data[16]={0};
	int ret = 0;

	printk("sis_irq client->addr=%d\n",client->addr);

	//w_data[0] = reg;
#if 0
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = w_data;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 15;
	msg[1].buf = t_data;

	ret = i2c_transfer(client->adapter, &msg[0], 2);
	if (ret < 0) {
		dev_err(&client->dev, ".............Read from device fails.\n");
		return false;
	}

	printk("t_data = %x %x %x %x %x %x %x %x %x %x %x\n", t_data[0], t_data[1], t_data[2], t_data[3], 
		t_data[4], t_data[5], t_data[6], t_data[7], t_data[8], t_data[9], t_data[10]);
#elif 0
	{

	ret = i2c_smbus_write_byte_data(client, 0, 0);
	if (WARN_ON(ret < 0)) {
		dev_err(&client->dev, "error %d clearing interrupt\n", ret);
		return IRQ_NONE;
	}

	ret = i2c_smbus_read_i2c_block_data(client, 0,16, t_data);
	if (WARN_ON(ret < 0)) {
		dev_err(&client->dev, "error %d reading event data\n", ret);
		return IRQ_NONE;
	}
#else
	if(mHOdmI2c ==NULL){
		printk("mHOdmI2c==NULL!!!!!\n");
		return IRQ_NONE;
	}

	NvOdmI2cStatus Error;
	NvOdmI2cTransactionInfo TransactionInfo[2];

	NvU8 REG = 0x0;

	TransactionInfo[0].Address = 0x0a;
	TransactionInfo[0].Buf = (char *)&REG;;
	TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
	TransactionInfo[0].NumBytes = 1;

	TransactionInfo[1].Address = 0x0a;
	TransactionInfo[1].Buf = t_data; 
	TransactionInfo[1].Flags = 0;
	TransactionInfo[1].NumBytes = 15;

	int dotry =0;
	do {
	    Error = NvOdmI2cTransaction(mHOdmI2c,
	                                TransactionInfo,
	                                2,
	                                100,
	                                500);
		dotry++;
		if(dotry>10){
			printk("timeout!!!!!........");
			break;
		}
	} while (Error == NvOdmI2cStatus_Timeout);
	printk("Error=%d........",Error);

	printk("t_data = %x %x %x %x %x %x %x %x %x %x %x\n", t_data[0], t_data[1], t_data[2], t_data[3], 
		t_data[4], t_data[5], t_data[6], t_data[7], t_data[8], t_data[9], t_data[10]);
#endif

	printk("data process.......");

	if(0!=sis_processdata(touch,t_data)){
		printk("fail\n");
	}
	else{
		printk("ok\n");	
	}
	
	return IRQ_HANDLED;
}

#endif

static u8 sis_processdata (struct sis_data* handle ,u8* data)
{
	u8* TouchData =data;
	u8 finger_num = 0;
	int i = 0;
	int pressure,size;
	//u16 x1=0, x2=0, y1=0, y2=0;
	
	static u16 prev_x0 = 0, prev_y0 = 0;
	u16 sTouchData[MAX_FINGERS][2];
	int trackPoint[2];

	switch (TouchData[BYTECNT] & 0xff)
	{
	case BYTECNT_NOTOUCH:
		finger_num = 0;
		break;
	case BYTECNT_SINGLE:
	case BYTECNT_MULTI:
		finger_num = (TouchData[PKTINFO] & PKT_RTM_TOUCHNUM);
		if (TouchData[P1TSTATE] & 0x01)
			finger_num = 0;
		break;	
	default:
		printk("err1.......");
		return -1;
	}

   	if (finger_num > 2)
   	{
    		finger_num = 0;
		printk("err2...finger_num=%d....",finger_num);
    		return -1;
    	}
#if RESOLUTION_MODE == 0
    	switch(finger_num){
	case 2:
		sTouchData[0][0] = SIS_MAX_X - ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]) / 2;
		sTouchData[0][1] = ((((u16)TouchData[Y1_H] & 0xff) << 8) | (u16)TouchData[Y1_L]) / 2;
		        
		sTouchData[1][0] = SIS_MAX_X - ((((u16)TouchData[X2_H] & 0xff) << 8) | (u16)TouchData[X2_L]) / 2;
		sTouchData[1][1] = ((((u16)TouchData[Y2_H] & 0xff) << 8) | (u16)TouchData[Y2_L]) / 2;

		trackPoint[0] = TouchData[P1TSTATE]>>4;
		trackPoint[1] = TouchData[P2TSTATE]>>4;

		pressure =TOOL_PRESSURE;
		size =TOOL_WIDTH_FINGER;
		break;
	case 1:
		sTouchData[0][0] = SIS_MAX_X - ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]) / 2;
		sTouchData[0][1] = ((((u16)TouchData[Y1_H] & 0xff) << 8) | (u16)TouchData[Y1_L]) / 2;

		trackPoint[0] = TouchData[P1TSTATE]>>4;

		pressure =TOOL_PRESSURE;
		size =TOOL_WIDTH_FINGER;
		break;

	case 0:
	default:
		sTouchData[0][0] = prev_x0;
		sTouchData[0][1] = prev_y0;
		        
		sTouchData[1][0] = 0;
		sTouchData[1][1] = 0;

		pressure =0;
		size =0;
		break;
	}
#else
    	switch(finger_num){
	case 2:
#if defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_10INCH)
		sTouchData[0][0] = ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]);
#else
		sTouchData[0][0] = SIS_MAX_X - ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]);
#endif
		sTouchData[0][1] = SIS_MAX_Y - ((((u16)TouchData[Y1_H] & 0xff) << 8) | (u16)TouchData[Y1_L]);
#if defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_10INCH)
		sTouchData[1][0] = ((((u16)TouchData[X2_H] & 0xff) << 8) | (u16)TouchData[X2_L]);
#else
		sTouchData[1][0] = SIS_MAX_X - ((((u16)TouchData[X2_H] & 0xff) << 8) | (u16)TouchData[X2_L]);
#endif
		sTouchData[1][1] = SIS_MAX_Y - ((((u16)TouchData[Y2_H] & 0xff) << 8) | (u16)TouchData[Y2_L]);

		//printk("p1 status=0x%x p2 status=0x%x\n",TouchData[P1TSTATE],TouchData[P2TSTATE]);

		trackPoint[0] = TouchData[P1TSTATE]>>4;
		trackPoint[1] = TouchData[P2TSTATE]>>4;

		pressure =TOOL_PRESSURE;
		size =TOOL_WIDTH_FINGER;
		break;
	case 1:
#if defined(CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_10INCH)
		sTouchData[0][0] = ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]);
#else
		sTouchData[0][0] = SIS_MAX_X - ((((u16)TouchData[X1_H] & 0xff) << 8) | (u16)TouchData[X1_L]);
#endif
		sTouchData[0][1] = SIS_MAX_Y - ((((u16)TouchData[Y1_H] & 0xff) << 8) | (u16)TouchData[Y1_L]);

		//printk("p1 status=0x%x\n",TouchData[P1TSTATE]);
		trackPoint[0] = TouchData[P1TSTATE]>>4;

		pressure =TOOL_PRESSURE;
		size =TOOL_WIDTH_FINGER;
		break;

	case 0:
	default:
		sTouchData[0][0] = prev_x0;
		sTouchData[0][1] = prev_y0;
		        
		sTouchData[1][0] = 0;
		sTouchData[1][1] = 0;

		pressure =0;
		size =0;
		break;
	}
#endif
	//printk("finger_num=%d,x0=%d,y0=%d\n",finger_num,sTouchData[0][0] ,sTouchData[0][1] );

	//input_report_key(handle->input_dev, BTN_TOUCH, finger_num);
	//input_report_key(handle->input_dev, BTN_2, finger_num == 2);
#if 0
	if (finger_num <= 1) {
		input_report_abs(handle->input_dev, ABS_X, sTouchData[0][0]);
		input_report_abs(handle->input_dev, ABS_Y, sTouchData[0][1]);
		prev_x0 = sTouchData[0][0];
		prev_y0 = sTouchData[0][1];
	} else if ((finger_num >= 2) && (finger_num <= MAX_FINGERS)) {
		int index = 1, offset = 0;
		int i;
		for (i = 0;i < 2; i++) {
			input_report_abs(handle->input_dev,
				ABS_HAT0X + offset, sTouchData[index][0]);
			input_report_abs(handle->input_dev,
				ABS_HAT0Y + offset, sTouchData[index][1]);
			index++;
			offset += 2;
		}
	}
#endif

#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_RESOLUTION_SWAP_XY

	int loop=0;
	for(loop=0;loop<finger_num;loop++){
		int tmpXY =sTouchData[loop][0];
		sTouchData[loop][0] =SIS_MAX_Y -sTouchData[loop][1];
		sTouchData[loop][1] =tmpXY;
	}
	
#endif

	if (finger_num <= 1) {
		prev_x0 = sTouchData[0][0];
		prev_y0 = sTouchData[0][1];
	}

	//input_report_key(handle->input_dev, BTN_TOUCH,
	//		 (finger_num == 1 || finger_num == 2));
	//input_report_key(handle->input_dev, BTN_2, (finger_num == 2));

	
	do {
		//input_report_abs(handle->input_dev,
		//	ABS_MT_TOUCH_MAJOR, pressure);
		//input_report_abs(handle->input_dev,
		//	ABS_MT_WIDTH_MAJOR, size);
		input_report_abs(handle->input_dev,
			ABS_MT_POSITION_X, sTouchData[i][0]);
		input_report_abs(handle->input_dev,
			ABS_MT_POSITION_Y, sTouchData[i][1]);
		input_report_abs(handle->input_dev, 
			ABS_MT_PRESSURE, pressure);
		input_report_abs(handle->input_dev, 
			ABS_MT_TRACKING_ID, trackPoint[i]);
		input_mt_sync(handle->input_dev);
		i++;
	} while (i < finger_num);

	
	input_sync(handle->input_dev);
	return 0;
}
#if 0
static irqreturn_t sis_irq2(int irq, void *dev_id)
{
	struct sis_data *touch = dev_id;
	struct i2c_client *client = touch->client;
	union sis_buff event;
	int ret, i;

	ret = i2c_smbus_read_i2c_block_data(client, X1_H,
					    sizeof(event.buff), event.buff);
	if (WARN_ON(ret < 0)) {
		dev_err(&client->dev, "error %d reading event data\n", ret);
		return IRQ_NONE;
	}
	ret = i2c_smbus_write_byte_data(client, C_FLAG, 0);
	if (WARN_ON(ret < 0)) {
		dev_err(&client->dev, "error %d clearing interrupt\n", ret);
		return IRQ_NONE;
	}

	input_report_key(touch->input_dev, BTN_TOUCH,
			 (event.data.fingers == 1 || event.data.fingers == 2));
	input_report_key(touch->input_dev, BTN_2, (event.data.fingers == 2));

	if (!event.data.fingers || (event.data.fingers > 2))
		goto out;

	for (i = 0; i < event.data.fingers; i++) {
		input_report_abs(touch->input_dev, ABS_MT_POSITION_X,
				 __be16_to_cpu(event.data.coord[i][0]));
		input_report_abs(touch->input_dev, ABS_MT_POSITION_Y,
				 __be16_to_cpu(event.data.coord[i][1]));
		input_report_abs(touch->input_dev, ABS_MT_TRACKING_ID, i + 1);
		input_mt_sync(touch->input_dev);
	}

out:
	input_sync(touch->input_dev);
	return IRQ_HANDLED;
}

#endif

static int sis_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct sis_i2c_ts_platform_data *pdata = client->dev.platform_data;
	struct sis_data *touch = NULL;
	struct input_dev *input_dev = NULL;
	int ret = 0;
	int port;

	touch = kzalloc(sizeof(struct sis_data), GFP_KERNEL);
	if (!touch) {
		dev_err(&client->dev, "%s: no memory\n", __func__);
		return -ENOMEM;
	}

	touch->gpio_reset = -EINVAL;
	touch->gpio_shutdown = -EINVAL;
	if (pdata) {
		ret = gpio_request(pdata->gpio_reset, "sis_reset");
		if (!ret) {
			ret = gpio_direction_output(pdata->gpio_reset, 1);
			if (ret < 0)
				gpio_free(pdata->gpio_reset);
		}

		if (!ret)
			touch->gpio_reset = pdata->gpio_reset;
		else
			dev_warn(&client->dev, "unable to configure sis reset GPIO\n");

		//jimmy add start
		ret = gpio_request(pdata->gpio_shutdown, "sis_shutdown");
		if (!ret) {
			ret = gpio_direction_output(pdata->gpio_shutdown, 1);
			if (ret < 0)
				gpio_free(pdata->gpio_shutdown);
		}

		if (!ret)
			touch->gpio_shutdown = pdata->gpio_shutdown;
		else
			dev_warn(&client->dev, "unable to configure sis shutdown GPIO\n");
		//jimmy add end
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "%s: no memory\n", __func__);
		kfree(touch);
		return -ENOMEM;
	}
	
	driver_paused = 0;
	touch->client = client;
	i2c_set_clientdata(client, touch);
	sis_chips_open(touch);//jimmy add 
	sis_reset(touch);
#if 0
	/* clear interrupt */
	ret = i2c_smbus_write_byte_data(touch->client, C_FLAG, 0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: clear interrupt failed\n",
			__func__);
		goto fail_i2c_or_register;
	}

	/* enable scanning */
	ret = i2c_smbus_write_byte_data(touch->client, CSR, CSR_SCAN_EN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: enable interrupt failed\n",
			__func__);
		goto fail_i2c_or_register;
	}
#endif
	touch->input_dev = input_dev;
	touch->input_dev->name = DRIVER_NAME;

	set_bit(EV_SYN, touch->input_dev->evbit);
	//set_bit(EV_KEY, touch->input_dev->evbit);
	set_bit(EV_ABS, touch->input_dev->evbit);
	//set_bit(BTN_TOUCH, touch->input_dev->keybit);
	//set_bit(BTN_2, touch->input_dev->keybit);

	/* expose multi-touch capabilities */
	set_bit(ABS_MT_POSITION_X, touch->input_dev->keybit);
	set_bit(ABS_MT_POSITION_Y, touch->input_dev->keybit);
	//set_bit(ABS_X, touch->input_dev->keybit);
	//set_bit(ABS_Y, touch->input_dev->keybit);

	/* all coordinates are reported in 0..4095 */
	input_set_abs_params(touch->input_dev, ABS_X, SIS_MIN_X, SIS_MAX_X, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_Y, SIS_MIN_Y, SIS_MAX_Y, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_HAT0X, SIS_MIN_X, SIS_MAX_X, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_HAT0Y, SIS_MIN_Y, SIS_MAX_Y, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_HAT1X, SIS_MIN_X, SIS_MAX_X, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_HAT1Y, SIS_MIN_Y, SIS_MAX_Y, 0, 0);

	input_set_abs_params(touch->input_dev, ABS_MT_POSITION_X, SIS_MIN_X, SIS_MAX_X, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_MT_POSITION_Y, SIS_MIN_Y, SIS_MAX_Y, 0, 0);

	//input_set_abs_params(touch->input_dev, ABS_MT_TOUCH_MAJOR,0, TOOL_PRESSURE, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_MT_WIDTH_MAJOR, 0,TOOL_WIDTH_MAX, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_MT_PRESSURE, 0,TOOL_PRESSURE, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_TOOL_WIDTH, 0,TOOL_WIDTH_MAX, 0, 0);

	//disable the track id, because the id will change ???
	//input_set_abs_params(touch->input_dev, ABS_MT_TRACKING_ID, 0, 0xf, 0, 0);
	

	ret = input_register_device(touch->input_dev);
	if (ret) {
		dev_err(&client->dev, "%s: input_register_device failed\n",
			__func__);
		goto fail_i2c_or_register;
	}
	
	ts_bak = touch;
	port =irq_to_gpio(touch->client->irq);

	mFinished =0;
	ret = gpio_request(port, DRIVER_NAME);
	if(ret <0){
		dev_err(&client->dev, "%s: gpio_request(%d) failed\n",
			__func__, touch->client->irq);
		goto fail_irq;
	}
	ret = gpio_direction_input(port);
	if (ret < 0) {
		dev_err(&client->dev, "%s: gpio_direction_input(%d) failed\n",
			__func__, touch->client->irq);
		gpio_free(port);
		goto fail_irq;
	}
	
#if 1
	/* get the irq */
	ret = request_threaded_irq(touch->client->irq, NULL, sis_irq,
				   IRQF_ONESHOT | IRQF_TRIGGER_LOW,
				   DRIVER_NAME, touch);
	if (ret) {
		dev_err(&client->dev, "%s: request_irq(%d) failed\n",
			__func__, touch->client->irq);
		gpio_free(port);
		goto fail_irq;
	}
#else

	ret = request_irq(touch->client->irq, sis_irq,
				    IRQF_TRIGGER_LOW,
				    DRIVER_NAME,
				    touch);
	if (ret) {
		dev_err(&client->dev, "%s: request_irq(%d) failed\n",
			__func__, touch->client->irq);
		goto fail_irq;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	touch->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	touch->early_suspend.suspend = sis_early_suspend;
	touch->early_suspend.resume = sis_late_resume;
	register_early_suspend(&touch->early_suspend);
#endif
	dev_info(&client->dev, "%s: initialized\n", __func__);

	//mHOdmI2c =NvOdmI2cOpen(NvOdmIoModule_I2c, 0);

	INIT_WORK(&touch->work, sis_work);//jimmy add

	touch->isResume = M_IS_RESUME;

	gpio_set_value(touch->gpio_reset, 0);
	msleep(15);
	sis_chips_shutdown(touch);
	msleep(15);
	sis_chips_open(touch);//jimmy add
	sis_reset(touch);//jimmy emp add 
	//enable_irq(touch->client->irq);

	return 0;

fail_irq:
	input_unregister_device(touch->input_dev);

fail_i2c_or_register:
	if (touch->gpio_reset >= 0)
		gpio_free(touch->gpio_reset);

	input_free_device(input_dev);
	kfree(touch);
	return ret;
}

static int sis_suspend(struct i2c_client *client, pm_message_t state)
{
	struct sis_data *touch = i2c_get_clientdata(client);
	printk("sis_suspend\n");
	disable_irq(client->irq);
	touch->isResume = M_IS_SUSPEND;
	schedule_work(&touch->work);
	return 0;
}

#if 0
static int sis_suspend2(struct i2c_client *client, pm_message_t state)
{
	struct sis_data *touch = i2c_get_clientdata(client);
	int ret;

	if (WARN_ON(!touch))
		return -EINVAL;

	disable_irq(client->irq);

	/* disable scanning and enable deep sleep */
	ret = i2c_smbus_write_byte_data(client, CSR, CSR_SLEEP_EN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: sleep enable fail\n", __func__);
		return ret;
	}

	return 0;
}
#endif

static int sis_resume(struct i2c_client *client)
{
	struct sis_data *touch = i2c_get_clientdata(client);
	//int ret = 0;

	printk("sis_resume\n");

	if (WARN_ON(!touch))
		return -EINVAL;
	touch->isResume = M_IS_RESUME;
	schedule_work(&touch->work);

	return 0;
}

#if 0
static int sis_resume2(struct i2c_client *client)
{
	struct sis_data *touch = i2c_get_clientdata(client);
	int ret = 0;

	if (WARN_ON(!touch))
		return -EINVAL;

	sis_reset(touch);

	/* enable scanning and disable deep sleep */
	ret = i2c_smbus_write_byte_data(client, C_FLAG, 0);
	if (ret >= 0)
		ret = i2c_smbus_write_byte_data(client, CSR, CSR_SCAN_EN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: scan enable fail\n", __func__);
		return ret;
	}

	enable_irq(client->irq);

	return 0;
}

#endif 

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sis_early_suspend(struct early_suspend *es)
{
	struct sis_data *touch;
	touch = container_of(es, struct sis_data, early_suspend);

	if (sis_suspend(touch->client, PMSG_SUSPEND) != 0)
		dev_err(&touch->client->dev, "%s: failed\n", __func__);
}

static void sis_late_resume(struct early_suspend *es)
{
	struct sis_data *touch;
	touch = container_of(es, struct sis_data, early_suspend);

	if (sis_resume(touch->client) != 0)
		dev_err(&touch->client->dev, "%s: failed\n", __func__);
}
#endif

static int sis_remove(struct i2c_client *client)
{
	struct sis_data *touch = i2c_get_clientdata(client);

	if (!touch)
		return -EINVAL;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&touch->early_suspend);
#endif
	gpio_free(irq_to_gpio(touch->client->irq));
	free_irq(touch->client->irq, touch);
	if (touch->gpio_reset >= 0)
		gpio_free(touch->gpio_reset);
	input_unregister_device(touch->input_dev);
	input_free_device(touch->input_dev);
	cancel_work_sync(&touch->work);//jimmy add
	kfree(touch);
	return 0;
}

static const struct i2c_device_id sis_ts_id[] = {
	{ DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver sis_driver = {
	.probe		= sis_probe,
	.remove		= sis_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= sis_suspend,
	.resume		= sis_resume,
#endif
	.id_table	= sis_ts_id,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __devinit sis_init(void)
{
	int e;

	e = i2c_add_driver(&sis_driver);
	if (e != 0) {
		pr_err("%s: failed to register with I2C bus with "
		       "error: 0x%x\n", __func__, e);
	}
	return e;
}

static void __exit sis_exit(void)
{
	mFinished=1;
	i2c_del_driver(&sis_driver);
}

asmlinkage long sys_sis_I2C_stop( void )
{
    printk(KERN_INFO "sys_sis_I2C_stop\n" );

    driver_paused = 1;

    //flush_workqueue( sis_wq );

    return 0;
}

asmlinkage long sys_sis_I2C_start( void )
{
    printk(KERN_INFO "sys_sis_I2C_start\n" );

    driver_paused = 0;

    return 0;
}


#define BUFFER_SIZE 16

asmlinkage long sys_sis_I2C_IO( unsigned char* data, int size )
{

    unsigned char cmd;
    int ret = 0;
    int i;
    
    unsigned char *kdata;
    
    printk(KERN_INFO "sys_sis_I2C_IO\n" );

    if ( ts_bak == 0 ) return -13;

    ret = access_ok( VERIFY_WRITE, data, BUFFER_SIZE );
    if ( !ret ) {
        printk(KERN_INFO "cannot access user space memory\n" );
        return -11;
    }

    kdata = kmalloc( BUFFER_SIZE, GFP_KERNEL );
    if ( kdata == 0 ) return -12;
    
    ret = copy_from_user( kdata, data, BUFFER_SIZE );
    if ( ret ) {
        printk(KERN_INFO "copy_from_user fail\n" );
        kfree( kdata );
        return -14;
    }
    
    cmd = kdata[0];
    
    printk(KERN_INFO "size = %d\n",size);
#ifndef _SMBUS_INTERFACE
	struct i2c_msg msg[2];
#endif
    printk(KERN_INFO "io cmd=%02x\n", cmd );

    if (size>1)
    {
        printk(KERN_INFO "write\n" );
#ifdef _SMBUS_INTERFACE
        ret = i2c_smbus_write_block_data( ts_bak->client, cmd, size-1, kdata+1 );
        ret = i2c_smbus_read_block_data( ts_bak->client, 0x02, kdata );
#else
//Write
//ret = i2c_smbus_write_block_data( ts_bak->client, cmd, size-1, data+1 );
        msg[0].addr = ts_bak->client->addr;
        msg[0].flags = 0;
        msg[0].len = size;
        msg[0].buf = (unsigned char *)kdata;

        ret = i2c_transfer(ts_bak->client->adapter, msg, 1);
        if (ret < 0) {
            printk(KERN_INFO "i2c_transfer write error %d\n", ret);
			kfree( kdata );
			return -21;
        }
//Read
//ret = i2c_smbus_read_block_data( ts_bak->client, 0x02, data );
        cmd = 0x00;
        msg[0].addr = ts_bak->client->addr;
        msg[0].flags = I2C_M_NOSTART;//set repeat start and Write data
        msg[0].len = 1;
        msg[0].buf = (unsigned char *)(&cmd);

        msg[1].addr = ts_bak->client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 16;
        msg[1].buf = (unsigned char *)kdata;

        ret = i2c_transfer(ts_bak->client->adapter, msg, 2);
        if (ret < 0) {
            printk(KERN_INFO "i2c_transfer read error %d\n", ret);
			kfree( kdata );
			return -21;
        }
#endif
    }
    else
    {
        //printk(KERN_INFO "read\n" );
#ifdef _SMBUS_INTERFACE
        ret = i2c_smbus_read_block_data( ts_bak->client, cmd, kdata );
#else
 		msg[0].addr = ts_bak->client->addr;
        msg[0].flags = I2C_M_NOSTART;//Write data
        msg[0].len = 1;
        msg[0].buf = (unsigned char *)(&cmd);

        msg[1].addr = ts_bak->client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 16;
        msg[1].buf = (unsigned char *)kdata;

        ret = i2c_transfer(ts_bak->client->adapter, msg, 2);
        if (ret < 0) {
            printk(KERN_INFO "i2c_transfer read error %d\n", ret);
			kfree( kdata );
			return -21;
        }
#endif
    }

#ifndef _SMBUS_INTERFACE
    ret = kdata[0];

    for ( i = 0; i < BUFFER_SIZE - 1; i++ ) {
	    kdata[i] = kdata[i+1];
    }
#endif

    printk( KERN_INFO "%d\n", ret );

    for ( i = 0; i < ret; i++ )
        printk( "%x ", kdata[i] );

    printk( "\n" );

    if ( copy_to_user( data, kdata, BUFFER_SIZE ) ) {
        printk(KERN_INFO "copy_to_user fail\n" );
        ret = -19;
    }    

    kfree( kdata );

    return ret;
}

#undef BUFFER_SIZE
module_init(sis_init);
module_exit(sis_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sis I2C touch driver");
