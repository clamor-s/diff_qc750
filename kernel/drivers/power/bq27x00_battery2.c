/*
 * BQ27x00 battery driver
 *
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
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
#include <linux/interrupt_bq27x00.h>
#include <linux/mfd/max77663-core.h>
#include <asm/mach-types.h>
#ifdef CONFIG_CHARGER_BQ24160

#include <linux/bq24160_charger.h>
#endif
#include <linux/wakelock.h>

/*
*modified by densy.lai@keenhi.com, for support ac,usb
*/

//#define BATTERY_POLL_PERIOD	    	20000

#define BATTERY_POLL_PERIOD	    	10000 

#define ENABLE_CHARGE_AC		1
#define ENABLE_CHARGE_USB		0

#define DRIVER_VERSION			"1.1.0"

#define BQ27x00_REG_TEMP		0x06
#define BQ27x00_REG_VOLT		0x08
#define BQ27x00_REG_AI			0x14
#define BQ27x00_REG_FLAGS		0x0a
#define BQ27x00_REG_TTE			0x16
#define BQ27x00_REG_TTF			0x18
#define BQ27x00_REG_TTECP		0x26

#define BQ27x00_REG_LMD			0x12 /* Last measured discharge */
#define BQ27x00_REG_NAC			0x0c /* Nominal available capaciy */
#define BQ27500_REG_DCAP		0x3C /* Design capacity */
#define BQ27x00_REG_AE			0x22 /* Available enery */
#define BQ27510_POWER_AVG		0x24
#define BQ27510_CYCLE_COUNT		0x2a

#define BQ27510_CTRO_RES0		0x00
#define BQ27510_CTRO_RES1		0x01

#define BQ27000_REG_RSOC		0x0B /* Relative State-of-Charge */
#define BQ27000_FLAG_CHGS		BIT(7)

#define BQ27500_REG_SOC			0x2c
#define BQ27500_FLAG_DSC		BIT(0)
#define BQ27500_FLAG_FC			BIT(9)

#define BQ27500_FLAG_SOCF		BIT(1)
#define BQ27500_FLAG_OTC		BIT(15)

#define RSOC_FIRST        0xAA;

#define BQ_Filter    1

#define ABS(x)		((x) < 0 ? (-x) : (x))

#define CHARGE_PIN				(('r'-'a')*8+6)
#define _debug_is 0
#if _debug_is
#define debug_print(...) pr_err(__VA_ARGS__)
#else
#define debug_print(...) 
#endif

/* If the system has several batteries we need a different name for each
 * of them...
 */
static int bq_if,battery_full=0;
static int   last_rsoc=RSOC_FIRST;
static int   last_rsoc0;
static int   last_current=1000;
static int  last_voltage=3620;
static int   last_temp=400;
static int   cur_temp=3031;
static int   nex_temp=400;
static int   rec_temp=3031;
unsigned count_time;

static led_status=0;
struct bq27x00_device_info *di0;

static int  rsoc_count;
static int  my_count=1;
static bool  mChargeEnable=1;
static bool  mChargeBatteryEnable=1;
static bool  mChargeControl=0;
static struct wake_lock mWakeLock;

static DEFINE_IDR(battery_id);
static DEFINE_MUTEX(battery_mutex);
void enableChargePin(int enable);

struct bq27x00_device_info;
struct bq27x00_access_methods {
	int (*read)(u8 reg, int *rt_value, int b_single,
		struct bq27x00_device_info *di);
};

enum bq27x00_chip { BQ27000, BQ27500 };

struct bq27x00_device_info {
	struct interrupt_bq27x00_platform_data *pdata;
	struct device 		*dev;
	int			id;
	struct bq27x00_access_methods	*bus;
	struct power_supply	bat;
#if ENABLE_CHARGE_AC
	struct power_supply* ac;
#endif
#if ENABLE_CHARGE_USB
	struct power_supply	usb;
#endif
	enum bq27x00_chip	chip;

	struct i2c_client	*client;
	struct timer_list*	battery_poll_timer;
	int present;
};


static enum power_supply_property bq27x00_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
/*	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	
	POWER_SUPPLY_PROP_CHARGE_FULL,	
	POWER_SUPPLY_PROP_CHARGE_NOW,	
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,	
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_POWER_AVG,	
	POWER_SUPPLY_PROP_CYCLE_COUNT,		
	POWER_SUPPLY_PROP_HEALTH,*/
};
#if ENABLE_CHARGE_AC
static char *power_supplied_to[] = {
	"battery",
};

static enum power_supply_property bq27x00_ac_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
#endif

#if ENABLE_CHARGE_USB

#endif
#ifdef CONFIG_CHARGER_BQ24160

static ssize_t charge_state_show(struct device *dev, struct kobj_attribute *attr,char *buf )  
{  
        if(mChargeEnable) sprintf(buf,"%s","1"); 
		else sprintf(buf,"%s","0");
	   
	   return strlen(buf); 

	
}  
static ssize_t charge_state_store(struct device *dev,struct kobj_attribute *attr,const char* buf, size_t count)  
{     
				if(!strncmp(buf, "0", 1)){
					if(mChargeEnable==1)
						{
							mChargeEnable=0;
							disable_charge();
						}
					}
				if(!strncmp(buf, "1", 1)){
					if(mChargeEnable==0)
						{
							mChargeEnable=1;
							enable_charge();
						}
					}
			if(mChargeControl==0)	
				mChargeControl=1;

              return count;    
} 

static struct device_attribute CHR_attributes[] = {
	__ATTR(charge_battery_state, 0666, charge_state_show, charge_state_store),
	__ATTR_NULL,
};
#endif
void updatePresent(struct bq27x00_device_info *di);
#if ENABLE_CHARGE_AC
static int bq27x00_get_ac_status(struct bq27x00_device_info* di);
#endif

/*
 * Common code for BQ27x00 devices
 */
static int bq27x00_write_reg8(struct i2c_client *client, u8 addr, u8 val) 
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


static int bq27x00_read(u8 reg, int *rt_value, int b_single,
			struct bq27x00_device_info *di)
{
	return di->bus->read(reg, rt_value, b_single, di);
}

/*
 * Return the battery temperature in tenths of degree Celsius
 * Or < 0 if something fails.
 */
static int bq27x00_battery_temperature(struct bq27x00_device_info *di)
{
	int ret;
	int i;
	

	if(((my_count<=3)||(my_count==5))){
		cur_temp=0;
		ret = bq27x00_read(BQ27x00_REG_TEMP, &cur_temp, 0, di);
		if (ret) {
			dev_err(di->dev, "error reading temperature\n");
			last_temp=300;
			return last_temp;
		}
		nex_temp=cur_temp;
		if(ABS(nex_temp-last_temp)<20)
				count_time++;
		else
			count_time=count_time;
			last_temp = cur_temp;

		printk("count_time=%d\n",count_time);			
		
		}
	if(count_time==4)
		{
			count_time=0;
			rec_temp = cur_temp;

			if(machine_is_birch())
			{

			if(max77663_acok_state())
			{

			if(mChargeControl==0)
			{
			   if(((cur_temp-2731)>=450)||((cur_temp-2731)<-20))
			   	{
			   	if(mChargeBatteryEnable==1)
			   		{
			   	mChargeBatteryEnable=0;
				disable_battery_charge();
			   		}
			   	}
			   else
			   	{
			   	if(mChargeBatteryEnable==0)
			   		{
			   	mChargeBatteryEnable=1;
				enable_charge();
			   		}
			   }
				}
			}
		}		

			//printk("cur_temp=%d\n",cur_temp-2731);

			return (cur_temp-2731);
				}
	else
			{
	
				//cur_temp=cur_temp - 2731;
		//printk("last_temp=%d\n",rec_temp-2731);				
				return (rec_temp-2731);
			}
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
static int bq27x00_battery_voltage(struct bq27x00_device_info *di)
{
	int ret;
	int volt = 0;
/*	if((my_count<=3)||(my_count==8)){
		ret = bq27x00_read(BQ27x00_REG_VOLT, &volt, 0, di);
		if (ret) {
			dev_err(di->dev, "error reading voltage\n");
			last_voltage=ret;
			return ret;
		}

		last_voltage=volt;
		
	}	
	printk("bq27x00_battery_voltage=%d\n",last_voltage);
	return last_voltage*1000 ;
*/

		if(last_rsoc0==100){
			last_voltage=4200;
			}
		else if (last_rsoc0>=80){
			last_voltage=4120;
			
			}
		else if(last_rsoc0>=60){
			last_voltage=3980;
			}
		else if (last_rsoc0>=40){
			last_voltage=3860;
			}
		else if(last_rsoc0>=20){
			last_voltage=3740;
			}
		else{
			last_voltage=3620;
			}	
	//printk("bq27x00_battery_voltage=%d\n",last_voltage);	
	  return last_voltage*1000;
}

/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
static int bq27x00_battery_current(struct bq27x00_device_info *di)
{
	int ret;
	int curr = 0;
	int flags = 0;
	if((my_count<=3)||(my_count==6)){
		ret = bq27x00_read(BQ27x00_REG_AI, &curr, 0, di);
		if (ret) {
			dev_err(di->dev, "error reading current\n");
			last_current=0;
			return 0;
		}

		if (di->chip == BQ27500) {
			/* bq27500 returns signed value */
			//curr = (int)(u32)curr;
		} else {
			ret = bq27x00_read(BQ27x00_REG_FLAGS, &flags, 0, di);
			if (ret < 0) {
				dev_err(di->dev, "error reading flags\n");
				last_current=0;
				return 0;
			}
			if (flags & BQ27000_FLAG_CHGS) {
				dev_dbg(di->dev, "negative current!\n");
				curr = -curr;
			}
		}
		//debug_print("bq27x00_battery_current======>EXE cOrg= %d cc=%d\n",curr,curr * 1000 );
		
		//printk("bq27x00_battery_current=%d\n",curr);
		last_current=curr;

		return curr;
	}
	else {  
	    return curr;
	}
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
static int bq27x00_battery_rsoc(struct bq27x00_device_info *di)
{
	int ret;
	int rsoc = 0;
	int rsoc_first=0xAA;  

	if((my_count<=3)||(my_count==7)){
		if (di->chip == BQ27500)
			ret = bq27x00_read(BQ27500_REG_SOC, &rsoc, 0, di);
		else
			ret = bq27x00_read(BQ27000_REG_RSOC, &rsoc, 1, di);
		
		if(ret <0 || ret >100){
			printk("workarround for fix the battery ret=%d\n",ret);
			return last_rsoc0; 
		}
		
		if (ret) {
			dev_err(di->dev, "error reading relative State-of-Charge\n");
			return last_rsoc0;
		}  
	     if (last_rsoc!=rsoc_first)	{    
				if((rsoc>=0)&&(rsoc<=100))     {
					if(((rsoc>=last_rsoc-3)&&(rsoc<=last_rsoc+3))||(rsoc_count>=3)){
						last_rsoc=rsoc;
						rsoc_count=0;
					//printk("bq27x00_battery_rsoc2=>EXE rsoc= %d---((rsoc*100)/90-11)=%d\n",rsoc,((rsoc*100)/90-11));  
					rsoc=((rsoc*100)/90-11);
		#ifdef CONFIG_CHARGER_BQ24160
				if(my_count>=3){
				if (max77663_acok_state()){
					led_status=1;
					if(rsoc==100){ 
						disable_led_state();
						}
						else{  
						enalbe_led_state();	
							}
						}
				else{
						if(led_status){
							enalbe_led_state();	
							led_status=0;
							}
					}
					}
		#endif	
			
					if(rsoc<0) rsoc=0;
					last_rsoc0=rsoc;
					       if(last_rsoc0==100)    battery_full=1;  
					            else battery_full=0;
					       return rsoc;}
						else{rsoc_count=rsoc_count+1; 
								}
				}
	     }
		
		else   
			{     if((rsoc>=0)&&(rsoc<=100))      last_rsoc = rsoc;    
		              else        last_rsoc=50;
				last_rsoc0=last_rsoc;     
			}
		
		
	return last_rsoc0 ;
}	
else  {  
          return last_rsoc0;  
}	
}

static int bq27x00_battery_status(struct bq27x00_device_info *di, 
				  union power_supply_propval *val)
{		
	if(mChargeEnable==0||mChargeBatteryEnable==0)
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
else
{
     if (max77663_acok_state()){
				if(battery_full==1) val->intval=POWER_SUPPLY_STATUS_FULL;
				else val->intval=POWER_SUPPLY_STATUS_CHARGING;
			}
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
}
                 return 0;
}

/*
 * Read a time register.
 * Return < 0 if something fails.
 */
static int bq27x00_battery_time(struct bq27x00_device_info *di, int reg,
				union power_supply_propval *val)
{
	int tval = 0;
	int ret;
	ret = bq27x00_read(reg, &tval, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading register %02x\n", reg);
		return ret;
	}
	//debug_print("bq27x00_battery_time======>EXE time_org= %d,tem_android= %d reg = 0x%x\n",tval,tval * 60,reg);
	if (tval == 65535)
		return -ENODATA;
	val->intval = tval * 60;
	return 0;
}
static  int bq27x00_battery_common_read(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{
	int tval = 0;
	int ret;
	ret = bq27x00_read(reg, &tval, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading register %02x\n", reg);
		return ret;
	}
	val->intval = tval * 1000;
	return 0;	
}

/* * Return the battery Last measured discharge in ¦ÌAh * Or < 0 if something fails. */
static inline int bq27x00_battery_read_lmd(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	return bq27x00_battery_common_read(di,reg,val);
}

/* * Return the battery Nominal available capaciy in ¦ÌAh * Or < 0 if something fails. */
static inline int bq27x00_battery_read_nac(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	return bq27x00_battery_common_read(di,reg,val);
}

/* * Return the battery Initial last measured discharge in ¦ÌAh * Or < 0 if something fails. */
static inline int bq27x00_battery_read_ilmd(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	return bq27x00_battery_common_read(di,reg,val);
}
/* * Return the battery Available energy in ¦ÌWh * Or < 0 if something fails. */
static inline int bq27x00_battery_energy(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{
	return bq27x00_battery_common_read(di,reg,val);
}

static int bq27510_battery_power_avg(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	int tval = 0;
	int ret;

	ret = bq27x00_read(reg, &tval, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading register %02x\n", reg);
		return ret;
	}
	debug_print("bq27510_battery_power_avg======>EXE power_org= %d,reg = 0x%x\n",tval,reg);
	val->intval = tval;
	return 0;
}
static int bq27510_battery_cycle_count(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	int tval = 0;
	int ret;

	ret = bq27x00_read(reg, &tval, 0, di);
	if (ret) {
		dev_err(di->dev, "error reading register %02x\n", reg);
		return ret;
	}
	debug_print("bq27510_battery_cycle_count======>EXE count_org= %d\n",tval);
	val->intval = tval;
	return 0;
}
static int bq27510_battery_health(struct bq27x00_device_info *di,int reg,union power_supply_propval *val)
{	
	int tval = 0;
	int ret;	
	if (di->chip == BQ27500) 
	{		
		ret = bq27x00_read(reg, &tval, 0, di);
		if (ret < 0) 
		{			
			dev_err(di->dev, "read failure\n");			
			return ret;		
		}
		debug_print("bq27510_battery_health======>EXE flag_org= 0x%x\n",tval);
		if (tval & BQ27500_FLAG_SOCF)			
			val->intval = POWER_SUPPLY_HEALTH_DEAD;		
		else if (tval & BQ27500_FLAG_OTC)		
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;		
		else			
			val->intval = POWER_SUPPLY_HEALTH_GOOD;		
		return 0;	
	}	
	return -1;
}
#define to_bq27x00_device_info_bat(x) container_of((x), \
				struct bq27x00_device_info, bat);
#define to_bq27x00_device_info_ac(x) container_of((x), \
				struct bq27x00_device_info, *ac);

static int bq27x00_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	
	struct bq27x00_device_info *di = to_bq27x00_device_info_bat(psy);
if(my_count<=3)  mdelay(50);  

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS: 
		
		ret = bq27x00_battery_status(di, val);  
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_PRESENT:
		
		val->intval = bq27x00_battery_voltage(di);
		if (psp == POWER_SUPPLY_PROP_PRESENT){
			printk("reading the ac status %d\n",val->intval);
			val->intval = val->intval <= 0 ? 0 : 1;
		}
		break;
		
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	 
		val->intval = bq27x00_battery_current(di);
		break;

	case POWER_SUPPLY_PROP_CAPACITY:
	
		val->intval =  bq27x00_battery_rsoc(di);
		break;
		
	case POWER_SUPPLY_PROP_TEMP: 
	        
		val->intval = bq27x00_battery_temperature(di);
		break;
/*	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = bq27x00_battery_time(di, BQ27x00_REG_TTE, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
		ret = bq27x00_battery_time(di, BQ27x00_REG_TTECP, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = bq27x00_battery_time(di, BQ27x00_REG_TTF, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;//used by li charge
		break;
	case  POWER_SUPPLY_PROP_CHARGE_FULL:	
		ret = bq27x00_battery_read_lmd(di, BQ27x00_REG_LMD, val);
		debug_print("bq27x00_battery_read_lmd======>EXE LMD_org= %d\n",val->intval);
		break;
	case  POWER_SUPPLY_PROP_CHARGE_NOW:	
		ret = bq27x00_battery_read_nac(di, BQ27x00_REG_NAC, val);
		debug_print("bq27x00_battery_read_nac======>EXE nac_org= %d\n",val->intval);
		break;
	case  POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		ret = bq27x00_battery_read_ilmd(di, BQ27500_REG_DCAP, val);
		debug_print("bq27x00_battery_read_ilmd======>EXE dcap_org= %d\n",val->intval);
		break;
	case  POWER_SUPPLY_PROP_ENERGY_NOW:
		ret = bq27x00_battery_energy(di, BQ27x00_REG_AE, val);
		debug_print("bq27x00_battery_energy======>EXE energy_org= %d\n",val->intval);
		break;
	case  POWER_SUPPLY_PROP_POWER_AVG:	
		ret = bq27510_battery_power_avg(di, BQ27510_POWER_AVG, val);
		break;
	case  POWER_SUPPLY_PROP_CYCLE_COUNT:	
		ret = bq27510_battery_cycle_count(di, BQ27510_CYCLE_COUNT, val);
		break;
*/		
	case  POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		//ret = bq27510_battery_health(di, BQ27x00_REG_FLAGS, val);
		break;

	default:
		return -EINVAL;
	}



	
#if 0
	if(psp ==POWER_SUPPLY_PROP_VOLTAGE_NOW ||psp ==POWER_SUPPLY_PROP_TEMP){
		//printk("bq27x00_battery_get_property psp=%d, intval=%d\n",psp,val->intval);

		if(psp==POWER_SUPPLY_PROP_TEMP){
			val->intval =260;
		}
	}
#endif


	return ret;
}

#if ENABLE_CHARGE_AC
static int bq27x00_get_ac_status(struct bq27x00_device_info* di)
{
	//int charger_gpio = irq_to_gpio(di->client->irq); 
	return 1;//!gpio_get_value(charger_gpio);
}
static int bq27x00_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{

		if (max77663_acok_state())
			val->intval = 1;//POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval =0;// POWER_SUPPLY_STATUS_DISCHARGING;


	return 0;
}
#endif

#if ENABLE_CHARGE_USB
static int bq27x00_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	return 0;
}
#endif

static void bq27x00_powersupply_init(struct bq27x00_device_info *di)
{
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = bq27x00_battery_props;
	di->bat.num_properties = ARRAY_SIZE(bq27x00_battery_props);
	di->bat.get_property = bq27x00_battery_get_property;
	di->bat.external_power_changed = NULL;
	di->bat.name = "battery";

	if(di->ac==NULL){
		return;
	}

#if ENABLE_CHARGE_AC
	di->ac->type = POWER_SUPPLY_TYPE_MAINS;
	di->ac->properties = bq27x00_ac_properties;
	di->ac->num_properties = ARRAY_SIZE(bq27x00_ac_properties);
	di->ac->get_property = bq27x00_ac_get_property;
	di->ac->external_power_changed = NULL;
	di->ac->supplied_to = power_supplied_to;
	di->ac->num_supplicants = ARRAY_SIZE(power_supplied_to);
	di->ac->name = "ac";
#endif
#if ENABLE_CHARGE_USB
	#error not implement usb
#endif
}

/*
 * i2c specific code
 */
unsigned int transBytes2UnsignedInt(unsigned char msb, unsigned char lsb)
{
  unsigned int tmp;
  
  tmp = ((msb << 8) & 0xFF00);
  return ((unsigned int)(tmp + lsb) & 0x0000FFFF);  
}


static int bq27x00_read_i2c(u8 reg, int *rt_value, int b_single,
			struct bq27x00_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[2];
    unsigned char data_test[2];
	
	int err;

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;
	data[0] = reg;

	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		if (!b_single)
			msg->len = 2;
		else
			msg->len = 1;

		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			if (!b_single)
				*rt_value = transBytes2UnsignedInt(data[1],data[0]);//get_unaligned_le16(data);
			else
				*rt_value = data[0];

			return 0;
		}
	}
	return err;
}

static irqreturn_t interrupt_ac_plug_isr(int irq, void *dev_id){	
	struct interrupt_plug_ac *bdata = dev_id;

	if(bdata==NULL){
		return IRQ_HANDLED;
	}

	printk("interrupt_ac_plug_isr~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	if(di0 !=NULL&& di0->ac!=NULL)
		power_supply_changed(di0->ac);

	my_count=4;

	return IRQ_HANDLED;
}
static void battery_poll_timer_func(unsigned long data)
{
	if(my_count>8)
		my_count=4;
	else
		my_count++;

	di0 =(struct bq27x00_device_info*)data;

	if(di0->ac!=NULL){
		power_supply_changed(di0->ac);
	}
	
	if(di0->present){
		power_supply_changed(&di0->bat);
	}

	printk("battery_poll_timer_func\n");
	
	mod_timer(di0->battery_poll_timer,
		jiffies + msecs_to_jiffies(BATTERY_POLL_PERIOD));
}

static irqreturn_t ac_present_irq(int irq, void *data)
{
	struct bq27x00_device_info *di;

	printk("ac_present_irq===============>EXE will change\n");
	di =(struct bq27x00_device_info*)data;

	power_supply_changed(di->ac);//jimmy add
	if(di->present){
		
	}
	return IRQ_HANDLED;
}

static void bq27x00_reset_lock(struct i2c_client *client)
{

    printk("bq27x00_reset_lock \n");
	bq27x00_write_reg8(client,BQ27510_CTRO_RES1,0x0);  //reset bq27541
	bq27x00_write_reg8(client,BQ27510_CTRO_RES0,0x41);
	mdelay(2000);
	bq27x00_write_reg8(client,BQ27510_CTRO_RES1,0x0);// lock 
	bq27x00_write_reg8(client,BQ27510_CTRO_RES0,0x20);
	mdelay(3000);

}
static int bq27x00_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	//char *name;
	struct bq27x00_device_info *di;
	struct bq27x00_access_methods *bus;
	struct interrupt_bq27x00_platform_data *pdata =  client->dev.platform_data;
	int num,i;
	int retval = 0;
	int rsoc=0;
	int volt=0;
	mChargeEnable=1;
	mChargeControl = 0;
	pr_err("bq27x00_battery_probe id=%s\n",id->name);
	mChargeEnable=1;
	mChargeControl=0;
	mChargeBatteryEnable=1;
	/* Get new ID for the new battery device */
	retval = idr_pre_get(&battery_id, GFP_KERNEL);
	if (retval == 0)
		return -ENOMEM;
	mutex_lock(&battery_mutex);
	retval = idr_get_new(&battery_id, client, &num);
	mutex_unlock(&battery_mutex);

	if (retval < 0)
		return retval;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}

	di->id = num;
	di->chip = id->driver_data;
	di->pdata = pdata;
	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate access method "
					"data\n");
		retval = -ENOMEM;
		goto batt_failed_3;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	//di->bat.name = name;
	bus->read = &bq27x00_read_i2c;
	di->bus = bus;
	di->client = client;
	updatePresent(di);

	if(pdata!=NULL && pdata->mHasAc){
		di->ac =kzalloc(sizeof(struct power_supply), GFP_KERNEL);
	}
	else{
		di->ac=NULL;
	}
	bq27x00_powersupply_init(di);

	bq27x00_read(BQ27500_REG_SOC, &rsoc, 0, di);
	mdelay(500);
	bq27x00_read(BQ27x00_REG_VOLT, &volt, 0, di);
	mdelay(500);
	
	if((volt<=2800)||(volt>4299)){
		bq27x00_reset_lock(client); //voltage<=3.0V or voltage >4.2V  soc reset
	}

	if((volt>4000)&&(rsoc<20)){  
		bq27x00_reset_lock(client); //voltage>4.0V and rsoc < 20%   soc reset
	}


	if(di->present){
		retval = power_supply_register(&client->dev, &di->bat);
		if (retval) {
			dev_err(&client->dev, "failed to register battery\n");
			goto batt_failed_4;
		}
	}

	if(di->ac!=NULL){
		retval = power_supply_register(&client->dev, di->ac);
		if (retval) {
			dev_err(&client->dev, "failed to register battery\n");
			goto batt_failed_5;
		}
	}
#ifdef CONFIG_CHARGER_BQ24160	
	 device_create_file(di->dev, &CHR_attributes);
#endif
	if(pdata!=NULL){		
		for (i = 0; i < pdata->nIrqs; i++) {			
			struct interrupt_plug_ac * nR = &pdata->int_plug_ac[i];
			retval = request_threaded_irq(nR->irq, NULL, interrupt_ac_plug_isr,	
				IRQF_TRIGGER_FALLING |IRQF_TRIGGER_RISING, "int_ac1", nR);			
			if (retval) {				
				dev_err(&client->dev, "Unable to register irq %d; error %d\n",					
					nR->irq, retval);				
				goto batt_failed_6;			
			}
		}
	}

	di->battery_poll_timer =kzalloc(sizeof(*di->battery_poll_timer), GFP_KERNEL);
	setup_timer(di->battery_poll_timer,
		battery_poll_timer_func, di);
	mod_timer(di->battery_poll_timer,
		jiffies + msecs_to_jiffies(BATTERY_POLL_PERIOD));

	if (di->chip != BQ27500) 
		enableChargePin(1);

	pr_err("%s=======>success\n",__func__);
	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);

	wake_lock_init(&mWakeLock, WAKE_LOCK_SUSPEND, "bq27x00_wakelock");

	return 0;
batt_failed_6:
	if(di->ac!=NULL){
		power_supply_unregister(di->ac);
	}
batt_failed_5:
	power_supply_unregister(&di->bat);
batt_failed_4:
	kfree(bus);
batt_failed_3:
	kfree(di);
batt_failed_2:
	if(di->ac!=NULL){
		kfree(di->ac);
	}
batt_failed_1:
	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, num);
	mutex_unlock(&battery_mutex);

	return retval;
}

void updatePresent(struct bq27x00_device_info *di)
{
	int flags = 0;
	int status;
	int ret;

	ret = bq27x00_read(BQ27x00_REG_FLAGS, &flags, 0, di);
	if (ret != 0) {
		di->present =0;
	}
	else{
		di->present =1;
	}

	dev_info(di->dev, "updatePresent present=%d ret=%d\n",di->present,ret);
}

void enableChargePin(int enable)
{
	gpio_request(CHARGE_PIN, "charge pin");
	gpio_direction_output(CHARGE_PIN, !enable);
}
static int bq27x00_battery_suspend(struct i2c_client *client)
{
	struct bq27x00_device_info *di = i2c_get_clientdata(client);
	int i;
	if(di->pdata!=NULL){
		for (i = 0; i < di->pdata->nIrqs; i++) {		
			disable_irq_nosync(di->pdata->int_plug_ac[i].irq);
		}
	}
	if(di->battery_poll_timer!=NULL)
		del_timer_sync(di->battery_poll_timer);

	return 0;
}
static int bq27x00_battery_resume(struct i2c_client *client)
{
	struct bq27x00_device_info *di = i2c_get_clientdata(client);
	int i;
	my_count=6; //refresh the battery

	//wake_lock_timeout(&mWakeLock, msecs_to_jiffies(10000));

	if(di->battery_poll_timer!=NULL){
		setup_timer(di->battery_poll_timer,
		battery_poll_timer_func, di);
		
		mod_timer(di->battery_poll_timer,
			jiffies + msecs_to_jiffies(4000));
	}

	if(di->pdata!=NULL){
		for (i = 0; i < di->pdata->nIrqs; i++) {		
			enable_irq(di->pdata->int_plug_ac[i].irq);
		}
	}
	return 0;
}
static int bq27x00_battery_remove(struct i2c_client *client)
{
	struct bq27x00_device_info *di = i2c_get_clientdata(client);
	int i;
	if(di->present){
		power_supply_unregister(&di->bat);
	}
	if(di->ac!=NULL){
		power_supply_unregister(&di->ac);
		kfree(di->ac);
		di->ac=NULL;
	}
	if(di->pdata!=NULL){
		for (i = 0; i < di->pdata->nIrqs; i++) {		
			free_irq(di->pdata->int_plug_ac[i].irq, &di->pdata->int_plug_ac[i]);
		}
	}
	//kfree(di->bat.name);
	if(di->battery_poll_timer!=NULL){
		del_timer_sync(di->battery_poll_timer);
		kfree(di->battery_poll_timer);
		di->battery_poll_timer=NULL;
	}

	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, di->id);
	mutex_unlock(&battery_mutex);

	kfree(di);

	return 0;
}

/*
 * Module stuff
 */

static const struct i2c_device_id bq27x00_id[] = {
	{ "bq27200", BQ27000 },	/* bq27200 is same as bq27000, but with i2c */
	{ "bq27500", BQ27500 },
	{},
};

static struct i2c_driver bq27x00_battery_driver = {
	.driver = {
		.name = "bq27x00-battery",
	},
	.suspend	= bq27x00_battery_suspend,
	.probe = bq27x00_battery_probe,
	.resume         = bq27x00_battery_resume,
	.remove = bq27x00_battery_remove,
	.id_table = bq27x00_id,
};

static int __init bq27x00_battery_init(void)
{
	int ret;

	ret = i2c_add_driver(&bq27x00_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register BQ27x00 driver\n");

	return ret;
}
module_init(bq27x00_battery_init);

static void __exit bq27x00_battery_exit(void)
{
	i2c_del_driver(&bq27x00_battery_driver);
}
module_exit(bq27x00_battery_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("BQ27x00 battery monitor driver");
MODULE_LICENSE("GPL");
