#ifndef __LINUX_BQ24160_CHARGER_H
#define __LINUX_BQ24160_CHARGER_H
#include <linux/regulator/machine.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/usb/otg.h>



int get_otg_vbus_status(void);
typedef void (*otg_callback) (void);
extern void get_callback_func(otg_callback ,otg_callback );

void disable_led_state(void);
void enalbe_led_state(void);
void enable_charge(void);
void disable_charge(void);
void disable_battery_charge(void);
void disable_charge_system_temperature(void);
void enable_charge_system_temperature(void);
void enable_charge_system_temperature_suspend(void);
struct bq24160_reg_data{
	int  reg0;		
	int  reg1;
	int  reg2;
	int  reg3;	
	int  reg4;
	int  reg5;
	int  reg6;
	int  reg7;
};
struct bq24160_charger_platform_data {
      unsigned int vbus_gpio;
      int otg_regulator_id;
 //      int regulator_id;
	int num_otg_consumer_supplies;
	struct regulator_consumer_supply *otg_consumer_supplies;
	
	int  bq24160_reg0;		
	int  bq24160_reg1;
	int  bq24160_reg2;
	int  bq24160_reg3;	
	int  bq24160_reg4;
	int  bq24160_reg5;
	int  bq24160_reg6;
	int  bq24160_reg7;

	int  bq24160_reg5_susp;

	
};
struct bq24160_charger {
	struct device	*dev;
/*	struct i2c_client	*client;
	
	void	*charger_cb_data;
	enum charging_states state;
	enum charger_type chrg_type;
	charging_callback_t	charger_cb; */

	int is_otg_enabled;
	struct regulator_dev    *rdev;
	struct regulator_desc   reg_desc;
	struct regulator_init_data      reg_init_data;
	struct regulator_dev    *otg_rdev;
	struct regulator_desc   otg_reg_desc;
	struct regulator_init_data      otg_reg_init_data;
};
#endif