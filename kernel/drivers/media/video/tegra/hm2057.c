/*
 * kernel/drivers/media/video/tegra
 *
 * Aptina MT9D115 sensor driver
 *
 * Copyright (C) 2010 NVIDIA Corporation
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

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "hm2057.h"
#include <linux/hrtimer.h>
#include <asm/mach-types.h>

#include <linux/platform_device.h>
#include <linux/syscalls.h>
#define DRIVER_VERSION "0.0.1"
char mDFileName[] = "/data/camera/effect.txt";
#define _debug_is 0
#if _debug_is
#define debug_print(...) pr_err(__VA_ARGS__)
#else
#define debug_print(...) 
#endif


/** Macro for determining the size of an array */
#define KH_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct sensor_reg {
	u16 addr;
	u8 val;
};

struct sensor_info {
	int mode;
	int current_wb;
	int current_exposure;
	u16 current_addr;
	atomic_t power_on_loaded;
	atomic_t sensor_init_loaded;
	struct mutex als_get_sensor_value_mutex;
	struct i2c_client *i2c_client;
	struct yuv_sensor_platform_data *pdata;
};

static struct sensor_info *gc0308_sensor_info;
static struct sensor_reg mode_common[] = {
{0x0022,0x00},// Reset
{0x0004,0x10},//
{0x0006,0x03},// Flip/Mirror
{0x000D,0x11},//; 20120220 to fix morie
{0x000E,0x11},// Binning ON
{0x000F,0x00},// IMGCFG
{0x0011,0x02},//
{0x0012,0x1C},// 2012.02.08
{0x0013,0x01},// 
{0x0015,0x02},//
{0x0016,0x80},//
{0x0018,0x00},//
{0x001D,0x40},//
{0x0020,0x00},//
{0x0023,0xFF},//def cb new ff
{0x008F,0x00},//for emi
{0x0025,0x80},// CKCFG 80 from system clock, 00 from PLL
{0x0026,0x87},// PLL1CFG should be 07 when system clock, should be 87 when PLL
{0x0027,0x30},// YUV output
{0x0040,0x20},// 20120224 for BLC stable
{0x0053,0x0A},//
{0x0044,0x06},// enable BLC_phase2
{0x0046,0xD8},// enable BLC_phase1, disable BLC_phase2 dithering
{0x004A,0x0A},// disable BLC_phase2 hot pixel filter
{0x004B,0x72},//
{0x0075,0x01},// in OMUX data swap for debug usage
{0x002A,0x1F},// Output=48MHz
{0x0070,0x5F},// 
{0x0071,0xFF},// 
{0x0072,0x55},//
{0x0073,0x50},//      ;
{0x0080,0xC8},// 2012.02.08
{0x0082,0xA2},//
{0x0083,0xF0},//
{0x0085,0x11},// Enable Thin-Oxide Case (Kwangoh kim), Set ADC power to 100% Enable thermal sensor control bit[7] 0:on 1:off 2012 02 13 (YL)
{0x0086,0x02},// K.Kim 2011.12.09
{0x0087,0x80},// K.Kim 2011.12.09
{0x0088,0x6C},// andrew  vcmi=0.3v  04/10  CTIA power 125%
{0x0089,0x2E},// 
{0x008A,0x7D},// 20120224 for BLC stable
{0x008D,0x20},//
{0x0090,0x00},// 1.5x(Change Gain Table )
{0x0091,0x10},// 3x  (3x CTIA)
{0x0092,0x11},// 6x  (3x CTIA + 2x PGA)
{0x0093,0x12},// 12x (3x CTIA + 4x PGA)
{0x0094,0x16},// 24x (3x CTIA + 8x PGA)
{0x0095,0x08},// 1.5x  20120217 for color shift
{0x0096,0x00},// 3x    20120217 for color shift 
{0x0097,0x10},// 6x    20120217 for color shift
{0x0098,0x11},// 12x   20120217 for color shift
{0x0099,0x12},// 24x   20120217 for color shift
{0x009A,0x06},// 24x  
{0x009B,0x34},//
{0x00A0,0x00},//
{0x00A1,0x04},// 2012.02.06(for Ver.C)
{0x011F,0xF7},// simple bpc P31 & P33[4] P40 P42 P44[5]
{0x0120,0x36},// 36:50Hz, 37:60Hz, BV_Win_Weight_En=1
{0x0121,0x83},// NSatScale_En=0, NSatScale=0
{0x0122,0x7B},//
{0x0123,0xC2},//
{0x0124,0xDE},//
{0x0125,0xFF},//
{0x0126,0x70},//
{0x0128,0x1F},//
{0x0132,0x10},//
{0x0131,0xBD},// simle bpc enable[4]
{0x0140,0x14},//
{0x0141,0x0A},//
{0x0142,0x14},//
{0x0143,0x0A},//
{0x0144,0x04},// Sort bpc hot pixel ratio
{0x0145,0x00},//
{0x0146,0x20},//
{0x0147,0x0A},//
{0x0148,0x10},//
{0x0149,0x0C},//
{0x014A,0x80},//
{0x014B,0x80},//
{0x014C,0x2E},//
{0x014D,0x2E},//
{0x014E,0x05},//
{0x014F,0x05},//
{0x0150,0x0D},//
{0x0155,0x00},//
{0x0156,0x10},//
{0x0157,0x0A},//
{0x0158,0x0A},//
{0x0159,0x0A},//
{0x015A,0x05},//
{0x015B,0x05},//
{0x015C,0x05},//
{0x015D,0x05},//
{0x015E,0x08},//
{0x015F,0xFF},//
{0x0160,0x50},// OTP BPC 2line & 4line enable
{0x0161,0x20},//
{0x0162,0x14},//
{0x0163,0x0A},//
{0x0164,0x10},// OTP 4line Strength
{0x0165,0x0A},//
{0x0166,0x0A},//
{0x018C,0x24},//
{0x018D,0x04},//; Cluster correction enable singal from thermal sensor (YL 2012 02 13)
{0x018E,0x00},//; Enable Thermal sensor control bit[7] (YL 2012 02 13)
{0x018F,0x11},// Cluster Pulse enable T1[0] T2[1] T3[2] T4[3]
{0x0190,0x80},// A11 BPC Strength[7:3], cluster correct P11[0]P12[1]P13[2]
{0x0191,0x47},// A11[0],A7[1],Sort[3],A13 AVG[6]
{0x0192,0x48},// A13 Strength[4:0],hot pixel detect for cluster[6]
{0x0193,0x64},//
{0x0194,0x32},//
{0x0195,0xc8},//
{0x0196,0x96},//
{0x0197,0x64},//
{0x0198,0x32},//
{0x0199,0x14},// A13 hot pixel th
{0x019A,0x20},// A13 edge detect th
{0x019B,0x14},//
{0x01B0,0x55},// G1G2 Balance
{0x01B1,0x0C},//
{0x01B2,0x0A},//
{0x01B3,0x10},//
{0x01B4,0x0E},//
{0x01BA,0x10},// BD
{0x01BB,0x04},//
{0x01D8,0x40},//
{0x01DE,0x60},//
{0x01E4,0x10},//
{0x01E5,0x10},//
{0x01F2,0x0C},//
{0x01F3,0x14},//
{0x01F8,0x04},//
{0x01F9,0x0C},//
{0x01FE,0x02},//
{0x01FF,0x04},//
{0x0220,0x00},// LSC
{0x0221,0xB0},//
{0x0222,0x00},//
{0x0223,0x80},//
{0x0224,0x8E},//
{0x0225,0x00},//
{0x0226,0x88},//
{0x022A,0x88},//
{0x022B,0x00},//
{0x022C,0x8C},//
{0x022D,0x13},//
{0x022E,0x0B},//
{0x022F,0x13},//
{0x0230,0x0B},//
{0x0233,0x13},//
{0x0234,0x0B},//
{0x0235,0x28},//
{0x0236,0x03},//
{0x0237,0x28},//
{0x0238,0x03},//
{0x023B,0x28},//
{0x023C,0x03},//
{0x023D,0x5C},//
{0x023E,0x02},//
{0x023F,0x5C},//
{0x0240,0x02},//
{0x0243,0x5C},//
{0x0244,0x02},//
{0x0251,0x0E},//
{0x0252,0x00},//
{0x0280,0x0A},//  	; Gamma
{0x0282,0x14},//
{0x0284,0x2A},//
{0x0286,0x50},//
{0x0288,0x60},//
{0x028A,0x6D},//
{0x028C,0x79},//
{0x028E,0x82},//
{0x0290,0x8A},//
{0x0292,0x91},//
{0x0294,0x9C},//
{0x0296,0xA7},//
{0x0298,0xBA},//
{0x029A,0xCD},//
{0x029C,0xE0},//
{0x029E,0x2D},//
{0x02A0,0x06},// Gamma by Alpha
{0x02E0,0x04},// CCM by Alpha
#if 0
{0x02C0,0x8F},// CCM
{0x02C1,0x01},//
{0x02C2,0x8F},//
{0x02C3,0x07},//
{0x02C4,0xE3},//
{0x02C5,0x07},//
{0x02C6,0xC1},//
{0x02C7,0x07},//
{0x02C8,0x70},//
{0x02C9,0x01},//
{0x02CA,0xD0},//
{0x02CB,0x07},//
{0x02CC,0xF7},//
{0x02CD,0x07},//
{0x02CE,0x5A},//
{0x02CF,0x07},//
{0x02D0,0xB0},//
{0x02D1,0x01},//
{0x0302,0x00},//
{0x0303,0x00},//
{0x0304,0x00},//
{0x02F0,0x80},//
{0x02F1,0x07},//
{0x02F2,0x8E},//
{0x02F3,0x00},//
{0x02F4,0xF2},//
{0x02F5,0x07},//
{0x02F6,0xCC},//
{0x02F7,0x07},//
{0x02F8,0x16},//
{0x02F9,0x00},//
{0x02FA,0x1E},//
{0x02FB,0x00},//
{0x02FC,0x9D},//
{0x02FD,0x07},//
{0x02FE,0xA6},//
{0x02FF,0x07},//
{0x0300,0xBD},//
{0x0301,0x00},//
{0x0305,0x00},//
{0x0306,0x00},//
{0x0307,0x00},//
#else
{0x02C0,0x8D},
{0x02C1,0x01},
{0x02C2,0xA5},
{0x02C3,0x07},
{0x02C4,0xCD},
{0x02C5,0x07},
{0x02C6,0xE0},
{0x02C7,0x07},
{0x02C8,0x91},
{0x02C9,0x01},
{0x02CA,0xBA},
{0x02CB,0x07},
{0x02CC,0xEF},
{0x02CD,0x07},
{0x02CE,0x86},
{0x02CF,0x07},
{0x02D0,0xA8},
{0x02D1,0x01},
{0x0302,0x00},
{0x0303,0x00},
{0x0304,0x00},
{0x02F0,0x83},
{0x02F1,0x07},
{0x02F2,0x78},
{0x02F3,0x00},
{0x02F4,0x07},
{0x02F5,0x00},
{0x02F6,0xAA},
{0x02F7,0x07},
{0x02F8,0xFC},
{0x02F9,0x07},
{0x02FA,0x33},
{0x02FB,0x00},
{0x02FC,0xA5},
{0x02FD,0x07},
{0x02FE,0x6C},
{0x02FF,0x07},
{0x0300,0xD0},
{0x0301,0x00},
{0x0305,0x00},
{0x0306,0x00},
{0x0307,0x00},
{0x0480,0x58},
#endif
{0x032D,0x00},//
{0x032E,0x01},//
{0x032F,0x00},//
{0x0330,0x01},//
{0x0331,0x00},//
{0x0332,0x01},//
{0x0333,0x82},// AWB channel offset
{0x0334,0x00},//
{0x0335,0x84},//
{0x0336,0x00},//; LED AWB gain
{0x0337,0x01},//
{0x0338,0x00},//
{0x0339,0x01},//
{0x033A,0x00},//
{0x033B,0x01},//
{0x033E,0x04},//
{0x033F,0x86},//
{0x0340,0x30},// AWB
{0x0341,0x44},//
{0x0342,0x4A},//
{0x0343,0x42},// CT1
{0x0344,0x74},//	;
{0x0345,0x4F},// CT2
{0x0346,0x67},//	;
{0x0347,0x5C},// CT3
{0x0348,0x59},//
{0x0349,0x67},// CT4
{0x034A,0x4D},//
{0x034B,0x6E},// CT5
{0x034C,0x44},//
{0x0350,0x80},//
{0x0351,0x80},//
{0x0352,0x18},//
{0x0353,0x18},//
{0x0354,0x6E},//
{0x0355,0x4A},//
{0x0356,0x86},//73
{0x0357,0xCA},//c0
{0x0358,0x06},//
{0x035A,0x06},//
{0x035B,0xA0},//
{0x035C,0x73},//
{0x035D,0x50},//
{0x035E,0xC0},//
{0x035F,0xA0},//
{0x0360,0x02},//
{0x0361,0x18},//
{0x0362,0x80},//
{0x0363,0x6C},//
{0x0364,0x00},//
{0x0365,0xF0},//
{0x0366,0x20},//
{0x0367,0x0C},//
{0x0369,0x00},//
{0x036A,0x10},//
{0x036B,0x10},//
{0x036E,0x20},//
{0x036F,0x00},//
{0x0370,0x10},//
{0x0371,0x18},//
{0x0372,0x0C},//
{0x0373,0x38},//
{0x0374,0x3A},//
{0x0375,0x13},//
{0x0376,0x22},//
{0x0380,0xFF},//
{0x0381,0x5A},//0x62
{0x0382,0x46},//0x4E
{0x038A,0x40},//
{0x038B,0x08},//
{0x038C,0xC1},//
{0x038E,0x50},//0x40

{0x038F,0x05},//09
{0x0390,0x10},//28

{0x0391,0x05},//
{0x0393,0x80},//
{0x0395,0x21},// AEAWB skip count
{0x0398,0x02},// AE Frame Control
{0x0399,0x84},//
{0x039A,0x03},//
{0x039B,0x25},//
{0x039C,0x03},//
{0x039D,0xC6},//
{0x039E,0x05},//
{0x039F,0x08},//
{0x03A0,0x06},//
{0x03A1,0x4A},//
{0x03A2,0x07},//
{0x03A3,0x8C},//
{0x03A4,0x0A},//
{0x03A5,0x10},//
{0x03A6,0x0C},//
{0x03A7,0x0E},//
{0x03A8,0x10},//
{0x03A9,0x18},//
{0x03AA,0x20},//
{0x03AB,0x28},//
{0x03AC,0x1E},//
{0x03AD,0x1A},//
{0x03AE,0x13},//
{0x03AF,0x0C},//
{0x03B0,0x0B},//
{0x03B1,0x09},//
{0x03B3,0x10},// AE window array
{0x03B4,0x00},//
{0x03B5,0x10},//
{0x03B6,0x00},//
{0x03B7,0xEA},//
{0x03B8,0x00},//
{0x03B9,0x3A},//
{0x03BA,0x01},//

{0x0420,0x84},// Digital Gain offset
{0x0421,0x00},//
{0x0422,0x00},//
{0x0423,0x83},//
{0x0430,0x08},// ABLC
{0x0431,0x28},//
{0x0432,0x10},//
{0x0433,0x08},//
{0x0435,0x0C},//
{0x0450,0xFF},//
{0x0451,0xE8},//
{0x0452,0xC4},//
{0x0453,0x88},//
{0x0454,0x00},//
{0x0458,0x70},//
{0x0459,0x03},//
{0x045A,0x00},//
{0x045B,0x30},//
{0x045C,0x00},//
{0x045D,0x70},//
{0x0466,0x14},//
{0x047A,0x00},// ELOFFNRB
{0x047B,0x00},// ELOFFNRY
{0x0480,0x5C},//
{0x0481,0x06},//
{0x0482,0x0C},//
{0x04B0,0x54},// Contrast
{0x04B6,0x30},//
{0x04B9,0x10},//
{0x04B3,0x10},//
{0x04B1,0x8E},//
{0x04B4,0x20},//
{0x0540,0x00},//
{0x0541,0x9D},// 60Hz Flicker
{0x0542,0x00},//
{0x0543,0xBC},// 50Hz Flicker
{0x0580,0x01},// Blur str sigma
{0x0581,0x0F},// Blur str sigma ALPHA
{0x0582,0x04},// Blur str sigma OD
{0x0594,0x00},// UV Gray TH
{0x0595,0x04},// UV Gray TH Alpha
{0x05A9,0x03},//
{0x05AA,0x40},//
{0x05AB,0x80},//
{0x05AC,0x0A},//
{0x05AD,0x10},//
{0x05AE,0x0C},//
{0x05AF,0x0C},//
{0x05B0,0x03},//
{0x05B1,0x03},//
{0x05B2,0x1C},//
{0x05B3,0x02},//
{0x05B4,0x00},//
{0x05B5,0x0C},// BlurW
{0x05B8,0x80},//
{0x05B9,0x32},//
{0x05BA,0x00},//
{0x05BB,0x80},//
{0x05BC,0x03},//
{0x05BD,0x00},//
{0x05BF,0x05},//
{0x05C0,0x10},// BlurW LowLight
{0x05C3,0x00},//
{0x05C4,0x0C},// BlurW Outdoor
{0x05C5,0x20},//
{0x05C7,0x01},//
{0x05C8,0x14},//
{0x05C9,0x54},//
{0x05CA,0x14},//
{0x05CB,0xE0},//
{0x05CC,0x20},//
{0x05CD,0x00},//
{0x05CE,0x08},//
{0x05CF,0x60},//
{0x05D0,0x10},//
{0x05D1,0x05},//
{0x05D2,0x03},//
{0x05D4,0x00},//
{0x05D5,0x05},//
{0x05D6,0x05},//
{0x05D7,0x05},//
{0x05D8,0x08},//
{0x05DC,0x0C},//
{0x05D9,0x00},//
{0x05DB,0x00},//
{0x05DD,0x0F},//
{0x05DE,0x00},//
{0x05DF,0x0A},//
{0x05E0,0xA0},// Scaler
{0x05E1,0x00},//
{0x05E2,0xA0},//
{0x05E3,0x00},//
{0x05E4,0x04},// Windowing
{0x05E5,0x00},//
{0x05E6,0x83},//
{0x05E7,0x02},//
{0x05E8,0x06},//
{0x05E9,0x00},//
{0x05EA,0xE5},//
{0x05EB,0x01},//
{0x0660,0x04},//
{0x0661,0x16},//
{0x0662,0x04},//
{0x0663,0x28},//
{0x0664,0x04},//
{0x0665,0x18},//
{0x0666,0x04},//
{0x0667,0x21},//
{0x0668,0x04},//
{0x0669,0x0C},//
{0x066A,0x04},//
{0x066B,0x25},//
{0x066C,0x00},//
{0x066D,0x12},//
{0x066E,0x00},//
{0x066F,0x80},//
{0x0670,0x00},//
{0x0671,0x0A},//
{0x0672,0x04},//
{0x0673,0x1D},//
{0x0674,0x04},//
{0x0675,0x1D},//
{0x0676,0x00},//
{0x0677,0x7E},//
{0x0678,0x01},//
{0x0679,0x47},//
{0x067A,0x00},//
{0x067B,0x73},//
{0x067C,0x04},//
{0x067D,0x14},//
{0x067E,0x04},//
{0x067F,0x28},//
{0x0680,0x00},//
{0x0681,0x22},//
{0x0682,0x00},//
{0x0683,0xA5},//
{0x0684,0x00},//
{0x0685,0x1E},//
{0x0686,0x04},//
{0x0687,0x1D},//
{0x0688,0x04},//
{0x0689,0x19},//
{0x068A,0x04},//
{0x068B,0x21},//
{0x068C,0x04},//
{0x068D,0x0A},//
{0x068E,0x04},//
{0x068F,0x25},//
{0x0690,0x04},//
{0x0691,0x15},//
{0x0698,0x20},//
{0x0699,0x20},//
{0x069A,0x01},//
{0x069C,0x10},//0x22
{0x069D,0x10},//
{0x069E,0x10},//
{0x069F,0x08},//
#if 0
{0x03BB,0x9F},
{0x03BC,0xCF},
{0x03BD,0xE7},
{0x03BE,0xF3},
{0x03BF,0x01},
{0x03C0,0xFF},
{0x03C1,0xFF},
{0x03E0,0x44},
{0x03E1,0x42},
{0x03E2,0x04},
{0x03E4,0x14},
{0x03E5,0x12},
{0x03E6,0x04},
{0x03E8,0x34},
{0x03E9,0x33},
{0x03EA,0x04},
{0x03EC,0x24},
{0x03ED,0x23},
{0x03EE,0x04},
{0x03F0,0x24},
{0x03F1,0x22},
{0x03F2,0x04},
#else
{0x03BB,0x00}, 
{0x03BC,0x00}, 
{0x03BD,0x01}, 
{0x03BE,0x00}, 
{0x03BF,0x00}, 
{0x03E0,0x00}, 
{0x03E1,0x00}, 
{0x03E2,0x00}, 
{0x03E4,0x00}, 
{0x03E5,0x00}, 
{0x03E6,0x00}, 
{0x03E8,0x00}, 
{0x03E9,0x03}, 
{0x03EA,0x00}, 
{0x03EC,0x00}, 
{0x03ED,0x00}, 
{0x03EE,0x00}, 
{0x03F0,0x00}, 
{0x03F1,0x00}, 
{0x03F2,0x00}, 
#endif
{0x03F4,0xFF},
{0x03F5,0xFF},
{0x03F6,0xFF},
{0x03F8,0xFF},
{0x03F9,0xFF},
{0x03FB,0xFF},
{0x04C0,0x03},
{0x04B0,0x50},
{0x038E,0x40},
{0x0381,0x48},
{0x0382,0x38},
{0x0156,0x10},
{0x015D,0x10},
{0x0220,0x10},
{0x0221,0xB3},
{0x0222,0x0C},
{0x0223,0x90},
{0x0224,0x8E},
{0x0225,0x01},
{0x0226,0x85},
{0x0227,0xFF},
{0x0229,0xFF},
{0x022A,0x8D},
{0x022B,0x55},
{0x022C,0x91},
{0x022D,0x13},
{0x022E,0x0B},
{0x022F,0x13},
{0x0230,0x0B},
{0x0231,0xFF},
{0x0233,0x13},
{0x0234,0x0B},
{0x0235,0xC4},
{0x0236,0x02},
{0x0237,0xC4},
{0x0238,0x02},
{0x023B,0xC4},
{0x023C,0x02},
{0x023D,0x43},
{0x023E,0x02},
{0x023F,0x43},
{0x0240,0x02},
{0x0243,0x43},
{0x0244,0x02},
{0x0251,0x10},
{0x0252,0x06},
{0x0660,0x04},
{0x0661,0x13},
{0x0662,0x04},
{0x0663,0x20},
{0x0664,0x04},
{0x0665,0x27},
{0x0666,0x04},
{0x0667,0x21},
{0x0668,0x00},
{0x0669,0x01},
{0x066A,0x04},
{0x066B,0x2B},
{0x066C,0x00},
{0x066D,0x1F},
{0x066E,0x00},
{0x066F,0x6D},
{0x0670,0x04},
{0x0671,0x09},
{0x0672,0x04},
{0x0673,0x1A},
{0x0674,0x04},
{0x0675,0x27},
{0x0676,0x00},
{0x0677,0xA3},
{0x0678,0x01},
{0x0679,0x61},
{0x067A,0x00},
{0x067B,0x65},
{0x067C,0x04},
{0x067D,0x1F},
{0x067E,0x04},
{0x067F,0x21},
{0x0680,0x00},
{0x0681,0x3F},
{0x0682,0x00},
{0x0683,0x96},
{0x0684,0x00},
{0x0685,0x13},
{0x0686,0x04},
{0x0687,0x1A},
{0x0688,0x04},
{0x0689,0x15},
{0x068A,0x04},
{0x068B,0x23},
{0x068C,0x04},
{0x068D,0x2A},
{0x068E,0x04},
{0x068F,0x27},
{0x0690,0x04},
{0x0691,0x0B},
{0x0698,0x20},
{0x0699,0x20},
{0x069A,0x01},
{0x069B,0xFF},
{0x069C,0x22},
{0x069D,0x10},
{0x069E,0x10},
{0x069F,0x08},



{0x0000,0x01},//
{0x0100,0x01},//
{0x0101,0x01},//
//{0x0005,0x01},// Turn on rolling shutter
{SENSOR_TABLE_END, 0x00}
};
//this is wrong regs config 
static struct sensor_reg mode_640x480[] = {
{0x0005,0x00},// Turn on rolling shutter
{0x0006,0x03}, //
{0x000D,0x11}, //
{0x000E,0x11}, //

/*
{0x0015,0x02},//
{0x0016,0x80},//

{0x0025,0x00},
{0x0026,0x87},
{0x002A,0x1F},
*/

{0x0011,0x02},
{0x0012,0x1C},
{0X0013,0x01},
{0x0070,0x5F},// 
{0x0071,0xFF},// 
{0x0072,0x55},//
{0x0073,0x50},//

{0x0540,0x00},//
{0x0541,0x9D},// 60Hz Flicker
{0x0542,0x00},//
{0x0543,0xBC},// 50Hz Flicker
{0x0380,0xFF},//

{0x011F,0x80}, //
{0x0125,0xFF}, //
{0x0126,0x70}, //
{0x0131,0xAD}, //
{0x0366,0x08}, // MinCTFCount, 08=20/4
{0x0433,0x10}, // ABLC LPoint, 10=40/4
{0x0435,0x14}, // ABLC UPoint, 14=50/4
{0x05E0,0xA0}, // Scaler
{0x05E1,0x00}, //
{0x05E2,0xA0}, //
{0x05E3,0x00}, //
{0x05E4,0x04}, // Windowing
{0x05E5,0x00}, //
{0x05E6,0x83}, //
{0x05E7,0x02}, //
{0x05E8,0x06}, //
{0x05E9,0x00}, //
{0x05EA,0xE5}, //
{0x05EB,0x01}, //

{0x0000,0x01}, //
{0x0100,0x01}, //
{0x0101,0x01}, //
{0x0005,0x01},// Turn on rolling shutter
{SENSOR_WAIT_MS,0x80},
{SENSOR_TABLE_END, 0x00}
};

static struct sensor_reg mode_800x600[] = {
{0x0005,0x00},// Turn on rolling shutter
{0x0006,0x03}, //
{0x000D,0x11}, //
{0x000E,0x11}, //
{0x0011,0x02},
{0x0012,0x1C},
{0X0013,0x01},
{0x0070,0x5F},// 
{0x0071,0xFF},// 
{0x0072,0x55},//
{0x0073,0x50},//

{0x0540,0x00},//
{0x0541,0x9D},// 60Hz Flicker
{0x0542,0x00},//
{0x0543,0xBC},// 50Hz Flicker
{0x0380,0xFF},//

{0x011F,0x80}, //
{0x0125,0xDF}, //
{0x0126,0x70}, //
{0x0131,0xBD}, //
{0x0366,0x08}, // MinCTFCount, 08=20/4
{0x0433,0x10}, // ABLC LPoint, 10=40/4
{0x0435,0x14}, // ABLC UPoint, 14=50/4
{0x05E0,0xA0}, // Scaler
{0x05E1,0x00}, //
{0x05E2,0xA0}, //
{0x05E3,0x00}, //
{0x05E4,0x05}, // Windowing
{0x05E5,0x00}, //
{0x05E6,0x24}, //
{0x05E7,0x03}, //
{0x05E8,0x08}, //
{0x05E9,0x00}, //
{0x05EA,0x5F}, //
{0x05EB,0x02}, //

{0x0000,0x01}, //
{0x0100,0x01}, //
{0x0101,0x01}, //
{0x0005,0x01},// Turn on rolling shutter
{SENSOR_WAIT_MS,0x80},
{SENSOR_TABLE_END, 0x00}
};
static struct sensor_reg mode_1280x720[] = {
{0x0005,0x00},// Turn on rolling shutter
{0x0006,0x0b}, //	
{0x000D,0x00}, //
{0x000E,0x00}, //

{0x0015,0x02},//
{0x0016,0xa0},//
/*
{0x0025,0x00},
{0x0026,0x87},
{0x002A,0x2F},
*/
{0x0012,0x08},
{0X0013,0x00},
{0x0070,0x5F},// 
{0x0071,0xFF},// 
{0x0072,0x99},//
//{0x0073,0x50},// 
{0x0540,0x00},//
{0x0541,0xbc},// 60Hz Flicker
{0x0542,0x00},//
{0x0543,0xe1},// 50Hz Flicker


{0x011F,0xFF}, //
{0x0125,0xDF}, //
{0x0126,0x70}, //
{0x0131,0xAC}, //
{0x0366,0x20}, // MinCTFCount
{0x0433,0x40}, // ABLC LPoint
{0x0435,0x50}, // ABLC UPoint

{0x05E4,0x08}, // Windowing
{0x05E5,0x00}, //
{0x05E6,0x07}, //
{0x05E7,0x05}, //
{0x05E8,0x06}, //
{0x05E9,0x00}, //
{0x05EA,0xD5}, //
{0x05EB,0x02}, //

{0x0000,0x01}, //
{0x0100,0x01}, //
{0x0101,0x01}, //

{0x0005,0x01},// Turn on rolling shutter
{SENSOR_WAIT_MS,0x80},

{SENSOR_TABLE_END, 0x00}	
	
};
static struct sensor_reg mode_960x720[] = {
{0x0006,0x08},
{0x000D,0x00},
{0x000E,0x00},
{0x011F,0x88},
{0x0125,0xDF},
{0x0126,0x70},
{0x0131,0xAC},
{0x0366,0x20},
{0x0398,0x01},
{0x0399,0xA8},
{0x039A,0x02},
{0x039B,0x12},
{0x039C,0x02},
{0x039D,0x7C},
{0x039E,0x03},
{0x039F,0x50},
{0x03A0,0x04},
{0x03A1,0x24},
{0x03A2,0x04},
{0x03A3,0xF8},
{0x03A4,0x06},
{0x03A5,0x50},
{0x03A6,0x1A},
{0x03A7,0x1A},
{0x03A8,0x1A},
{0x03A9,0x1A},
{0x03AA,0x1E},
{0x03AB,0x1E},
{0x03AC,0x17},
{0x03AD,0x19},
{0x03AE,0x14},
{0x03AF,0x13},
{0x03B0,0x14},
{0x03B1,0x14},
{0x0433,0x40},
{0x0435,0x50},
{0x05E4,0x08},
{0x05E5,0x00},
{0x05E6,0x07},
{0x05E7,0x3C},
{0x05E8,0x06},
{0x05E9,0x00},
{0x05EA,0xD5},
{0x05EB,0x02},
{0x0698,0x20},
{0x0699,0x20},
{0x0000,0x01},
{0x0100,0x01},
{0x0101,0x01},
{SENSOR_WAIT_MS,0x80},
{SENSOR_TABLE_END, 0x00}
};

static struct sensor_reg mode_1600x1200[] = {
{0x0005,0x00},// Turn on rolling shutter
{0x0006,0x03}, //
{0x000D,0x00}, //
{0x000E,0x00}, //
{0x0540,0x00},//
{0x0541,0x9b},// 60Hz Flicker
{0x0542,0x00},//
{0x0543,0xba},// 50Hz Flicker

{0x011F,0xFF}, //
{0x0125,0xDF}, //
{0x0126,0x70}, //
{0x0131,0xAC}, //
{0x0366,0x20}, // MinCTFCount
{0x0433,0x40}, // ABLC LPoint
{0x0435,0x50}, // ABLC UPoint
{0x05E4,0x0A}, // Windowing
{0x05E5,0x00}, //
{0x05E6,0x49}, //
{0x05E7,0x06}, //
{0x05E8,0x0A}, //
{0x05E9,0x00}, //
{0x05EA,0xB9}, //
{0x05EB,0x04}, //
{0x0000,0x01}, //
{0x0100,0x01}, //
{0x0101,0x01}, //

{0x0005,0x01},// Turn on rolling shutter
{SENSOR_WAIT_MS,0x80},

{SENSOR_TABLE_END, 0x00}
};
static  struct sensor_reg sensor_WhiteB_Auto[]=
{
	{0x0380, 0xff},  
	{0x0101, 0xFF}, 
	{SENSOR_TABLE_END, 0x00}
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static  struct sensor_reg sensor_WhiteB_CloudyDaylight[]=
{
	{0x0380, 0xFD},
	{0x032D, 0x70},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},            
	{0x0331, 0x08},            
	{0x0332, 0x01},            
	{0x0101, 0xFF},     
	{SENSOR_TABLE_END, 0x00}

};
/* ClearDay Colour Temperature : 5000K - 6500K  */
static  struct sensor_reg sensor_WhiteB_Daylight[]=
{
    //Sunny
	{0x0380, 0xFD},    
	{0x032D, 0x60},    
	{0x032E, 0x01},    
	{0x032F, 0x00},    
	{0x0330, 0x01},    
	{0x0331, 0x20},    
	{0x0332, 0x01},    
	{0x0101, 0xFF},    
	{SENSOR_TABLE_END, 0x00}
};
/* Office Colour Temperature : 3500K - 5000K  */
static  struct sensor_reg sensor_WhiteB_Incandescent[]=
{
    //Office
	{0x0380, 0xFD},//Disable AWB   
	{0x032D, 0x00},                
	{0x032E, 0x01},//Red           
	{0x032F, 0x14},                
	{0x0330, 0x01},//Green         
	{0x0331, 0xD6},                
	{0x0332, 0x01},//Blue          
	{0x0101, 0xFF},                
	{SENSOR_TABLE_END, 0x00}
    
};
/* Home Colour Temperature : 2500K - 3500K  */
static  struct sensor_reg sensor_WhiteB_Fluorescent[]=
{
    //Home
	{0x0380, 0xFD},   //Disable AWB
	{0x032D, 0x34},                
	{0x032E, 0x01},  //Red         
	{0x032F, 0x00},                
	{0x0330, 0x01}, //Green        
	{0x0331, 0x92},                
	{0x0332, 0x01}, //Blue         
	{0x0101, 0xFF},                
	{SENSOR_TABLE_END, 0x00}
};


static  struct sensor_reg sensor_Effect_Normal[] =
{
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 	
	{0x0488, 0x10},
	{0x0486, 0x00},
	{0x0487, 0xFF},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{SENSOR_TABLE_END, 0x00}	
};

static  struct sensor_reg sensor_Effect_WandB[] =
{
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{0x0488, 0x11},
	{0x0486, 0x80},
	{0x0487, 0x80},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{SENSOR_TABLE_END, 0x00}	
};

static  struct sensor_reg sensor_Effect_Sepia[] =
{
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{0x0488, 0x10},
	{0x0486, 0x00},
	{0x0487, 0xFF},
	{0x0120, 0x27},	
	{0x0101, 0xFF},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},	
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Effect_Negative[] =
{
    //Negative
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},     
	{0x0488, 0x10},
	{0x0486, 0x00},
	{0x0487, 0xFF},
	{0x0120, 0x37},	
	{0x0101, 0xFF},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},
	{SENSOR_TABLE_END, 0x00}
};
static  struct sensor_reg sensor_Effect_Bluish[] =
{
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{0x0488, 0x11},
	{0x0486, 0xB0},
	{0x0487, 0x80},
	{0x0120, 0x37},	
	{0x0101, 0xFF},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},   
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Effect_Green[] =
{
    //  Greenish
	{0x0005, 0x00},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF}, 
	{0x0488, 0x11},
	{0x0486, 0x60},
	{0x0487, 0x60},
	{0x0120, 0x27},	
	{0x0101, 0xFF},
	{0x0005, 0x01},
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},  
	{SENSOR_TABLE_END, 0x00}
};



static  struct sensor_reg sensor_Exposure0[]=
{
	{0x04c0,0x00},
	{0x038E,0x48},
	{0x0381,0x50},
	{0x0382,0x40},
	{0x0100,0xFF},
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Exposure1[]=
{

	{0x04c0,0x20},
	{0x038E,0x58},
	{0x0381,0x60},
	{0x0382,0x50},
	{0x0100,0xFF},
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Exposure2[]=
{
	{0x04c0,0x40},
	{0x038E,0x68},
	{0x0381,0x70},
	{0x0382,0x60},
	{0x0100,0xFF},
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Exposure3[]=
{
	{0x04c0,0xA0},
	{0x038E,0x38},
	{0x0381,0x40},
	{0x0382,0x30},
	{0x0100,0xFF},
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Exposure4[]=
{
	{0x04c0,0xC0},
	{0x038E,0x28},
	{0x0381,0x30},
	{0x0382,0x20},
	{0x0100,0xFF},
	{SENSOR_TABLE_END, 0x00}
};

#if 0
static  struct sensor_reg sensor_Exposure5[]=
{
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_Exposure6[]=
{
   {SENSOR_TABLE_END, 0x00}
};
#endif

static  struct sensor_reg sensor_SceneAuto[] =
{

	{0x038F,0x05},//09
	{0x0390,0x80},//28


	{0x02E0, 0x02}, 
	{0x0481, 0x08}, 
	{0x04B1, 0x00},  
	{0x04B4, 0x00}, 
	//command update start
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},   
	//command update end
	{SENSOR_TABLE_END, 0x00}
};

static  struct sensor_reg sensor_SceneNight[] =
{
	{0x038F,0x0a},//09
	{0x0390,0x00},//28
	
	{0x02E0, 0x02}, 
	{0x0481, 0x08}, 
	{0x04B1, 0x00},  
	{0x04B4, 0x00},  
	
	{0x0000, 0xFF}, 
	{0x0100, 0xFF}, 
	{0x0101, 0xFF},   
	{SENSOR_TABLE_END, 0x00}
};

enum {
	SENSOR_MODE_COMMON,
	SENSOR_MODE_640x480,
	SENSOR_MODE_800x600,
	SENSOR_MODE_1280x720,
	SENSOR_MODE_960x720,
	SENSOR_MODE_1600x1200
};

static struct sensor_reg *mode_table[] = {
	[SENSOR_MODE_COMMON]   = mode_common,
	[SENSOR_MODE_640x480]   = mode_640x480,
	[SENSOR_MODE_800x600]   = mode_800x600,
	[SENSOR_MODE_1280x720]  = mode_1280x720,
	[SENSOR_MODE_960x720]  = mode_960x720,
	[SENSOR_MODE_1600x1200]   = mode_1600x1200
};

static int sensor_read_reg8_addr16(struct i2c_client *client, u16 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err<0)
		return -EINVAL;

        //swap(*(data+2),*(data+3)); //swap high and low byte to match table format
	memcpy(val, data+2, 1);

	return 0;
}
#if 0
static void dump_regs(struct i2c_client *client, const struct sensor_reg table[]){
	const struct sensor_reg *next;
	u8 val;
	int err;
    
	pr_info("hm2057 %s \n",__func__);
    	next = table ;       

	for (next = table; next->addr!= SENSOR_TABLE_END; next++) {
		
	       err = sensor_read_reg8_addr16(client, next->addr,&val);
		
		if (err<0){
			pr_err("%s: read  0x%x failed\n", __func__,
				next->addr);
		}
		if(next->val!=val)
		debug_print("%s:####{WRITE_REG_DATA8, addr=0x%4x val = 0x%2x\n",__func__,next->addr,val);
		
	}
}
#endif 
static int sensor_write_reg8_addr16(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	
	data[2] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("mt9p111 : i2c transfer failed, retrying %x %x %d\n",
		       addr, val, err);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}
#if 0
static int sensor_write_reg8_addr8(struct i2c_client *client, u8 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	u8 data[2];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = addr & 0xFF;
    	data[1] = val;
	
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("gc0308 : i2c transfer failed, retrying %x %x %d\n",
		       addr, val, err);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}

static int sensor_write_reg16(struct i2c_client *client, u16 addr, u16 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[4];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
        data[2] = (u8) (val >> 8);
	data[3] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 4;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("gc0308 : i2c transfer failed, retrying %x %x %d\n",
		       addr, val, err);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}

#endif

static int sensor_write_table(struct i2c_client *client,
			      const struct sensor_reg table[])
{
	const struct sensor_reg *next;
	int err=0;
	u8 data;
    
	debug_print("hm2057 %s \n",__func__);
       next = table ;       

	for (next = table; next->addr!= SENSOR_TABLE_END; next++) {
		if (next->addr == SENSOR_WAIT_MS) {
			msleep(next->val);
			continue;
		}
		data = next->val;
		if(!machine_is_wikipad()) {
			if(next->addr == 0x008f)continue;
		}
		if(machine_is_wikipad()) {
			if(next->addr == 0x0023)data = 0xaa;
		}

	       err = sensor_write_reg8_addr16(client, next->addr,data);
		if (err){
			pr_err("%s: write  0x%x failed\n", __func__,
				next->addr);
			return err;
		}

	}
	return 0;
}

static int sensor_set_mode(struct sensor_info *info, struct hm2057_mode *mode)
{
	int sensor_table;
	int err;
	//int status;
       u8 val,SENSOR_ID1=0x0001,SENSOR_ID2=0x0002;
	u16 exp = 0x0000;
	u8 exp_l = 0x00;
	u8 exp_h = 0x00;
	u8 val1= 0x00;
	u8 icurragain= 0x00;

	if ( machine_is_n710() || machine_is_itq700() || machine_is_itq701() || machine_is_mm3201()||machine_is_n1010() || machine_is_wikipad())
	{
	    mode_table[SENSOR_MODE_COMMON][2].val    = 0x00;
	    mode_table[SENSOR_MODE_640x480][0].val   = 0x00;
	    mode_table[SENSOR_MODE_1280x720][0].val  = 0x08;
	    mode_table[SENSOR_MODE_1600x1200][0].val = 0x00;
	}

	debug_print("%s: xres %u yres %u\n",__func__, mode->xres, mode->yres);
	 if (mode->xres == 640 && mode->yres == 480)
		sensor_table = SENSOR_MODE_640x480;
	else if (mode->xres == 800 && mode->yres == 600)
		sensor_table = SENSOR_MODE_800x600;
	else if(mode->xres == 1280 && mode->yres == 720)//1280x720
		sensor_table = SENSOR_MODE_1280x720;
	else if(mode->xres == 960 && mode->yres == 720)//960x720
		sensor_table = SENSOR_MODE_960x720;
	else if(mode->xres == 1600 && mode->yres == 1200){//1280x720
		sensor_table = SENSOR_MODE_1600x1200;
	}else {
		pr_err("%s: invalid preview resolution supplied to set mode %d %d\n",
		       __func__, mode->xres, mode->yres);
		return -EINVAL;
	}
	if(_debug_is){
	err = sensor_read_reg8_addr16(info->i2c_client, SENSOR_ID1,&val);
	debug_print("%s: read  0x%x =0x%x\n",__func__,SENSOR_ID1,val);
	if (err<0||val!=0x20){
		pr_err("%s: read  0x%x failed\n", __func__,
			SENSOR_ID1);
		return err;
	}
	err = sensor_read_reg8_addr16(info->i2c_client, SENSOR_ID2,&val);
	debug_print("%s: read  0x%x =0x%x\n",__func__,SENSOR_ID2,val);
	if (err<0||val!=0x56){
		pr_err("%s: read  0x%x failed\n", __func__,
			SENSOR_ID2);
		return err;
	}

	err = sensor_read_reg8_addr16(info->i2c_client, 0x0003,&val);
	debug_print("%s: read  0x%x =0x%x\n",__func__,0x0003,val);

	err = sensor_read_reg8_addr16(info->i2c_client, 0x0023,&val);
	debug_print("%s: read  0x%x =0x%x\n",__func__,0x0023,val);
	}
	
    //check already program the sensor mode, Aptina support Context B fast switching capture mode back to preview mode
    //we don't need to re-program the sensor mode for 640x480 table
	if(sensor_table != SENSOR_MODE_1600x1200)
	{
	
		if(atomic_read(&info->power_on_loaded)&&!atomic_read(&info->sensor_init_loaded)){
				err = sensor_write_table(info->i2c_client, mode_table[SENSOR_MODE_COMMON]);
				pr_err("%s:hm2057 camera init setting reg uninit will load reg!\n",__func__);
				if (err){
					pr_err("%s:hm2057 camera init setting reg failed!\n",__func__);
					atomic_set(&info->sensor_init_loaded, 0);
	    				return err;
				}
				atomic_set(&info->sensor_init_loaded, 1);
				if(_debug_is){
				err = sensor_read_reg8_addr16(info->i2c_client, 0x0023,&val);
				debug_print("%s: read  0x%x =0x%x\n",__func__,0x0023,val);
				}
		}
	}else{
		
		 err = sensor_write_reg8_addr16(info->i2c_client, 0x0380,0xFC);
		if (err){
			pr_err("%s: write  0x%x failed\n", __func__,
				0x0380);
			return err;
		}

		
		sensor_read_reg8_addr16(info->i2c_client, 0x0018,&icurragain);
		if(icurragain>0){

			sensor_read_reg8_addr16(info->i2c_client, 0x0015,&val);
			sensor_read_reg8_addr16(info->i2c_client, 0x0016,&val1);	
			exp =((val<<8) | val1);//hm2057
			//exp = (exp/2);
			exp_h = (exp&0xff00)>>8;
			exp_l =  exp&0x00ff;

			err = sensor_write_reg8_addr16(info->i2c_client, 0x0015,exp_h);
			if (err){
				pr_err("%s: write  0x%x failed\n", __func__,
					0x0015);
				return err;
			}
			err = sensor_write_reg8_addr16(info->i2c_client, 0x0016,exp_l);
			if (err){
				pr_err("%s: write  0x%x failed\n", __func__,
					0x0016);
				return err;
			}
			//pr_err("========================================================0x0015 = 0x%x 0x0016=0x%x 0x0018=0x%x\n",exp_h,exp_l,icurragain);

			err = sensor_write_reg8_addr16(info->i2c_client, 0x0018,(icurragain>4?icurragain=3:--icurragain));
			if (err){
				pr_err("%s: write  0x%x failed\n", __func__,
					0x0018);
				return err;
			}

		}
	}

    err = sensor_write_table(info->i2c_client, mode_table[sensor_table]);
    if (err)
        return err;

		switch(info->current_wb){
			case YUV_Whitebalance_Auto:
				debug_print("######################YUV_Whitebalance_Auto EXE!!!!!!!!\n");
				err = sensor_write_table(info->i2c_client, sensor_WhiteB_Auto);
			 break;
	               case YUV_Whitebalance_Incandescent:
			   	debug_print("######################YUV_Whitebalance_Incandescent EXE!!!!!!!!\n");
	                 	err = sensor_write_table(info->i2c_client, sensor_WhiteB_Incandescent);
	                break;
                case YUV_Whitebalance_Daylight:
		   	debug_print("######################YUV_Whitebalance_Daylight EXE!!!!!!!!\n");
                 	err = sensor_write_table(info->i2c_client, sensor_WhiteB_Daylight);
                     break;
                case YUV_Whitebalance_Fluorescent:
		   	debug_print("######################YUV_Whitebalance_Fluorescent EXE!!!!!!!!\n");
                 	err = sensor_write_table(info->i2c_client, sensor_WhiteB_Fluorescent);
                     break;
		  case YUV_Whitebalance_CloudyDaylight:
		   	debug_print("######################YUV_Whitebalance_CloudyDaylightEXE!!!!!!!!\n");
                 	err = sensor_write_table(info->i2c_client, sensor_WhiteB_CloudyDaylight);
                     break;
                default:
			debug_print("######################YUV_white balance default EXE!!!!!!!!\n");
                     break;
		}
	     switch(info->current_exposure)
            {
                case YUV_YUVExposure_Positive2:
		   debug_print("######################YUV_YUVExposure_Positive2 EXE!!!!!!!!\n");
                 err = sensor_write_table(info->i2c_client, sensor_Exposure2);
                     break;
                case YUV_YUVExposure_Positive1:
		   debug_print("######################YUV_YUVExposure_Positive1 EXE!!!!!!!!\n");
                 err = sensor_write_table(info->i2c_client, sensor_Exposure1);
                     break;
                case YUV_YUVExposure_Number0:
		   debug_print("######################YUV_YUVExposure_Number0 EXE!!!!!!!!\n");
                 err = sensor_write_table(info->i2c_client, sensor_Exposure0);
                     break;
                case YUV_YUVExposure_Negative1:
		   debug_print("######################YUV_YUVExposure_Negative1 EXE!!!!!!!!\n");
                 err = sensor_write_table(info->i2c_client, sensor_Exposure3);
                     break;
		  case YUV_YUVExposure_Negative2:
		   debug_print("######################YUV_YUVExposure_Negative2 EXE!!!!!!!!\n");
                 err = sensor_write_table(info->i2c_client, sensor_Exposure4);
                     break;
                default:
			debug_print("######################YUV_exposure default EXE!!!!!!!!\n");
                     break;
            }

	info->mode = sensor_table;
	//sensor_read_reg8_addr16(info->i2c_client, 0x0380,&val);
	//pr_err("0x0380 = 0x%x \n",val);
	//err = sensor_write_reg8_addr16(info->i2c_client, 0x0005,0x01);
	return 0;
}

static int hm2057_get_status(struct sensor_info *info,
		struct hm2057_status *dev_status)
{
	int err = 0;
       dev_status->status = 0;

	return err;
}

static long sensor_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct sensor_info *info = file->private_data;
    int err=0;

	debug_print("hm2057 %s\n cmd %d", __func__, cmd);
    
	switch (cmd) 
    {
    	case HM2057_IOCTL_SET_MODE:
    	{
    		struct hm2057_mode mode;
    		if (copy_from_user(&mode,
    				   (const void __user *)arg,
    				   sizeof(struct hm2057_mode))) {
    			return -EFAULT;
    		}
		
    		return sensor_set_mode(info, &mode);
    	}
        case HM2057_IOCTL_SET_COLOR_EFFECT:
        {
            u8 coloreffect;

        	if (copy_from_user(&coloreffect,
        			   (const void __user *)arg,
        			   sizeof(coloreffect))) {
        		return -EFAULT;
        	}

            switch(coloreffect)
            {
                case YUV_ColorEffect_None:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_Normal);
                     break;
                case YUV_ColorEffect_Mono:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_WandB);
                     break;
                case YUV_ColorEffect_Sepia:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_Sepia);
                     break;
                case YUV_ColorEffect_Negative:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_Negative);
                     break;
                case YUV_ColorEffect_Solarize:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_Bluish);
                     break;
                case YUV_ColorEffect_Posterize:
                 err = sensor_write_table(info->i2c_client, sensor_Effect_Green);
                     break;
                default: 	
                     break;
            }

            if (err)
    	        return err;

            return 0;
        }
        case HM2057_IOCTL_SET_WHITE_BALANCE:
        {
            u8 whitebalance;

        	if (copy_from_user(&whitebalance,
        			   (const void __user *)arg,
        			   sizeof(whitebalance))) {
        		return -EFAULT;
        	}

            switch(whitebalance)
            {
                case YUV_Whitebalance_Auto:
                 err = sensor_write_table(info->i2c_client, sensor_WhiteB_Auto);
		    info->current_wb =YUV_Whitebalance_Auto; 
                     break;
                case YUV_Whitebalance_Incandescent:
                 err = sensor_write_table(info->i2c_client, sensor_WhiteB_Incandescent);
		     info->current_wb =YUV_Whitebalance_Incandescent; 
                     break;
                case YUV_Whitebalance_Daylight:
                 err = sensor_write_table(info->i2c_client, sensor_WhiteB_Daylight);
		   info->current_wb =YUV_Whitebalance_Daylight; 
                     break;
                case YUV_Whitebalance_Fluorescent:
                 err = sensor_write_table(info->i2c_client, sensor_WhiteB_Fluorescent);
		    info->current_wb =YUV_Whitebalance_Fluorescent; 
                     break;
		  case YUV_Whitebalance_CloudyDaylight:
                 err = sensor_write_table(info->i2c_client, sensor_WhiteB_CloudyDaylight);
		    info->current_wb =YUV_Whitebalance_CloudyDaylight; 
                     break;
                default:
			 info->current_wb =YUV_Whitebalance_Auto; 
                     break;
            }

            if (err)
    	        return err;

            return 0;
        }
	 case HM2057_IOCTL_SET_YUV_EXPOSURE:
        {
	      u8 yuvexposure;
        	if (copy_from_user(&yuvexposure,
        			   (const void __user *)arg,
        			   sizeof(yuvexposure))) {
        		return -EFAULT;
        	}

            switch(yuvexposure)
            {
                case YUV_YUVExposure_Positive2:
                 err = sensor_write_table(info->i2c_client, sensor_Exposure2);
		    info->current_exposure = YUV_YUVExposure_Positive2;
                     break;
                case YUV_YUVExposure_Positive1:
                 err = sensor_write_table(info->i2c_client, sensor_Exposure1);
		   info->current_exposure = YUV_YUVExposure_Positive1;
                     break;
                case YUV_YUVExposure_Number0:
                 err = sensor_write_table(info->i2c_client, sensor_Exposure0);
		   info->current_exposure = YUV_YUVExposure_Number0;
                     break;
                case YUV_YUVExposure_Negative1:
                 err = sensor_write_table(info->i2c_client, sensor_Exposure3);
		   info->current_exposure = YUV_YUVExposure_Negative1;
                     break;
		  case YUV_YUVExposure_Negative2:
                 err = sensor_write_table(info->i2c_client, sensor_Exposure4);
		   info->current_exposure = YUV_YUVExposure_Negative2;
                     break;
                default:
			info->current_exposure = YUV_YUVExposure_Number0;
                     break;
            }
            if (err)
    	        return err;
            return 0;
        }
	 case HM2057_IOCTL_SET_SCENE_MODE:
        {
              u8 scene_mode;
        	if (copy_from_user(&scene_mode,
        			   (const void __user *)arg,
        			   sizeof(scene_mode))) {
        		return -EFAULT;
        	}

            switch(scene_mode)
            {
			 case YUV_YUVSCENE_Auto:
	                 err = sensor_write_table(info->i2c_client, sensor_SceneAuto);
	                 	break;
	                case YUV_YUVSCENE_Night:
	             		err = sensor_write_table(info->i2c_client, sensor_SceneNight);
	                 	break;
			  default:
                    		 break;
		}
              if (err)
    	        return err;

            return 0;
        }
	 case HM2057_IOCTL_GET_STATUS:
	{
		struct hm2057_status dev_status;
		if (copy_from_user(&dev_status,
				   (const void __user *)arg,
				   sizeof(struct hm2057_status))) {
			return -EFAULT;
		}

		err = hm2057_get_status(info, &dev_status);
		if (err)
			return err;
		if (copy_to_user((void __user *)arg, &dev_status,
				 sizeof(struct hm2057_status))) {
			return -EFAULT;
		}
		return 0;
	}
    	default:
    		return -EINVAL;
	}
	return 0;
}

static int sensor_open(struct inode *inode, struct file *file)
{
	debug_print("hm2057 %s\n",__func__);
	file->private_data = gc0308_sensor_info;
	if (gc0308_sensor_info->pdata && gc0308_sensor_info->pdata->power_on){
		gc0308_sensor_info->pdata->power_on();
		atomic_set(&gc0308_sensor_info->power_on_loaded, 1);
		atomic_set(&gc0308_sensor_info->sensor_init_loaded, 0);
	}
	return 0;
}

static int sensor_release(struct inode *inode, struct file *file)
{
	debug_print("hm2057 %s\n",__func__);
	if (gc0308_sensor_info->pdata && gc0308_sensor_info->pdata->power_off){
		gc0308_sensor_info->pdata->power_off();
		atomic_set(&gc0308_sensor_info->power_on_loaded, 0);
		atomic_set(&gc0308_sensor_info->sensor_init_loaded, 0);
	}
	file->private_data = NULL;
	return 0;
}


static const struct file_operations sensor_fileops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.unlocked_ioctl = sensor_ioctl,
	.release = sensor_release,
};

static struct miscdevice sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_NAME,
	.fops = &sensor_fileops,
};

static u8 get_sensor_8len_reg(struct i2c_client *client, u16 reg)
{
    int err;
    u8 val;
     
    err = sensor_read_reg8_addr16(client, reg, &val);

    if (err)
      return 0;

    return val;
}
static ssize_t hm_camera_effect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_info *info = gc0308_sensor_info;
    ssize_t res = 0;

	pr_err("%s:==========>current_addr=========>0x%04x\n",__func__,info->current_addr);

	mutex_lock(&info->als_get_sensor_value_mutex);
	res = snprintf(buf, 1024,
		"0x%02x\n",get_sensor_8len_reg(info->i2c_client,info->current_addr));
	mutex_unlock(&info->als_get_sensor_value_mutex);


	
	return res;
}
#if 0
static ssize_t hm_camera_effect_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sensor_info *info = gc0308_sensor_info;
    //sscanf(buf, "%s",&vales);
   u16 addr = 0x0000;
   u8 val = 0x00;
   int err=0;
   unsigned int orgfs;
   long lfile=-1;
    char data[33];
   memset(data, 0, sizeof(data));
   strncpy(data,buf,sizeof(data)); 
   if(strncmp(data, "txt", 3)){
	   addr = simple_strtoul(buf, NULL, 16);
	   if(count >= 8)
	   	val = simple_strtoul(buf+5, NULL, 16);
	    mutex_lock(&info->als_get_sensor_value_mutex);
	    if(count >= 8){
		    err = sensor_write_reg8_addr16(info->i2c_client,addr, val);
		    if (err){
				pr_err("%s:=========>wirte 0x%04x fail!err=%d\n",__func__,addr,err);
		    	}
	    }else
	    		val = get_sensor_8len_reg(info->i2c_client,addr);
	   info->current_addr = addr;
	   mutex_unlock(&info->als_get_sensor_value_mutex);
   }else{
	orgfs = get_fs();
// Set segment descriptor associated to kernel space
	set_fs(KERNEL_DS);
	
        lfile=sys_open(mDFileName,O_WRONLY|O_CREAT, 0777);
	if (lfile < 0)
	{
 	 	printk("sys_open %s error!!. %ld\n",mDFileName,lfile);
	}
	else
	{
   	 	sys_lseek(lfile, 142, 0);  
		   	
		sys_read(lfile, data, sizeof(data)-1);
		printk("cid_show: =%s\n",data);
		
	 	sys_close(lfile);
	}
	set_fs(orgfs);
    }
    
    pr_err("%s:==> addr = 0x%04x,vales = 0x%02x,count=%d,inputLen=%d\n",__func__,addr, val,count,strlen(buf));
    return count;
}
#endif
static DEVICE_ATTR(hm_camera_effect, S_IROTH/*|S_IWOTH*/, hm_camera_effect_show,NULL);

static struct attribute *ets_attributes[] =
{

    &dev_attr_hm_camera_effect.attr,
    NULL,
};

static struct attribute_group ets_attr_group =
{
    .attrs = ets_attributes,
};


static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;
	struct platform_device * sensor_t8ev5_dev;
	debug_print("hm2057 %s\n",__func__);

	gc0308_sensor_info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (!gc0308_sensor_info) {
		pr_err("hm2057 : Unable to allocate memory!\n");
		return -ENOMEM;
	}

	err = misc_register(&sensor_device);
	if (err) {
		pr_err("hm2057 : Unable to register misc device!\n");
		goto EXIT;
	}
	gc0308_sensor_info->pdata = client->dev.platform_data;
	gc0308_sensor_info->i2c_client = client;
	atomic_set(&gc0308_sensor_info->power_on_loaded, 0);
	atomic_set(&gc0308_sensor_info->sensor_init_loaded, 0);
	
	sensor_t8ev5_dev = platform_device_register_simple("hm2057", -1, NULL, 0);
       if (IS_ERR(sensor_t8ev5_dev))
       {
           printk ("t8ev5 sensor_dev_init: error\n");
           //goto EXIT2;
       }
	err = sysfs_create_group(&sensor_t8ev5_dev->dev.kobj, &ets_attr_group);
      if (err !=0)
       {
         dev_err(&client->dev,"%s:create sysfs group error", __func__);
        //goto EXIT2;
       }
	mutex_init(&gc0308_sensor_info->als_get_sensor_value_mutex);
	i2c_set_clientdata(client, gc0308_sensor_info);
	pr_info("hm2057 %s register successfully!\n",__func__);
	return 0;
EXIT:
	kfree(gc0308_sensor_info);
	return err;
}

static int sensor_remove(struct i2c_client *client)
{
	struct sensor_info *info;

	debug_print("hm2057 %s\n",__func__);
	info = i2c_get_clientdata(client);
	misc_deregister(&sensor_device);
	mutex_destroy(&info->als_get_sensor_value_mutex);
	kfree(info);
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ "hm2057", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name = "hm2057",
		.owner = THIS_MODULE,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static int __init sensor_init(void)
{
	debug_print("hm2057 %s\n",__func__);
	return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit sensor_exit(void)
{
	debug_print("hm2057 %s\n",__func__);
	i2c_del_driver(&sensor_i2c_driver);
}

module_init(sensor_init);
module_exit(sensor_exit);

