#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>   
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/err.h>

#include <linux/usb/otg.h>
#include <linux/simple_otg_vbus.h>
#include <linux/bq24160_charger.h>
#include <asm/mach-types.h>


#define BQ24160_REG_STATUS_CONTROL					0x00
#define BQ24160_REG_SUPPLY_STATUS					0x01
#define BQ24160_REG_CONTROL							0x02
#define BQ24160_REG_BATTERY_VOLTAGE					0x03
#define BQ24160_REG_VENDER_REVISION					0x04
#define BQ24160_REG_TERMINATION_FAST_CHARGE		0x05
#define BQ24160_REG_VOLTAGE_DPPM						0x06
#define BQ24160_REG_SAFETY_TIMER						0x07
#define    WATCHDOG_TIMER            						 1

#define BT_POLL_TIME												10000

static bool mCharge=1;
static bool mCharge_system_temperature=1;
struct bq24160_charger_platform_data *pdata;
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
	 struct delayed_work  work;
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
	if(err<=0) 
	 	printk("bq24160  i2c read erro!");
		
	msg->flags = I2C_M_RD;
	err = i2c_transfer(client->adapter, msg, 1);

	//   printk("bq24160_read_reg8 err=%d,data[0]=%d\n," ,err,data[0] );
	*rt_value=data[0];

	return err;
}


void disable_led_state(void) 
{
if(mCharge==0)
		return;
	if(!machine_is_nabi2()){
	  bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL, 0x54);
	}

} 

void enalbe_led_state(void) 
{
if(mCharge==0)
	return;
	if(!machine_is_nabi2()){
  		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL, 0x5c);
		}

} 

static int bq24160_i2c_init(struct i2c_client *client)
{
		int ret=0;
if(mCharge==0)
	return ret;
		bq24160_write_reg8(client, BQ24160_REG_BATTERY_VOLTAGE, pdata->bq24160_reg3); // set charge regulation voltage 4.2v
		bq24160_write_reg8(client, BQ24160_REG_TERMINATION_FAST_CHARGE, pdata->bq24160_reg5);//set changer current 0.1A , 1.2A
		
		bq24160_write_reg8(client, BQ24160_REG_VOLTAGE_DPPM, 0x2B); //vin dppm votage4.44v   v-usb dppm=4.6
		bq24160_write_reg8(client, BQ24160_REG_CONTROL, 0x5c); //change enable ,i usb limit=1.5A, 
		return ret;
}
static void bq24160_work_func(struct work_struct *work)
{
#if WATCHDOG_TIMER	  
		//mod_timer(&bq24160_info.bq24160_timer,jiffies + msecs_to_jiffies(10000));	
	   //bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_STATUS_CONTROL, 0x80);
#endif
		bq24160_i2c_init(bq24160_info.this_client);
		power_supply_changed(&bq24160_info.ac);

		if(bq24160_info.this_client->irq)
			enable_irq(bq24160_info.this_client->irq);

}

#if WATCHDOG_TIMER
static void bq24160_timer_function(unsigned long data)
{
		//queue_work(bq24160_info.bq24160_wq, &bq24160_info.work);
		printk("bq24160_timer_function poll\n");
		power_supply_changed(&bq24160_info.ac);
}
#endif

static irqreturn_t bq24160_charger_irq(int irq, void *dev_id)
{
	disable_irq_nosync(bq24160_info.this_client->irq);
	cancel_delayed_work_sync(&bq24160_info.work);
	queue_delayed_work(bq24160_info.bq24160_wq, &bq24160_info.work,msecs_to_jiffies(200));		
	printk("bq24160_charger_irq\n");
	return IRQ_HANDLED;
}

static enum power_supply_property bq24160_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int get_online_state()
{	
		int ret;
		int rt_value;

		if(!machine_is_nabi2()){
			ret=bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_STATUS_CONTROL,&rt_value);
			if(ret<0)
				return 0;
		}
		printk("get_online_state=0x%x\n",rt_value);
		
		rt_value=((rt_value>>4)&0x7);
		if(rt_value==0||rt_value==0x7 ||rt_value==0x6 ||rt_value==0x1 ){
			return 0;
		}
		else{
			return 1;
		}
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
	int rt_value;

	ret=bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
	if(ret<0)
	{
		goto erro2;
	}
	
	if(rt_value<0x80)
	{
		ret=-1;
		goto erro2;
	}
erro2:
	return ret;
}

static int bq24160_charger_remove(struct i2c_client *client)
{
	int ret=0;
	del_timer_sync(&bq24160_info.bq24160_timer);
	power_supply_unregister(&bq24160_info.ac);
	disable_irq_wake(client->irq);
	free_irq(client->irq,NULL);
	return ret;
}
static void charger_otg_status(enum usb_otg_state to, enum usb_otg_state from, void *data)
{
	if(!machine_is_nabi2()){
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
}

static int bq24160_enable_otg(struct regulator_dev *otg_rdev)
{
	int ret;
     if(!machine_is_nabi2()){
 		printk("bq24160_enable_otg\n");
       bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SUPPLY_STATUS, 0x08); //OTG supply present. Lockout USB input for charging. 
       mdelay(50);
       gpio_set_value(pdata->vbus_gpio,1);
		charger->is_otg_enabled = 1;
	}
	return ret;
}

static int bq24160_disable_otg(struct regulator_dev *otg_rdev)
{
	int ret;
	if(!machine_is_nabi2()){
		printk("bq24160_disable_otg\n");
       gpio_set_value(pdata->vbus_gpio,0);
		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SUPPLY_STATUS, 0x00); //otg out
		charger->is_otg_enabled = 0;
	}
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

void enable_charge()
{
	int rt_value;
    printk("enable_charge!\n");
	mCharge=1;
	mCharge_system_temperature=1;
	bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
	printk("enable_charge_REG_CONTROL_state=0x%x\n",rt_value);
	enable_irq(bq24160_info.this_client->irq);
	bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SAFETY_TIMER, 0xc8); //satety timer	
    bq24160_i2c_init(bq24160_info.this_client);
		setup_timer(&bq24160_info.bq24160_timer,bq24160_timer_function, 0);
	mod_timer(&bq24160_info.bq24160_timer,jiffies + msecs_to_jiffies(BT_POLL_TIME));
	bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
	printk("enable_charge_REG_CONTROL_state_eable_charge=0x%x\n",rt_value);
}
void disable_battery_charge()
{
   	int rt_value;
    printk("disable_charge!\n");
	mCharge=0;
	disable_irq(bq24160_info.this_client->irq);
	del_timer_sync(&bq24160_info.bq24160_timer);
	//cancel_delayed_work_sync(&bq24160_info.work);
	bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL, 0x5e);
	bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
	printk("disable_charge_REG_CONTROL_state=0x%x\n",rt_value);
}

void disable_charge()
{
   	int rt_value;
    printk("disable_charge!\n");
	mCharge=0;
	disable_irq(bq24160_info.this_client->irq);
	del_timer_sync(&bq24160_info.bq24160_timer);
	//cancel_delayed_work_sync(&bq24160_info.work);
	bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL, 0x80);
	bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
	printk("disable_charge_REG_CONTROL_state=0x%x\n",rt_value);
}

void disable_charge_system_temperature()

{
    printk("disable_charge_system_temperature!\n");
	mCharge=0;
	mCharge_system_temperature=0;;
	bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SAFETY_TIMER, 0xee);//Disable safety timers
	bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_TERMINATION_FAST_CHARGE, 0x02);//set changer current 550mA
//	bq24160_read_reg8(bq24160_info.this_client, BQ24160_REG_CONTROL,&rt_value);
//	printk("disable_charge_REG_CONTROL_state=0x%x\n",rt_value);
}
void enable_charge_system_temperature()
{
		mCharge=1;
		printk("enable_charge_system_temperature!\n");
		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SAFETY_TIMER, 0xc8); //satety timer		
		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_TERMINATION_FAST_CHARGE, pdata->bq24160_reg5);//set changer current 0.1A , 1.2A
}
void enable_charge_system_temperature_suspend()
{
		mCharge=1;
		printk("enable_charge_system_temperature_suspend!\n");
		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_SAFETY_TIMER, 0xc8); //satety timer		
		bq24160_write_reg8(bq24160_info.this_client, BQ24160_REG_TERMINATION_FAST_CHARGE, pdata->bq24160_reg5_susp);//set changer current 0.1A , 1.2A
}

static int bq24160_chrger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{	
	int err;
	bq24160_info.this_client=client;
	mCharge=1;
	mCharge_system_temperature=1;
	err=check_i2c();
	if(err<0)
	{
		printk("bq24160 check_i2c error!\n");
		goto err1;
	}
		
	bq24160_info.bq24160_wq = create_workqueue("novatek_wq");
	if (!bq24160_info.bq24160_wq) {
		printk(KERN_ALERT "creat workqueue faiked\n");
		return -ENOMEM; 
	}

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;	

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;		
	pdata = client->dev.platform_data;
	charger->dev = &client->dev;
		
	printk("pdata->bq24160_reg3=%x\n",pdata->bq24160_reg3);
	printk("pdata->bq24160_reg5=%x\n",pdata->bq24160_reg5);	   
	   
  	//device_create_file(charger->dev, &CHR_attributes);
	bq24160_write_reg8(client, BQ24160_REG_CONTROL, 0x80); //reset disable
	bq24160_write_reg8(client, BQ24160_REG_SAFETY_TIMER, 0xc8); //satety timer	
	bq24160_i2c_init(client);
	
	bq24160_info.ac.name = "bq24160_ac";
	bq24160_info.ac.type = POWER_SUPPLY_TYPE_MAINS;
	bq24160_info.ac.properties = bq24160_charger_props;
	bq24160_info.ac.num_properties = ARRAY_SIZE(bq24160_charger_props);
	bq24160_info.ac.get_property = bq24160_bci_get_property;
	bq24160_info.ac.external_power_changed = NULL;
	
	INIT_DELAYED_WORK(&bq24160_info.work, bq24160_work_func);
	err = power_supply_register(&client->dev, &bq24160_info.ac);
	if (err){
		printk("power_supply_register bq24160_info.ac  fail\n");
		goto err1;
	}
	
	err = request_threaded_irq(client->irq, NULL, bq24160_charger_irq,	
				IRQF_TRIGGER_FALLING |IRQF_TRIGGER_RISING, "bq24160", NULL);		
    if (err < 0) {
        printk("bq24160_chrger_probe: request irq failed\n");
		goto err1;       
    }

	err = enable_irq_wake(client->irq);
	if (err)
		printk("%s enable_irq_wake error=%d \n", __FUNCTION__, err);
	
#if WATCHDOG_TIMER
	init_timer(&bq24160_info.bq24160_timer);
	setup_timer(&bq24160_info.bq24160_timer,bq24160_timer_function, 0);
	mod_timer(&bq24160_info.bq24160_timer,jiffies + msecs_to_jiffies(BT_POLL_TIME));	
#endif
	err = gpio_request( pdata->vbus_gpio, "otg_gpio");
	if (err){
		printk("request pdata->vbus_gpio fail\n");
		goto err1;
	}
	gpio_direction_output( pdata->vbus_gpio,0);
	
   charger->is_otg_enabled=0;
   charger->otg_reg_desc.name="vbus_otg";
	charger->otg_reg_desc.ops   = &bq24160_tegra_otg_regulator_ops;
	charger->otg_reg_desc.type  = REGULATOR_CURRENT;
	charger->otg_reg_desc.id    = pdata->otg_regulator_id;
	charger->otg_reg_desc.type  = REGULATOR_CURRENT;
	charger->otg_reg_desc.owner = THIS_MODULE;

	charger->otg_reg_init_data.supply_regulator         		= NULL;
	charger->otg_reg_init_data.num_consumer_supplies	= pdata->num_otg_consumer_supplies;
	charger->otg_reg_init_data.regulator_init						= NULL;
	charger->otg_reg_init_data.consumer_supplies				= pdata->otg_consumer_supplies;
	charger->otg_reg_init_data.driver_data							= charger;
	charger->otg_reg_init_data.constraints.name				= "vbus_otg";
	charger->otg_reg_init_data.constraints.min_uA				= 0;
	charger->otg_reg_init_data.constraints.max_uA			= 500000;

	charger->otg_reg_init_data.constraints.valid_modes_mask =
					REGULATOR_MODE_NORMAL |
					REGULATOR_MODE_STANDBY;

	charger->otg_reg_init_data.constraints.valid_ops_mask =
					REGULATOR_CHANGE_MODE |
					REGULATOR_CHANGE_STATUS |
					REGULATOR_CHANGE_CURRENT;

	charger->otg_rdev = regulator_register(&charger->otg_reg_desc, charger->dev,
		&charger->otg_reg_init_data, charger);
	
	if (IS_ERR(charger->otg_rdev)) {
		printk(&client->dev, "failed to register %s\n",
				charger->otg_reg_desc.name);
		}
err1:	
	return err;
}

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
	if(mCharge == 0)
		return 0;
#if WATCHDOG_TIMER
	setup_timer(&bq24160_info.bq24160_timer,bq24160_timer_function, 0);
	mod_timer(&bq24160_info.bq24160_timer,jiffies + msecs_to_jiffies(BT_POLL_TIME));	
#endif

	bq24160_i2c_init(client);
	return 0;
}

static const struct i2c_device_id bq24160_id[] = {
	{ "bq24160", bq24160 },	
	{ "bq24161", bq24161 },
	{},
};

static struct i2c_driver bq24160_battery_driver = {
	.driver = {
		.name = "bq24160_charger", 
	}, 	
	.probe = bq24160_chrger_probe,
	.suspend		= bq24160_charger_suspend,	
	.resume         = bq24160_cahrger_resume,
	.remove = bq24160_charger_remove,
	.id_table = bq24160_id,
};

static int __init bq24160_battery_init(void)
{
	int ret;

	ret = i2c_add_driver(&bq24160_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register bq24160 driver\n");
	return ret;
}

module_init(bq24160_battery_init);
static void __exit bq24160_battery_exit(void)
{ 
	i2c_del_driver(&bq24160_battery_driver); 
}
module_exit(bq24160_battery_exit);
MODULE_AUTHOR("tianpei.wan@keenhi.com");
MODULE_DESCRIPTION("bq24160 battery monitor driver");
MODULE_LICENSE("GPL");

