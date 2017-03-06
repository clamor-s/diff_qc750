/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <pthread.h>
#include "nvos.h"
#include "codec.h"
#include "ad1937.h"
#include "max9485.h"
#include "pcf8574.h"
#include "cpld.h"

#define I2C_DEVICE "/dev/i2c-0"
#define I2C_M_WR        0

/**
 * Codec
 */
struct tagCodec
{
    int fd;
};

/**
 * MAX9485
 */
static NvU8 max9485_enable =
    MAX9485_MCLK_DISABLE |
    MAX9485_CLK_OUT1_ENABLE |
    MAX9485_CLK_OUT2_DISABLE |
    MAX9485_SAMPLING_RATE_DOUBLED |
    MAX9485_OUTPUT_SCALING_FACTOR_256;

static const NvU8 max9485_disable =
    MAX9485_MCLK_DISABLE |
    MAX9485_CLK_OUT1_DISABLE |
    MAX9485_CLK_OUT2_DISABLE;

/**
 * AD1937
 */
static NvU8 ad1937_i2s1_enable[17] =
{
    // AD1937_PLL_CLK_CTRL_0
    AD1937_PLL_CLK_CTRL_0_PWR_ON |
    AD1937_PLL_CLK_CTRL_0_MCLKI_512 |
    AD1937_PLL_CLK_CTRL_0_MCLKO_512 |
    AD1937_PLL_CLK_CTRL_0_PLL_INPUT_MCLKI |
    AD1937_PLL_CLK_CTRL_0_INTERNAL_MASTER_CLK_ENABLE,

    // AD1937_PLL_CLK_CTRL_1
    AD1937_PLL_CLK_CTRL_1_DAC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_ADC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_VREF_DISABLE,

    // AD1937_DAC_CTRL_0
    AD1937_DAC_CTRL_0_PWR_ON |
    AD1937_DAC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ |
    AD1937_DAC_CTRL_0_DSDATA_DELAY_1 |
    AD1937_DAC_CTRL_0_FMT_STEREO,

    // AD1937_DAC_CTRL_1
    AD1937_DAC_CTRL_1_DBCLK_ACTIVE_EDGE_MIDCYCLE |
    AD1937_DAC_CTRL_1_DBCLK_PER_FRAME_64 |
    AD1937_DAC_CTRL_1_DLRCLK_POLARITY_LEFT_LOW |
    AD1937_DAC_CTRL_1_DLRCLK_MASTER |
    AD1937_DAC_CTRL_1_DBCLK_MASTER |
    AD1937_DAC_CTRL_1_DBCLK_SOURCE_INTERNAL |
    AD1937_DAC_CTRL_1_DBCLK_POLARITY_NORMAL,

    // AD1937_DAC_CTRL_2
    AD1937_DAC_CTRL_2_MASTER_UNMUTE |
    AD1937_DAC_CTRL_2_DEEMPHASIS_FLAT |
    AD1937_DAC_CTRL_2_WORD_WIDTH_16_BITS |          // sample size
    AD1937_DAC_CTRL_2_DAC_OUTPUT_POLARITY_NORMAL,

    // AD1937_DAC_MUTE_CTRL
    AD1937_DAC_MUTE_CTRL_DAC1L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC1R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4R_UNMUTE,

    // AD1937_DAC_VOL_CTRL_DAC1L
    0,

    // AD1937_DAC_VOL_CTRL_DAC1R
    0,

    // AD1937_DAC_VOL_CTRL_DAC2L
    0,

    // AD1937_DAC_VOL_CTRL_DAC2R
    0,

    // AD1937_DAC_VOL_CTRL_DAC3L
    0,

    // AD1937_DAC_VOL_CTRL_DAC3R
    0,

    // AD1937_DAC_VOL_CTRL_DAC4L
    0,

    // AD1937_DAC_VOL_CTRL_DAC4R
    0,

    // AD1937_ADC_CTRL_0
    AD1937_ADC_CTRL_0_PWR_ON |
    AD1937_ADC_CTRL_0_HIGH_PASS_FILTER_OFF |
    AD1937_ADC_CTRL_0_ADC1L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC1R_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2R_UNMUTE |
    AD1937_ADC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ,

    // AD1937_ADC_CTRL_1
    AD1937_ADC_CTRL_1_WORD_WIDTH_16_BITS |          // sample size
    AD1937_ADC_CTRL_1_ASDATA_DELAY_1 |
    AD1937_ADC_CTRL_1_FMT_STEREO |
    AD1937_ADC_CTRL_1_ABCLK_ACTIVE_EDGE_MIDCYCLE,

    // AD1937_ADC_CTRL_2
    AD1937_ADC_CTRL_1_ALRCLK_FMT_50_50 |
    AD1937_ADC_CTRL_1_ABCLK_POLARITY_FALLING_EDGE |
    AD1937_ADC_CTRL_1_ALRCLK_POLARITY_LEFT_LOW |
    AD1937_ADC_CTRL_1_ALRCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_PER_FRAME_64 |
    AD1937_ADC_CTRL_1_ABCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_SOURCE_ABCLK_PIN
};
static NvU8 ad1937_i2s2_enable[17] =
{
    // AD1937_PLL_CLK_CTRL_0
    AD1937_PLL_CLK_CTRL_0_PWR_ON |
    AD1937_PLL_CLK_CTRL_0_MCLKI_512 |
    AD1937_PLL_CLK_CTRL_0_MCLKO_OFF |
    AD1937_PLL_CLK_CTRL_0_PLL_INPUT_DLRCLK |
    AD1937_PLL_CLK_CTRL_0_INTERNAL_MASTER_CLK_ENABLE,

    // AD1937_PLL_CLK_CTRL_1
    AD1937_PLL_CLK_CTRL_1_DAC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_ADC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_VREF_ENABLE,

    // AD1937_DAC_CTRL_0
    AD1937_DAC_CTRL_0_PWR_ON |
    AD1937_DAC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ |
    AD1937_DAC_CTRL_0_DSDATA_DELAY_0 |
    AD1937_DAC_CTRL_0_FMT_STEREO,

    // AD1937_DAC_CTRL_1
    AD1937_DAC_CTRL_1_DBCLK_ACTIVE_EDGE_MIDCYCLE |
    AD1937_DAC_CTRL_1_DBCLK_PER_FRAME_64 |
    AD1937_DAC_CTRL_1_DLRCLK_POLARITY_LEFT_HIGH |
    AD1937_DAC_CTRL_1_DLRCLK_SLAVE |
    AD1937_DAC_CTRL_1_DBCLK_SLAVE |
    AD1937_DAC_CTRL_1_DBCLK_SOURCE_DBCLK_PIN |
    AD1937_DAC_CTRL_1_DBCLK_POLARITY_INVERTED,

    // AD1937_DAC_CTRL_2
    AD1937_DAC_CTRL_2_MASTER_UNMUTE |
    AD1937_DAC_CTRL_2_DEEMPHASIS_44_1_KHZ |
    AD1937_DAC_CTRL_2_WORD_WIDTH_16_BITS |          // sample size
    AD1937_DAC_CTRL_2_DAC_OUTPUT_POLARITY_NORMAL,

    // AD1937_DAC_MUTE_CTRL
    AD1937_DAC_MUTE_CTRL_DAC1L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC1R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4R_UNMUTE,

    // AD1937_DAC_VOL_CTRL_DAC1L
    0,

    // AD1937_DAC_VOL_CTRL_DAC1R
    0,

    // AD1937_DAC_VOL_CTRL_DAC2L
    0,

    // AD1937_DAC_VOL_CTRL_DAC2R
    0,

    // AD1937_DAC_VOL_CTRL_DAC3L
    0,

    // AD1937_DAC_VOL_CTRL_DAC3R
    0,

    // AD1937_DAC_VOL_CTRL_DAC4L
    0,

    // AD1937_DAC_VOL_CTRL_DAC4R
    0,

    // AD1937_ADC_CTRL_0
    AD1937_ADC_CTRL_0_PWR_ON |
    AD1937_ADC_CTRL_0_HIGH_PASS_FILTER_OFF |
    AD1937_ADC_CTRL_0_ADC1L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC1R_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2R_UNMUTE |
    AD1937_ADC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ,

    // AD1937_ADC_CTRL_1
    AD1937_ADC_CTRL_1_WORD_WIDTH_16_BITS |          // sample size
    AD1937_ADC_CTRL_1_ASDATA_DELAY_1 |
    AD1937_ADC_CTRL_1_FMT_STEREO |
    AD1937_ADC_CTRL_1_ABCLK_ACTIVE_EDGE_MIDCYCLE,

    // AD1937_ADC_CTRL_2
    AD1937_ADC_CTRL_1_ALRCLK_FMT_50_50 |
    AD1937_ADC_CTRL_1_ABCLK_POLARITY_FALLING_EDGE |
    AD1937_ADC_CTRL_1_ALRCLK_POLARITY_LEFT_LOW |
    AD1937_ADC_CTRL_1_ALRCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_PER_FRAME_64 |
    AD1937_ADC_CTRL_1_ABCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_SOURCE_ABCLK_PIN
};

static const NvU8 ad1937_tdm_enable[17] =
{
    // AD1937_PLL_CLK_CTRL_0
    AD1937_PLL_CLK_CTRL_0_PWR_OFF |
    AD1937_PLL_CLK_CTRL_0_MCLKI_512 |
    AD1937_PLL_CLK_CTRL_0_MCLKO_512 |
    AD1937_PLL_CLK_CTRL_0_PLL_INPUT_MCLKI |
    AD1937_PLL_CLK_CTRL_0_INTERNAL_MASTER_CLK_ENABLE,

    // AD1937_PLL_CLK_CTRL_1
    AD1937_PLL_CLK_CTRL_1_DAC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_ADC_CLK_MCLK |
    AD1937_PLL_CLK_CTRL_1_VREF_DISABLE,

    // AD1937_DAC_CTRL_0
    AD1937_DAC_CTRL_0_PWR_ON |
    AD1937_DAC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ |
    AD1937_DAC_CTRL_0_DSDATA_DELAY_0 |
    AD1937_DAC_CTRL_0_FMT_TDM_SINGLE_LINE,          // TDM

    // AD1937_DAC_CTRL_1
    AD1937_DAC_CTRL_1_DBCLK_ACTIVE_EDGE_MIDCYCLE |
    AD1937_DAC_CTRL_1_DBCLK_PER_FRAME_256 |         // TDM
    AD1937_DAC_CTRL_1_DLRCLK_POLARITY_LEFT_HIGH |   // TDM
    AD1937_DAC_CTRL_1_DLRCLK_MASTER |
    AD1937_DAC_CTRL_1_DBCLK_MASTER |
    AD1937_DAC_CTRL_1_DBCLK_SOURCE_INTERNAL |
    AD1937_DAC_CTRL_1_DBCLK_POLARITY_INVERTED,      // TDM

    // AD1937_DAC_CTRL_2
    AD1937_DAC_CTRL_2_MASTER_UNMUTE |
    AD1937_DAC_CTRL_2_DEEMPHASIS_FLAT |
    AD1937_DAC_CTRL_2_WORD_WIDTH_16_BITS |          // sample size
    AD1937_DAC_CTRL_2_DAC_OUTPUT_POLARITY_NORMAL,

    // AD1937_DAC_MUTE_CTRL
    AD1937_DAC_MUTE_CTRL_DAC1L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC1R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC2R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC3R_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4L_UNMUTE |
    AD1937_DAC_MUTE_CTRL_DAC4R_UNMUTE,

    // AD1937_DAC_VOL_CTRL_DAC1L
    0,

    // AD1937_DAC_VOL_CTRL_DAC1R
    0,

    // AD1937_DAC_VOL_CTRL_DAC2L
    0,

    // AD1937_DAC_VOL_CTRL_DAC2R
    0,

    // AD1937_DAC_VOL_CTRL_DAC3L
    0,

    // AD1937_DAC_VOL_CTRL_DAC3R
    0,

    // AD1937_DAC_VOL_CTRL_DAC4L
    0,

    // AD1937_DAC_VOL_CTRL_DAC4R
    0,

    // AD1937_ADC_CTRL_0
    AD1937_ADC_CTRL_0_PWR_ON |
    AD1937_ADC_CTRL_0_HIGH_PASS_FILTER_ON |
    AD1937_ADC_CTRL_0_ADC1L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC1R_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2L_UNMUTE |
    AD1937_ADC_CTRL_0_ADC2R_UNMUTE |
    AD1937_ADC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ,

    // AD1937_ADC_CTRL_1
    AD1937_ADC_CTRL_1_WORD_WIDTH_16_BITS |          // sample size
    AD1937_ADC_CTRL_1_ASDATA_DELAY_0 |
    AD1937_ADC_CTRL_1_FMT_TDM_SINGLE_LINE |         // TDM
    AD1937_ADC_CTRL_1_ABCLK_ACTIVE_EDGE_MIDCYCLE,   // TDM

    // AD1937_ADC_CTRL_2
    AD1937_ADC_CTRL_1_ALRCLK_FMT_50_50 |
    AD1937_ADC_CTRL_1_ABCLK_POLARITY_RISING_EDGE |  // TDM
    AD1937_ADC_CTRL_1_ALRCLK_POLARITY_LEFT_HIGH |   // TDM
    AD1937_ADC_CTRL_1_ALRCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_PER_FRAME_256 |         // TDM
    AD1937_ADC_CTRL_1_ABCLK_SLAVE |
    AD1937_ADC_CTRL_1_ABCLK_SOURCE_ABCLK_PIN
};

static const NvU8 ad1937_disable[17] =
{
    // AD1937_PLL_CLK_CTRL_0
    AD1937_PLL_CLK_CTRL_0_PWR_OFF,

    // AD1937_PLL_CLK_CTRL_1
    0,

    // AD1937_DAC_CTRL_0
    AD1937_DAC_CTRL_0_PWR_OFF,

    // AD1937_DAC_CTRL_1
    0,

    // AD1937_DAC_CTRL_2
    0,

    // AD1937_DAC_MUTE_CTRL
    0,

    // AD1937_DAC_VOL_CTRL_DAC1L
    0,

    // AD1937_DAC_VOL_CTRL_DAC1R
    0,

    // AD1937_DAC_VOL_CTRL_DAC2L
    0,

    // AD1937_DAC_VOL_CTRL_DAC2R
    0,

    // AD1937_DAC_VOL_CTRL_DAC3L
    0,

    // AD1937_DAC_VOL_CTRL_DAC3R
    0,

    // AD1937_DAC_VOL_CTRL_DAC4L
    0,

    // AD1937_DAC_VOL_CTRL_DAC4R
    0,

    // AD1937_ADC_CTRL_0
    AD1937_ADC_CTRL_0_PWR_OFF,

    // AD1937_ADC_CTRL_1
    0,

    // AD1937_ADC_CTRL_2
    0
};

/**
 * PCF8574
 */
static const NvU8 pcf8574_enable_DAP1 =
    PCF8574_ANALOG_ROUTE_NVTEST |
    PCF8574_DIGITAL_ROUTE_NVTEST |
    PCF8574_DAP1_ENABLE |
    PCF8574_DAP2_DISABLE |
    PCF8574_DAP3_DISABLE |
    PCF8574_SERIAL_OEM1;

static const NvU8 pcf8574_enable_DAP2 =
    PCF8574_ANALOG_ROUTE_NVTEST |
    PCF8574_DIGITAL_ROUTE_NVTEST |
    PCF8574_DAP1_DISABLE |
    PCF8574_DAP2_ENABLE |
    PCF8574_DAP3_DISABLE |
    PCF8574_SERIAL_OEM1;

static const NvU8 pcf8574_enable_DAP3 =
    PCF8574_ANALOG_ROUTE_NVTEST |
    PCF8574_DIGITAL_ROUTE_NVTEST |
    PCF8574_DAP1_DISABLE |
    PCF8574_DAP2_DISABLE |
    PCF8574_DAP3_ENABLE |
    PCF8574_SERIAL_OEM1;

static const NvU8 pcf8574_disable =
    PCF8574_ANALOG_ROUTE_NVTEST |
    PCF8574_DIGITAL_ROUTE_NVTEST |
    PCF8574_DAP1_DISABLE |
    PCF8574_DAP2_DISABLE |
    PCF8574_DAP3_DISABLE |
    PCF8574_SERIAL_OEM1;

static NvError NvOsPrivExtractSkuInfo(NvU32 *info, NvU32 field)
{
    char    buf[30] = { 0 };
    FILE*   skuinfo;
    char    *p;

    skuinfo = fopen("/proc/skuinfo","r");
    if(!skuinfo)
    {
        return NvError_NotSupported;
    }

    fread(buf, 1, sizeof(buf), skuinfo);
    fclose(skuinfo);

    p = strtok(buf, "-");
    while( field-- && p)
    {
        p = strtok(NULL, "-");
    }

    if (!p || 0 == (*info = strtoul(p, NULL, 10)))
    {
        fprintf(stderr, "%s() Warning: Could not retrieve sku info.\n", __func__);
        return NvSuccess;
    }

    return NvSuccess;
}


static int i2c_write(int fd, unsigned int addr, unsigned int val)
{
    struct i2c_rdwr_ioctl_data msg_rdwr;
    struct i2c_msg i2cmsg;
    int i;
    unsigned char _buf[20];

    _buf[0] = val;

    msg_rdwr.msgs = &i2cmsg;
    msg_rdwr.nmsgs = 1;

    i2cmsg.addr = addr;
    i2cmsg.flags = 0;
    i2cmsg.len = 1;
    i2cmsg.buf = _buf;
    if ((i = ioctl(fd, I2C_RDWR, &msg_rdwr)) < 0) {
        return -1;
    }
    return 0;
}


static int i2c_write_subaddr(int fd, unsigned int addr, unsigned int offset, unsigned char val)
{
    struct i2c_rdwr_ioctl_data msg_rdwr;
    struct i2c_msg i2cmsg;
    int i;
    unsigned char _buf[20];

    _buf[0] = offset;
    _buf[1] = val;

    msg_rdwr.msgs = &i2cmsg;
    msg_rdwr.nmsgs = 1;

    i2cmsg.addr = addr;
    i2cmsg.flags = 0;
    i2cmsg.len = 2;
    i2cmsg.buf = _buf;

    if ((i = ioctl(fd, I2C_RDWR, &msg_rdwr)) < 0) {
        return -1;
    }
    return 0;
}

static int i2c_read(int fd, int addr, unsigned char *pval)
{
     struct i2c_rdwr_ioctl_data msg_rdwr;
     struct i2c_msg i2cmsg[2];
     int i;
     unsigned char buf[2];

     msg_rdwr.msgs = i2cmsg;
     msg_rdwr.nmsgs = 2;
     buf[0]=0;
     i2cmsg[0].addr = addr;
     i2cmsg[0].flags = I2C_M_WR | I2C_M_NOSTART;
     i2cmsg[0].len = 1;
     i2cmsg[0].buf = (__u8*) &buf;
     i2cmsg[1].addr = addr;
     i2cmsg[1].flags = I2C_M_RD;
     i2cmsg[1].len = 1;
     i2cmsg[1].buf = (__u8*) &buf;

     if ((i = ioctl(fd, I2C_RDWR, &msg_rdwr)) < 0) {
          return -1;
     }
     *pval = buf[0] & 0xff;
     return 0;
}

static int i2c_read_subaddr(int fd, int addr, int offset, unsigned char *pval)
{
     struct i2c_rdwr_ioctl_data msg_rdwr;
     struct i2c_msg i2cmsg[2];
     int i;
     unsigned char buf[2];

     msg_rdwr.msgs = i2cmsg;
     msg_rdwr.nmsgs = 2;
     buf[0]=offset;
     i2cmsg[0].addr = addr;
     i2cmsg[0].flags = I2C_M_WR | I2C_M_NOSTART;
     i2cmsg[0].len = 1;
     i2cmsg[0].buf = (__u8*)&buf;

     i2cmsg[1].addr = addr;
     i2cmsg[1].flags = I2C_M_RD;
     i2cmsg[1].len = 1;
     i2cmsg[1].buf = (__u8*)&buf;

     if ((i = ioctl(fd, I2C_RDWR, &msg_rdwr)) < 0) {
         return -1;
     }
     *pval = buf[0] & 0xff;
     return 0;
}

NvError
CodecOpen(Codec*    phCodec,
          CodecMode mode,
          int       sampleRate)
{
    Codec   hCodec;
    NvU8    i;
    NvBool  base_board;
    NvU32   i2c_address;
    NvU32   i2c_offset;
    NvU32   i2c_value;
    NvU32   sku_id = 0;
    NvError err = NvSuccess;

    if (!phCodec)
    {
        err = NvError_BadParameter;
        fprintf(stderr, "%s() Error: Bad paramerter. Null codec handle.\n", __func__);
        goto ERR_0;
    }

    if (mode != CodecMode_I2s1 &&
        mode != CodecMode_I2s2 &&
        mode != CodecMode_Tdm1 &&
        mode != CodecMode_Tdm2 )
    {
        err = NvError_BadParameter;
        fprintf(stderr, "%s() Error: Bad paramerter. Invalid code mode.\n", __func__);
        goto ERR_1;
    }

    err = NvOsPrivExtractSkuInfo(&sku_id, 2);
    if (err != NvSuccess)
        goto ERR_1;

    // Allocate Codec handle
    hCodec = NvOsAlloc(sizeof(struct tagCodec));
    if (!hCodec)
    {
        err = NvError_InsufficientMemory;
        fprintf(stderr, "%s() Error: InsufficientMemory for codec handle.\n", __func__);
        goto ERR_1;
    }
    NvOsMemset(hCodec, 0, sizeof(struct tagCodec));

    // Open I2c
    hCodec->fd = open(I2C_DEVICE, O_RDWR);
    if (hCodec->fd < 0 )
    {
        fprintf(stderr, "%s() Error: Could not open I2C device %s.\n", __func__, I2C_DEVICE);
        goto ERR_2;
    }

    // Program MAX9485
    switch (sampleRate)
    {
        case 48:
        {
            max9485_enable |= MAX9485_SAMPLING_FREQUENCY_48KHZ;
        }
        break;

        case 44:
        default:
        {
            max9485_enable |= MAX9485_SAMPLING_FREQUENCY_44_1KHZ; 
        }
        break;
    }

    err = i2c_write(hCodec->fd, MAX9485_I2C_ADDR, max9485_enable);
    if (err)
    {
        fprintf(stderr, "%s() Error: I2C write on MAX9485 failed.\n", __func__);
        goto ERR_2;
    }

    //For SKU13 we operate in the RJM format
    if(sku_id == 13)
    {
         ad1937_i2s1_enable[2]  = ad1937_i2s2_enable[2]   = AD1937_DAC_CTRL_0_PWR_ON |
                                                            AD1937_DAC_CTRL_0_SAMPLE_RATE_32_44_1_48_KHZ |
                                                            AD1937_DAC_CTRL_0_DSDATA_DELAY_16 |
                                                            AD1937_DAC_CTRL_0_FMT_STEREO;

         ad1937_i2s1_enable[15] = ad1937_i2s2_enable[15]  = AD1937_ADC_CTRL_1_WORD_WIDTH_16_BITS |
                                                            AD1937_ADC_CTRL_1_ASDATA_DELAY_16 |
                                                            AD1937_ADC_CTRL_1_FMT_STEREO |
                                                            AD1937_ADC_CTRL_1_ABCLK_ACTIVE_EDGE_MIDCYCLE;
    }

    // Program AD1937
    if (mode == CodecMode_I2s1)
    {
        for (i = 0; i < 17; ++i)
        {
            err = i2c_write_subaddr(hCodec->fd, AD1937_I2C_ADDR, i, ad1937_i2s1_enable[i]);
            if (err)
            {
                fprintf(stderr, "%s() Error: I2C write on AD1937 failed for codec mode %d.\n", __func__, mode);
                goto ERR_2;
            }
        }
    }
    else if (mode == CodecMode_I2s2)
    {
        for (i = 0; i < 17; ++i)
        {
            err = i2c_write_subaddr(hCodec->fd, AD1937_I2C_ADDR, i, ad1937_i2s2_enable[i]);
            if (err)
            {
                fprintf(stderr, "%s() Error: I2C write on AD1937 failed for codec mode %d.\n", __func__, mode);
                goto ERR_2;
            }
        }
    }
    else
    {
        for (i = 0; i < 17; ++i)
        {
            err = i2c_write_subaddr(hCodec->fd, AD1937_I2C_ADDR, i, ad1937_tdm_enable[i]);
            if (err)
            {
                fprintf(stderr, "%s() Error: I2C write on AD1937 failed for codec mode %d.\n", __func__, mode);
                goto ERR_2;
            }
        }
    }

    err = i2c_write_subaddr(hCodec->fd, CPLD_I2C_ADDR, CPLD_WRITE_ENABLE_OFFSET, CPLD_WRITE_ENABLE_VALUE);
    // if able to program CPLD that means base board is E1888
    if (err == 0)
    {
        base_board  = BASE_BOARD_E1888;
        i2c_address = CPLD_I2C_ADDR;
        i2c_offset  = CPLD_WRITE_OFFSET;
        printf("Base board E1888\n");
    }
    else // else E1155
    {
        base_board  = BASE_BOARD_E1155;
        i2c_address = PCF8574_I2C_ADDR;
        i2c_offset  = 0;
        printf("Base board E1155\n");
    }

    //configure i2c address, offset, value
    switch (mode)
    {
        default:
            goto ERR_2;

        case CodecMode_I2s1:
        case CodecMode_Tdm1:
        {
            i2c_value = (base_board == BASE_BOARD_E1888?
                                            CPLD_ENABLE_DAP1:
                                            pcf8574_enable_DAP1);
        }
        break;

        case CodecMode_Tdm2:
        case CodecMode_I2s2:
        {
            if (sku_id == 1 ||
                sku_id == 8 ||
                sku_id == 9 )
            {
                i2c_value = (base_board == BASE_BOARD_E1888?
                                                CPLD_ENABLE_DAP3:
                                                pcf8574_enable_DAP3);
            }
            else
            {
                i2c_value = (base_board == BASE_BOARD_E1888?
                                                CPLD_ENABLE_DAP2:
                                                pcf8574_enable_DAP2);
            }
        }
        break;
    }

    // Program PCF8574 (I/O EXPANDER) for E1155 base board
    err = i2c_write_subaddr(hCodec->fd, i2c_address, i2c_offset, i2c_value);
    if (err)
    {
        fprintf(stderr, "%s() Error: I2C write on PCF8574/CPLD failed.\n", __func__);
        goto ERR_2;
    }

    // Save Codec handle
    *phCodec = hCodec;
    return NvSuccess;

ERR_2:
    NvOsFree(hCodec);

ERR_1:
    *phCodec = NULL;

ERR_0:
    return err;
}

void
CodecClose(
    Codec hCodec
    )
{
    int i;

    if (hCodec)
    {
        // Program PCF8574 (I/O EXPANDER)
        i2c_write(hCodec->fd, PCF8574_I2C_ADDR, pcf8574_disable);

        // Program AD1937
        for (i = 0; i < 17; ++i)
        {
            i2c_write_subaddr(hCodec->fd, AD1937_I2C_ADDR, i, ad1937_disable[i]);
        }

        // Program MAX9485
        i2c_write(hCodec->fd, MAX9485_I2C_ADDR, max9485_disable);

        close(hCodec->fd);

        // Free Codec
        NvOsFree(hCodec);
    }
}

NvError
CodecDump(
    Codec hCodec
    )
{
    int     i;
    NvU8    val;
    NvError err;

    if (!hCodec)
    {
        return NvError_BadParameter;
    }

    // Read MAX9485
    err = i2c_read(hCodec->fd, MAX9485_I2C_ADDR, &val);
    if (err)
    {
        return err;
    }
    printf("MAX9485: 0x%.2x\n", val);

    // Read PCF8574 (I/O EXPANDER)
    err = i2c_read(hCodec->fd, PCF8574_I2C_ADDR, &val);
    if (err)
    {
        return err;
    }
    printf("PCF8574: 0x%.2x\n", val);

    // Read AD1937
    printf("AD1937 : ");
    for (i = 0; i < 17; ++i)
    {
        err = i2c_read_subaddr(hCodec->fd, AD1937_I2C_ADDR, i, &val);
        if (err)
        {
            return err;
        }

        printf("0x%.2x ", val);
    }
    printf("\n");

    return NvSuccess;
}
