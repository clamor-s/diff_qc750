/*
 * Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA APX ODM Kit:
 *         Implementation of the ODM Query API</b>
 *
 * @b Description: Implements the query functions for ODMs that may be
 *                 accessed at boot-time, runtime, or anywhere in between.
 */

#include "nvodm_query.h"
#include "nvodm_query_gpio.h"
#include "nvodm_query_memc.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_pins.h"
#include "nvodm_query_pins_ap20.h"
#include "tegra_devkit_custopt.h"
#include "nvodm_keylist_reserved.h"
#include "nvrm_drf.h"
#include "nvos.h"

// Although AP16 Concorde2 and AP16 Vail boards can support PMU
// interrupt, keep it disabled for now because of PMU VBUS input
// latch problem
#define NVODM_PMU_INT_ENABLED (0)

// Enable to use codec as master, MAX9548 clock drives AD1937 which in turns drives TEGRA
#define CODEC_MASTER

//Find out if the codec exists @ 0x0E
//HACK is needed because p852 config is used on LUPIN/SMEG and e1155 configs.
//The codecs/i2s interfaces used will be different.
static NvBool IsE1155Config(void)
{
    NvOdmI2cStatus I2cTransStatus = NvOdmI2cStatus_Timeout;
    NvOdmI2cTransactionInfo TransactionInfo[2];
    NvOdmServicesI2cHandle hOdmService;
    NvU32 I2CInstance =0;
    NvU8 ReadBuffer[2] = {0};
    hOdmService = NvOdmI2cOpen(NvOdmIoModule_I2c,I2CInstance);
    ReadBuffer[0] = (1 & 0xFF); //Read 1st offset

    TransactionInfo[0].Address = 0xE;
    TransactionInfo[0].Buf = ReadBuffer;
    TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = (0xE | 0x1);
    TransactionInfo[1].Buf = ReadBuffer;
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 1;
/* I2C Settings */
#define I2C_TIMEOUT   1000    // 1 seconds
#define I2C_SCLK      100     // 400KHz
    // Read data from PMU at the specified offset
    I2cTransStatus = NvOdmI2cTransaction(hOdmService, &TransactionInfo[0], 2,
            I2C_SCLK, I2C_TIMEOUT);
    return (NvBool)(I2cTransStatus == 0);
}


static const NvOdmDownloadTransport
s_NvOdmQueryDownloadTransportSetting = NvOdmDownloadTransport_None;

static const NvU8
s_NvOdmQueryDeviceNamePrefixValue[] = {'T','e','g','r','a',0};

static const NvOdmQuerySpiDeviceInfo s_NvOdmQuerySpiDeviceInfoTable [] =
{
    {NvOdmQuerySpiSignalMode_0, NV_TRUE},    // Spi1_Devices_0 (chip sel 0)
    {NvOdmQuerySpiSignalMode_0, NV_TRUE},    // Spi2_Devices_0 (chip sel 0)
    {NvOdmQuerySpiSignalMode_0, NV_TRUE, NV_TRUE},    // Spi3_slave
    {NvOdmQuerySpiSignalMode_0, NV_TRUE, NV_TRUE}    // Spi4_slave
};
// Spi idle signal state
static const NvOdmQuerySpiIdleSignalState s_NvOdmQuerySpiIdleSignalStateLevel[] =
{
    {NV_FALSE, NvOdmQuerySpiSignalMode_0, NV_FALSE}    // Spi 1
};
// We can have two I2s Instances
static const NvOdmQueryI2sInterfaceProperty s_NvOdmQueryI2sInterfacePropertySetting[] =
{
    {
#ifdef CODEC_MASTER
        NvOdmQueryI2sMode_Slave,               // Mode
#else
        NvOdmQueryI2sMode_Master,
#endif
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S,        // I2sDataCommunicationFormat
        NV_FALSE,                               // IsFixedMCLK
        0                                       // FixedMCLKFrequency
    },
    {
#ifdef CODEC_MASTER
        NvOdmQueryI2sMode_Slave,               // Mode
#else
        NvOdmQueryI2sMode_Master,
#endif

        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S,        // I2sDataCommunicationFormat
        NV_FALSE,                               // IsFixedMCLK
        0                                       // FixedMCLKFrequency
    }
};
//For SMEG codec is connected to both the I2S
static const NvOdmQueryI2sInterfaceProperty s_NvOdmQueryI2sInterfacePropertySetting_SMEGBoard[] =
{
    {
        NvOdmQueryI2sMode_Slave,
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S,        // I2sDataCommunicationFormat
        NV_FALSE,                               // IsFixedMCLK
        0                                       // FixedMCLKFrequency
    },
    {
        NvOdmQueryI2sMode_Master,
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S,        // I2sDataCommunicationFormat
        NV_FALSE,                               // IsFixedMCLK
        0                                       // FixedMCLKFrequency
    }
};


static const NvOdmQuerySpdifInterfaceProperty s_NvOdmQuerySpdifInterfacePropertySetting =
{
    NvOdmQuerySpdifDataCaptureControl_FromLeft
};

static const NvOdmQueryAc97InterfaceProperty s_NvOdmQueryAc97InterfacePropertySetting =
{
    NV_FALSE,
    NV_FALSE,
    NV_FALSE,
    NV_FALSE,
    NV_TRUE
};

// Add support for the Codec Formats
// It must the order (dapIndex) how the codec is connected to the Dap port
static const NvOdmQueryI2sACodecInterfaceProp s_NvOdmQueryI2sACodecInterfacePropSetting[] =
{
    {
#if defined(CODEC_MASTER)
        NV_TRUE,                                // IsCodecMaster
#else
        NV_FALSE,                               // IsCodecMaster
#endif
        0,                                      // DapPortIndex
        0x0E,                                   // DevAddress
        NV_FALSE,                               // IsUsbmode
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sCodecLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S         // I2sCodecDataCommFormat
    }
};
// Add support for the Codec Formats
// It must the order (dapIndex) how the codec is connected to the Dap port
static const NvOdmQueryI2sACodecInterfaceProp s_NvOdmQueryI2sACodecInterfacePropSetting_SMEGBoard[] =
{
    {
        NV_FALSE,                               // IsCodecMaster
        NvOdmDapPort_Dap1,                      // DapPortIndex
        0x0E,                                   // DevAddress
        NV_FALSE,                               // IsUsbmode
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sCodecLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S         // I2sCodecDataCommFormat
    },
    {
        NV_TRUE,                               // IsCodecMaster
        NvOdmDapPort_Dap2,                      // DapPortIndex
        0x0E,                                   // DevAddress
        NV_FALSE,                               // IsUsbmode
        NvOdmQueryI2sLRLineControl_LeftOnLow,   // I2sCodecLRLineControl
        NvOdmQueryI2sDataCommFormat_I2S         // I2sCodecDataCommFormat
    }
};

static const NvOdmQueryDapPortConnection s_NvOdmQueryDapPortConnectionTable[] =
{
#if defined(CODEC_MASTER)
    { NvOdmDapConnectionIndex_Music_Path,2,
    { {NvOdmDapPort_I2s1, NvOdmDapPort_Dap1, NV_FALSE},
      {NvOdmDapPort_Dap1, NvOdmDapPort_I2s1, NV_TRUE}
    }},
#else
    {NvOdmDapConnectionIndex_Music_Path, 2,
    { {NvOdmDapPort_I2s1, NvOdmDapPort_Dap1, NV_TRUE},
      {NvOdmDapPort_Dap1, NvOdmDapPort_I2s1, NV_FALSE}
    }},
#endif


};
//SMEG Board configuration
static const NvOdmQueryDapPortConnection s_NvOdmQueryDapPortConnectionTable_SMEGBoard[] =
{
   {NvOdmDapConnectionIndex_Music_Path, 2,
    {
        {NvOdmDapPort_I2s2, NvOdmDapPort_Dap2, NV_FALSE},
        {NvOdmDapPort_Dap2, NvOdmDapPort_I2s2, NV_TRUE}
    }
   }
};

// Ap20 support 5 dap ports
// For port is connected to DAC(I2s) then PortMode is not valid- as Dac would be driving it
static const NvOdmQueryDapPortProperty s_NvOdmQueryDapPortInfoTable[] =
{
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Reserved
    // I2S1 (DAC1) <-> DAP1 <-> HIFICODEC
    {NvOdmDapPort_I2s1, NvOdmDapPort_HifiCodecType,
        {2, 16, 44100, NvOdmQueryI2sDataCommFormat_I2S}},   // Dap1
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Dap2
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Dap3
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Dap4
};
//SMEG Board configuration
static const NvOdmQueryDapPortProperty s_NvOdmQueryDapPortInfoTable_SMEGBoard[] =
{
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Reserved
    // I2S2 (DAC2) <-> DAP2 <-> HIFICODEC
    {NvOdmDapPort_I2s1, NvOdmDapPort_HifiCodecType,
        {2, 16, 44100, NvOdmQueryI2sDataCommFormat_I2S}},   // Dap1
    {NvOdmDapPort_I2s2, NvOdmDapPort_HifiCodecType,
        {2, 16, 44100, NvOdmQueryI2sDataCommFormat_I2S}},   // Dap2
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Dap3
    {NvOdmDapPort_None, NvOdmDapPort_None , {0, 0, 0, 0} }, // Dap4
};

// Wake Events
static NvOdmWakeupPadInfo s_NvOdmWakeupPadInfo[] =
{
    {NV_FALSE,  0, NvOdmWakeupPadPolarity_Low},     // Wake Event  0 - ulpi_data4 (UART_RI)
    {NV_FALSE,  1, NvOdmWakeupPadPolarity_High},    // Wake Event  1 - gp3_pv[3] (BB_MOD, MODEM_RESET_OUT)
    {NV_FALSE,  2, NvOdmWakeupPadPolarity_High},    // Wake Event  2 - dvi_d3
    {NV_FALSE,  3, NvOdmWakeupPadPolarity_Low},     // Wake Event  3 - sdio3_dat1
    {NV_FALSE,  4, NvOdmWakeupPadPolarity_High},    // Wake Event  4 - hdmi_int (HDMI_HPD)
    {NV_FALSE,  5, NvOdmWakeupPadPolarity_High},    // Wake Event  5 - vgp[6] (VI_GP6, Flash_EN2)
    {NV_FALSE,  6, NvOdmWakeupPadPolarity_High},    // Wake Event  6 - gp3_pu[5] (GPS_ON_OFF, GPS_IRQ)
    {NV_FALSE,  7, NvOdmWakeupPadPolarity_AnyEdge}, // Wake Event  7 - gp3_pu[6] (GPS_INT, BT_IRQ)
    {NV_FALSE,  8, NvOdmWakeupPadPolarity_AnyEdge}, // Wake Event  8 - gmi_wp_n (MICRO SD_CD)
    {NV_FALSE,  9, NvOdmWakeupPadPolarity_High},    // Wake Event  9 - gp3_ps[2] (KB_COL10)
    {NV_FALSE, 10, NvOdmWakeupPadPolarity_High},    // Wake Event 10 - gmi_ad21 (Accelerometer_TH/TAP)
    {NV_FALSE, 11, NvOdmWakeupPadPolarity_Low},     // Wake Event 11 - spi2_cs2 (PEN_INT, AUDIO-IRQ)
    {NV_FALSE, 12, NvOdmWakeupPadPolarity_Low},     // Wake Event 12 - spi2_cs1 (HEADSET_DET, not used)
    {NV_FALSE, 13, NvOdmWakeupPadPolarity_Low},     // Wake Event 13 - sdio1_dat1
    {NV_FALSE, 14, NvOdmWakeupPadPolarity_High},    // Wake Event 14 - gp3_pv[6] (WLAN_INT)
    {NV_TRUE,  15, NvOdmWakeupPadPolarity_AnyEdge}, // Wake Event 15 - gmi_ad16  (SPI3_DOUT, DTV_SPI4_CS1)
    {NV_FALSE, 16, NvOdmWakeupPadPolarity_High},    // Wake Event 16 - rtc_irq
    {NV_FALSE, 17, NvOdmWakeupPadPolarity_High},    // Wake Event 17 - kbc_interrupt
    {NV_FALSE, 18, NvOdmWakeupPadPolarity_Low},     // Wake Event 18 - pwr_int (PMIC_INT)
    {NV_FALSE, 19, NvOdmWakeupPadPolarity_High},    // Wake Event 19 - usb_vbus_wakeup[0]
    {NV_FALSE, 20, NvOdmWakeupPadPolarity_High},    // Wake Event 20 - usb_vbus_wakeup[1]
    {NV_FALSE, 21, NvOdmWakeupPadPolarity_Low},     // Wake Event 21 - usb_iddig[0]
    {NV_FALSE, 22, NvOdmWakeupPadPolarity_Low},     // Wake Event 22 - usb_iddig[1]
    {NV_FALSE, 23, NvOdmWakeupPadPolarity_Low},     // Wake Event 23 - gmi_iordy (HSMMC_CLK)
    {NV_FALSE, 24, NvOdmWakeupPadPolarity_High},    // Wake Event 24 - gp3_pv[2] (BB_MOD, MODEM WAKEUP_AP15, SPI-SS)
    {NV_FALSE, 25, NvOdmWakeupPadPolarity_High},    // Wake Event 25 - gp3_ps[4] (KB_COL12)
    {NV_FALSE, 26, NvOdmWakeupPadPolarity_High},    // Wake Event 26 - gp3_ps[5] (KB_COL10)
    {NV_FALSE, 27, NvOdmWakeupPadPolarity_High},    // Wake Event 27 - gp3_ps[0] (KB_COL8)
    {NV_FALSE, 28, NvOdmWakeupPadPolarity_Low},     // Wake Event 28 - gp3_pq[6] (KB_ROW6)
    {NV_FALSE, 29, NvOdmWakeupPadPolarity_Low},     // Wake Event 29 - gp3_pq[7] (KB_ROW6)
    {NV_FALSE, 30, NvOdmWakeupPadPolarity_High}     // Wake Event 30 - dap1_dout (DAP1_DOUT)
};

/*
 * SDRAM configuration table includes external memory controller (EMC) timing
 * parameters for the pre-defined set of SDRAM frequencies, and EMC core
 * voltages. This table is used by DVFS.
 *
 * Currently only DFS entries for maximum core voltage 1.2V should be included.
 *
 * TODO: update the usage description below when different core voltages are
 * supported by DVS.
 *
 * SDRAM frequency is always half of EMC frequency. The EMC frequency is limited
 * to the frequency F_PLLM of memory PLL set by the boot-loader. Hence, SDRAM
 * frequency never exceeds Fmax = F_PLLM/2.
 *
 * During OS initialization this table is searched for the entry that contains
 * (Fmax) configuration. If found, SDRAM clock frequency is set to Fmax. Then,
 * after OS initialization, table entries for SDRAM frequencies evenly divided
 * from Fmax (e.g. Fmax/2, Fmax/4, Fmax/6, etc.) will be utilized by dynamic
 * frequency scaling mechanism . If table does not include (Fmax) configuration
 * boot-loader SDRAM clock setting is never changed.
*/
static const NvOdmSdramControllerConfig s_NvOdmEmcConfigTable[] =
{
    {
        16650,      /* SDRAM frequency */
        950,        /* EMC core voltage */
        0x01010201, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110101, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x0000005F, /* TIMING 4 */
        0x00214111, /* Timing 5 */
        0x00000003, /* FBIO_CFG6 */
        0x0B0B0B0B, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        27750,      /* SDRAM frequency */
        950,        /* EMC core voltage */
        0x01020202, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110101, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x0000009F, /* TIMING 4 */
        0x00214111, /* Timing 5 */
        0x00000003, /* FBIO_CFG6 */
        0x0B0B0B0B, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        33250,      /* SDRAM frequency */
        950,        /* EMC core voltage */
        0x01020302, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110101, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x000000DF, /* TIMING 4 */
        0x00314111, /* Timing 5 */
        0x00000003, /* FBIO_CFG6 */
        0x0B0B0B0B, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        41625,      /* SDRAM frequency */
        950,        /* EMC core voltage */
        0x01020503, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110101, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x0000011F, /* TIMING 4 */
        0x00515122, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0A0A0A0A, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        66500,      /* SDRAM frequency */
        1000,       /* EMC core voltage */
        0x02030504, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110202, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x000001DF, /* TIMING 4 */
        0x00525222, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0B0B0B0B, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        83250,      /* SDRAM frequency */
        1000,       /* EMC core voltage */
        0x02040605, /* TIMING 0 */
        0x03010304, /* TIMING 1 */
        0x00110202, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x0000025F, /* TIMING 4 */
        0x00626222, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0B0B0B0B, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        133000,     /* SDRAM frequency */
        1100,       /* EMC core voltage */
        0x03060F08, /* TIMING 0 */
        0x04010304, /* TIMING 1 */
        0x00120303, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x000003DF, /* TIMING 4 */
        0x00F38344, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0A0A0A0A, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        166500,     /* SDRAM frequency */
        1200,       /* EMC core voltage */
        0x0307110A, /* TIMING 0 */
        0x05010304, /* TIMING 1 */
        0x00120303, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x000004DF, /* TIMING 4 */
        0x01139355, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0A0A0A0A, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    },
    {
        200000,     /* SDRAM frequency */
        1200,       /* EMC core voltage */
        0x0308140b, /* TIMING 0 */
        0x05010404, /* TIMING 1 */
        0x00120404, /* TIMING 2 */
        0x00087101, /* TIMING 3 */
        0x000005df, /* TIMING 4 */
        0x01446366, /* Timing 5 */
        0x00000004, /* FBIO_CFG6 */
        0x0C0C0C0C, /* FBIO_DQSIB_DLY */
        0x00000000  /* FBIO_QUSE_DLY */
    }
};

static const NvU8 s_NvOdmQueryManufacturerSetting[] =
    {'N','V','I','D','I','A',0};
static const NvU8 s_NvOdmQueryModelSetting[] = { 'A','P','2','0',0 };

static const NvU8 s_NvOdmQueryPlatformSetting[] =
    { 'P','8','5','2',0 };
static const NvU8 s_NvOdmQueryProjectNameSetting[] =
    { 'F','i','r','e','f','l','y','2',0 };
static const NvOdmQuerySdioInterfaceProperty s_NvOdmQuerySdioInterfaceProperty[4] =
{
    { NV_TRUE,   0, NV_FALSE, 0x4, NvOdmQuerySdioSlotUsage_Media  },
    { NV_FALSE, 10, NV_TRUE,  0x8, NvOdmQuerySdioSlotUsage_wlan   },
    { NV_TRUE,   0, NV_FALSE, 0x4, NvOdmQuerySdioSlotUsage_unused },
    { NV_FALSE,  0, NV_TRUE,  0x4, NvOdmQuerySdioSlotUsage_Media   }
};

/* --- Function Implementations ---*/
static NvU32
GetBctKeyValue(void)
{
    NvOdmServicesKeyListHandle hKeyList = NULL;
    NvU32 BctCustOpt = 0;

    hKeyList = NvOdmServicesKeyListOpen();
    if (hKeyList)
    {
        BctCustOpt =
            NvOdmServicesGetKeyValue(hKeyList,
                                     NvOdmKeyListId_ReservedBctCustomerOption);
        NvOdmServicesKeyListClose(hKeyList);
    }

    return BctCustOpt;
}

NvOdmDebugConsole
NvOdmQueryDebugConsole(void)
{
    NvU32 CustOpt = GetBctKeyValue();
    switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, CONSOLE, CustOpt))
    {
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DEFAULT:
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DCC:
        return NvOdmDebugConsole_Dcc;
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_NONE:
        return NvOdmDebugConsole_None;
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_UART:
        return NvOdmDebugConsole_UartA +
            NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, CONSOLE_OPTION, CustOpt);
    default:
        return NvOdmDebugConsole_None;
    }
}

NvOdmDownloadTransport NvOdmQueryDownloadTransport(void)
{
    NvU32 CustOpt = GetBctKeyValue();
    switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, TRANSPORT, CustOpt))
    {
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_NONE:
        return NvOdmDownloadTransport_None;
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_USB:
        return NvOdmDownloadTransport_Usb;
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_ETHERNET:
        switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, ETHERNET_OPTION, CustOpt))
        {
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_ETHERNET_OPTION_SPI:
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_ETHERNET_OPTION_DEFAULT:
        default:
            return NvOdmDownloadTransport_SpiEthernet;
        }
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_UART:
        switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, UART_OPTION, CustOpt))
        {
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_B:
            return NvOdmDownloadTransport_UartB;
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_C:
            return NvOdmDownloadTransport_UartC;
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_DEFAULT:
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_A:
        default:
            return NvOdmDownloadTransport_UartA;
        }
    case TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_DEFAULT:
    default:
        return s_NvOdmQueryDownloadTransportSetting;
    }
}

const NvU8* NvOdmQueryDeviceNamePrefix(void)
{
    return s_NvOdmQueryDeviceNamePrefixValue;
}

const NvOdmQuerySpiDeviceInfo *
NvOdmQuerySpiGetDeviceInfo(
    NvOdmIoModule OdmIoModule,
    NvU32 ControllerId,
    NvU32 ChipSelect)
{
    if (OdmIoModule == NvOdmIoModule_Spi)
    {
        switch (ControllerId)
        {
            case 0:
                if (ChipSelect == 0)
                    return &s_NvOdmQuerySpiDeviceInfoTable[0];
                break;
            case 1:
                if (ChipSelect == 0)
                    return &s_NvOdmQuerySpiDeviceInfoTable[1];
                break;
            case 2:
                if (ChipSelect == 0)
                    return &s_NvOdmQuerySpiDeviceInfoTable[2];
                break;
            case 3:
                if (ChipSelect == 0)
                    return &s_NvOdmQuerySpiDeviceInfoTable[3];
                break;
            default:
                break;
        }
        return NULL;
    }
    return NULL;
}

const NvOdmQuerySpiIdleSignalState *
NvOdmQuerySpiGetIdleSignalState(
    NvOdmIoModule OdmIoModule,
    NvU32 ControllerId)
{
    if (OdmIoModule == NvOdmIoModule_Spi)
    {
        if (ControllerId == 0)
            return &s_NvOdmQuerySpiIdleSignalStateLevel[0];
    }
    return NULL;
}

const NvOdmQueryI2sInterfaceProperty *
NvOdmQueryI2sGetInterfaceProperty(
    NvU32 I2sInstanceId)
{
    if ((I2sInstanceId == 0) || (I2sInstanceId == 1))
    {
        if(!IsE1155Config())
            return &s_NvOdmQueryI2sInterfacePropertySetting_SMEGBoard[I2sInstanceId];
        else
            return &s_NvOdmQueryI2sInterfacePropertySetting[I2sInstanceId];
    }
    return NULL;
}

const NvOdmQueryDapPortProperty *
NvOdmQueryDapPortGetProperty(
    NvU32 DapPortId)
{
    if (DapPortId > 0 && DapPortId < NV_ARRAY_SIZE(s_NvOdmQueryDapPortInfoTable) )
    {
        if(!IsE1155Config())
            return  &s_NvOdmQueryDapPortInfoTable_SMEGBoard[DapPortId];
        else
            return &s_NvOdmQueryDapPortInfoTable[DapPortId];

    }
    return NULL;
}

const NvOdmQueryDapPortConnection*
NvOdmQueryDapPortGetConnectionTable(
    NvU32 ConnectionIndex)
{
    NvU32 TableIndex = 0;
    for( TableIndex = 0;
         TableIndex < NV_ARRAY_SIZE(s_NvOdmQueryDapPortConnectionTable);
         TableIndex++)
    {
        if (s_NvOdmQueryDapPortConnectionTable[TableIndex].UseIndex
                                                    == ConnectionIndex)
        {
            if(!IsE1155Config())
                return &s_NvOdmQueryDapPortConnectionTable_SMEGBoard[TableIndex];
            else
                return &s_NvOdmQueryDapPortConnectionTable[TableIndex];
        }
    }
    return NULL;
}

const NvOdmQuerySpdifInterfaceProperty *
NvOdmQuerySpdifGetInterfaceProperty(
    NvU32 SpdifInstanceId)
{
    if (SpdifInstanceId == 0)
        return &s_NvOdmQuerySpdifInterfacePropertySetting;

    return NULL;
}

const NvOdmQueryAc97InterfaceProperty *
NvOdmQueryAc97GetInterfaceProperty(
    NvU32 Ac97InstanceId)
{
    if (Ac97InstanceId == 0)
        return &s_NvOdmQueryAc97InterfacePropertySetting;

    return NULL;
}

const NvOdmQueryI2sACodecInterfaceProp *
NvOdmQueryGetI2sACodecInterfaceProperty(
    NvU32 AudioCodecId)
{
    NvU32 NumInstance = sizeof(s_NvOdmQueryI2sACodecInterfacePropSetting)/
                            sizeof(s_NvOdmQueryI2sACodecInterfacePropSetting[0]);
    if (AudioCodecId < NumInstance)
    {
        if(!IsE1155Config())
            return &s_NvOdmQueryI2sACodecInterfacePropSetting_SMEGBoard[AudioCodecId];
        else
            return &s_NvOdmQueryI2sACodecInterfacePropSetting[AudioCodecId];
    }

    return NULL;
}

/**
 * This function is called from early boot process.
 * Therefore, it cannot use global variables.
 */
NvBool NvOdmQueryAsynchMemConfig(
    NvU32 ChipSelect,
    NvOdmAsynchMemConfig *pMemConfig)
{
    return NV_FALSE;
}

const void*
NvOdmQuerySdramControllerConfigGet(NvU32 *pEntries, NvU32 *pRevision)
{
    if (pEntries)
        *pEntries = 0;
    return NULL;
}

NvOdmQueryOscillator NvOdmQueryGetOscillatorSource(void)
{
    return NvOdmQueryOscillator_Xtal;
}

NvU32 NvOdmQueryGetOscillatorDriveStrength(void)
{
    /// Oscillator drive strength range is 0 to 0x3F
    return 0x07;
}

const NvOdmWakeupPadInfo *NvOdmQueryGetWakeupPadTable(NvU32 *pSize)
{
    if (pSize)
        *pSize = NV_ARRAY_SIZE(s_NvOdmWakeupPadInfo);

    return (const NvOdmWakeupPadInfo *) s_NvOdmWakeupPadInfo;
}

const NvU8* NvOdmQueryManufacturer(void)
{
    return s_NvOdmQueryManufacturerSetting;
}

const NvU8* NvOdmQueryModel(void)
{
    return s_NvOdmQueryModelSetting;
}

const NvU8* NvOdmQueryPlatform(void)
{
    return s_NvOdmQueryPlatformSetting;
}

const NvU8* NvOdmQueryProjectName(void)
{
    return s_NvOdmQueryProjectNameSetting;
}

#define EXT 0     // external pull-up/down resistor
#define INT_PU 1  // internal pull-up
#define INT_PD 2  // internal pull-down

#define HIGHSPEED 1
#define SCHMITT 1
#define VREF    1
#define OHM_50 3
#define OHM_100 2
#define OHM_200 1
#define OHM_400 0

static const NvOdmPinAttrib pin_config[] = {

    // Set pad control for the sdio1 - SDIO1 pad control register
    { NvOdmPinRegister_Ap20_PadCtrl_SDIO1CFGPADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(HIGHSPEED, !SCHMITT, OHM_50, 31, 31, 3, 3) },

    // Set pad control for the sdio3 - SDIO3 pad control register
    { NvOdmPinRegister_Ap20_PadCtrl_SDIO3CFGPADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(HIGHSPEED, !SCHMITT, OHM_50, 31, 31, 3, 3) },

    { NvOdmPinRegister_Ap20_PullUpDown_A,
      NVODM_QUERY_PIN_AP20_PULLUPDOWN_A(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0) },

    // Set pad control for the sdio4- ATCCFG1 and ATCCFG2 pad control register
    { NvOdmPinRegister_Ap20_PadCtrl_ATCFG1PADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 31, 31, 3, 3) },

    { NvOdmPinRegister_Ap20_PadCtrl_ATCFG2PADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 10, 10, 3, 3) },

    // Set pad control for I2C1 pins
    { NvOdmPinRegister_Ap20_PadCtrl_DBGCFGPADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 31, 31, 3, 3) },

    // Enable schmitt trigger
    { NvOdmPinRegister_Ap20_PadCtrl_VICFG1PADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 31, 31, 3, 3) },

    { NvOdmPinRegister_Ap20_PadCtrl_VICFG2PADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 31, 31, 3, 3) },

    // Set pad control for I2C2 (DDC) pins
    { NvOdmPinRegister_Ap20_PadCtrl_DDCCFGPADCTRL,
      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, SCHMITT, OHM_50, 31, 31, 3, 3) },

    // Commenting out for now as this breaks LUPIN
    // Set pad control for LCD2 pins
    //    { NvOdmPinRegister_Ap20_PadCtrl_LCDCFG2PADCTRL,
    //      NVODM_QUERY_PIN_AP20_PADCTRL_AOCFG1PADCTRL(!HIGHSPEED, !SCHMITT, OHM_50, 0, 31, 3, 3) }
};

NvBool
NvOdmPlatformConfigure(int Config)
{
    return NV_TRUE;
}

NvU32
NvOdmQueryPinAttributes(const NvOdmPinAttrib** pPinAttributes)
{
    if (pPinAttributes)
    {
        *pPinAttributes = &pin_config[0];
        return NV_ARRAY_SIZE(pin_config);
    }
    return 0;
}

NvBool NvOdmQueryGetPmuProperty(NvOdmPmuProperty* pPmuProperty)
{
    return NV_FALSE;
}

/**
 * Gets the lowest soc power state supported by the hardware
 *
 * @returns information about the SocPowerState
 */
const NvOdmSocPowerStateInfo* NvOdmQueryLowestSocPowerState(void)
{
    static NvOdmSocPowerStateInfo PowerStateInfo;
    const static NvOdmSocPowerStateInfo* pPowerStateInfo = NULL;

    if (pPowerStateInfo == NULL)
    {
        // Lowest power state.
        PowerStateInfo.LowestPowerState = NvOdmSocPowerState_Active;

        pPowerStateInfo = (const NvOdmSocPowerStateInfo*) &PowerStateInfo;
    }

    return (pPowerStateInfo);
}

const NvOdmUsbProperty*
NvOdmQueryGetUsbProperty(NvOdmIoModule OdmIoModule,
                         NvU32 Instance)
{
    static const NvOdmUsbProperty Usb1Property =
    {
        NvOdmUsbInterfaceType_Utmi,
        NvOdmUsbChargerType_UsbHost,
        20,
        NV_TRUE,
        NvOdmUsbModeType_Host,
        NvOdmUsbIdPinType_None,
        NvOdmUsbConnectorsMuxType_None,
        NV_TRUE
    };

     static NvOdmUsbProperty Usb2Property =
     {
        NvOdmUsbInterfaceType_UlpiExternalPhy,
        NvOdmUsbChargerType_UsbHost,
        20,
        NV_TRUE,
        NvOdmUsbModeType_Host,
        NvOdmUsbIdPinType_None,
        NvOdmUsbConnectorsMuxType_None,
        NV_TRUE
    };

    static const NvOdmUsbProperty Usb3Property =
    {
        NvOdmUsbInterfaceType_Utmi,
        NvOdmUsbChargerType_UsbHost,
        20,
        NV_TRUE,
        NvOdmUsbModeType_Host,
        NvOdmUsbIdPinType_CableId,
        NvOdmUsbConnectorsMuxType_None,
        NV_TRUE
    };

    NvU8 IsULPIChipPresent = 1;

    // Probe for presence of ULPI chip. When chip not present, ULPI_DIR
    // pin is high due to a pull-up. When present, the pin is driven
    // low by the chip.
    // We cannot preserve this info statically since very early calls always
    // end up detecting the chip erroneously.
    // NVTODO: Remove this code once all boards are known to have the ULPI chip.
    {
        NvOdmServicesGpioHandle hGpio;

        hGpio = NvOdmGpioOpen();
        if (hGpio != NULL)
        {
            NvOdmGpioPinHandle hPinULPIDetect;
            NvU32 state;

            // Acquiring Pin Handle for ULPI_DIR
            hPinULPIDetect = NvOdmGpioAcquirePinHandle(hGpio, 'y' - 'a', 1);

            // Setting pin as Input
            NvOdmGpioConfig(hGpio, hPinULPIDetect, NvOdmGpioPinMode_InputData);

            NvOdmGpioGetState(hGpio, hPinULPIDetect, &state);

            // state low means ULPI chip is present
            IsULPIChipPresent = !state;

            if (!IsULPIChipPresent)
                NvOsDebugPrintf("ULPI PHY not detected.\n");

            NvOdmGpioReleasePinHandle(hGpio, hPinULPIDetect);
            NvOdmGpioClose(hGpio);
        }
    }

    if (OdmIoModule == NvOdmIoModule_Usb && Instance == 0)
        return &(Usb1Property);

    if (OdmIoModule == NvOdmIoModule_Usb && Instance == 1)
    {
        // We cannot return NULL in case ULPI not detected or kernel panics.
        // So change UsbMode dynamically.
        // NVTODO: Remove this code once all boards are known to have the ULPI
        // chip.
        if (!IsULPIChipPresent)
            Usb2Property.UsbMode = NvOdmUsbModeType_None;

        return &(Usb2Property);
    }

    if (OdmIoModule == NvOdmIoModule_Usb && Instance == 2)
    {
        static NvOdmServicesGpioHandle hGpio = NULL;
        static NvOdmGpioPinHandle hPinMux;
        static NvOdmGpioPinHandle hPinReset;

        if (hGpio == NULL)
        {
            hGpio = NvOdmGpioOpen();
            if (hGpio != NULL)
            {
                // Acquiring Pin Handles for USB mux controller
                hPinMux = NvOdmGpioAcquirePinHandle(hGpio, 'd' - 'a', 4);

                NvOdmGpioConfig(hGpio, hPinMux, NvOdmGpioPinMode_Output);

                // Setting pin high if ULPI detected (normal), or low
                // if ULPI not detected (missing chip). If no ULPI, USB3 is
                // routed to connector instead of to local LAN9500.
                // NVTODO: Remove this code once all boards are known to have
                // the ULPI chip. MUX should be set to 0x1 in normal case.
                if (!IsULPIChipPresent)
                    NvOsDebugPrintf("ULPI PHY not detected. Routing USB3 to hub.\n");
                NvOdmGpioSetState(hGpio, hPinMux, IsULPIChipPresent ? 0x1 : 0x0);

                // Acquiring Pin Handles for USB mux controller
                hPinReset = NvOdmGpioAcquirePinHandle(hGpio, 'd' - 'a', 3);

                // Apply reset
                NvOdmGpioConfig(hGpio, hPinReset, NvOdmGpioPinMode_Output);
                NvOdmGpioSetState(hGpio, hPinReset, 0x0);
                NvOsWaitUS(1);
                NvOdmGpioSetState(hGpio, hPinReset, 0x1);

                //NvOdmGpioReleasePinHandle(hGpio, hPinMux);
                //NvOdmGpioReleasePinHandle(hGpio, hPinReset);
            }
        }
        return &(Usb3Property);
    }

    return (const NvOdmUsbProperty *)NULL;
}

const NvOdmQuerySdioInterfaceProperty* NvOdmQueryGetSdioInterfaceProperty(NvU32 Instance)
{
    return &s_NvOdmQuerySdioInterfaceProperty[Instance];
}

const NvOdmQueryHsmmcInterfaceProperty* NvOdmQueryGetHsmmcInterfaceProperty(NvU32 Instance)
{
    return NULL;
}

NvU32
NvOdmQueryGetBlockDeviceSectorSize(NvOdmIoModule OdmIoModule)
{
    return 0;
}

const NvOdmQueryOwrDeviceInfo* NvOdmQueryGetOwrDeviceInfo(NvU32 Instance)
{
    return NULL;
}

const NvOdmQuerySpifConfig* NvOdmQueryGetSpifConfig(NvU32 Instance, NvU32 DeviceId)
{
    return NULL;
}

NvU32
NvOdmQuerySpifWPStatus(
    NvU32 DeviceId,
    NvU32 StartBlock,
    NvU32 NumberOfBlocks)
{
    return 0;
}

const NvOdmGpioWakeupSource *NvOdmQueryGetWakeupSources(NvU32 *pCount)
{
    *pCount = 0;
    return NULL;
}

typedef struct
{
    NvU32 pg_rdy;
    NvU32 pg_seq;
    NvU32 mux;
    NvU32 hold;
    NvU32 adv;
    NvU32 ce;
    NvU32 we;
    NvU32 oe;
    NvU32 wait;
} SnorChipParams;

static const SnorChipParams nor_params[] =
{
    {
        120,
        35,
        5,
        25,
        50,
        35,
        50,
        50,
        70
    },
    {
        120,
        25,
        5,
        25,
        50,
        35,
        5,
        120,
        5
    }
};

#define TIME_TO_CNT(timing) (((((timing) * (Freq)) + 1000000 - 1) / 1000000) - 1)

void NvOdmQueryGetSnorTimings(NvU32 Freq, SnorTimings *timings, enum NorTiming NorTimings)
{
    if (NorTimings >= NOR_TIMING_Max)
        NorTimings = NOR_DEFAULT_TIMING;
    timings->timing0 = (TIME_TO_CNT(nor_params[NorTimings].pg_rdy) << 28) |
        (TIME_TO_CNT(nor_params[NorTimings].pg_seq) << 20) |
        (TIME_TO_CNT(nor_params[NorTimings].mux) << 12) |
        (TIME_TO_CNT(nor_params[NorTimings].hold) << 8) |
        (TIME_TO_CNT(nor_params[NorTimings].adv) << 4) |
        TIME_TO_CNT(nor_params[NorTimings].ce);
    timings->timing1 = (TIME_TO_CNT(nor_params[NorTimings].we) << 16) |
        (TIME_TO_CNT(nor_params[NorTimings].oe) << 8) |
        TIME_TO_CNT(nor_params[NorTimings].wait);
}

NvBool NvOdmQueryGetModemId(NvU8 *ModemId)
{
    /* Implement this function based on the need */
    return NV_FALSE;
}

NvU32
NvOdmQueryGetWifiOdmData(void)
{
    // No Odm bits reserved for Wifi Chip Selection
    return 0;
}

NvBool NvOdmQueryGetSkuOverride(NvU8 *SkuOverride)
{
    return NV_FALSE;
}

NvU32 NvOdmQueryGetOdmdataField(NvU32 CustOpt, NvU32 Field)
{
    NvU32 FieldValue = 0;

    switch (Field)
    {
    case NvOdmOdmdataField_DebugConsole:
        FieldValue = NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, CONSOLE, CustOpt);
        break;
    case NvOdmOdmdataField_DebugConsoleOptions:
        switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, CONSOLE, CustOpt))
        {
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DEFAULT:
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DCC:
            FieldValue = NvOdmDebugConsole_Dcc;
            break;
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_NONE:
            FieldValue = NvOdmDebugConsole_None;
            break;
        case TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_UART:
            FieldValue = NvOdmDebugConsole_UartA +
                NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, CONSOLE_OPTION, CustOpt);
            break;
        default:
            FieldValue = NvOdmDebugConsole_None;
            break;
        }
        break;
    default:
        FieldValue = 0;
        break;
    }
    return FieldValue;
}
