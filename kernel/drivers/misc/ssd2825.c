/*

 * keenhi ssd2825(T007) mipi bridge (SPI bus) - Android version
 *
 * Copyright (C) 2011-2012 keenhi Inc.
 *
 * Licensed under the GPL-2 or later.
 *
 * Version : 0.04
 */

//=============================================================================
//INCLUDED FILES
//=============================================================================
#include <linux/input.h>	// BUS_SPI
#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>	// wake_up_process()
#include <linux/kthread.h>	// kthread_create()、kthread_run()
#include <asm/uaccess.h>	// copy_to_user(),
#include <linux/miscdevice.h>
#include <asm/siginfo.h>	// siginfo
#include <linux/rcupdate.h>	// rcu_read_lock
#include <linux/sched.h>	// find_task_by_pid_type
#include <linux/syscalls.h>	// sys_clock_gettime()
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include <linux/ssd2825.h>	
//=============================================================================
//DEFINITIONS
//=============================================================================

#define MAX_SPI_FREQ_HZ      50000000
#define TS_PEN_UP_TIMEOUT    msecs_to_jiffies(50)

#define UDELAY(n)   (udelay(n))

#define MDELAY(n)   (mdelay(n))

#define max_khz_rate(x) (x)*1000
#define max_mhz_rate(x) (x)*1000*1000

#define SCREEN_AUO_W  0x0300
#define SCREEN_AUO_H 0x0400

#define SCREEN_CPT_W  0x0556
#define SCREEN_CPT_H 0x0300
 //  
 // 750*1000
#define MAX_SPEED_RATE  2*1000*1000//100*1000 //1MHZ
#define DEBUG 1

#define KEENHI_DC_OUT_TYPE_DSI 0
#define KEENHI_DC_OUT_TYPE_RGB 1
#define KEENHI_DC_OUT_TYPE_SSD2828_AUO 2
#define DC_CTRL_TYPE      KEENHI_DC_OUT_TYPE_SSD2828_AUO
//=============================================================================
//STRUCTURE DECLARATION
//=============================================================================

//=============================================================================
//GLOBAL VARIABLES DECLARATION
//=============================================================================

static struct spi_device *g_spi;
static int g_init = 0;
//=============================================================================
// Description:
//      ssd2825 spi interface.
// Input:
//      N/A
// Output:
//      1:succeed
//      0:failed
//=============================================================================
int ssd2825_spi_read(u8 u8addr, u16 * rxbuf)
{
	static DEFINE_MUTEX(lock);
	u8 dataAddr[1];
	int status;
	u8 data[2];
	size_t len =2 ;
	struct spi_message message;
	struct spi_transfer x[2];

	if (!mutex_trylock(&lock)) {
		printk("keenhi: ssd2825_spi_read trylock fail\n");
		return -EINVAL;
	}
	spi_message_init(&message);
	memset(x, 0, sizeof x);
	memset(data,0,sizeof(data));
	dataAddr[0] = u8addr;
	
	x[0].len = 1;
	x[0].tx_buf = dataAddr;
	x[0].bits_per_word=8;
	x[0].speed_hz = MAX_SPEED_RATE;
	spi_message_add_tail(&x[0], &message);

	x[1].len = len;
	x[1].rx_buf = data;
	x[1].bits_per_word=16;
	x[1].speed_hz = MAX_SPEED_RATE;
	spi_message_add_tail(&x[1], &message);
	status = spi_sync(g_spi, &message);
	
	*rxbuf = ((data[1]<<8)|data[0]);
	mutex_unlock(&lock);
	return status;		// 0 = succeed
}


int ssd2825_spi_write(u8 * txbuf, size_t len)
{
	return spi_write(g_spi, txbuf, len);
}

static int tsc2005_write( unsigned int u24Addr,int len)
{

	struct spi_transfer xfer = {
		.tx_buf		= &u24Addr,
		.len		= len,
		.bits_per_word	= 24,
		.speed_hz = MAX_SPEED_RATE,
	};
	struct spi_message msg;
	int error;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	error = spi_sync(g_spi, &msg);
	if (error) {
		dev_err(&g_spi->dev,
			"%s: failed, register: value: %x, error: %d\n",
			__func__, u24Addr, error);
		return error;
	}

	return 0;
}

static __inline int ssd2825_spi_byte_write(unsigned int u24Addr, unsigned int u24Value)
{
	int iErrorCode;

	iErrorCode = tsc2005_write(u24Addr, 3);
	if (iErrorCode != 0) {
		dev_err(&g_spi->dev,"SSD2825_spi_write_byte failed:Reg=%x", u24Addr);
		return 0;	//fail
	}
	return 1;
}
static __inline void ssd2828_write_com(unsigned int cmd)
{
	ssd2825_spi_byte_write(cmd,0);
}
static __inline void ssd2828_write_register(unsigned int data)
{
	ssd2825_spi_byte_write(data,0);
}
static int deinit_all_gpio(struct ssd2828_data *data)
{
	if(data->reg){
		regulator_put(data->reg);
	}

	if(data->pdata->lcd_1v2_gpio>0){
		gpio_free(data->pdata->lcd_1v2_gpio);
	}
	
	if(data->pdata->lcd_vdds_en_gpio){
		gpio_free(data->pdata->lcd_vdds_en_gpio);
	}
	if(data->pdata->lcd_en_3v3_gpio){
		gpio_free(data->pdata->lcd_en_3v3_gpio);
	}
	if(data->pdata->mipi_shutdown_gpio){
		gpio_free(data->pdata->mipi_shutdown_gpio);
	}
	if(data->pdata->mipi_reset_gpio){
		gpio_free(data->pdata->mipi_reset_gpio);
	}
	return 0;
}
static int __devexit ssd2825_spi_remove(struct spi_device *spi)
{
	struct ssd2828_data *data = dev_get_drvdata(&spi->dev);
	deinit_all_gpio(data);
	kfree(data);
	return 0;
}


static int Ssd2825_deinit(struct spi_device *spi)
{
	pr_err("%s:==============>EXE\n",__func__);
	struct ssd2828_data *data = dev_get_drvdata(&spi->dev);
	static DEFINE_MUTEX(sleep_locks);
	 if (!mutex_trylock(&sleep_locks)) {
			pr_err("keenhi: %s suspend trylock fail\n",__func__);
	}
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x720304);

	ssd2828_write_com(0x7000B9);
	ssd2828_write_register(0x720000);            //PLL on
	mutex_unlock(&sleep_locks);
	//if(data->pdata->lcd_en_3v3_gpio){
		//gpio_set_value(data->pdata->lcd_en_3v3_gpio, 0);
	//}
#if 0
	if(data->pdata->lcd_vdds_en_gpio){
		gpio_set_value(data->pdata->lcd_vdds_en_gpio, 0);
	}

	if(data->pdata->mipi_reset_gpio){
		gpio_set_value(data->pdata->mipi_reset_gpio, 0);
	}
	if(data->pdata->mipi_shutdown_gpio){
		gpio_set_value(data->pdata->mipi_shutdown_gpio, 0);
	}
	if(data->reg){
		regulator_disable(data->reg);
	}
#endif
	return 0;
}
#if 1
static int init_all_gpio(struct ssd2828_data *data)
{
	int err;
	if(data->pdata->lcd_1v2_gpio <=0){
		data->reg = regulator_get(&data->spi->dev, "avdd_dsi_csi");
		if(!data->reg){
			dev_err(&data->spi->dev,"%s: regulator get failed !\n", __func__);
			//return -1; for teset
		}
		regulator_enable(data->reg);
	}else{
		err = gpio_request(data->pdata->lcd_1v2_gpio, "lvds_avdd_en");
		if (err < 0) 
			dev_err(&data->spi->dev,"%s: gpio_request failed %d\n", __func__, data->pdata->lcd_1v2_gpio);
		gpio_direction_output(data->pdata->lcd_1v2_gpio, 1);
		data->reg=NULL;
	}
	if(data->pdata->lcd_vdds_en_gpio){
		err = gpio_request(data->pdata->lcd_vdds_en_gpio, "lcd-vdds");
		if (err < 0) 
			dev_err(&data->spi->dev,"%s: gpio_request failed %d\n", __func__, data->pdata->lcd_vdds_en_gpio);
#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
		gpio_direction_output(data->pdata->lcd_vdds_en_gpio, 1);
#else
		gpio_direction_output(data->pdata->lcd_vdds_en_gpio, 0);
#endif
	}

	if(data->pdata->lcd_en_3v3_gpio){
		err = gpio_request(data->pdata->lcd_en_3v3_gpio, "lcd-3v3-en");
		if (err < 0) 
			dev_err(&data->spi->dev,"%s: gpio_request failed %d\n", __func__, data->pdata->lcd_en_3v3_gpio);
#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
		gpio_direction_output(data->pdata->lcd_en_3v3_gpio, 1);
#else
		//gpio_direction_output(data->pdata->lcd_en_3v3_gpio, 0);
#endif
	}

	if(data->pdata->mipi_shutdown_gpio){
		err = gpio_request(data->pdata->mipi_shutdown_gpio, "ssd-shud");
		if (err < 0) 
			dev_err(&data->spi->dev,"%s: gpio_request failed %d\n", __func__, data->pdata->mipi_shutdown_gpio);
		gpio_direction_output(data->pdata->mipi_shutdown_gpio, 0);
	}
	if(data->pdata->mipi_reset_gpio){
		err = gpio_request(data->pdata->mipi_reset_gpio, "ssd-reset");
		if (err < 0) 
			dev_err(&data->spi->dev,"%s: gpio_request failed %d\n", __func__, data->pdata->mipi_reset_gpio);
		gpio_direction_output(data->pdata->mipi_reset_gpio, 1);
	}
	return 0;
	
}
#endif
static int Ssd2825_init(struct spi_device *spi)
{
	struct ssd2828_data *data = dev_get_drvdata(&spi->dev);
	u16 vale=0;
	u16 check_w=0;
	u16 check_h=0;
	bool isConfiged = false;
	pr_err("%s:============>max rate=%d chip_selec=%d mode=%d\n",__func__,spi->max_speed_hz,spi->chip_select,spi->mode);
	static DEFINE_MUTEX(locks);

	ssd2828_write_com(0x7000B4);
	ssd2825_spi_read(0x73,&check_w);
    	pr_err("Uboot--0x7000B4 vale is: 0x%04x\n",check_w);

	ssd2828_write_com(0x7000B5);
	ssd2825_spi_read(0x73,&check_h);
    	pr_err("Uboot--0x7000B5 vale is: 0x%04x\n",check_h);
#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
	isConfiged =(check_w==SCREEN_CPT_W&&check_h==SCREEN_CPT_H);
#else
	isConfiged =(check_w==SCREEN_AUO_W&&check_h==SCREEN_AUO_H&&g_init);
#endif

	if(isConfiged){
		if (!mutex_trylock(&locks)) {
			pr_err("keenhi: %s resume trylock fail\n",__func__);
		}
		if(data->pdata->lcd_vdds_en_gpio){
			pr_err("%s:===============>ssd2828 is Init\n",__func__);
			//gpio_set_value(data->pdata->lcd_en_3v3_gpio, 1);
		}
		ssd2828_write_com(0x7000B9);
		ssd2828_write_register(0x720001);            //PLL on
		if(data->pdata->lcd_vdds_en_gpio){
			pr_err("%s:===============>ssd2828 is Init\n",__func__);
			//gpio_set_value(data->pdata->lcd_en_3v3_gpio, 0);
		}
		ssd2828_write_com(0x7000B7);
		ssd2828_write_register(0x72030B);
		mutex_unlock(&locks);
		if(data->pdata->lcd_en_3v3_gpio){
			pr_err("%s:===============>ssd2828 is Init\n",__func__);
			//gpio_set_value(data->pdata->lcd_en_3v3_gpio, 1);
		}
		return 0;
	}

	if (!mutex_trylock(&locks)) {
		pr_err("keenhi: %s trylock fail\n",__func__);
		//return -EINVAL;
	}

	if(data->reg){
		regulator_enable(data->reg);
	}

	if(data->pdata->lcd_1v2_gpio>0){
		gpio_set_value(data->pdata->lcd_1v2_gpio, 1);
	}

#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
	if(data->pdata->lcd_vdds_en_gpio){
		gpio_set_value(data->pdata->lcd_vdds_en_gpio, 1);
	}

	if(data->pdata->lcd_en_3v3_gpio){
		gpio_set_value(data->pdata->lcd_en_3v3_gpio, 1);
	}
#endif

	if(data->pdata->mipi_shutdown_gpio){
		gpio_set_value(data->pdata->mipi_shutdown_gpio, 0);
	}

	
	gpio_set_value(data->pdata->mipi_reset_gpio, 0);

     	mdelay(25);//old 25ms

    	gpio_set_value(data->pdata->mipi_reset_gpio, 1);

     	mdelay(120);

#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
	ssd2828_write_com(0x7000B1);
	ssd2828_write_register(0x720202);            //HSA = 2, VSA = 2 0x72020A
	
	ssd2828_write_com(0x7000B2);
	ssd2828_write_register(0x721030);            //VBP = 16, HBP = 48 // 0x721030

	ssd2828_write_com(0x7000B3);
	ssd2828_write_register(0x721A6C);            //VFP = 26, HFP = 108 //0x721A6C

	ssd2828_write_com(0x7000B4);
	ssd2828_write_register(0x720556);            //H active = 1366 0x720556

	ssd2828_write_com(0x7000B5);
	ssd2828_write_register(0x720300);            //V active = 768 0x720300

	ssd2828_write_com(0x7000B6);
	ssd2828_write_register(0x722007);            //B: Burst mode, 7: Non-burst mode SYNC event
	
	ssd2828_write_com(0x7000c9); //delay adj
	ssd2828_write_register(0x721805);

	ssd2828_write_com(0x7000ca); //delay adj
	ssd2828_write_register(0x722B06);

	ssd2828_write_com(0x7000DE);
	ssd2828_write_register(0x720001);            //4 Lanes

	ssd2828_write_com(0x7000D6);
	ssd2828_write_register(0x720008);            //BGR color arrangement //0x720008

	ssd2828_write_com(0x7000c4);
	ssd2828_write_register(0x720001);           //enable BTA

	ssd2828_write_com(0x7000BA);
	ssd2828_write_register(0x72C022);            //TX_CLK = 26MHz, PLL = 884MHz //21 ///15 ok

	ssd2828_write_com(0x7000BB);
	ssd2828_write_register(0x720008);            //LP freq. setting

	ssd2828_write_com(0x7000B9);
	ssd2828_write_register(0x720001);            //PLL on

	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x720343);

	ssd2828_write_com(0x7000B8);
	ssd2828_write_register(0x720000);            //VC = 00

	ssd2828_write_com(0x7000BC);
	ssd2828_write_register(0x720001);
	
	//LCD leave sleep mode
	ssd2828_write_com(0x7000BF);
	ssd2828_write_register(0x720011);

	mdelay(200);//200ms

	ssd2828_write_com(0x7000BC);
	ssd2828_write_register(0x720001);

	//LCD display on
	ssd2828_write_com(0x7000BF);
	ssd2828_write_register(0x720029);

	mdelay(200);// 200ms

	//Video mode on
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x72034B);
#else

	if(data->pdata->lcd_en_3v3_gpio){
		gpio_set_value(data->pdata->lcd_en_3v3_gpio, 1);
	}
	if(data->pdata->lcd_vdds_en_gpio){
		gpio_set_value(data->pdata->lcd_vdds_en_gpio, 1);
	}

	ssd2828_write_com(0x7000B1);
	ssd2828_write_register(0x723240);            //HSA = 2, VSA = 2 0x72020A

	ssd2828_write_com(0x7000B2);
	ssd2828_write_register(0x725078);            //VBP = 16, HBP = 48 // 0x721030

	ssd2828_write_com(0x7000B3);
	ssd2828_write_register(0x72243C);            //VFP = 26, HFP = 108 //0x721A6C

	ssd2828_write_com(0x7000B4);
	ssd2828_write_register(0x720300);            //H active = 1366 0x720556

	ssd2828_write_com(0x7000B5);
	ssd2828_write_register(0x720400);            //V active = 768 0x720300

	ssd2828_write_com(0x7000B6);
	ssd2828_write_register(0x72000B);            //B: Burst mode, 7: Non-burst mode SYNC event

	ssd2828_write_com(0x7000DE);
	ssd2828_write_register(0x720003);            //4 Lanes

	ssd2828_write_com(0x7000D6);
	ssd2828_write_register(0x720004);            //BGR color arrangement //0x720008

	ssd2828_write_com(0x7000B9);
	ssd2828_write_register(0x720000);            //PLL on

	ssd2828_write_com(0x7000BA);
	ssd2828_write_register(0x728014);            //TX_CLK = 26MHz, PLL = 884MHz //21 ///15 ok

	ssd2828_write_com(0x7000BB);
	ssd2828_write_register(0x720008);            //LP freq. setting LP CLK

	ssd2828_write_com(0x7000B9);
	ssd2828_write_register(0x720001);            //PLL on

	ssd2828_write_com(0x7000C4);
	ssd2828_write_register(0x720001);           //enable BTA

	ssd2828_write_com(0x7000B7); // enter lp mode
	ssd2828_write_register(0x720342);
	
	ssd2828_write_com(0x7000B8);
	ssd2828_write_register(0x720000);            //VC = 00

	ssd2828_write_com(0x7000BC); //set packet size
	ssd2828_write_register(0x720000);


	//LCD leave sleep mode
	ssd2828_write_com(0x7000BF);
	ssd2828_write_register(0x720011);
	
	mdelay(200);//200ms

	//LCD display on
	ssd2828_write_com(0x7000BF);
	ssd2828_write_register(0x720029);
	
	if(data->pdata->lcd_vdds_en_gpio){
		gpio_set_value(data->pdata->lcd_vdds_en_gpio, 0);
	}
	mdelay(200);// 200ms
	
	
	//Video mode on
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x72030B);
#endif
	mutex_unlock(&locks);
if(DEBUG){

	ssd2828_write_com(0x7000B0);
	ssd2825_spi_read(0x73,&vale);
    	pr_err("Uboot--0x7000B0 vale is: 0x%04x\n",vale);
	vale=0;

	ssd2828_write_com(0x7000B2);
	ssd2825_spi_read(0x73,&vale);
    	pr_err("Uboot--0x7000B2 vale is: 0x%04x\n",vale);
	vale=0;

	ssd2828_write_com(0x7000B3);
	ssd2825_spi_read(0x73,&vale);
    	pr_err("Uboot--0x7000B3 vale is: 0x%04x\n",vale);
	vale=0;

	ssd2828_write_com(0x7000B4);
	ssd2825_spi_read(0x73,&vale);
    	pr_err("Uboot--0x7000B4 vale is: 0x%04x\n",vale);
	vale=0;
	}
	g_init = 1;
	return 0;
}
static void first_init_ssd2828(){
#if(DC_CTRL_TYPE == KEENHI_DC_OUT_TYPE_RGB)
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x720343);
	
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x72034B);
#else
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x720342);
	
	ssd2828_write_com(0x7000B7);
	ssd2828_write_register(0x72030B);
#endif
	pr_err("%s:===============>first_init_ssd2828 is Init\n",__func__);
	//gpio_set_value(gpio, 1);
	
}
static int __devinit ssd2825_spi_probe(struct spi_device *spi)
{
	int error;
	struct ssd2828_data *data;
	struct ssd2828_device_platform_data *pdata = spi->dev.platform_data;
	pr_err("%s:===========================>\n",__func__);
	if (!pdata) {
		dev_err(&spi->dev, "no platform data and no register func for ssd2828 driver\n");
		return -EINVAL;
	}
	spi->bits_per_word = 24;
	spi->max_speed_hz= MAX_SPEED_RATE;
	spi->chip_select=1;
	spi->mode = SPI_MODE_0;
	error=spi_setup(spi);
	if (error < 0) {
		dev_err(&spi->dev, "spi_setup failed!\n");
		return error;
	}
	g_spi = spi;
	//first_init_ssd2828();
	data = kzalloc(sizeof(struct ssd2828_data), GFP_KERNEL);
	if (!data) {
		error = -ENOMEM;
		return error;
	}
	
	data->pdata = pdata;
	data->spi = spi;
	data->ssd_init_callback = Ssd2825_init;
	data->ssd_deinit_callback =Ssd2825_deinit;
	init_all_gpio(data);
	dev_set_drvdata(&spi->dev, data);
	if(pdata->register_resume)
	pdata->register_resume(data);

	Ssd2825_init(spi);
	pr_err("%s:=================>\n",__func__);
	return 0;
}
static int ssd2825_suspend(struct spi_device *spi, pm_message_t mesg)
{
	
	//gpio_free(112);
	return 0;
}
static int	ssd2825_resume(struct spi_device *spi){

	
	return 0;
}
static const struct spi_device_id ssd_id[] = {
	{ "ssd_2828", 0 },
	{ }
};
static struct spi_driver ssd2825_spi_driver = {
	.driver = {
			.name = "ssd_2825",
			.bus = &spi_bus_type,
			.owner = THIS_MODULE,

			},
	//.id_table	= ssd_id,
	.probe = ssd2825_spi_probe,
	.suspend= ssd2825_suspend,
	.resume= ssd2825_resume,
	.remove = __devexit_p(ssd2825_spi_remove),
};

static int __init ssd2825_spi_init(void)
{
	return spi_register_driver(&ssd2825_spi_driver);
}
//module_init(ssd2825_spi_init);
module_init(ssd2825_spi_init);
static void __exit ssd2825_spi_exit(void)
{
	
	spi_unregister_driver(&ssd2825_spi_driver);
}
module_exit(ssd2825_spi_exit);

MODULE_AUTHOR("Valentine Hsu <valentine.hsu@rad-ic.com>");
MODULE_DESCRIPTION("Raydium touchscreen SPI bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:raydium-t007");
