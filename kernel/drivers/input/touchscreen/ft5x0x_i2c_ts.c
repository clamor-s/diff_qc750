/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *    note: only support mulititouch    Wenfs 2010-10-01
 */

//#define CONFIG_FTS_CUSTOME_ENV
#define TEGRA_GPIO_PW1		177
#define TOOL_PRESSURE	100
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/i2c/ft5x0x_i2c_ts.h>
#include<linux/i2c/ft5x0x_ts_data.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>
/* -------------- global variable definition -----------*/
static struct i2c_client *this_client;
static REPORT_FINGER_INFO_T _st_finger_infos[CFG_MAX_POINT_NUM];
static unsigned int _sui_irq_num=0;//= IRQ_EINT(6);
static int _si_touch_num = 0; 
struct FT5X0X_i2c_ts_platform_data *ft_pdata=NULL;
static int g_Screen_Max_x=0;
static int g_Screen_Max_y=0;

static  int tsp_keycodes[CFG_NUMOFKEYS] ={

        KEY_MENU,
        KEY_HOME,
        KEY_BACK,
        KEY_SEARCH
};

static  char *tsp_keyname[CFG_NUMOFKEYS] ={

        "Menu",
        "Home",
        "Back",
        "Search"
};

static bool tsp_keystatus[CFG_NUMOFKEYS];

#define REG_DEVICE_MODE 0x00
#define REG_GEST_ID		0x01
#define REG_TD_STATUS	0x02

#define INDEX_TOUCH_XH	0x00
#define INDEX_TOUCH_XL	0x01
#define INDEX_TOUCH_YH	0x02
#define INDEX_TOUCH_YL	0x03

#define TP_POINT_SIZE 	6
#define TP_POINT_START 	0x03

#define BUFFER_SIZE		(REG_TD_STATUS+1+CFG_MAX_POINT_NUM*TP_POINT_SIZE)

#define TP_DOWN			0x00
#define TP_UP			0x01
#define TP_MOVE			0x02

typedef struct _TP_DATA{
	int x;
	int y;
	int pressure;
}TP_DATA;




/***********************************************************************
    [function]: 
		           callback:              read data from ctpm by i2c interface;
    [parameters]:
			    buffer[in]:            data buffer;
			    length[in]:           the length of the data buffer;
    [return]:
			    FTS_TRUE:            success;
			    FTS_FALSE:           fail;
************************************************************************/
static bool i2c_read_interface(u8* pbt_buf, int dw_lenth)
{
    int ret;
    
    ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

    if(ret<=0)
    {
        printk("[TSP]i2c_read_interface error\n");
        return FTS_FALSE;
    }
  
    return FTS_TRUE;
}



/***********************************************************************
    [function]: 
		           callback:               write data to ctpm by i2c interface;
    [parameters]:
			    buffer[in]:             data buffer;
			    length[in]:            the length of the data buffer;
    [return]:
			    FTS_TRUE:            success;
			    FTS_FALSE:           fail;
************************************************************************/
static bool  i2c_write_interface(u8* pbt_buf, int dw_lenth)
{
    int ret;
    ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
    if(ret<=0)
    {
        printk("[TSP]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
        return FTS_FALSE;
    }

    return FTS_TRUE;
}



/***********************************************************************
    [function]: 
		           callback:                 read register value ftom ctpm by i2c interface;
    [parameters]:
                        reg_name[in]:         the register which you want to read;
			    rx_buf[in]:              data buffer which is used to store register value;
			    rx_length[in]:          the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static bool fts_register_read(u8 reg_name, u8* rx_buf, int rx_length)
{
	u8 read_cmd[2]= {0};
	u8 cmd_len 	= 0;

	read_cmd[0] = reg_name;
	cmd_len = 1;	

	/*send register addr*/
	if(!i2c_write_interface(&read_cmd[0], cmd_len))
	{
		return FTS_FALSE;
	}

	/*call the read callback function to get the register value*/		
	if(!i2c_read_interface(rx_buf, rx_length))
	{
		return FTS_FALSE;
	}
	return FTS_TRUE;
}




/***********************************************************************
    [function]: 
		           callback:                read register value ftom ctpm by i2c interface;
    [parameters]:
                        reg_name[in]:         the register which you want to write;
			    tx_buf[in]:              buffer which is contained of the writing value;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static bool fts_register_write(u8 reg_name, u8* tx_buf)
{
	u8 write_cmd[2] = {0};

	write_cmd[0] = reg_name;
	write_cmd[1] = *tx_buf;

	/*call the write callback function*/
	return i2c_write_interface(write_cmd, 2);
}


#ifdef CONFIG_HAS_EARLYSUSPEND
/***********************************************************************
    [function]: 
		           callback:                early suspend function interface;
    [parameters]:
                        handler:                 early suspend callback pointer
    [return]:
                        NULL
************************************************************************/
static void fts_ts_suspend(struct early_suspend *handler)
{
       u8 cmd;
	   
  	gpio_set_value(ft_pdata->gpio_shutdown,0);
      msleep(20);
	cmd=PMODE_HIBERNATE;
	printk("\n [TSP]:device will suspend! \n");
	fts_register_write(FT5X0X_REG_PMODE,&cmd);
}


/***********************************************************************
    [function]: 
		           callback:                power resume function interface;
    [parameters]:
                        handler:                 early suspend callback pointer
    [return]:
                        NULL
************************************************************************/
static void fts_ts_resume(struct early_suspend *handler)
{
      u8 cmd;

	
	msleep(20);
	gpio_set_value(ft_pdata->gpio_shutdown,1);           //wantianpei add
	cmd=PMODE_ACTIVE;
	printk("\n [TSP]:device will resume from sleep! \n");
	fts_register_write(ft_pdata->gpio_shutdown, &cmd);
}
#endif  


/***********************************************************************
    [function]: 
		           callback:        report to the input system that the finger is put up;
    [parameters]:
                         null;
    [return]:
                         null;
************************************************************************/
static void fts_ts_release(void)
{
    struct FTS_TS_DATA_T *data = i2c_get_clientdata(this_client);
    int i;
    int i_need_sync = 0;
    for ( i= 0; i<CFG_MAX_POINT_NUM; ++i )
    {
        if ( _st_finger_infos[i].u2_pressure == -1 )
            continue;

        _st_finger_infos[i].u2_pressure = 0;


        input_report_abs(data->input_dev, ABS_MT_POSITION_X, _st_finger_infos[i].i2_x);
        input_report_abs(data->input_dev, ABS_MT_POSITION_Y, _st_finger_infos[i].i2_y);
        input_report_abs(data->input_dev, ABS_MT_PRESSURE, _st_finger_infos[i].u2_pressure);
        input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, _st_finger_infos[i].ui2_id);
	 	input_mt_sync(data->input_dev);

        i_need_sync = 1;

        if ( _st_finger_infos[i].u2_pressure == 0 )
            _st_finger_infos[i].u2_pressure= -1;
    }

     if (i_need_sync)
     {
        input_sync(data->input_dev);
      }
	 
    _si_touch_num = 0;
}






/***********************************************************************
    [function]: 
		           callback:                 read touch  data ftom ctpm by i2c interface;
    [parameters]:
			    rxdata[in]:              data buffer which is used to store touch data;
			    length[in]:              the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static int fts_i2c_rxdata(u8 *rxdata, int length)
{
      int ret;
      struct i2c_msg msg;

	
      msg.addr = this_client->addr;
      msg.flags = 0;
      msg.len = 1;
      msg.buf = rxdata;
      ret = i2c_transfer(this_client->adapter, &msg, 1);
	  
	if (ret < 0)
		pr_err("msg %s i2c write error: %d\n", __func__, ret);
		
      msg.addr = this_client->addr;
      msg.flags = I2C_M_RD;
      msg.len = length;
      msg.buf = rxdata;
      ret = i2c_transfer(this_client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("msg %s i2c write error: %d\n", __func__, ret);
		
	return ret;
}





/***********************************************************************
    [function]: 
		           callback:                send data to ctpm by i2c interface;
    [parameters]:
			    txdata[in]:              data buffer which is used to send data;
			    length[in]:              the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static int fts_i2c_txdata(u8 *txdata, int length)
{
	int ret;

	struct i2c_msg msg;

      msg.addr = this_client->addr;
      msg.flags = 0;
      msg.len = length;
      msg.buf = txdata;
	ret = i2c_transfer(this_client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

void dumpBuf(u8* buf,int size){

	int i=0;
	
	for(i=0;i<size;i++){
		printk(" buf[%d]=0x%x ",i,buf[i]);
	}
	printk("\n");
}

int fts_read_data(void)
{
    struct FTS_TS_DATA_T *data = i2c_get_clientdata(this_client);
    struct FTS_TS_EVENT_T *event = &data->event;
    u8 buf[BUFFER_SIZE];
	
	int pointCount;
	int curNum;
	u8 pointStatus;
	int ret;
	int x,y;
	int pressure;
	int id;
	int realCount;

	static TP_DATA tp_data[CFG_MAX_POINT_NUM] ={0};

again:

    buf[0] = 0;
	
    ret=fts_i2c_rxdata(buf, BUFFER_SIZE); 
	
    if (ret > 0)  
    {
		realCount =buf[REG_TD_STATUS]&0x0F;
	
		pointCount =CFG_MAX_POINT_NUM;
		if(pointCount>CFG_MAX_POINT_NUM){
			pointCount =CFG_MAX_POINT_NUM;
		}

		//printk("point count=%d realCount=%d\n",pointCount,realCount);

		u8* curBuffer =&buf[TP_POINT_START];
		for(curNum=0;curNum<pointCount;curNum++){

			x = curBuffer[INDEX_TOUCH_XH]& 0x0f;
            x = (x<<8) | curBuffer[INDEX_TOUCH_XL];

			y = curBuffer[INDEX_TOUCH_YH]& 0x0f;
			y = (y<<8) | curBuffer[INDEX_TOUCH_YL];

			pointStatus =curBuffer[INDEX_TOUCH_XH] >>6;
			id =curBuffer[INDEX_TOUCH_YH] >>4;

			if(id==0xf){
				continue;
			}

			//printk("pointStatus=0x%x,id=%x\n",pointStatus,id);

			curBuffer +=TP_POINT_SIZE;

			if(pointStatus ==TP_DOWN || pointStatus ==TP_MOVE){
				pressure =TOOL_PRESSURE;
			}
			else if(pointStatus==TP_UP){
				pressure =0;
			}
			else
				continue;
			if(machine_is_birch())
			{
			x=x*800/500;
			y=y*1280/2040;
			}
//			temp=x;
//			x=y;
	//		y=abs(600-temp);
//			printk("x=%d,y=%d, pressure=%d\n",x,y,pressure);

			input_report_abs(data->input_dev, ABS_MT_POSITION_X,  x);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y,  y);
			
            input_report_abs(data->input_dev, ABS_MT_PRESSURE, pressure);

   			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, id);   
            input_mt_sync(data->input_dev);
			
		}

		input_sync(data->input_dev);

		if(realCount>0){
			//schedule();
			//goto again;
		}
        
    }
}



/***********************************************************************
    [function]: 
		           callback:            gather the finger information and calculate the X,Y
		                                   coordinate then report them to the input system;
    [parameters]:
                         null;
    [return]:
                         null;
************************************************************************/
int fts_read_data1(void)
{
    struct FTS_TS_DATA_T *data = i2c_get_clientdata(this_client);
    struct FTS_TS_EVENT_T *event = &data->event;
    u8 buf[32];
    static int key_id=0x80;
	
    int i,id,temp,i_count,ret = -1;
    int touch_point_num = 0, touch_event, x, y, pressure, size,x2,y2;
    REPORT_FINGER_INFO_T touch_info[CFG_MAX_POINT_NUM];

	TP_DATA pointData[CFG_MAX_POINT_NUM];
	int pointCount;

   
    i_count = 0;

    do 
    {
        buf[0] = 3;
        id = 0xe;  
		
        ret=fts_i2c_rxdata(buf, 12); 
		
	// printk("id=%d;point2-X-L=%d;\n",buf[2],buf[9]);
		//printk("buf[%d]=0x%x, buf[%d]=0x%x,buf[%d]=0x%x, buf[%d]=0x%x\n",10,buf[10],11,buf[11], 0,buf[0], 1, buf[1]);
        if (ret > 0)  
        {

			pointCount =buf[REG_TD_STATUS]&0x0F;
			if(pointCount>CFG_MAX_POINT_NUM){
				pointCount =CFG_MAX_POINT_NUM;
			}
			
            
            id = buf[2]>>4;
	   
	  
            touch_event = buf[0]>>6;     
            if (id >= 0 && id< CFG_MAX_POINT_NUM)  
            {
            
                temp = buf[0]& 0x0f;
                temp = temp<<8;
                temp = temp | buf[1];

                y= temp; 

                temp = (buf[2])& 0x0f;
                temp = temp<<8;
                temp = temp | buf[3];
			#if LINUX_FT5X0X_TS_XY	
				x= g_Screen_Max_x-temp; 
			#else
				x=y;
				y = temp; 
		  	#endif

		  		temp = buf[6]& 0x0f;
                temp = temp<<8;
                temp = temp | buf[7];
                y2= temp;  
				
                temp = (buf[8])& 0x0f;
                temp = temp<<8;
                temp = temp | buf[9];
		  	#if LINUX_FT5X0X_TS_XY	
		  		x2= g_Screen_Max_x-temp; 
		  	#else
		  		x2=y2;
		  		y2 = temp; 
		  	#endif	  
               // printk("x2=%d;y2=%d",x2,y2);
                
                pressure = TOOL_PRESSURE;//buf[4] & 0x3f; //
                size = buf[5]&0xf0;
                size = (id<<8)|size;
                touch_event = buf[0]>>6; 

				printk("touch_event=0x%x\n",touch_event);

                if (touch_event == 0)  //press down
                {
                	_st_finger_infos[0].u2_pressure=TOOL_PRESSURE;
		  			_st_finger_infos[1].u2_pressure=TOOL_PRESSURE;

                    _st_finger_infos[0].i2_x= (int16_t)x;
                    _st_finger_infos[0].i2_y= (int16_t)y;

		      		_st_finger_infos[1].i2_x= (int16_t)x2;
                    _st_finger_infos[1].i2_y= (int16_t)y2;

                    _st_finger_infos[id].ui2_id  =  size;
                    _si_touch_num ++;
                }
                else if (touch_event == 1) //up event
                {
    
                    _st_finger_infos[0].u2_pressure=0;
  		      		_st_finger_infos[1].u2_pressure=0; 
                }
				
                else if (touch_event == 2) //move
                {
                   _st_finger_infos[0].u2_pressure=TOOL_PRESSURE;
  		      		_st_finger_infos[1].u2_pressure=TOOL_PRESSURE;

                    _st_finger_infos[0].i2_x= (int16_t)x;
                    _st_finger_infos[0].i2_y= (int16_t)y;

		      		_st_finger_infos[1].i2_x= (int16_t)x2;
                    _st_finger_infos[1].i2_y= (int16_t)y2;
					
					
                    _st_finger_infos[id].ui2_id  = size; 
                    _si_touch_num ++;
                }
                else        
                    /*bad event, ignore*/
                     continue;  
               
                for( i= 0; i<CFG_MAX_POINT_NUM; ++i )   
                {
            		if(_st_finger_infos[i].i2_x <= 0 || _st_finger_infos[i].i2_y <= 0 ||_st_finger_infos[i].i2_y > g_Screen_Max_y
						||_st_finger_infos[i].i2_x > g_Screen_Max_x){
						continue;
            		}
                    
					printk("i=%d;x= %d;y=%d;",i,_st_finger_infos[i].i2_x,_st_finger_infos[i].i2_y); 
					printk("u2_pressure=%d;ui2_id=%d\n",_st_finger_infos[i].u2_pressure,_st_finger_infos[i].ui2_id); 
		
		
                    input_report_abs(data->input_dev, ABS_MT_POSITION_X,  _st_finger_infos[i].i2_x);
                    input_report_abs(data->input_dev, ABS_MT_POSITION_Y,  _st_finger_infos[i].i2_y);	
					
                    input_report_abs(data->input_dev, ABS_MT_PRESSURE, _st_finger_infos[i].u2_pressure);

		   			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, i+1);   
                    input_mt_sync(data->input_dev);

                    if(_st_finger_infos[i].u2_pressure == 0 )
                    {
                        _st_finger_infos[i].u2_pressure= -1;
                    }
                    
                }
                _st_finger_infos[0].i2_x= 0;
                _st_finger_infos[0].i2_y=0;

	      		_st_finger_infos[1].i2_x= 0;
                _st_finger_infos[1].i2_y= 0;
                

                input_sync(data->input_dev);

                if (_si_touch_num == 0 )
                {
                    fts_ts_release();
                }
                _si_touch_num = 0; 
		     }    
						  		
	           }	
		
	        else
	        {
	            printk("[TSP] ERROR: in %s, line %d, ret = %d\n",
	                __FUNCTION__, __LINE__, ret);
	        }

        i_count ++;
    }while( id != 0xf && i_count < 12);
 
	event->touch_point = touch_point_num;        
	if (event->touch_point == 0) 
	        return 1; 

	switch (event->touch_point) {
	    case 5:
	        event->x5           = touch_info[4].i2_x;
	        event->y5           = touch_info[4].i2_y;
	        event->pressure5 = touch_info[4].u2_pressure;
	    case 4:
	        event->x4          = touch_info[3].i2_x;
	        event->y4          = touch_info[3].i2_y;
	        event->pressure4= touch_info[3].u2_pressure;
	    case 3:
	        event->x3          = touch_info[2].i2_x;
	        event->y3          = touch_info[2].i2_y;
	        event->pressure3= touch_info[2].u2_pressure;
	    case 2:
	        event->x2          = touch_info[1].i2_x;
	        event->y2          = touch_info[1].i2_y;
	        event->pressure2= touch_info[1].u2_pressure;
	    case 1:
	        event->x1          = touch_info[0].i2_x;
	        event->y1          = touch_info[0].i2_y;
	        event->pressure1= touch_info[0].u2_pressure;
	        break;
	    default:
	        return -1;
	}
    
    return 0;
}



static void fts_work_func(struct work_struct *work)
{
    fts_read_data();    
}




static irqreturn_t fts_ts_irq(int irq, void *dev_id)
{
    struct FTS_TS_DATA_T *ft5x0x_ts = dev_id;
    if (!work_pending(&ft5x0x_ts->pen_event_work)) {
        queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
    }

    return IRQ_HANDLED;
}

static irqreturn_t fts_ts_irq2(int irq, void *dev_id)
{
     fts_read_data();

    return IRQ_HANDLED;
}



/***********************************************************************
[function]: 
                      callback:         send a command to ctpm.
[parameters]:
			  btcmd[in]:       command code;
			  btPara1[in]:     parameter 1;    
			  btPara2[in]:     parameter 2;    
			  btPara3[in]:     parameter 3;    
                      num[in]:         the valid input parameter numbers, 
                                           if only command code needed and no 
                                           parameters followed,then the num is 1;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
    u8 write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    return i2c_write_interface(write_cmd, num);
}




/***********************************************************************
[function]: 
                      callback:         write a byte data  to ctpm;
[parameters]:
			  buffer[in]:       write buffer;
			  length[in]:      the size of write data;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool byte_write(u8* buffer, int length)
{
    
    return i2c_write_interface(buffer, length);
}




/***********************************************************************
[function]: 
                      callback:         read a byte data  from ctpm;
[parameters]:
			  buffer[in]:       read buffer;
			  length[in]:      the size of read data;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool byte_read(u8* buffer, int length)
{
    return i2c_read_interface(buffer, length);
}





#define    FTS_PACKET_LENGTH        128

static unsigned char CTPM_FW[]=
{
//#include "ft_app.i"
};




/***********************************************************************
[function]: 
                        callback:          burn the FW to ctpm.
[parameters]:
			    pbt_buf[in]:     point to Head+FW ;
			    dw_lenth[in]:   the length of the FW + 6(the Head length);    
[return]:
			    ERR_OK:          no error;
			    ERR_MODE:      fail to switch to UPDATE mode;
			    ERR_READID:   read id fail;
			    ERR_ERASE:     erase chip fail;
			    ERR_STATUS:   status error;
			    ERR_ECC:        ecc error.
************************************************************************/
E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(u8* pbt_buf, int dw_lenth)
{
    u8  cmd,reg_val[2] = {0};
    u8  packet_buf[FTS_PACKET_LENGTH + 6];
    u8  auc_i2c_write_buf[10];
    u8  bt_ecc;
	
    int  j,temp,lenght,i_ret,packet_number, i = 0;
    int  i_is_new_protocol = 0;
	

    /******write 0xaa to register 0xfc******/
    cmd=0xaa;
    fts_register_write(0xfc,&cmd);
    mdelay(50);
	
     /******write 0x55 to register 0xfc******/
    cmd=0x55;
    fts_register_write(0xfc,&cmd);
    printk("[TSP] Step 1: Reset CTPM test\n");
   
    mdelay(10);   


    /*******Step 2:Enter upgrade mode ****/
    printk("\n[TSP] Step 2:enter new update mode\n");
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i ++;
        i_ret = fts_i2c_txdata(auc_i2c_write_buf, 2);
        mdelay(5);
    }while(i_ret <= 0 && i < 10 );

    if (i > 1)
    {
        i_is_new_protocol = 1;
    }

    /********Step 3:check READ-ID********/        
    cmd_write(0x90,0x00,0x00,0x00,4);
    byte_read(reg_val,2);
    if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
    {
        printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }
    else
    {
        return ERR_READID;
        //i_is_new_protocol = 1;
    }
    

     /*********Step 4:erase app**********/
    if (i_is_new_protocol)
    {
        cmd_write(0x61,0x00,0x00,0x00,1);
    }
    else
    {
        cmd_write(0x60,0x00,0x00,0x00,1);
    }
    mdelay(1500);
    printk("[TSP] Step 4: erase. \n");



    /*Step 5:write firmware(FW) to ctpm flash*/
    bt_ecc = 0;
    printk("[TSP] Step 5: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
        mdelay(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("[TSP] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(&packet_buf[0],temp+6);    
        mdelay(20);
    }

    /***********send the last six byte**********/
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(&packet_buf[0],7);  
        mdelay(20);
    }

    /********send the opration head************/
    cmd_write(0xcc,0x00,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[TSP] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
        return ERR_ECC;
    }

    /*******Step 7: reset the new FW**********/
    cmd_write(0x07,0x00,0x00,0x00,1);

    return ERR_OK;
}




int fts_ctpm_fw_upgrade_with_i_file(void)
{
   u8*     pbt_buf = FTS_NULL;
   int i_ret;
    
   pbt_buf = CTPM_FW;
   i_ret =  fts_ctpm_fw_upgrade(pbt_buf,sizeof(CTPM_FW));
   
   return i_ret;
}

unsigned char fts_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;
	
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[ui_sz - 2];
    }
    else
        return 0xff; 
 
}


static int fts_init_panel()
{
	if(machine_is_nabi2_3d()|| machine_is_nabi_2s() ||machine_is_qc750() ||  machine_is_n710() ||  machine_is_itq700() || machine_is_itq701() || machine_is_n750() || machine_is_wikipad() || machine_is_cybtt10_bk()||machine_is_birch())
	{
		g_Screen_Max_x = SCREEN_MAX_WIDTH_3D;
		g_Screen_Max_y = SCREEN_MAX_HEIGHT_3D;
	} 
	else if(machine_is_nabi2_xd() || machine_is_n1010()|| machine_is_n1020()||machine_is_itq1000()||
			machine_is_n1011() || machine_is_n1050())
	{
		g_Screen_Max_x = SCREEN_MAX_WIDTH_XD;
		g_Screen_Max_y = SCREEN_MAX_HEIGHT_XD;
	} 
	else {
	    g_Screen_Max_x = SCREEN_MAX_WIDTH;
       	g_Screen_Max_y = SCREEN_MAX_HEIGHT;
	}
	
	return 0;
}

static int fts_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{    
   
    struct FTS_TS_DATA_T *ft5x0x_ts;
    struct input_dev *input_dev; 
   ft_pdata = client->dev.platform_data;          //wantianpei add
    int err = 0;
    unsigned char reg_value;
    unsigned char reg_version;
    int i;


    _sui_irq_num =client->irq;   // IRQ_EINT(6);         //wantianpei add
    gpio_direction_output(ft_pdata->gpio_shutdown, 1);
    gpio_direction_output(ft_pdata->gpio_shutdown, 1); 
    msleep(15);


    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        err = -ENODEV;
        goto exit_check_functionality_failed;
    }

    ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
    if (!ft5x0x_ts)    {
        err = -ENOMEM;
        goto exit_alloc_data_failed;
    }

    this_client = client;
    i2c_set_clientdata(client, ft5x0x_ts);

    INIT_WORK(&ft5x0x_ts->pen_event_work, fts_work_func);

    ft5x0x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
    if (!ft5x0x_ts->ts_workqueue) {
        err = -ESRCH;
        goto exit_create_singlethread;
    }

	
   /***wait CTP to bootup normally***/
    msleep(200); 
    
    fts_register_read(FT5X0X_REG_FIRMID, &reg_version,1);
    printk("[TSP] firmware version = 0x%2x\n", reg_version);
    fts_register_read(FT5X0X_REG_REPORT_RATE, &reg_value,1);
    printk("[TSP]firmware report rate = %dHz\n", reg_value*10);
    fts_register_read(FT5X0X_REG_THRES, &reg_value,1);
    printk("[TSP]firmware threshold = %d\n", reg_value * 4);
    fts_register_read(FT5X0X_REG_NOISE_MODE, &reg_value,1);
    printk("[TSP]nosie mode = 0x%2x\n", reg_value);

#ifdef CONFIG_HAS_EARLYSUSPEND
    printk("\n [TSP]:register the early suspend \n");
    ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ft5x0x_ts->early_suspend.suspend = fts_ts_suspend;
    ft5x0x_ts->early_suspend.resume	 = fts_ts_resume;
    register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

  #if 0
    if (fts_ctpm_get_upg_ver() != reg_version)  
    {
        printk("[TSP] start upgrade new verison 0x%2x\n", fts_ctpm_get_upg_ver());
        msleep(200);
        err =  fts_ctpm_fw_upgrade_with_i_file();
        if (err == 0)
        {
            printk("[TSP] ugrade successfuly.\n");
            msleep(300);
            fts_register_read(FT5X0X_REG_FIRMID, &reg_value,1);
            printk("FTS_DBG from old version 0x%2x to new version = 0x%2x\n", reg_version, reg_value);
        }
        else
        {
            printk("[TSP]  ugrade fail err=%d, line = %d.\n",
                err, __LINE__);
        }
        msleep(4000);
    }
  #endif

     //err=request_threaded_irq(_sui_irq_num,NULL, fts_ts_irq, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_LOW, "qt602240_ts", ft5x0x_ts);

#if 1
  
    err = request_irq(_sui_irq_num, fts_ts_irq, IRQF_TRIGGER_FALLING, "qt602240_ts", ft5x0x_ts);

    if (err < 0) {
        dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
        goto exit_irq_request_failed;
    }
#else
/* get the irq */
	err = request_threaded_irq(_sui_irq_num, NULL, fts_ts_irq2,
				   IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
				   "1qt602240_ts", ft5x0x_ts);
	if (err) {
		dev_err(&client->dev, "%s: request_irq(%d) failed\n",
			__func__, _sui_irq_num);
		gpio_free(_sui_irq_num);
		goto exit_irq_request_failed;
	}
#endif
	
    disable_irq(_sui_irq_num);

    input_dev = input_allocate_device();
    if (!input_dev) {
        err = -ENOMEM;
        dev_err(&client->dev, "failed to allocate input device\n");
        goto exit_input_dev_alloc_failed;
    }
    
    ft5x0x_ts->input_dev = input_dev;

   fts_init_panel();
    /***setup coordinate area******/
    set_bit(EV_SYN, ft5x0x_ts->input_dev->evbit);
    set_bit(EV_ABS, ft5x0x_ts->input_dev->evbit);
  //  set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
    set_bit(ABS_MT_POSITION_X, ft5x0x_ts->input_dev->keybit);
    set_bit(ABS_MT_POSITION_Y, ft5x0x_ts->input_dev->keybit);
 //   set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	
    /****** for multi-touch *******/
    for (i=0; i<CFG_MAX_POINT_NUM; i++)   
        _st_finger_infos[i].u2_pressure = -1;
	
 	input_set_abs_params(ft5x0x_ts->input_dev, ABS_X, 0, g_Screen_Max_x - 1, 0, 0);
	input_set_abs_params(ft5x0x_ts->input_dev, ABS_Y, 0, g_Screen_Max_y - 1, 0, 0);
	
     input_set_abs_params(ft5x0x_ts->input_dev, ABS_MT_POSITION_X, 0, g_Screen_Max_x - 1, 0, 0);
    input_set_abs_params(ft5x0x_ts->input_dev, ABS_MT_POSITION_Y, 0, g_Screen_Max_y - 1, 0, 0);
	
    input_set_abs_params(ft5x0x_ts->input_dev, ABS_MT_PRESSURE, 0,TOOL_PRESSURE, 0, 0);	
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 0xe, 0, 0);
/*    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 30, 0, 0);
   

  
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(BTN_TOUCH, input_dev->keybit) ;
    */

    ft5x0x_ts->input_dev->keycode = tsp_keycodes;
    for(i = 0; i < CFG_NUMOFKEYS; i++)
    {
        input_set_capability(ft5x0x_ts->input_dev, EV_KEY, ((int*)input_dev->keycode)[i]);
        tsp_keystatus[i] = KEY_RELEASE;
    }

    ft5x0x_ts->input_dev->name        = FT5X0X_NAME;
    err = input_register_device(input_dev);
    if (err) {
        dev_err(&client->dev,
        "fts_ts_probe: failed to register input device: %s\n",
        dev_name(&client->dev));
        goto exit_input_register_device_failed;
    } 
	
       gpio_set_value(ft_pdata->gpio_shutdown,0); //wantianpei add   
          msleep(15);
	gpio_set_value(ft_pdata->gpio_shutdown,1);
	 
    enable_irq(_sui_irq_num);   
	
    printk("[TSP] file(%s), function (%s), -- end\n", __FILE__, __FUNCTION__);
    return 0;

exit_input_register_device_failed:
    input_free_device(input_dev);
exit_input_dev_alloc_failed:
    free_irq(_sui_irq_num, ft5x0x_ts);
exit_irq_request_failed:
    cancel_work_sync(&ft5x0x_ts->pen_event_work);
    destroy_workqueue(ft5x0x_ts->ts_workqueue);
exit_create_singlethread:
    printk("[TSP] ==singlethread error =\n");  
    i2c_set_clientdata(client, NULL);
    kfree(ft5x0x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed: 
    return err;
}



static int __devexit fts_ts_remove(struct i2c_client *client)
{
    struct FTS_TS_DATA_T *ft5x0x_ts;
    
    ft5x0x_ts = (struct FTS_TS_DATA_T *)i2c_get_clientdata(client);
    free_irq(_sui_irq_num, ft5x0x_ts);
    input_unregister_device(ft5x0x_ts->input_dev);
    kfree(ft5x0x_ts);
    cancel_work_sync(&ft5x0x_ts->pen_event_work);
    destroy_workqueue(ft5x0x_ts->ts_workqueue);
    i2c_set_clientdata(client, NULL);
    return 0;
}



static const struct i2c_device_id ft5x0x_ts_id[] = {
    { FT5X0X_NAME, 0 },{ }
};


MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static struct i2c_driver fts_ts_driver = {
    .probe        = fts_ts_probe,
    .remove        = __devexit_p(fts_ts_remove),
    .id_table    = ft5x0x_ts_id,
    .driver    = {
        .name = FT5X0X_NAME,
    },
};

static int __init fts_ts_init(void)
{
    return i2c_add_driver(&fts_ts_driver);
}


static void __exit fts_ts_exit(void)
{
    i2c_del_driver(&fts_ts_driver);
}



module_init(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_AUTHOR("<duxx@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
