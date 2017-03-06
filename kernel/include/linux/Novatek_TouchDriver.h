/*
 * include/linux/novatek_touchdriver.h
 *
 * Copyright (C) 2010 - 2011 Novatek, Inc.
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

#ifndef 	_LINUX_NOVATEK_TOUCH_H 
#define		_LINUX_NOVATEK_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

//*************************TouchScreen Work Part*****************************

#define NOVATEK_I2C_NAME "nt1103-ts"
#define	NOVATEK_I2C_SCL	200*1000
/*******************Step 1: define resolution *****************/
//define default resolution of the touchscreen
//#define TOUCH_MAX_HEIGHT 	960//1024			
//#define TOUCH_MAX_WIDTH	1920 //	1984

//define default resolution of LCM
#define SCREEN_MAX_HEIGHT   600
#define SCREEN_MAX_WIDTH    1024
#define SCREEN_MAX_HEIGHT_3D	1280
#define SCREEN_MAX_WIDTH_3D		800
#define SCREEN_MAX_WIDTH_XD	1366
#define SCREEN_MAX_HEIGHT_XD		768
#define TOOL_PRESSURE	100
#if 0
#define INT_PORT  	    S3C64XX_GPN(15)						//Int IO port  S3C64XX_GPL(10)
#ifdef INT_PORT
	#define TS_INT 		gpio_to_irq(INT_PORT)			//Interrupt Number,EINT18(119)
	#define INT_CFG    	S3C_GPIO_SFN(2)					//IO configer as EINT
#else
	#define TS_INT	0
#endif	
#else
/**********************Step 2: Setting Interrupt******************************/
//#define BABBAGE_NOVATEK_TS_RST1  (('n'-'a')*8+5)
//#define BABBAGE_NOVATEK_TS_INT1 (('z'-'a')*8+3)
#define BABBAGE_NOVATEK_TS_RST1  (('k'-'a')*8+7)
#define BABBAGE_NOVATEK_TS_INT1 (('j'-'a')*8+0)
#define INT_PORT BABBAGE_NOVATEK_TS_INT1
#define TS_INT gpio_to_irq(INT_PORT)
#define TS_GPIO irq_to_gpio(INT_PORT)

#endif
/******************Step 3: Setting Reset option**************************/
#define Novatek_HWINT_LowLeval()    gpio_direction_output(BABBAGE_NOVATEK_TS_INT1, 0);gpio_set_value(BABBAGE_NOVATEK_TS_INT1, 0)
#define Nvoatek_HWINT_HighLeval()   gpio_set_value(BABBAGE_NOVATEK_TS_INT1, 1);gpio_direction_input(BABBAGE_NOVATEK_TS_INT1)
#define Novatek_HWRST_LowLeval()    (gpio_set_value(BABBAGE_NOVATEK_TS_RST1, 0))
#define Nvoatek_HWRST_HighLeval()   (gpio_set_value(BABBAGE_NOVATEK_TS_RST1, 1))
//whether need send cfg?
struct tp_event {
	u16	x;
	u16	y;
    	s16 id;
	u16	pressure;
	u8  touch_point;
	u8  flag;
};
#define DRIVER_SEND_CFG
#define NT11002   0
#define NT11003   1
#define IC_DEFINE   NT11003
#if IC_DEFINE == NT11002
#define IIC_BYTENUM    4
#elif IC_DEFINE == NT11003
#define IIC_BYTENUM    6
#endif
//set trigger mode
#define INT_TRIGGER		0

#define POLL_TIME		10	//actual query spacing interval:POLL_TIME+6

#define NOVATEK_MULTI_TOUCH
#ifdef NOVATEK_MULTI_TOUCH
	#define MAX_FINGER_NUM	 5	//5
#else
	#define MAX_FINGER_NUM	1	
#endif
//#define  HAVE_TOUCH_KEY
#ifdef HAVE_TOUCH_KEY
#define MENU        21
#define HOME        22
#define BACK        23
#define VOLUMEDOWN  24
#define VOLUMEUP    25
#endif

//#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
#define __TOUCH_RESET__
struct novatek_ts_data {
	uint16_t addr;
	uint8_t bad_data;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_reset;		//use RESET flag
	int use_irq;		//use EINT flag
	int read_mode;		//read moudle mode,20110221 by andrew
	struct hrtimer timer;
	struct work_struct  work;
#ifdef __TOUCH_RESET__
	struct work_struct  reset_work;
#endif
	char phys[32];
	int retry;
	struct early_suspend early_suspend;
	int (*power)(struct novatek_ts_data * ts, int on);
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t max_touch_num;
	uint8_t int_trigger_type;
	uint8_t green_wake_mode;
};

enum resolution {
	TOUCH_MAX_HEIGHT_CNT = 960,
	TOUCH_MAX_WIDTH_CNT = 1920,
	TOUCH_MAX_HEIGHT_EDT = 960,
	TOUCH_MAX_WIDTH_EDT = 1728,
	TOUCH_MAX_HEIGHT_CMI = 1024,
	TOUCH_MAX_WIDTH_CMI = 1728,
	TOUCH_MAX_HEIGHT_JIN = 960,
	TOUCH_MAX_WIDTH_JIN = 1920,
	TOUCH_MAX_HEIGHT_RTR = 960,
	TOUCH_MAX_WIDTH_RTR = 1920,
	TOUCH_MAX_HEIGHT_SOE = 960,
	TOUCH_MAX_WIDTH_SOE = 1920,
	TOUCH_MAX_HEIGHT_XD = 768,
	TOUCH_MAX_WIDTH_XD = 1408,
	TOUCH_MAX_HEIGHT_EDT_2S = 1152,
	TOUCH_MAX_WIDTH_EDT_2S = 1792,
	TOUCH_MAX_HEIGHT_EDT_XD =1536,//2688,
	TOUCH_MAX_WIDTH_EDT_XD =2688,
	TOUCH_MAX_HEIGHT_GFT =800,//2688,
	TOUCH_MAX_WIDTH_GFT =1280,

};

enum  __touch__{
	TOUCH_ERROR = -1,
	EDT_TOUCH = 0x03,
	CNT_TOUCH =0x07,
	CNT_TOUCH_ITO =0x05,
	CMI_TOUCH = 0x06,
	JIN_TOUCH = 0x01,
	RTR_TOUCH = 0x04,
	SOE_TOUCH = 0x02,
	WIDTH_TOUCH = 0x08,
	RTR_TOUCH_2S = 0x0c,
	EDT_TOUCH_2S = 0x0e,
	EDT_TOUCH_BIRCH_055=0x0f,
};
int touch_flag=CNT_TOUCH;
int touch_type;
int touch_type1;
#define NVT_TOUCH_CTRL_DRIVER 
#ifdef  NVT_TOUCH_CTRL_DRIVER
#define MAX_BUFFER_SIZE 128
struct nvt_flash_data {
	rwlock_t lock;
	unsigned char bufferIndex;
	unsigned int length;
	unsigned int buffer[MAX_BUFFER_SIZE];
	struct i2c_client *client;
};

#endif
#ifdef __TOUCH_RESET__
//#define CONFIG_TOUCHSCREEN_SSL_DEBUG
int TimerCount1,TimerCount2;
#define MaxTimerCount1  5 //200
#define MaxTimerCount2  2//30
#define TIMERCOUNT   20
int FlagInCtpIsr;
int TouchCaliState = 0;
int TouchCount = 0;

#define MicroTimeTInterupt	10000000// 100Hz - 10,000,000us
#endif
static const char *novatek_ts_name = "nt1103-ts";
static struct workqueue_struct *novatek_wq;
struct i2c_client * i2c_connect_client_novatek = NULL; 
static struct proc_dir_entry *novatek_proc_entry;
static struct kobject *novatek_debug_kobj;
	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void novatek_ts_early_suspend(struct early_suspend *h);
static void novatek_ts_late_resume(struct early_suspend *h);
#endif 

//*****************************End of Part I *********************************

//*************************Touchkey Surpport Part*****************************
//#define HAVE_TOUCH_KEY
#ifdef HAVE_TOUCH_KEY
	#define READ_COOR_ADDR 0x00
	const uint16_t touch_key_array[]={
									  KEY_MENU,				//MENU
									  KEY_HOME,				//HOME
									  KEY_SEND				//CALL
									  
									 }; 
	#define MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#else
	#define READ_COOR_ADDR 0x00
#endif
//*****************************End of Part II*********************************

//*************************Firmware Update part*******************************
#define CONFIG_TOUCHSCREEN_NOVATEK_IAP
#ifdef CONFIG_TOUCHSCREEN_NOVATEK_IAP
//#define UPDATE_NEW_PROTOCOL

unsigned int oldcrc32 = 0xFFFFFFFF;
unsigned int crc32_table[256];
unsigned int ulPolynomial = 0x04c11db7;
unsigned char rd_cfg_addr;
unsigned char rd_cfg_len;
unsigned char g_enter_isp = 0;

static int novatek_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data);
static int novatek_update_read( char *page, char **start, off_t off, int count, int *eof, void *data );

#define PACK_SIZE 					64					//update file package size
#define MAX_TIMEOUT					60000				//update time out conut
#define MAX_I2C_RETRIES				20					//i2c retry times

//I2C buf address
#define ADDR_CMD					80
#define ADDR_STA					81
#ifdef UPDATE_NEW_PROTOCOL
	#define ADDR_DAT				0
#else
	#define ADDR_DAT				82
#endif

//moudle state
#define NEW_UPDATE_START			0x01
#define UPDATE_START				0x02
#define SLAVE_READY					0x08
#define UNKNOWN_ERROR				0x00
#define FRAME_ERROR					0x10
#define CHECKSUM_ERROR				0x20
#define TRANSLATE_ERROR				0x40
#define FLASH_ERROR					0X80

//error no
#define ERROR_NO_FILE				2	//ENOENT
#define ERROR_FILE_READ				23	//ENFILE
#define ERROR_FILE_TYPE				21	//EISDIR
#define ERROR_GPIO_REQUEST			4	//EINTR
#define ERROR_I2C_TRANSFER			5	//EIO
#define ERROR_NO_RESPONSE			16	//EBUSY
#define ERROR_TIMEOUT				110	//ETIMEDOUT

//update steps
#define STEP_SET_PATH              1
#define STEP_CHECK_FILE            2
#define STEP_WRITE_SYN             3
#define STEP_WAIT_SYN              4
#define STEP_WRITE_LENGTH          5
#define STEP_WAIT_READY            6
#define STEP_WRITE_DATA            7
#define STEP_READ_STATUS           8
#define FUN_CLR_VAL                9
#define FUN_CMD                    10
#define FUN_WRITE_CONFIG           11

//fun cmd
#define CMD_DISABLE_TP             0
#define CMD_ENABLE_TP              1
#define CMD_READ_VER               2
#define CMD_READ_RAW               3
#define CMD_READ_DIF               4
#define CMD_READ_CFG               5
#define CMD_SYS_REBOOT             101

//read mode
#define MODE_RD_VER                1
#define MODE_RD_RAW                2
#define MODE_RD_DIF                3
#define MODE_RD_CFG                4


#endif
//*****************************End of Part III********************************

// Chip Reset define 
#define  HW_RST      0
#define  SW_RST      1

#if 0
#define KFprintk(x...) printk(x)
#else
#define KFprintk(x...) do{} while(0)
#endif
static struct early_suspend novatek_power; //for ealy suspend
static struct i2c_client *this_client;

#define _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_
#ifdef _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_
enum
{
  RS_OK         = 0,
  RS_INIT_ER    = 8,
  RS_ERAS_ER    = 9,
  RS_FLCS_ER    = 10,
  RS_WD_ER      = 11
} ;
#endif

#endif /* _LINUX_NOVATEK_TOUCH_H */
