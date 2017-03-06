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
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/i2c/leds-tlc59116.h>
#include<linux/i2c/leds-tlc59116_data.h>

struct TLC59116_i2c_ts_platform_data *pdata_reset;
struct i2c_client * i2c_tlc59116_client = NULL; 
#if 0
static struct bq24160_charger *charger;
typedef void (*callback_t)(enum usb_otg_state to,
		enum usb_otg_state from, void *args);
extern int register_otg_callback(callback_t cb, void *args);
static struct bq_info
{
	 struct timer_list	bq24160_timer; 
	 struct i2c_client *this_client;
	 struct power_supply	ac;
	 struct power_supply	usb;
	 struct workqueue_struct *bq24160_wq;
	 struct work_struct  work;
};

struct bq_info  bq24160_info;
enum bq24160_chip { bq24160, bq24161 }; 

static int bq24160_write_reg8(struct i2c_client *client, u8 addr, u8 val) 
{
		int err;
		struct i2c_msg msg;
		unsigned char data[2];
		

		if (!client->adapter)
			return -ENODEV;
		
		data[0] = (u8) addr;	
		data[1] = (u8) val;
		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 2;
		msg.buf = data; 


		err = i2c_transfer(client->adapter, &msg, 1);
	      if(err<=0)  printk("bq24160  i2c write erro!");

		return err;
}

static unsigned char  bq24160_read_reg8(struct i2c_client *client, u8 addr,int*rt_value)
{
		int err;
		
		struct i2c_msg msg[1];
		unsigned char data[2];

		if (!client->adapter)
			return -ENODEV;
		
		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 1;
		msg->buf = data;
		data[0] = addr;
	       err = i2c_transfer(client->adapter, msg, 1);
		 if(err<=0)  printk("bq24160  i2c read erro!");
			
	       msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);

	     //   printk("bq24160_read_reg8 err=%d,data[0]=%d\n," ,err,data[0] );
	        *rt_value=data[0];

		return err;
}

static int bq24160_i2c_init(struct i2c_client *client)
{
		int ret=0;

//		bq24160_write_reg8(client, BQ24160_REG_CONTROL, 0x80); //reset disable
//		bq24160_write_reg8(client, BQ24160_REG_SAFETY_TIMER, 0xc8); //satety timer	

		bq24160_write_reg8(client, BQ24160_REG_BATTERY_VOLTAGE, pdata->bq24160_reg3); // set charge regulation voltage 4.2v
		bq24160_write_reg8(client, BQ24160_REG_TERMINATION_FAST_CHARGE, pdata->bq24160_reg5);//set changer current 0.1A , 1.2A

		bq24160_write_reg8(client, BQ24160_REG_VOLTAGE_DPPM, 0x2B); //vin dppm votage4.44v   v-usb dppm=4.6
		
		bq24160_write_reg8(client, BQ24160_REG_CONTROL, 0x5c); //change enable ,i usb limit=1.5A, 

		return ret;
}
static void bq24160_work_func(struct work_struct *work)
{
		int ret=0;
		int*rt_value;



#if WATCHDOG_TIMER	  
			mod_timer(&bq24160_info.bq24160_timer,jiffies + msecs_to_jiffies(10000));	


			bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_STATUS_CONTROL, 0x80);
#endif



		bq24160_i2c_init(bq24160_info.this_client);
		power_supply_changed(&bq24160_info.ac);
		
err1:
		return ret;


}

#if WATCHDOG_TIMER
static void bq24160_timer_function(unsigned long data)
{
	  

		queue_work(bq24160_info.bq24160_wq, &bq24160_info.work);
		printk("bq24160_timer_function\n");
}
#endif

static irqreturn_t bq24160_charger_irq(int irq, void *dev_id)
{
		queue_work(bq24160_info.bq24160_wq, &bq24160_info.work);

		
		printk("bq24160_charger_irq\n");
		    return IRQ_HANDLED;
}

static enum power_supply_property bq24160_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,

};

static int get_online_state()
{	
		int ret;
		int*rt_value;

		rt_value = kzalloc(sizeof(*rt_value), GFP_KERNEL);
			if (!rt_value) {
				printk("failed to allocate device info data\n");
				ret = -ENOMEM;
				goto err1;

			}
		ret=bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_STATUS_CONTROL,rt_value);
		*rt_value=*rt_value>>4;
		if(*rt_value==0x04){

			ret=1; //usb in
			}
		else if(*rt_value==0x03){
			ret=1; //DC in
			}
		else{
			ret=0;
			}
		
		
err1:		return ret;

}

static int bq24160_bci_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
		//struct power_supply *bci = dev_get_drvdata(psy->dev->parent);


		switch (psp) {
			
		case POWER_SUPPLY_PROP_ONLINE:
			//printk("POWER_SUPPLY_PROP_ONLINE\n");

			val->intval=get_online_state();		
			break;

			
		default:
			return -EINVAL;
		}

		return 0;
}



static int check_i2c()
{
	int ret;
	int*rt_value;

	rt_value = kzalloc(sizeof(*rt_value), GFP_KERNEL);
		if (!rt_value) {
			printk("failed to allocate device info data\n");
			ret = -ENOMEM;
		goto erro1;

		}
	ret=bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,rt_value);
	if(ret<0)
		{
		goto erro2;
		}
	
	if(*rt_value<0x80)
		{
		ret=-1;
		goto erro2;
		}

	
	return ret;
erro2:
	   kfree(rt_value);
erro1:
	return ret;
}

static int bq24160_charger_remove(struct i2c_client *client)
{
	int ret=0;
	del_timer_sync(&bq24160_info.bq24160_timer);
	power_supply_unregister(&bq24160_info.ac);
	free_irq(client->irq,NULL);
	return ret;
}
static void charger_otg_status(enum usb_otg_state to, enum usb_otg_state from, void *data)
{
	struct i2c_client *client=((struct i2c_client *)data);
		printk("charger_otg_status 1");
	if ((from == OTG_STATE_A_SUSPEND) && (to == OTG_STATE_A_HOST)) {//otg in
	       bq24160_write_reg8(client, BQ24160_REG_SUPPLY_STATUS, 0x08); //OTG supply present. Lockout USB input for charging. 
		printk("charger_otg_status 2");

	} else //if ((from == OTG_STATE_A_HOST) && (to == OTG_STATE_A_SUSPEND))
	{//otg out
	       bq24160_write_reg8(client, BQ24160_REG_SUPPLY_STATUS, 0x00); 
		printk("charger_otg_status 3");

	}
}

static int bq24160_enable_otg(struct regulator_dev *otg_rdev)
{
	int ret;
     
 	printk("bq24160_enable_otg\n");
       bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SUPPLY_STATUS, 0x08); //OTG supply present. Lockout USB input for charging. 
       gpio_set_value(pdata->vbus_gpio,1);
	charger->is_otg_enabled = 1;

	return ret;
}

static int bq24160_disable_otg(struct regulator_dev *otg_rdev)
{
	int ret;
	printk("bq24160_disable_otg\n");
       gpio_set_value(pdata->vbus_gpio,0);

        bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SUPPLY_STATUS, 0x00); //otg out
	charger->is_otg_enabled = 0;
    
	return ret;
}

static int bq24160_is_otg_enabled(struct regulator_dev *otg_rdev)
{

	printk("bq24160_is_otg_enabled\n");
	return charger->is_otg_enabled;
	
}


static struct regulator_ops bq24160_tegra_otg_regulator_ops = {
	.enable = bq24160_enable_otg,
	.disable = bq24160_disable_otg,
	.is_enabled = bq24160_is_otg_enabled,
};
#endif
#if 1
int tlc59116_i2c_Read(struct i2c_client *client,  char * writebuf, int writelen,
                    char *readbuf, int readlen)
{
    int ret;

    if(writelen > 0)
    {
        struct i2c_msg msgs[] =
        {
            {
                .addr	= client->addr,
                .flags	= 0,
                .len	= writelen,
                .buf	= writebuf,
            },
            {
                .addr	= client->addr,
                .flags	= I2C_M_RD,
                .len	= readlen,
                .buf	= readbuf,
            },
        };
        ret = i2c_transfer(client->adapter, msgs, 2);
        if (ret < 0)
            pr_err("function:%s. i2c read error: %d\n", __func__, ret);
    }
    else
    {
        struct i2c_msg msgs[] =
        {
            {
                .addr	= client->addr,
                .flags	= I2C_M_RD,
                .len	= readlen,
                .buf	= readbuf,
            },
        };
        ret = i2c_transfer(client->adapter, msgs, 1);
        if (ret < 0)
            pr_err("function:%s. i2c read error: %d\n", __func__, ret);
    }
    return ret;
}
/*
*write data by i2c
*/
int tlc59116_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret;

    struct i2c_msg msg[] =
    {
        {
            .addr	= client->addr,
            .flags	= 0,
            .len	= writelen,
            .buf	= writebuf,
        },
    };

    ret = i2c_transfer(client->adapter, msg, 1);
    if (ret < 0)
        pr_err("%s i2c write error: %d\n", __func__, ret);

    return ret;
}




int tlc59116_write_reg(struct i2c_client * client, u8 regaddr, u8 regvalue)
{
    unsigned char buf[2] = {0};
    buf[0] = regaddr;
    buf[1] = regvalue;

    return tlc59116_i2c_Write(client, buf, sizeof(buf));
}

int tlc59116_read_reg(struct i2c_client * client, u8 regaddr, u8 * regvalue)
{
    return tlc59116_i2c_Read(client, &regaddr, 1, regvalue, 1);
}

#endif

static int tlc56116_mode_init(struct i2c_client *client)
{

     unsigned char TLC59116;
     unsigned char TLC59116_M;
     int i;
     unsigned char TLC_address;

    //tlc59116_read_reg(TLC59116_MODE1, &TLC_MODE,1);
   // printk("[LEDS]TLC_MODE = 0x%2x\n", TLC_MODE);
       //  tlc59116_read_reg(TLC59116_ALLCALLADR, &TLC_address,1);
    //printk("[LEDS] TLC_address = 0x%2x\n", TLC_address);
	//tlc59116_write_reg(client,TLC59116_MODE1,0x00);
//for(i=0;i<31;i++)
//{
	//tlc59116_read_reg(client,TLC59116_MODE1+i,&TLC59116);
	//printk("reg %d = 0x%2x\n",i,TLC59116);

	
//}
	//tlc59116_read_reg(client,TLC59116_MODE1,&TLC59116_M);
	//printk("TLC59116_MODE1  = 0x%2x\n",TLC59116_M);
//	printk("tlc56116_mode_init\n");
	
    tlc59116_write_reg(client,TLC59116_MODE1,0x01);
    
    tlc59116_write_reg(client,TLC59116_MODE2,0x00);
    
	//tlc59116_read_reg(client,TLC59116_MODE2,&TLC59116_M);

//	printk("TLC59116_MODE2  = 0x%2x\n",TLC59116_M);
        
	//tlc59116_write_reg(client,TLC59116_LEDOUT0,0xff);
	
	//tlc59116_read_reg(client,TLC59116_LEDOUT0,&TLC59116);

	//
	//printk("TLC59116_LEDOUT0  = 0x%2x\n",TLC59116);
	
	//tlc59116_write_reg(client,TLC59116_LEDOUT1,0x0f);
	//tlc59116_write_reg(client,TLC59116_LEDOUT2,0x0f);
	tlc59116_write_reg(client,TLC59116_LEDOUT0,0xff);
	//tlc59116_read_reg(client,TLC59116_LEDOUT0,&TLC59116);
	//printk("TLC59116_LEDOUT0  = 0x%2x\n",TLC59116);	
	tlc59116_write_reg(client,TLC59116_LEDOUT1,0x03);
	
	//tlc59116_read_reg(client,TLC59116_LEDOUT1,&TLC59116);
	//printk("TLC59116_LEDOUT1  = 0x%2x\n",TLC59116);	
	tlc59116_write_reg(client,TLC59116_LEDOUT2,0x00);
	
	//tlc59116_read_reg(client,TLC59116_LEDOUT2,&TLC59116);
	//printk("TLC59116_LEDOUT2  = 0x%2x\n",TLC59116);
	
	tlc59116_write_reg(client,TLC59116_LEDOUT3,0x3f);
//	tlc59116_read_reg(client,TLC59116_LEDOUT3,&TLC59116);
//	printk("TLC59116_LEDOUT3  = 0x%2x\n",TLC59116);

	tlc59116_write_reg(client,TLC59116_PWM0,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM1,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM2,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM3,0x08);
	tlc59116_write_reg(client,TLC59116_PWM4,0xB2);
	//tlc59116_write_reg(client,TLC59116_PWM5,0xB2);
	//tlc59116_write_reg(client,TLC59116_PWM6,0xB2);
//	tlc59116_write_reg(client,TLC59116_PWM7,0xB2);
	//tlc59116_write_reg(client,TLC59116_PWM8,0xB2);		
//	tlc59116_write_reg(client,TLC59116_PWM9,0xB2);
//	tlc59116_write_reg(client,TLC59116_PWM10,0xB2);
//	tlc59116_write_reg(client,TLC59116_PWM11,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM12,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM13,0xB2);
	tlc59116_write_reg(client,TLC59116_PWM14,0xB2);
//	tlc59116_write_reg(client,TLC59116_PWM15,0xB2);
		



     
}
static void tlc_leds_work_func(struct work_struct *work)
{
	struct tlc59116_device_date *tlc = container_of(to_delayed_work(work), struct tlc59116_device_date, leds_work);
	printk("tlc_leds_work_func\n");
  tlc59116_write_reg(i2c_tlc59116_client,TLC59116_LEDOUT0,0xff);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_LEDOUT1,0xff);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_LEDOUT2,0xff);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_LEDOUT3,0xff);
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void tlc59116_early_suspend(struct early_suspend *h)
{
	//struct novatek_ts_data *ts;
	//ts = container_of(h, struct novatek_ts_data, early_suspend);
	//novatek_ts_suspend(ts->client, PMSG_SUSPEND);
     unsigned char TLC59116;
	// int i;

 // for(i=0;i<2;i++)
//{
	//tlc59116_read_reg(i2c_tlc59116_client,TLC59116_MODE1+i,&TLC59116);
//	printk("reg %d = 0x%2x\n",i,TLC59116);

	
//}  
//	tlc56116_mode_init(i2c_tlc59116_client);


    //gpio_direction_output(pdata_reset->reset, 1);
	gpio_direction_output(pdata_reset->reset, 0);
    msleep(4);
    gpio_direction_output(pdata_reset->reset, 1);
    
    tlc59116_write_reg(i2c_tlc59116_client,TLC59116_MODE1,0x01);
    
    tlc59116_write_reg(i2c_tlc59116_client,TLC59116_MODE2,0x00);    
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_LEDOUT3,0xff);
	tlc59116_read_reg(i2c_tlc59116_client,TLC59116_LEDOUT3,&TLC59116);
	printk("reg = 0x%2x\n",TLC59116);
	
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_PWM12,0x00);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_PWM13,0xB2);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_PWM14,0xB2);
	tlc59116_write_reg(i2c_tlc59116_client,TLC59116_PWM15,0x00);
	
}

static void tlc59116_ts_late_resume(struct early_suspend *h)
{
	gpio_direction_output(pdata_reset->reset, 0);
    msleep(4);
    gpio_direction_output(pdata_reset->reset, 1);
	tlc56116_mode_init(i2c_tlc59116_client);
}
#endif

static int tlc59116_control_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{	
        struct tlc59116_device_date *tlc = NULL; 
	 	int ret=0;
	 		printk("tlc59116_control_probe\n");

 	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		printk( "Must have I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	tlc = kzalloc(sizeof(*tlc), GFP_KERNEL);
	if (tlc == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	} 
    
	pdata_reset =client->dev.platform_data;
	//gpio_direction_output(pdata_reset->reset, 0);
   // msleep(4);
   // gpio_direction_output(pdata_reset->reset, 1);


    //pdata->reset;reset pin 
    i2c_tlc59116_client =client;
	//INIT_WORK(&tlc->leds_work,tlc_leds_work_func); 

	//	hrtimer_init(&tlc->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		//tlc->timer.function = tlc_leds_work_func;
		//hrtimer_start(&tlc->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

   // tlc56116_mode_init(i2c_tlc59116_client);
   // gpio_direction_output(pdata_reset->reset, 0);
    //msleep(1);
   // gpio_direction_output(pdata_reset->reset, 1);


#ifdef CONFIG_HAS_EARLYSUSPEND
	tlc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	tlc->early_suspend.suspend = tlc59116_early_suspend;
	tlc->early_suspend.resume = tlc59116_ts_late_resume;
	register_early_suspend(&tlc->early_suspend);
#endif

/*#ifdef CONFIG_HAS_EARLYSUSPEND
    printk("\n [TSP]:register the early suspend \n");
    ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ft5x0x_ts->early_suspend.suspend = fts_ts_suspend;
    ft5x0x_ts->early_suspend.resume	 = fts_ts_resume;
    register_early_suspend(&ft5x0x_ts->early_suspend);
#endif
*/
err_check_functionality_failed:
err_alloc_data_failed:
	//hrtimer_cancel(&tlc->timer);
     return ret;


}
#if 0

static int bq24160_charger_suspend(struct i2c_client *client)
{  
#if WATCHDOG_TIMER 
    del_timer_sync(&bq24160_info.bq24160_timer);
#endif
bq24160_write_reg8(client, BQ24160_REG_TERMINATION_FAST_CHARGE, pdata->bq24160_reg5_susp);//set changer current 0.1A , 1.2A
	return 0;
}
static int bq24160_cahrger_resume(struct i2c_client *client)
{
#if WATCHDOG_TIMER
	setup_timer(&bq24160_info.bq24160_timer,bq24160_timer_function, 0);
#endif	

              bq24160_i2c_init(client);

	return 0;
}

#endif
static const struct i2c_device_id tlc59116_id[] = {
	{ "tlc59116", 0 },	
	{},
};

static struct i2c_driver tlc59116_driver = {

	.probe = tlc59116_control_probe,
	//.suspend		= bq24160_charger_suspend,	
	//.resume         = bq24160_cahrger_resume,
	//.remove = bq24160_charger_remove,
	.id_table = tlc59116_id,
	.driver = {
		.name	= "tlc59116",
		.owner = THIS_MODULE,
	},
};

static int __init tlc59116_init(void)
{
	int ret;
printk("tlc59116_init \n");
  ret = i2c_add_driver(&tlc59116_driver);
	
	return ret;
}


static void __exit tlc59116_exit(void)
{ 
	i2c_del_driver(&tlc59116_driver); 
    //hrtimer_cancel(&tlc->timer);

}
module_init(tlc59116_init);
module_exit(tlc59116_exit);
MODULE_AUTHOR("lee.liu@keenhi.com");
MODULE_DESCRIPTION("tlc59116_control driver");
MODULE_LICENSE("GPL");






