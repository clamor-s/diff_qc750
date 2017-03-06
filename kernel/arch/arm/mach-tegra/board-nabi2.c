/*
 * arch/arm/mach-tegra/board-kai.c
 *
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/spi/spi.h>
#include <linux/tegra_uart.h>
#include <linux/memblock.h>
#include <linux/spi-tegra.h>
#include <linux/nfc/pn544.h>
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#include <linux/regulator/consumer.h>
#include <linux/smb349-charger.h>
#include <linux/max17048_battery.h>
#include <linux/leds.h>
#include <linux/i2c/at24.h>
#include <linux/rfkill-gpio.h>

#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <mach/thermal.h>
#include <mach/tegra_fiq_debugger.h>
#include <linux/nfc/bcm2079x.h>
#include "board.h"
#include "clock.h"
#include "board-nabi2.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"
#include "wdt-recovery.h"

#include<linux/i2c/ft5x0x_ts_data.h>
#include<linux/novatek_ts_data.h>

#include <linux/simple_otg_vbus.h>

#if defined(CONFIG_SND_SOC_TEGRA_SOC_TLV320AIC325X) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_TLV320AIC325X)
	#include <mach/tegra_aic325x_pdata.h>
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5631) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5631)
	#include <mach/tegra_rt5631_pdata.h>
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5639) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5639)
	#include <mach/tegra_asoc_pdata.h>
#endif


#ifdef CONFIG_TEGRA_THERMAL_THROTTLE
static struct throttle_table throttle_freqs_tj[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1000000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  760000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  760000,  NO_CAP,  NO_CAP,  NO_CAP },
/*	      {  620000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  620000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  620000,  437000,  NO_CAP,  NO_CAP },
	      {  620000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  250000,  375000 },
	      {  475000,  352000,  250000,  375000 },
	      {  475000,  247000,  204000,  375000 },
	      {  475000,  247000,  204000,  204000 },
	      {  475000,  247000,  204000,  204000 },
	{ CPU_THROT_LOW,  247000,  204000,  102000 },*/
};
#endif

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static struct throttle_table throttle_freqs_tskin[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1000000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  760000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  760000,  NO_CAP,  NO_CAP,  NO_CAP },
/*	      {  620000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  620000,  NO_CAP,  NO_CAP,  NO_CAP },
	      {  620000,  437000,  NO_CAP,  NO_CAP },
	      {  620000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  NO_CAP,  NO_CAP },
	      {  475000,  352000,  250000,  375000 },
	      {  475000,  352000,  250000,  375000 },
	      {  475000,  247000,  204000,  375000 },
	      {  475000,  247000,  204000,  204000 },
	      {  475000,  247000,  204000,  204000 },
	{ CPU_THROT_LOW,  247000,  204000,  102000 },
	*/
};
#endif

static struct balanced_throttle throttle_list[] = {
#ifdef CONFIG_TEGRA_THERMAL_THROTTLE
	{
		.id = BALANCED_THROTTLE_ID_TJ,
		.throt_tab_size = ARRAY_SIZE(throttle_freqs_tj),
		.throt_tab = throttle_freqs_tj,
	},
#endif
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	{
		.id = BALANCED_THROTTLE_ID_SKIN,
		.throt_tab_size = ARRAY_SIZE(throttle_freqs_tskin),
		.throt_tab = throttle_freqs_tskin,
	},
#endif
};
#ifndef CONFIG_SENSORS_TMP401
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
/* Skin timer trips */
static struct tegra_thermal_timer_trip skin_therm_timer_trips[] = {
	/* duration,  temp, hysteresis */
	{       600, 48000, 5000 },
	{      1800, 45000, 5000 },
	{        -1, 43000, 8000 }, /* The duration -1 equals infinity. */
};

/* Skin active throttle size must be one. */
static struct throttle_table throttle_freqs_tskin_active_0[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1200000,  NO_CAP,  NO_CAP,  NO_CAP },
};

static struct throttle_table throttle_freqs_tskin_active_1[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1000000,  NO_CAP,  NO_CAP,  NO_CAP },
};

static struct skin_therm_active_throttle skin_therm_actives[] = {
	{
		.bthrot = {
			.id = BALANCED_THROTTLE_ID_SKIN + 1,
			.throt_tab_size =
				ARRAY_SIZE(throttle_freqs_tskin_active_0),
			.throt_tab = throttle_freqs_tskin_active_0,
		},
		.trip_temp = 33000,
		.hysteresis = 0,
	},
	{
		.bthrot = {
			.id = BALANCED_THROTTLE_ID_SKIN + 2,
			.throt_tab_size =
				ARRAY_SIZE(throttle_freqs_tskin_active_1),
			.throt_tab = throttle_freqs_tskin_active_1,
		},
		.trip_temp = 38000,
		.hysteresis = 0,
	},
};
#endif

/* All units are in millicelsius */
static struct tegra_thermal_data thermal_data = {
	.shutdown_device_id = THERMAL_DEVICE_ID_NCT_EXT,
	.temp_shutdown = 90000,
#if defined(CONFIG_TEGRA_EDP_LIMITS) || defined(CONFIG_TEGRA_THERMAL_THROTTLE)
	.throttle_edp_device_id = THERMAL_DEVICE_ID_NCT_EXT,
#endif
#ifdef CONFIG_TEGRA_EDP_LIMITS
	.edp_offset = TDIODE_OFFSET,  /* edp based on tdiode */
	.hysteresis_edp = 3000,
#endif
#ifdef CONFIG_TEGRA_THERMAL_THROTTLE
	.temp_throttle = 85000,
	.tc1 = 0,
	.tc2 = 1,
	.passive_delay = 2000,
#endif
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	.skin_device_id = THERMAL_DEVICE_ID_SKIN,
	.temp_throttle_skin = 43000,
	.skin_timer_trip_data = {
		.trips = skin_therm_timer_trips,
		.trip_size = ARRAY_SIZE(skin_therm_timer_trips),
	},

	.skin_actives = skin_therm_actives,
	.skin_active_size = ARRAY_SIZE(skin_therm_actives),
#endif
};
#else
/* Skin active throttle size must be one. */
static struct throttle_table throttle_freqs_tskin_active_0[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1200000,  NO_CAP,  NO_CAP,  NO_CAP },
};

static struct throttle_table throttle_freqs_tskin_active_1[] = {
	      /*    CPU,    CBUS,    SCLK,     EMC */
	      { 1000000,  NO_CAP,  NO_CAP,  NO_CAP },
};
static struct skin_therm_active_throttle skin_therm_actives[] = {
	{
		.bthrot = {
			.id = BALANCED_THROTTLE_ID_SKIN + 1,
			.throt_tab_size =
				ARRAY_SIZE(throttle_freqs_tskin_active_0),
			.throt_tab = throttle_freqs_tskin_active_0,
		},
		.trip_temp = 33000,
		.hysteresis = 0,
	},
	{
		.bthrot = {
			.id = BALANCED_THROTTLE_ID_SKIN + 2,
			.throt_tab_size =
				ARRAY_SIZE(throttle_freqs_tskin_active_1),
			.throt_tab = throttle_freqs_tskin_active_1,
		},
		.trip_temp = 38000,
		.hysteresis = 0,
	},
};

/* All units are in millicelsius */
static struct tegra_thermal_data thermal_data = {
	.shutdown_device_id = THERMAL_DEVICE_ID_NCT_EXT,
	.temp_shutdown = 90000,
#if defined(CONFIG_TEGRA_EDP_LIMITS) || defined(CONFIG_TEGRA_THERMAL_THROTTLE)
	.throttle_edp_device_id = THERMAL_DEVICE_ID_NCT_EXT,
#endif
#ifdef CONFIG_TEGRA_EDP_LIMITS
	.edp_offset = TDIODE_OFFSET,  /* edp based on tdiode */
	.hysteresis_edp = 3000,
#endif
#ifdef CONFIG_TEGRA_THERMAL_THROTTLE
	.temp_throttle = 85000,
	.tc1 = 0,
	.tc2 = 1,
	.passive_delay = 2000,
#endif
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	.skin_device_id = THERMAL_DEVICE_ID_SKIN,
	.temp_throttle_skin = 45000, //45000 wlp modify 20130903
        .tc1_skin = 0,
        .tc2_skin = 1,
        .passive_delay_skin = 5000,

	 .skin_actives = skin_therm_actives,
	 .skin_active_size = ARRAY_SIZE(skin_therm_actives),
        .skin_temp_offset = 5680,  // 8066,
        .skin_period = 2100,
        .skin_devs_size = 2,
       .skin_devs = {
        {
                "nct_ext_coeff",
                THERMAL_DEVICE_ID_NCT_EXT,
                {
                	5, 2, 0 ,-2 ,-2, -1, -2 ,-4, 0, -2, 2, 0, -1, -2 ,-3 ,-1, -1, -2, -2, -11,
                //      -36, -3, 4, 5, 4, 3, 0, -2,0, 2, 2, -2, -1, -1, 1, 0, 0, -3, -8, -45,
                }
        },
        {
                "nct_int_coeff",
                THERMAL_DEVICE_ID_NCT_INT,
                {
                     20, 7, 3, 3, 2, 3, 3, 3, 2, 2, 4, 4, 3, 3, 3 ,1, 0 ,3 ,7, 26,
              //        23, 12, 7, 7, 9, 9, 7, 7, 10, 4, 8, 9, 8, 5, 7, 5, 3, 1, 2, 0,
                }
        },
	 },

#endif
};

#endif
/* wl128x BT, FM, GPS connectivity chip */
struct ti_st_plat_data ti_wilink_pdata = {
	.nshutdown_gpio = TEGRA_GPIO_PU0,
	.dev_name = BLUETOOTH_UART_DEV_NAME,
	.flow_cntrl = 1,
	.baud_rate = 3000000,
};

static struct platform_device ti_wl128x_device = {
	.name		= "kim",
	.id		= -1,
	.dev.platform_data = &ti_wilink_pdata,
};
#if 0
static struct platform_device ti_btwilink_device = {
	.name = "btwilink",
	.id = -1,
};
#endif
static noinline void __init ti_bt_st(void)
{
	platform_device_register(&ti_wl128x_device);
	//platform_device_register(&ti_btwilink_device);
}

static struct resource ti_bluesleep_resources[] = {
	[0] = {
		.name = "gpio_host_wake",
			.start  = TEGRA_GPIO_PS6,
			.end    = TEGRA_GPIO_PS6,
			.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
			.start	= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PS6),
			.end	= TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PS6),
			.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};


static struct platform_device ti_bluesleep_device = {
	.name		= "bluesleep",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ti_bluesleep_resources),
	.resource	= ti_bluesleep_resources,
};


static noinline void __init ti_tegra_setup_tibluesleep(void)
{
	platform_device_register(&ti_bluesleep_device);
}

static struct resource cardhu_bluedroid_pm_resources[] = {
	[0] = {
		.name   = "shutdown_gpio",
		.start  = TEGRA_GPIO_PD0,
		.end    = TEGRA_GPIO_PD0,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
	[2] = {
		.name = "gpio_ext_wake",
		.start  = TEGRA_GPIO_PU1,
		.end    = TEGRA_GPIO_PU1,
		.flags  = IORESOURCE_IO,
	},
	[3] = {
		.name = "gpio_host_wake",
		.start  = TEGRA_GPIO_PU6,
		.end    = TEGRA_GPIO_PU6,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device cardhu_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(cardhu_bluedroid_pm_resources),
	.resource       = cardhu_bluedroid_pm_resources,
};

static noinline void __init cardhu_setup_bluedroid_pm(void)
{
	cardhu_bluedroid_pm_resources[1].start =
		cardhu_bluedroid_pm_resources[1].end =
				gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&cardhu_bluedroid_pm_device);
	return;
}

static __initdata struct tegra_clk_init_table kai_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	3187500,	true},
	{ "blink",	"clk_32k",	32768,		true},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "i2s4",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"clk_m",	12000000,	false},
	{ "dam0",	"clk_m",	12000000,	false},
	{ "dam1",	"clk_m",	12000000,	false},
	{ "dam2",	"clk_m",	12000000,	false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	{ "vi_sensor",	"pll_p",	150000000,	false},
	{ "i2c1",	"pll_p",	3200000,	false},
	{ "i2c2",	"pll_p",	3200000,	false},
	{ "i2c3",	"pll_p",	3200000,	false},
	{ "i2c4",	"pll_p",	3200000,	false},
	{ "i2c5",	"pll_p",	3200000,	false},
	{ NULL,		NULL,		0,		0},
};
/*
static struct pn544_i2c_platform_data nfc_pdata = {
	.irq_gpio = TEGRA_GPIO_PX0,
	.ven_gpio = TEGRA_GPIO_PS7,
	.firm_gpio = TEGRA_GPIO_PR3,
};

static struct i2c_board_info __initdata kai_nfc_board_info[] = {
	{
		I2C_BOARD_INFO("pn544", 0x28),
		.platform_data = &nfc_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PX0),
	},
};
*/
//#ifdef CONFIG_BCM2079X_I2C
static struct bcm2079x_platform_data bcm2079x_pdata = {
	.irq_gpio =TEGRA_GPIO_PV1, //TEGRA_GPIO_PU6,
	.en_gpio = TEGRA_GPIO_PY2,
	.wake_gpio = TEGRA_GPIO_PU0,
};

static struct i2c_board_info i2c_devs14[] __initdata = {
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PV1),
		.platform_data = &bcm2079x_pdata,
		//.flags = I2C_FUNC_10BIT_ADDR,
	},
};
//#endif

static struct tegra_i2c_platform_data kai_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PC4, 0},
	.sda_gpio		= {TEGRA_GPIO_PC5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data kai_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.is_clkon_always = true,
	.scl_gpio		= {TEGRA_GPIO_PT5, 0},
	.sda_gpio		= {TEGRA_GPIO_PT6, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data kai_i2c3_platform_data = {
	.adapter_nr	= 2,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PBB1, 0},
	.sda_gpio		= {TEGRA_GPIO_PBB2, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data kai_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 10000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PV4, 0},
	.sda_gpio		= {TEGRA_GPIO_PV5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data kai_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PZ6, 0},
	.sda_gpio		= {TEGRA_GPIO_PZ7, 0},
	.arb_recovery = arb_lost_recovery,
};

struct max17048_battery_model max17048_mdata = {
	.rcomp          = 170,
	.soccheck_A     = 252,
	.soccheck_B     = 254,
	.bits           = 19,
	.alert_threshold = 0x00,
	.one_percent_alerts = 0x40,
	.alert_on_reset = 0x40,
	.rcomp_seg      = 0x0800,
	.hibernate      = 0x3080,
	.vreset         = 0x9696,
	.valert         = 0xD4AA,
	.ocvtest        = 55600,
};

static struct i2c_board_info kai_i2c4_max17048_board_info[] = {
	{
		I2C_BOARD_INFO("max17048", 0x36),
		.platform_data = &max17048_mdata,
	},
};



#if defined(CONFIG_SND_SOC_TEGRA_SOC_TLV320AIC325X) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_TLV320AIC325X)

static struct tegra_aic325x_platform_data kai_tlv320aic325x_pdata = {
        .irq_active_low = 0,
        .micdet_cfg = 0,
        .micdet_delay = 100,
        .gpio_spkr_en = TEGRA_GPIO_SPKR_EN,
        .gpio_aic325x_reset = TEGRA_GPIO_RT5631_RST,
};

static struct i2c_board_info __initdata tlv320aic325x_board_info = {

		I2C_BOARD_INFO("tlv320aic325x", 0x18),
		.platform_data=&kai_tlv320aic325x_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_HP_DET),
};
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5631) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5631)
	static struct i2c_board_info __initdata rt5631_board_info = {
	I2C_BOARD_INFO("rt5631", 0x1a),
	.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_CDC_IRQ),
};
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5639) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5639)
	static struct i2c_board_info __initdata rt5639_board_info = {
	I2C_BOARD_INFO("rt5639", 0x1c),
	.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_CDC_IRQ),
};
#endif

static void kai_i2c_init(void)
{
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	tegra_i2c_device1.dev.platform_data = &kai_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &kai_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &kai_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &kai_i2c4_platform_data;
	tegra_i2c_device5.dev.platform_data = &kai_i2c5_platform_data;

	platform_device_register(&tegra_i2c_device5);
	platform_device_register(&tegra_i2c_device4);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device1);

	//i2c_register_board_info(4, kai_i2c4_smb349_board_info,
	//	ARRAY_SIZE(kai_i2c4_smb349_board_info));

#if defined(CONFIG_SND_SOC_TEGRA_SOC_TLV320AIC325X) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_TLV320AIC325X)
	if(machine_is_nabi2_xd() || machine_is_nabi_2s() ||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700() || machine_is_itq701()  || machine_is_mm3201()
		|| machine_is_n1010() || machine_is_n750() || machine_is_birch() || machine_is_wikipad() ||machine_is_ns_14t004()){
		i2c_register_board_info(4,  &tlv320aic325x_board_info, 1);
		}
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5631) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5631)
	if( machine_is_nabi2_3d() || machine_is_nabi2())
		i2c_register_board_info(4, &rt5631_board_info, 1);
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5639) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5639)
		i2c_register_board_info(4, &rt5639_board_info, 1);
#endif
	//i2c_register_board_info(4, &kai_eeprom_mac_add, 1);

	i2c_register_board_info(4, kai_i2c4_max17048_board_info,
		ARRAY_SIZE(kai_i2c4_max17048_board_info));

	//i2c_register_board_info(0, kai_nfc_board_info, 1);
	
	//register BCM20793B3
	if(machine_is_nabi2_xd()||machine_is_nabi_2s()){
		printk("i2c init bcm20793B3 device \n");
		i2c_register_board_info(0, i2c_devs14, ARRAY_SIZE(i2c_devs14));
	}
}

static struct platform_device *kai_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
	&tegra_uarte_device,
};
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "clk_m"},
	[1] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[2] = {.name = "pll_m"},
#endif
};

static struct tegra_uart_platform_data kai_uart_pdata;
static struct tegra_uart_platform_data kai_loopback_uart_pdata;

static void __init uart_debug_init(void)
{
	int debug_port_id;

	debug_port_id = get_tegra_uart_debug_port_id();
	if (debug_port_id < 0)
		debug_port_id = 3;

	switch (debug_port_id) {
	case 0:
		/* UARTA is the debug port. */
		pr_info("Selecting UARTA as the debug console\n");
		kai_uart_devices[0] = &debug_uarta_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uarta");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uarta_device.dev.platform_data))->mapbase;
		break;

	case 1:
		/* UARTB is the debug port. */
		pr_info("Selecting UARTB as the debug console\n");
		kai_uart_devices[1] = &debug_uartb_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uartb");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uartb_device.dev.platform_data))->mapbase;
		break;

	case 2:
		/* UARTC is the debug port. */
		pr_info("Selecting UARTC as the debug console\n");
		kai_uart_devices[2] = &debug_uartc_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uartc");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uartc_device.dev.platform_data))->mapbase;
		break;

	case 3:
		/* UARTD is the debug port. */
		pr_info("Selecting UARTD as the debug console\n");
		kai_uart_devices[3] = &debug_uartd_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uartd");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uartd_device.dev.platform_data))->mapbase;
		break;

	case 4:
		/* UARTE is the debug port. */
		pr_info("Selecting UARTE as the debug console\n");
		kai_uart_devices[4] = &debug_uarte_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uarte");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uarte_device.dev.platform_data))->mapbase;
		break;

	default:
		pr_info("The debug console id %d is invalid, Assuming UARTA",
			debug_port_id);
		kai_uart_devices[0] = &debug_uarta_device;
		debug_uart_clk = clk_get_sys("serial8250.0", "uarta");
		debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uarta_device.dev.platform_data))->mapbase;
		break;
	}
}

static void __init kai_uart_init(void)
{
	struct clk *c;
	int i;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	kai_uart_pdata.parent_clk_list = uart_parent_clk;
	kai_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	kai_loopback_uart_pdata.parent_clk_list = uart_parent_clk;
	kai_loopback_uart_pdata.parent_clk_count =
						ARRAY_SIZE(uart_parent_clk);
	kai_loopback_uart_pdata.is_loopback = true;
	tegra_uarta_device.dev.platform_data = &kai_uart_pdata;
	tegra_uartb_device.dev.platform_data = &kai_uart_pdata;
	tegra_uartc_device.dev.platform_data = &kai_uart_pdata;
	tegra_uartd_device.dev.platform_data = &kai_uart_pdata;
	/* UARTE is used for loopback test purpose */
	tegra_uarte_device.dev.platform_data = &kai_loopback_uart_pdata;

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs()) {
		uart_debug_init();
		/* Clock enable for the debug channel */
		if (!IS_ERR_OR_NULL(debug_uart_clk)) {
			pr_info("The debug console clock name is %s\n",
						debug_uart_clk->name);
			c = tegra_get_clock_by_name("pll_p");
			if (IS_ERR_OR_NULL(c))
				pr_err("Not getting the parent clock pll_p\n");
			else
				clk_set_parent(debug_uart_clk, c);

			clk_enable(debug_uart_clk);
			clk_set_rate(debug_uart_clk, clk_get_rate(c));
		} else {
			pr_err("Not getting the clock %s for debug console\n",
					debug_uart_clk->name);
		}
	}

	platform_add_devices(kai_uart_devices,
				ARRAY_SIZE(kai_uart_devices));
}

static struct platform_device tegra_camera = {
	.name = "tegra_camera",
	.id = -1,
};

static struct platform_device *kai_spi_devices[] __initdata = {
	&tegra_spi_device1,
};
static struct platform_device *nabi2_xd_spi_devices[] __initdata = {
	&tegra_spi_device2,
};
static struct spi_clk_parent spi_parent_clk[] = {
	[0] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[1] = {.name = "pll_m"},
	[2] = {.name = "clk_m"},
#else
	[1] = {.name = "clk_m"},
#endif
};

static struct tegra_spi_platform_data kai_spi1_pdata = {
		.is_dma_based           = true,
		.max_dma_buffer         = (128),
		.is_clkon_always        = false,
		.max_rate               = 100000000,
};

static void __init kai_spi_init(void)
{
	int i;
	struct clk *c;

	for (i = 0; i < ARRAY_SIZE(spi_parent_clk); ++i) {
		c = tegra_get_clock_by_name(spi_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						spi_parent_clk[i].name);
			continue;
		}
		spi_parent_clk[i].parent_clk = c;
		spi_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}

	kai_spi1_pdata.parent_clk_list = spi_parent_clk;
	kai_spi1_pdata.parent_clk_count = ARRAY_SIZE(spi_parent_clk);
	tegra_spi_device1.dev.platform_data = &kai_spi1_pdata;
	if(machine_is_nabi2_xd())
		platform_add_devices(nabi2_xd_spi_devices,
					ARRAY_SIZE(nabi2_xd_spi_devices));
	else
		platform_add_devices(kai_spi_devices,
					ARRAY_SIZE(kai_spi_devices));

}


static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};


#if defined(CONFIG_SND_SOC_TEGRA_SOC_TLV320AIC325X) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_TLV320AIC325X)

	static struct tegra_aic325x_platform_data kai_audio_ti_pdata = {
	        .gpio_spkr_en           = TEGRA_GPIO_SPKR_EN,
	        .gpio_hp_det            = TEGRA_GPIO_HP_DET,
	        .gpio_hp_mute           = -1,
	        .gpio_int_mic_en        = -1,
	        .gpio_ext_mic_en        = -1,
	        .gpio_aic325x_reset = TEGRA_GPIO_RT5631_RST,
	        .i2s_param[HIFI_CODEC]	= {
		.audio_port_id	= 0,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_I2S,
	},
	//	.audio_port_id[NUM_I2S_DEVICES] = 1,
	};


	static struct platform_device kai_audio_ti_device = {
		.name	= "tegra-snd-aic325x",
		.id	= 0,
		.dev	= {
			.platform_data  = &kai_audio_ti_pdata,
		},
	};
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5631) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5631)

	static struct tegra_rt5631_platform_data kai_audio_pdata = {
		.gpio_spkr_en		= TEGRA_GPIO_SPKR_EN,
		.gpio_hp_det		= TEGRA_GPIO_HP_DET,
		.gpio_hp_mute		= -1,
		.gpio_int_mic_en	= TEGRA_GPIO_INT_MIC_EN,
		.gpio_ext_mic_en	= TEGRA_GPIO_EXT_MIC_EN,
	};

	static struct platform_device kai_audio_device = {
		.name	= "tegra-snd-rt5631",
		.id	= 0,
		.dev	= {
			.platform_data = &kai_audio_pdata,
		},
	};
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5639) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5639)
static struct tegra_asoc_platform_data kai_audio_pdata = {
	.gpio_spkr_en		= TEGRA_GPIO_SPKR_EN,
	.gpio_hp_det		= TEGRA_GPIO_HP_DET,
	.gpio_hp_mute		= -1,
	.gpio_int_mic_en	= TEGRA_GPIO_INT_MIC_EN,
	.gpio_ext_mic_en	= TEGRA_GPIO_EXT_MIC_EN,
	.gpio_ldo1_en		= TEGRA_GPIO_PX2,
	.i2s_param[HIFI_CODEC]	= {
		.audio_port_id	= 0,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_I2S,
	},

};

static struct platform_device kai_audio_device = {
	.name	= "tegra-snd-rt5639",
	.id	= 0,
	.dev	= {
		.platform_data = &kai_audio_pdata,
	},
};
#endif

static struct gpio_led kai_led_info[] = {
	{
		.name			= "statled",
		.default_trigger	= "default-on",
		.gpio			= TEGRA_GPIO_STAT_LED,
		.active_low		= 1,
		.retain_state_suspended	= 0,
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data kai_leds_pdata = {
	.leds		= kai_led_info,
	.num_leds	= ARRAY_SIZE(kai_led_info),
};

static struct platform_device kai_leds_gpio_device = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data = &kai_leds_pdata,
	},
};

#if defined(CONFIG_MRVLSD8787_RFKILL)
static struct platform_device kai_mrvl8787_rfkill_device = {
	.name = "mrvlsd8787_rfkill",
	.id 			= -1,
};
#endif

static struct platform_device *kai_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_IOVMM_SMMU) || defined(CONFIG_TEGRA_IOMMU_SMMU)
	&tegra_smmu_device,
#endif
	&tegra_wdt0_device,
	&tegra_wdt1_device,
	&tegra_wdt2_device,
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
	&tegra_camera,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device1,
	&tegra_i2s_device3,
	&tegra_i2s_device4,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
#if defined(CONFIG_MRVLSD8787_RFKILL)
	&kai_mrvl8787_rfkill_device,
#endif
	&tegra_pcm_device,
	&kai_leds_gpio_device,
	&tegra_hda_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
};

static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = false,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio =-1,
		.vbus_reg ="usb_vbus_otg",
		.hot_plug = true,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 15,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};


static struct tegra_usb_platform_data tegra_ehci2_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode	= TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.vbus_reg = NULL,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,

	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
    .port_otg = false,
    .has_hostpc = true,
    .phy_intf = TEGRA_USB_PHY_INTF_UTMI,
    .op_mode	= TEGRA_USB_OPMODE_HOST,
    .u_data.host = {
        .vbus_gpio = -1,
        .vbus_reg = NULL,
        .hot_plug = true,
        .remote_wakeup_supported = true,
        .power_off_on_suspend = true,
    },
    .u_cfg.utmi = {
        .hssync_start_delay = 0,
        .elastic_limit = 16,
        .idle_wait_delay = 17,
        .term_range_adj = 6,
        .xcvr_setup = 8,
        .xcvr_lsfslew = 2,
        .xcvr_lsrslew = 2,
        .xcvr_setup_offset = 0,
        .xcvr_use_fuses = 1,
    },
};


static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
};

#if CONFIG_USB_SUPPORT
static void kai_usb_init(void)
{
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	/* Setup the udc platform data */
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;

	tegra_ehci2_device.dev.platform_data = &tegra_ehci2_utmi_pdata;
	platform_device_register(&tegra_ehci2_device);

	if(machine_is_qc750())
	{
	    tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
	    platform_device_register(&tegra_ehci3_device);
	}
}

#else
static void kai_usb_init(void) { }
static void kai_modem_init(void) { }
#endif

static void kai_audio_init(void)
{
	struct board_info board_info;

	tegra_get_board_info(&board_info);

#if defined(CONFIG_SND_SOC_TEGRA_SOC_TLV320AIC325X) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_TLV320AIC325X)	
	if(machine_is_nabi2_xd() || machine_is_nabi_2s() ||machine_is_qc750() ||  machine_is_n710() || machine_is_itq700() || machine_is_itq701() ||machine_is_mm3201()
		|| machine_is_n1010() || machine_is_n750() || machine_is_birch() || machine_is_wikipad() || machine_is_ns_14t004()) {
		kai_audio_ti_pdata.codec_name = "tlv320aic325x.4-0018";
		kai_audio_ti_pdata.codec_dai_name = "TLV320AIC325x";	
		platform_device_register(&kai_audio_ti_device);
		 if(machine_is_birch()){
                   device_init_wakeup(&kai_audio_ti_device.dev, 1);
                   device_set_wakeup_enable(&kai_audio_ti_device.dev, 1);
              }

	}
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5631) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5631)
	if(machine_is_nabi2_3d() || machine_is_nabi2()){
		kai_audio_pdata.codec_name = "rt5631.4-001a";
		kai_audio_pdata.codec_dai_name = "RT5631-HIFI";
		platform_device_register(&kai_audio_device);
		}
#endif
#if defined(CONFIG_SND_SOC_TEGRA_RT5639) && defined(CONFIG_MACH_HAS_SND_SOC_TEGRA_RT5639)
		kai_audio_pdata.codec_name = "rt5639.4-001c";
		kai_audio_pdata.codec_dai_name = "rt5639-aif1";
		platform_device_register(&kai_audio_device);
#endif

}


static struct FT5X0X_i2c_ts_platform_data FT5X0X_data = {
      .gpio_shutdown = TEGRA_GPIO_PK7,
};

static const struct i2c_board_info ventana_i2c_bus1_FT5X0X_info[] = {
	{
	 I2C_BOARD_INFO("ft5x0x_ts", 0x38),           //0x70),     
	 .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PJ0),
	 .platform_data=&FT5X0X_data,
	 },
};
#ifdef CONFIG_TOUCHSCREEN_FT5X0X
static int __init kai_touch_init_Focaltech(void)
{

	//tegra_gpio_enable(TEGRA_GPIO_PD2);//wantianpei add  

	i2c_register_board_info(1, ventana_i2c_bus1_FT5X0X_info, 1);

	//gpio_request(TEGRA_GPIO_PD2, "tp_enable");
	//gpio_direction_output(TEGRA_GPIO_PD2, 1);
	//gpio_set_value(TEGRA_GPIO_PD2,1);
	
	gpio_request(TEGRA_GPIO_TOUCH_DETECT, "tp_detect");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_0, "tp_detect0");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_0);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_1, "tp_detect1");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_1);	
	
	gpio_request(TEGRA_GPIO_TOUCH_DETECT_2, "tp_detect2");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_2);
	
	gpio_request(TEGRA_GPIO_PK7, "tp_enable2");
	gpio_direction_output(TEGRA_GPIO_PK7, 1);
	gpio_set_value(TEGRA_GPIO_PK7,1);

	return 0;
}
#endif

#if defined (CONFIG_TOUCHSCREEN_NT11003_2)


static int nt1103_detect(void)
{
	int  touch_detect = gpio_get_value(TEGRA_GPIO_TOUCH_DETECT);
	int touch_detect1 =  gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_0);
	int touch_detect2=  gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_1);
	int touch_detect3= gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_2);
	int status;
	
	status = (touch_detect<<3) |(touch_detect1 <<2) |(touch_detect2<<1) |(touch_detect3) ;
	
	return status;
}
static  struct novatek_i2c_platform_data  nt1103_ts_data={
	.detect = nt1103_detect,	
};
static const struct i2c_board_info ventana_i2c_bus1_NT11003_info[] = {
	{
	 I2C_BOARD_INFO("nt1103-ts", 0x01),           //0x70),     
	 .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PJ0),
	 .platform_data = &nt1103_ts_data,
	},
};

static int __init NT11003_touch_init(void)
{
	printk("=====touch====\n");
	gpio_request(TEGRA_GPIO_PK7, "tp_rst");
	gpio_direction_output(TEGRA_GPIO_PK7, 1);
	
	gpio_request(TEGRA_GPIO_PJ0, "tp_int");
	gpio_direction_input(TEGRA_GPIO_PJ0);	


	gpio_request(TEGRA_GPIO_PZ3, "tp_rst18");
	gpio_direction_input(TEGRA_GPIO_PZ3);
	
	gpio_request(TEGRA_GPIO_PN5, "tp_int18");
	gpio_direction_input(TEGRA_GPIO_PN5);	


	
	gpio_request(TEGRA_GPIO_TOUCH_DETECT, "tp_detect");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_0, "tp_detect0");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_0);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_1, "tp_detect1");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_1);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_2, "tp_detect2");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_2);
	
	i2c_register_board_info(1, ventana_i2c_bus1_NT11003_info, 1);

	return 0;
}


#endif

#if defined (CONFIG_TOUCHSCREEN_GOODIX_GT9XX)
static const struct i2c_board_info ventana_i2c_bus1_Goodix_GT9XX[] = {
	{
	 I2C_BOARD_INFO("Goodix-TS", (0x5d)),           //0x70),     
	// .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PJ0),
	// .platform_data = &nt1103_ts_data,
	},
};

static int __init Goodix_GT9XX_touch_init(void)
{
//init gpio for auto detect which TP is used
	gpio_request(TEGRA_GPIO_TOUCH_DETECT, "tp_detect");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_0, "tp_detect0");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_0);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_1, "tp_detect1");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_1);	
	
	gpio_request(TEGRA_GPIO_TOUCH_DETECT_2, "tp_detect2");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_2);

	i2c_register_board_info(1, ventana_i2c_bus1_Goodix_GT9XX, 1);

	return 0;
}


#endif

#ifdef CONFIG_SIMPLE_OTG_VBUS

static  const struct simple_otg_vbus mOtgVbusData ={
	.mVbusPin =TEGRA_GPIO_PH1,
};

static struct platform_device simple_otg_device = {
	.name = "simple_otg_vbus",
	.id   = -1,
	.dev	= {
		.platform_data = &mOtgVbusData,
	},
};
#endif

static void __init tegra_usb_vbus(void)
{
	gpio_request(TEGRA_GPIO_PH1, "init_vbus");
	gpio_direction_output(TEGRA_GPIO_PH1, 0);
	gpio_free(TEGRA_GPIO_PH1);

#ifdef CONFIG_SIMPLE_OTG_VBUS
	platform_device_register(&simple_otg_device);
#endif

}




static int focal_or_novatek_tp_detect(void)
{
	int  touch_detect,touch_detect1,touch_detect2,touch_detect3;
	int status;
		gpio_request(TEGRA_GPIO_TOUCH_DETECT, "tp_detect");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_0, "tp_detect0");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_0);	

	gpio_request(TEGRA_GPIO_TOUCH_DETECT_1, "tp_detect1");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_1);	
	
	gpio_request(TEGRA_GPIO_TOUCH_DETECT_2, "tp_detect2");
	gpio_direction_input(TEGRA_GPIO_TOUCH_DETECT_2);
	touch_detect = gpio_get_value(TEGRA_GPIO_TOUCH_DETECT);
	touch_detect1 =  gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_0);
	touch_detect2=  gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_1);
	touch_detect3= gpio_get_value(TEGRA_GPIO_TOUCH_DETECT_2);
	
	
	status = (touch_detect<<3) |(touch_detect1 <<2) |(touch_detect2<<1) |(touch_detect3) ;
	
	return status;
}

static void __init tegra_kai_init(void)
{

	tegra_thermal_init(&thermal_data,
				throttle_list,
				ARRAY_SIZE(throttle_list));

	tegra_clk_init_from_table(kai_clk_init_table);

	if(machine_is_n710()){
		tegra_soc_device_init("N710");
	} else if(machine_is_qc750()){
		tegra_soc_device_init("QC750");
	} else if(machine_is_itq700()){
		tegra_soc_device_init("itq700");
}
	else if(machine_is_mm3201()){
		tegra_soc_device_init("mm3201");
	}
	else if(machine_is_itq701())
	{	
		tegra_soc_device_init("itq701");
	}
	
	kai_pinmux_init();
	kai_i2c_init();
       kai_spi_init();
	tegra_usb_vbus();
	kai_usb_init();
#ifdef CONFIG_TEGRA_EDP_LIMITS
	kai_edp_init();
#endif
	kai_uart_init();
	kai_audio_init();
	platform_add_devices(kai_devices, ARRAY_SIZE(kai_devices));
	tegra_ram_console_debug_init();
	tegra_io_dpd_init();
	kai_sdhci_init();
	kai_regulator_init();
	kai_suspend_init();
	if(machine_is_birch())//support novatek and focal
	{
		switch(focal_or_novatek_tp_detect())
		{
			case 0x0c://edt focal ic
			{
				#ifdef CONFIG_TOUCHSCREEN_FT5X0X
				kai_touch_init_Focaltech();
				#endif
			}
			break;
			case 0x0f://rtr novatek ic
			case 0x08://edt novatek ic
			{
				#ifdef CONFIG_TOUCHSCREEN_NT11003_2	
				NT11003_touch_init();
				#endif
			}
			break;
			default:
			break;
		}
	}
	else
	{
#ifdef CONFIG_TOUCHSCREEN_FT5X0X
	kai_touch_init_Focaltech();
#endif
#ifdef CONFIG_TOUCHSCREEN_NT11003_2	
		NT11003_touch_init();
#endif
#ifdef CONFIG_TOUCHSCREEN_GOODIX_GT9XX	
			Goodix_GT9XX_touch_init();
#endif
	}
	kai_keys_init();
	kai_panel_init();
	if(machine_is_nabi2_xd() || machine_is_nabi_2s() || machine_is_nabi2_3d() ||machine_is_nabi2() || machine_is_wikipad()) {
//		bcm_bt_rfkill_init();
//		bcm_setup_bluesleep();
		cardhu_setup_bluedroid_pm();
	} else {
		ti_bt_st();
		ti_tegra_setup_tibluesleep();
	}
	//kai_nfc_init();
	kai_sensors_init();
	
	kai_pins_state_init();
	kai_emc_init();
	//tegra_release_bootloader_fb();
	//kai_modem_init();
#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif
	tegra_serial_debug_init(TEGRA_UARTD_BASE, INT_WDT_CPU, NULL, -1, -1);
	
}

static void __init kai_ramconsole_reserve(unsigned long size)
{
	tegra_ram_console_debug_reserve(SZ_1M);
}

static void __init tegra_kai_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	/* support 1920X1200 with 24bpp */
	tegra_reserve(0, SZ_8M + SZ_1M, SZ_8M + SZ_1M);
#else
	tegra_reserve(SZ_128M, SZ_8M, SZ_8M);
#endif
	kai_ramconsole_reserve(SZ_1M);
}

MACHINE_START(NABI2, "MT799")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(NABI2_3D, "NABI2_3D")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(NABI2_XD, "NABI2_XD")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(NABI_2S, "NABI_2S")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(QC750, "QC750")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(N710, "N710")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(N1010, "N1010")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(N750, "N750")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(WIKIPAD, "WIKIPAD")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(ITQ700, "ITQ700")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END

MACHINE_START(BIRCH, "BIRCH")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END
MACHINE_START(MM3201, "MM3201")
        .boot_params    = 0x80000100,
        .map_io         = tegra_map_common_io,
        .reserve        = tegra_kai_reserve,
        .init_early     = tegra_init_early,
        .init_irq       = tegra_init_irq,
        .timer          = &tegra_timer,
        .init_machine   = tegra_kai_init,
MACHINE_END
MACHINE_START(NS_14T004, "NS_14T004")
	.boot_params	= 0x80000100,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_kai_reserve,
	.init_early	= tegra_init_early,
	.init_irq	= tegra_init_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_kai_init,
MACHINE_END
MACHINE_START(ITQ701, "ITQ701")
        .boot_params    = 0x80000100,
        .map_io         = tegra_map_common_io,
        .reserve        = tegra_kai_reserve,
        .init_early     = tegra_init_early,
        .init_irq       = tegra_init_irq,
        .timer          = &tegra_timer,
        .init_machine   = tegra_kai_init,
MACHINE_END
