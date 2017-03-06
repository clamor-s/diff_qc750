/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "aos.h"
#include "nvaboot.h"
#include "fastboot.h"
#include "nvbootargs.h"
#include "nvappmain.h"
#include "nv3p_transport.h"
#include "prettyprint.h"
#include "nvutil.h"
#include "nvbu.h"
#include "nvrm_boot.h"
#include "nvboot_bit.h"
#include "nvboot_bct.h"
#include "nvddk_kbc.h"
#include "nvddk_blockdevmgr.h"
#include "nvrm_power_private.h"
#include "nvbct.h"
#include "nvbl_memmap_nvap.h"

#include "nvodm_query.h"
#include "nvodm_pmu.h"
#include "nvodm_query_discovery.h"
#include "nvodm_scrollwheel.h"
#include "nvodm_keyboard.h"

#include "recovery_utils.h"
#include "nv_rsa.h"
#include "nvfs_defs.h"
#include "nvstormgr.h"
#include "nvbl_operatingmodes.h"
#include "nvstormgr.h"
#include "nvfuse.h"
#include "arapbpm.h"
#include "libfdt/libfdt.h"

#ifdef CONFIG_TRUSTED_FOUNDATIONS
#include "nvaboot_tf.h"
#endif
//added by jimmy
#define MAKE_SHOW_LOGO
#ifndef MAKE_SHOW_LOGO
#define debug_print(...) FastbootStatus(__VA_ARGS__)
#else
#ifdef LOGO_QC750
#include "bootload_logo_qc750.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N710)
#include "bootload_logo_n710.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N750)
#include "bootload_logo_n750.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N750_NEUTRAL)
#include "bootload_logo_n750_neutral.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N711)
#include "bootload_logo_n711.h"
#include "low_battery.h"
#elif defined(LOGO_N712)
#include "bootload_logo_n712.h"
#include "low_battery.h"
#elif defined(LOGO_N710_NEUTRAL)
#include "bootload_logo_n710_neutral.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N1010)
#include "bootload_logo_n1010.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N1050)
#include "bootload_logo_n1010.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N1020)
#include "bootload_logo_n1010_neutral.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_W1011A)
#include "bootload_logo_w1011a.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N1010_NEUTRAL)
#include "bootload_logo_n1010_neutral.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_N1011)
#include "bootload_logo_n1011.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_NS_14T004)
#include "bootload_logo_ns_14t004.h"
#include "low_battery.h"
#elif defined(LOGO_ITQ1000)
#include "bootload_logo_itq1000.h"
#include "precharge_logo_itq1000.h"
#elif defined(LOGO_WIKIPAD)
#include "bootload_logo_wikipad.h"
#include "low_battery_1280x800.h"
#elif defined(LOGO_ITQ700)
#include "bootload_logo_itq700.h"
#include "precharge_logo_itq700.h"
#elif defined(LOGO_N750_HP)
#include "bootloader_logo_hp_left.h"
#include "precharge_logo_hp.h"
#elif defined(LOGO_CYBTT10_BK)
#include "bootload_logo_cybtt10_bk.h"
#include "precharge_logo_cybtt10_bk.h"
//#include "precharge_logo_itq700.h"
//#include "precharge_logo_cybtt10_bk.h"
#elif defined(LOGO_MM3201)
#include "bootload_logo_mm3201.h"
#include "precharge_logo_itq701.h"
#elif defined(LOGO_ITQ701)
#include "bootload_logo_itq701.h"
#include "precharge_logo_itq701.h"
#elif defined(LOGO_NABI2S)
#include "bootload_logo_mt799s.h"
#include "low_battery_1280x800.h"
#else
#include "bootload_logo.h"
#include "low_battery.h"
#endif
#define debug_print(...) NvOsDebugPrintf(__VA_ARGS__)
#endif

#define SHOW_IMAGE_MENU 0
#define NVIDIA_GREEN 0x7fb900UL
#define ANDROID_MAGIC "ANDROID!"
#define ANDROID_MAGIC_SIZE 8
#define ANDROID_BOOT_NAME_SIZE 16
#define DO_JINGLE 0
#define JINGLE_CHANNELS 1
#define JINGLE_BITSPERSAMPLE 16
#define JINGLE_SAMPLERATE 44100

//  ATAG values defined in linux kernel's arm/include/asm/setup.h file
#define ATAG_CORE         0x54410001
#define ATAG_REVISION     0x54410007
#define ATAG_INITRD2      0x54420005
#define ATAG_CMDLINE      0x54410009
#define ATAG_SERIAL       0x54410006

#define FDT_SIZE_BL_DT_NODES (2048)

#define MACHINE_TYPE_HARMONY            2731
#define MACHINE_TYPE_VENTANA            2927
#define MACHINE_TYPE_WHISTLER           3241
#define MACHINE_TYPE_TEGRA_GENERIC      3333
#define MACHINE_TYPE_CARDHU             3436
#define MACHINE_TYPE_KAI                3897
#define MACHINE_TYPE_ARUBA              3437
#define MACHINE_TYPE_TEGRA_ENTERPRISE   3512
#define MACHINE_TYPE_P1852              3651
#define MACHINE_TYPE_TEGRA_P852         3667
#define MACHINE_TYPE_TAI                4311
#define MACHINE_TYPE_TEGRA_FDT          0xFFFFFFFF
#define MACHINE_TYPE_NABI2              4901
#define MACHINE_TYPE_TDW720F            4902
#define MACHINE_TYPE_CM9000             4903
#define MACHINE_TYPE_NABI2_3D             4904
#define MACHINE_TYPE_NABI2_XD              4905
#define MACHINE_TYPE_NABI_2S		4906
#define MACHINE_TYPE_QC750		   4909
#define MACHINE_TYPE_N710		   4910
#define MACHINE_TYPE_N1010		   4911
#define MACHINE_TYPE_N750		   4912
#define MACHINE_TYPE_WIKIPAD		   4913
#define MACHINE_TYPE_ITQ700		4914
#define MACHINE_TYPE_NS_14T004		4915
#define MACHINE_TYPE_ITQ1000		4916
#define MACHINE_TYPE_ITQ701             4917
#define MACHINE_TYPE_N1011             4918
#define MACHINE_TYPE_N1050             4919
#define MACHINE_TYPE_N711		           4920

#define MACHINE_TYPE_N1020             4921

#define MACHINE_TYPE_N020		           4925
#define MACHINE_TYPE_CYBTT10_BK		       4926
#define MACHINE_TYPE_W1011A            4927
#define MACHINE_TYPE_N712		           4928
#define MACHINE_TYPE_MM3201		       4929
#define MACHINE_TYPE_BIRCH		       4930
#define APBDEV_PMC_SCRATCH0_0_KEENHI_REBOOT_FLAG_BIT 29
#define APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT 30
#define APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT 31
#define APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_BIT 1

struct fdt_header *fdt_hdr = NULL;

//added by jimmy
typedef struct NvOdmImagerI2cConnectionRec
{
    NvU32 DeviceAddr;
    NvU32 I2cPort;
    NvOdmServicesI2cHandle hOdmI2c;
    // The write and read routines need to know how to
    // interpret the address/data given.

} NvOdmImagerI2cConnection;

struct machine_map
{
    const char* name;
    NvU32       id;
};

static const struct machine_map machine_types[] =
{
    {"Harmony", MACHINE_TYPE_HARMONY},
    {"Ventana", MACHINE_TYPE_VENTANA},
    {"Whistler",  MACHINE_TYPE_WHISTLER},
    {"Aruba",   MACHINE_TYPE_ARUBA},
    {"Cardhu",  MACHINE_TYPE_CARDHU},
    {"Kai",  MACHINE_TYPE_KAI},
    {"P1852",  MACHINE_TYPE_P1852},
    {"Enterprise",  MACHINE_TYPE_TEGRA_ENTERPRISE},
    {"P852", MACHINE_TYPE_TEGRA_P852},
    {"Tai",  MACHINE_TYPE_TAI},
    {"mt799", MACHINE_TYPE_NABI2},
    {"tdw720f", MACHINE_TYPE_TDW720F},
    {"cm9000", MACHINE_TYPE_CM9000},
    {"nabi2_3d", MACHINE_TYPE_NABI2_3D},
    {"nabi2_xd", MACHINE_TYPE_NABI2_XD},
    {"nabi_2s", MACHINE_TYPE_NABI_2S},
    {"qc750", MACHINE_TYPE_QC750},
    {"n710", MACHINE_TYPE_N710},
    {"n1010", MACHINE_TYPE_N1010},
    {"n750", MACHINE_TYPE_N750},
    {"wikipad", MACHINE_TYPE_WIKIPAD},
	{"itq700",MACHINE_TYPE_ITQ700},
    {"ns_14t004",MACHINE_TYPE_NS_14T004},
    {"itq1000",MACHINE_TYPE_ITQ1000},
    {"itq701",MACHINE_TYPE_ITQ701},
    {"n1011", MACHINE_TYPE_N1011},
    {"n1050", MACHINE_TYPE_N1050},
	  {"n711",MACHINE_TYPE_N711},
    {"n1020", MACHINE_TYPE_N1020},
	  {"n020",MACHINE_TYPE_N020},
	{"cybtt10_bk",MACHINE_TYPE_CYBTT10_BK},
	    {"w1011a", MACHINE_TYPE_W1011A},
	{"n712",MACHINE_TYPE_N712},
	{"mm3201",MACHINE_TYPE_MM3201},
	{"birch",MACHINE_TYPE_BIRCH},
};

#define BOARD_ID_E1206 0x0C06

#define TEMP_BUFFER_SIZE (1*1024*1024)

#define BOOTLOADER_NAME "bootloader"
#define MICROBOOT_NAME "microboot"
#define BCT_NAME        "bcttable"

#define FB_CHECK_ERROR_CLEANUP(expr) \
    do \
    { \
        e = (expr); \
        if (e == NvError_Timeout) \
            continue; \
        if (e != NvSuccess) \
            goto fail; \
    } while (0);

static const char *BootloaderMenuString[]=
{
    "Boot Normally\n",
    "Fastboot Protocol\n",
    "Recovery Kernel\n",
    "Forced Recovery\n"
};

#if SHOW_IMAGE_MENU
#include "android.h"
#include "usb.h"
static Icon *s_RadioImages[] =
{
    &s_android,
    &s_usb
};
#endif

enum
{
    MENU_SELECT_LINUX,
    MENU_SELECT_USB,
    MENU_SELECT_RCK,
    MENU_SELECT_FORCED_RECOVERY
};
enum
{
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,
    KEY_IGNORE = 0x7fffffff
};
enum {
	
	SYSTEM_REBOOT  = 0,
	POWERKEY_PRESS = 1,
	POWERKEY_LPRESS = 2,
	POWER_NOPRESS = 3,
	USB_CHARGING,
	USB_NOCHARGING,

};
typedef struct AndroidBootImgRec
{
    unsigned char magic[ANDROID_MAGIC_SIZE];
    unsigned kernel_size;
    unsigned kernel_addr;

    unsigned ramdisk_size;
    unsigned ramdisk_addr;

    unsigned second_size;
    unsigned second_addr;

    unsigned tags_addr;
    unsigned page_size;
    unsigned unused[2];

    unsigned char name[ANDROID_BOOT_NAME_SIZE];
    unsigned char cmdline[ANDROID_BOOT_CMDLINE_SIZE];

    unsigned id[8];
} AndroidBootImg;

const PartitionNameMap gs_PartitionTable[] =
{
    { "recovery", RECOVERY_KERNEL_PARTITION, FileSystem_Basic},
    { "boot", KERNEL_PARTITON, FileSystem_Basic},
    { "dtb", "DTB", FileSystem_Basic},
    { "ubuntu", "UBN", FileSystem_Basic},
    { "system", "APP", FileSystem_Ext4},
    { "cache", "CAC", FileSystem_Ext4},
    { "misc", "MSC", FileSystem_Basic},
    { "staging", "USP", FileSystem_Basic},
    { "userdata", "UDA", FileSystem_Ext4},
    { BCT_NAME, "BCT", FileSystem_Basic},
    { BOOTLOADER_NAME, "EBT", FileSystem_Basic},
    { MICROBOOT_NAME, "NVC", FileSystem_Basic}
};

enum
{
    PLATFORM_NAME_INDEX = 0,
    MSG_FASTBOOT_MENU_SEL_INDEX,
    MSG_NO_KEY_DRIVER_INDEX
};

static const char *s_WhistleBLMsg[] =
{
    "Whistler\0",
    "Press scroll wheel to select, Scroll for selection move\n",
    "Scroll wheel not found.. Booting OS\n"
};
static const char *s_HarmonyBLMsg[] =
{
    "Harmony\0",
    "Press <Enter> to select, Arrow key (Left, Right) for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_TangoBLMsg[] =
{
    "Tango\0",
    "Press <Menu> to select, Home(Left) and Back(Right) for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_VentanaBLMsg[] =
{
    "Ventana\0",
    "Press <Wake> to select, Home(Left) and Back(Right) for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_CardhuBLMsg[] =
{
    "Cardhu\0",
    "Press <Volume Down> to select, <Volume Up> for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_KaiBLMsg[] =
{
    "Kai\0",
    "Press <Volume Down> to select, <Volume Up> for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_EnterpriseBLMsg[] =
{
    "Enterprise\0",
    "Press <Menu> to select, Home(Left) and Back(Right) for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_TaiBLMsg[] =
{
    "Tai\0",
    "Checking for RCK.. press key <Menu> in 5 sec to enter RCK\n",
    "Press <Menu> to select, Home(Left) and Back(Right) for selection move\n",
    "Key driver not found.. Booting OS\n"
};

static const char *s_DefaultBLMsg[] =
{
    "Unknown\0"
    "Use scroll wheel or keyboard for movement and selection\n",
    "Neither Scroll Wheel nor Keyboard are detected ...Booting OS\n",
};

static const char **s_PlatformBlMsg = NULL;

NvAbootHandle gs_hAboot = NULL;
NvBool g_OdmKbdExists = NV_FALSE;
NvDdkKbcHandle g_hDdkKbc = NULL;
NvOdmScrollWheelHandle g_hScroll = NULL;
NvOsSemaphoreHandle g_hScrollSema = NULL;
NvOsSemaphoreHandle g_hDdkKbcSema = NULL;

NvU32 g_wb0_address;

extern PrettyPrintContext s_PrintCtx;

static NvRmSurface s_Framebuffer;
static NvRmSurface s_Frontbuffer;
static void       *s_FrontbufferPixels = NULL;
static NvDdkBlockDevMgrDeviceId s_SecondaryBoot = 0;
static NvRmMemHandle s_hWarmbootHandle;
extern NvU32        bl_nDispController;
static NvRmDeviceHandle s_hRm = NULL;
static NvBool s_RecoveryKernel = NV_FALSE;

#ifdef LPM_BATTERY_CHARGING
#include "nvodm_charging.h"
#include "nvaboot_usbf.h"
static NvOdmChargingConfigData s_ChargingConfigData;
static char s_DispMessage[255];
static NvOdmPmuDeviceHandle s_pPmu;
static NvU8 Reboot_cmd = 0,power_key_boot = 0;
//lpm header declarations
static NvError NvAppDoCharge(NvU32 OSChrgInPercent, NvU32 ExitChrgInPercent);
static void NvAppShutDownDisplay(void);
static void NvAppNotifyOnDisplay(void);
static NvError NvAppChargeBatteryIfLow(void);
#ifdef MAKE_LOW_BATTERY_DECT
static NvError KhAppChargeBatteryIfLow(RadioMenu *BootLogoMenu,NvU32 logoX,NvU32 logoY);//added by wantianpei
#endif
#ifdef MAKE_POWER_KEY_2SE_DECT
static NvError NvAppCheckPowerDownState(void);//added by jimmy
#endif
static int NvCheckForceRecoveryMode();
static void NvAppHalt(void);
#endif

NvBool NvBlQueryGetNv3pServerFlag(void);

static NvError LoadKernel(NvU32 , const char *,NvU8 *, NvU32 );
static NvError StartFastbootProtocol(NvU32);

static NvBool HasValidKernelImage(NvU8**, NvU32 *, const char *);
static const PartitionNameMap *MapFastbootToNvPartition(const char*);
static NvError BootAndroid(
    void *KernelImage,
    NvU32 LoadSize,
    NvU32 RamBase);

static NvError SetupTags(
    const AndroidBootImg *hdr,
    NvUPtr RamdiskPa);
static int BootloaderMenu(RadioMenu *Menu, NvU32 Timeout);
static NvError MainBootloaderMenu(NvU32 RamBase, NvU32 TimeOut);
static void ShowPlatformBlMsg();
static NvU32 BootloaderTextMenu(
    TextMenu *Menu,
    NvU32 Timeout);
static NvU8 GetKeyEvent();

static const char *ErrorName(NvError err)
{
    switch(err)
    {
        #define NVERROR(name, value, str) case NvError_##name: return #name;
        #include "nverrval.h"
        #undef NVERROR
        default:
            break;
    }
    return "Unknown error code";
}

NvU32 TwoHundredHzSquareWave(NvU8* Buff, NvU32 MaxSize)
{
    static NvU32 Remain = 0;
    static NvU32 Toggle = 0;
    const NvU32 OnSamples = JINGLE_SAMPLERATE / 200;
    const NvU32 OffSamples = (JINGLE_SAMPLERATE+199) / 200;
    NvU32 NumSamples = MaxSize / ((JINGLE_BITSPERSAMPLE/8) * JINGLE_CHANNELS);
    NvU32 i;

    #if JINGLE_BITSPERSAMPLE==16
    NvU16* Buff16 = (NvU16*)Buff;
    #elif JINGLE_BITSPERSAMPLE==32
    NvU32* Buff32 = (NvU32*)Buff;
    #endif

    if (!Remain)
        Remain = OnSamples;

    for (i=0; i < NumSamples; i += JINGLE_CHANNELS)
    {
        NvU32 j;
        for (j=0; j<JINGLE_CHANNELS; j++)
        {
            #if JINGLE_BITSPERSAMPLE==16
            Buff16[i+j] = (Toggle) ? 0x7fff : 0x8000;
            #elif JINGLE_BITSPERSAMPLE==32
            Buff32[i+j] = (Toggle) ? 0x7ffffffful : 0x80000000ul;
            #endif
        }

        Remain--;
        if (!Remain)
        {
            Toggle ^= 1;
            Remain = (Toggle) ? OffSamples : OnSamples;
        }
    }
    return MaxSize;
}

static void InitBlMessage(void)
{
    NvU32 i;
    const NvU8 *OdmPlatformName = NvOdmQueryPlatform();
    const char **BlMsgs[] =
    {
        s_WhistleBLMsg,
        s_VentanaBLMsg,
        s_CardhuBLMsg,
        s_KaiBLMsg,
        s_EnterpriseBLMsg,
        s_TaiBLMsg,
        NULL
    };
    NvOdmBoardInfo BoardInfo;

    if (!OdmPlatformName)
    {
         s_PlatformBlMsg = s_DefaultBLMsg;
         return;
    }

    /* First check for harmony and if it is harmonmy then for tango*/
    if (!NvOsMemcmp(OdmPlatformName, s_HarmonyBLMsg[PLATFORM_NAME_INDEX],
                                   sizeof(OdmPlatformName)))
    {
        if (NvOdmPeripheralGetBoardInfo(BOARD_ID_E1206, &BoardInfo))
            s_PlatformBlMsg = s_TangoBLMsg;
        else
            s_PlatformBlMsg = s_HarmonyBLMsg;
        return;
    }
    for (i =0; BlMsgs[i] != NULL; ++i)
    {
         if (!NvOsMemcmp(OdmPlatformName, BlMsgs[i][PLATFORM_NAME_INDEX],
                                            sizeof(OdmPlatformName)))
         {
              s_PlatformBlMsg = BlMsgs[i];
              return;
         }
    }
    s_PlatformBlMsg = s_DefaultBLMsg;
}

static void DeinitInputKeys(void)
{
    if (g_hDdkKbc)
    {
        NvDdkKbcStop(g_hDdkKbc);
        if (g_hDdkKbcSema)
        {
            NvOsSemaphoreDestroy(g_hDdkKbcSema);
            g_hDdkKbcSema = NULL;
        }
        NvDdkKbcClose(g_hDdkKbc);
        g_hDdkKbc = NULL;
    }
    if (s_hRm)
    {
        NvRmClose(s_hRm);
        s_hRm = NULL;
    }
    if (g_hScrollSema)
    {
        NvOsSemaphoreDestroy(g_hScrollSema);
        g_hScrollSema = NULL;
    }
    if (g_hScroll)
    {
        NvOdmScrollWheelClose(g_hScroll);
        g_hScroll = NULL;
    }
}

static int mInputInited =0;

static NvError InitInputKeys(void)
{
    NvError e = NvSuccess;

    if(mInputInited==1){
	return e;
   }
    mInputInited =1;

    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&g_hScrollSema, 0));
    g_hScroll = NvOdmScrollWheelOpen(
        (NvOdmOsSemaphoreHandle)g_hScrollSema,
            (NvOdmScrollWheelEvent_RotateClockWise |
                NvOdmScrollWheelEvent_RotateAntiClockWise |
                    NvOdmScrollWheelEvent_Press));

    if (!g_hScroll && !g_OdmKbdExists)
    {
        g_OdmKbdExists = NvOdmKeyboardInit();
    }
    if (!g_hScroll && !g_OdmKbdExists)
    {
        NV_CHECK_ERROR_CLEANUP(NvRmOpen(&s_hRm, 0));
        NV_CHECK_ERROR_CLEANUP(NvDdkKbcOpen(s_hRm, &g_hDdkKbc));
        NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&g_hDdkKbcSema, 0));
        NV_CHECK_ERROR_CLEANUP(NvDdkKbcStart(g_hDdkKbc, g_hDdkKbcSema));
    }

fail:
    if (e != NvSuccess)
        DeinitInputKeys();

    return e;
}

static void ClearDisplay()
{
    NvU32 Size = NvRmSurfaceComputeSize(&s_Frontbuffer);
    NvOsMemset(s_FrontbufferPixels, 0, Size);
}

static NvBool IsPartitionExposed(const char *PartitionName)
{
    NvU32 PartitionIdGPT;
    NvU32 PartitionId;
    NvU32 Id;
    NvError e;
    NvBool RetVal = NV_FALSE;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName("GP1", &Id));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(PartitionName, &PartitionId));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName("GPT", &PartitionIdGPT));

    Id = NvPartMgrGetNextId(Id);
    while (Id != 0 && Id != PartitionIdGPT)
    {
        if (Id == PartitionId)
        {
            RetVal = NV_TRUE;
            break;
        }
        Id = NvPartMgrGetNextId(Id);
    }

fail:
    return RetVal;
}

static NvError SetOdmData ( NvU32 OdmData)
{
    NvError e = NvSuccess;
    NvU32 Size = 0;
    NvU32 BctLength = 0;
    NvBctHandle Bct = NULL;
    NvBitHandle Bit = NULL;
    NvU32 Instance = 1;
    NvBlOperatingMode op_mode;
    NvU32 bct_part_id;
    NvU8 *buf = 0;
    NvRmDeviceHandle hRm = NULL;
    NvU32 startTime, endTime;

    startTime = NvOsGetTimeMS();
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&BctLength, NULL, NULL));
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &Bct));
    Size = sizeof(OdmData);
    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(Bct, NvBctDataType_OdmOption,
            &Size, &Instance, &OdmData)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrGetIdByName( "BCT", &bct_part_id )
    );

    op_mode = NvFuseGetOperatingMode();

    /* re-sign and write the BCT */
    buf = NvOsAlloc(BctLength);
    NV_ASSERT(buf);
    if (!buf)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvBuBctCryptoOps(Bct, op_mode, &BctLength, buf,
            NvBuBlCryptoOp_EncryptAndSign )
    );
    NV_CHECK_ERROR_CLEANUP(NvBitInit(&Bit));
    NV_CHECK_ERROR_CLEANUP(
        NvBuBctUpdate(Bct, Bit, bct_part_id, BctLength, buf )
    );

    /* Updating OdmData in SCRATCH20 */
    NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
    NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
        APBDEV_PMC_SCRATCH20_0, OdmData);
    NvRmClose(hRm);
    endTime = NvOsGetTimeMS();
    NvOsDebugPrintf("Time taken in updating odmdata: %u ms\n", endTime - startTime);

fail:
    return e;

}

#define WAIT_WHILE_LOCKING 10000
#define WAIT_IN_BOOTLOADER_MENU 10000
#define UNLOCKED_BIT_IN_ODMDATA 8
#define WAIT_BIT_IN_ODMDATA 12
#define DISP_FONT_HEIGHT 16
#define BLANK_LINES_BEFORE_MENU 1
#define BLANK_LINES_AFTER_MENU 1
static NvBool s_IsUnlocked = NV_FALSE;
static const char *s_UnlockMsg[] =
{
    "Don't unlock it\n",
    "Unlock it\n"
};
static const char *s_LockMsg[] =
{
    "Don't Lock it\n",
    "Lock it\n"
};
static const char *s_PartitionType[] =
{
    "basic",
    "yaffs2",
    "ext4"
};
static const char *FastbootProtocolMenuString[]=
{
    "Bootloader\n",
    "Continue\n",
    "Reboot-bootloader\n",
    "Reboot\n",
    "Poweroff\n"
};

enum
{
    BOOTLOADER,
    CONTINUE,
    REBOOT_BOOTLOADER,
    REBOOT,
    POWEROFF,
    BOOT,
    FASTBOOT
};

#define CHECK_COMMAND(y) (!(NvOsMemcmp(CmdData, y, NvOsStrlen(y))))
#define COPY_RESPONSE(y) NvOsMemcpy(Response + NvOsStrlen(Response), y, \
    NvOsStrlen(y))
#define SEND_RESPONSE(y) \
    do \
    { \
        FB_CHECK_ERROR_CLEANUP(Nv3pTransportSendTimeOut( \
            hTrans, (NvU8*)y, NvOsStrlen(y), 0, TimeOutMs)); \
        NvOsDebugPrintf("Response sent: %s\n",y); \
    } while (0);

#define FASTBOOT_MAX_SIZE 256

static void SetFastbootBit()
{
    NvU32 reg = 0;
    NvRmDeviceHandle hRm = NULL;

    /* Writing SCRATCH0 to start fastboot protocol on reboot */
    NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
    reg = NV_REGR(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
        APBDEV_PMC_SCRATCH0_0);

    reg = reg | (1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
    NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
        APBDEV_PMC_SCRATCH0_0, reg);
    NvRmClose(hRm);
}

static NvError FastbootGetOneVar(const char *CmdData, char *Response)
{
    NvError e = NvSuccess;
    const char *YesStr = "yes";
    const char *NoStr = "no";

    if (CHECK_COMMAND("version-bootloader"))
        COPY_RESPONSE("1.0");
    else if (CHECK_COMMAND("version-baseband"))
        COPY_RESPONSE("2.0");
    else if (CHECK_COMMAND("version"))
        COPY_RESPONSE("0.4");
    else if (CHECK_COMMAND("serialno"))
    {
        NvU32 serialno[4];
        NvRmDeviceHandle hRm = NULL;

        NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
        NV_CHECK_ERROR_CLEANUP(
            NvRmQueryChipUniqueId(hRm, sizeof(serialno), &serialno));
        NvRmClose(hRm);
        NvOsSnprintf(Response + NvOsStrlen(Response), 17, "%08x%08x",
            serialno[1], serialno[0]);
    }
    else if (CHECK_COMMAND("mid"))
        COPY_RESPONSE("001");
    else if (CHECK_COMMAND("product"))
        COPY_RESPONSE((char *)NvOdmQueryPlatform());
    else if (CHECK_COMMAND("secure"))
        COPY_RESPONSE(s_IsUnlocked ? NoStr : YesStr);
    else if (CHECK_COMMAND("unlocked"))
            COPY_RESPONSE(s_IsUnlocked ? YesStr : NoStr);
    else if (CHECK_COMMAND("partition-size:"))
    {
        NvU64 Start, Num;
        NvU32 SectorSize;
        char *Name = (char *)(CmdData + NvOsStrlen("partition-size:"));
        const PartitionNameMap *NvPart = MapFastbootToNvPartition(Name);

        if (NvPart)
        {
            NV_CHECK_ERROR_CLEANUP(
                NvAbootGetPartitionParameters(gs_hAboot, NvPart->NvPartName,
                    &Start, &Num, &SectorSize)
            );
            NvOsSnprintf(Response + NvOsStrlen(Response), 19, "0x%08x%08x",
                (NvU32)((Num*SectorSize)>>32),(NvU32)(Num*SectorSize));
        }
    }
    else if (CHECK_COMMAND("partition-type:"))
    {
        char *Name = (char *)(CmdData + NvOsStrlen("partition-type:"));
        const PartitionNameMap *NvPart = MapFastbootToNvPartition(Name);

        if (NvPart)
        {
            NvOsSnprintf(Response + NvOsStrlen(Response), 7, "%s",
                s_PartitionType[NvPart->FileSystem]);
        }
    }

fail:
    return e;
}

static NvError FastbootGetVar(Nv3pTransportHandle hTrans, const char *CmdData)
{
    NvError e = NvSuccess;
    NvU32 TimeOutMs = 1000;
    char Response[FASTBOOT_MAX_SIZE];

    if (CHECK_COMMAND("all"))
    {
        NvU32 i,j;
        char TempCmd[FASTBOOT_MAX_SIZE];
        const char *Variables[] =
        {
            "version-bootloader",
            "version-baseband",
            "version",
            "serialno",
            "mid",
            "product",
            "secure",
            "unlocked"
        };
        const char *PartitionCmd[]=
        {
            "partition-size:",
            "partition-type:"
        };
        const char *FlashablePartition[] =
        {
            "bootloader",
            "recovery",
            "boot",
            "system",
            "cache",
            "userdata"
        };
        for (i = 0; i < NV_ARRAY_SIZE(Variables); ++i)
        {
            NvOsMemset(Response, 0, FASTBOOT_MAX_SIZE);
            COPY_RESPONSE("INFO");
            COPY_RESPONSE(Variables[i]);
            COPY_RESPONSE(": ");
            FastbootGetOneVar(Variables[i], Response);
            SEND_RESPONSE(Response);
        }

        /* partition-type and partition-size */
        for (i = 0; i < NV_ARRAY_SIZE(FlashablePartition); ++i)
        {
            for (j = 0; j < NV_ARRAY_SIZE(PartitionCmd); j++)
            {
                NvOsMemset(Response, 0, FASTBOOT_MAX_SIZE);
                NvOsMemset(TempCmd, 0, FASTBOOT_MAX_SIZE);
                COPY_RESPONSE("INFO");
                COPY_RESPONSE(PartitionCmd[j]);
                COPY_RESPONSE(FlashablePartition[i]);
                NvOsMemcpy(TempCmd, Response + NvOsStrlen("INFO"),
                NvOsStrlen(Response + NvOsStrlen("INFO")));
                COPY_RESPONSE(": ");
                FastbootGetOneVar(TempCmd, Response);
                SEND_RESPONSE(Response);
            }
        }
        NvOsMemset(Response, 0, FASTBOOT_MAX_SIZE);
        COPY_RESPONSE("OKAY");
        SEND_RESPONSE(Response);
    }
    else
    {
        NvOsMemset(Response, 0, FASTBOOT_MAX_SIZE);
        COPY_RESPONSE("OKAY");
        FastbootGetOneVar(CmdData, Response);
        SEND_RESPONSE(Response);
    }

fail:
    return e;
}

static NvError FastbootProtocol(Nv3pTransportHandle hTrans, NvU8** pBootImg,
        NvU32 *pLoadSize, NvU32 *ExitOption)
{
    NvError e = NvSuccess;
    NvError UsbError = NvError_UsbfCableDisConnected;
    NvBool IsOdmSecure = NV_FALSE;
    NvU8 *CmdData = NULL;
    NvU8 *Image = NULL;
    NvU8 *data_buffer = NULL;
    NvU32 ImageSize = 0;
    NvU32 TimeOutMs = 1000;
    NvU32 OdmData = 0;
    NvU32 BytesRcvd = 0;
    NvU32 X = 0;
    NvU32 Y = 0;
    NvU32 FastbootMenuOption;
    TextMenu *FastbootProtocolMenu = NULL;
    NvStorMgrFileHandle hStagingFile = NULL;
    char *partName = NULL;
    const char *GetVarCmd = "getvar:";
    const char *DownloadCmd = "download:";
    const char *FlashCmd = "flash:";
    const char *EraseCmd = "erase:";
    const char *OemCmd = "oem ";
    const char *OkayResp = "OKAY";

    OdmData = nvaosReadPmcScratch();
    if (!(OdmData & (1 << UNLOCKED_BIT_IN_ODMDATA)))
        s_IsUnlocked = NV_TRUE;

    if (NvFuseGetOperatingMode() == NvBlOperatingMode_OdmProductionSecure)
        IsOdmSecure = NV_TRUE;

    CmdData = NvOsAlloc(FASTBOOT_MAX_SIZE);
    if (!CmdData)
        return NvError_InsufficientMemory;

    partName = NvOsAlloc(NVPARTMGR_PARTITION_NAME_LENGTH);
    if (partName == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset(partName, 0x0, NVPARTMGR_PARTITION_NAME_LENGTH);

    FastbootProtocolMenu = FastbootCreateTextMenu(
        NV_ARRAY_SIZE(FastbootProtocolMenuString), 0,
            FastbootProtocolMenuString);
    ClearDisplay();
    ShowPlatformBlMsg();
    Y = s_PrintCtx.y;
    FastbootMenuOption = BootloaderTextMenu(FastbootProtocolMenu, 0);

    while (1)
    {
        NvOsMemset(CmdData, 0, FASTBOOT_MAX_SIZE);

        /* Handle USB traffic */
        if ((UsbError == NvError_UsbfCableDisConnected) || BytesRcvd)
        {
            UsbError = Nv3pTransportReceiveTimeOut(hTrans, CmdData,
                FASTBOOT_MAX_SIZE, &BytesRcvd, 0, 0);
        }
        if (UsbError != NvError_UsbfCableDisConnected)
        {
            UsbError = Nv3pTransportReceiveIfComplete(hTrans, CmdData,
                FASTBOOT_MAX_SIZE, &BytesRcvd);
        }

        /* Handle key events */
        if (!BytesRcvd)
        {
            NvU32 keyEvent = GetKeyEvent();
            switch (keyEvent)
            {
                case KEY_UP:
                    FastbootTextMenuSelect(FastbootProtocolMenu, NV_FALSE);
                    FastbootDrawTextMenu(FastbootProtocolMenu, X, Y);
                    break;
                case KEY_DOWN:
                    FastbootTextMenuSelect(FastbootProtocolMenu, NV_TRUE);
                    FastbootDrawTextMenu(FastbootProtocolMenu, X, Y);
                    break;
                case KEY_ENTER:
                    *ExitOption = FastbootProtocolMenu->CurrOption;
                    e = NvSuccess;
                    goto fail;
            }
            continue;
        }
        NvOsDebugPrintf("Cmd Rcvd: %s\n", CmdData);

        /* Handle Fastboot Command */
        if (CHECK_COMMAND(OemCmd))
        {
            if (!NvOsMemcmp(CmdData+NvOsStrlen(OemCmd), "lock",
                NvOsStrlen("lock")))
            {
                if (s_IsUnlocked == NV_TRUE)
                {
                    TextMenu *LockMenu = FastbootCreateTextMenu(
                        NV_ARRAY_SIZE(s_LockMsg), 0, s_LockMsg);
                    SEND_RESPONSE("INFOShowing Options on Display.");
                    SEND_RESPONSE("INFOUse device keys for selection.");
                    ClearDisplay();
                    ShowPlatformBlMsg();
                    FastbootMenuOption =
                        BootloaderTextMenu(LockMenu, WAIT_WHILE_LOCKING);
                    if (FastbootMenuOption == 1)
                    {
                        SEND_RESPONSE("INFOLocking...");
                        NV_CHECK_ERROR_CLEANUP(
                            SetOdmData(OdmData |
                                (1 << UNLOCKED_BIT_IN_ODMDATA))
                        );
                        s_IsUnlocked = NV_FALSE;
                        SEND_RESPONSE("INFOBootloader is locked now.");
                    }
                    else
                    {
                        SEND_RESPONSE("INFOExit without Locking.");
                    }
                    ClearDisplay();
                    ShowPlatformBlMsg();
                    FastbootMenuOption =
                        BootloaderTextMenu(FastbootProtocolMenu, 0);
                }
                else
                {
                    SEND_RESPONSE("INFOBootloader is already locked.");
                }
            }
            else if (!NvOsMemcmp(CmdData+NvOsStrlen(OemCmd), "unlock",
                NvOsStrlen("unlock")))
            {
                if (s_IsUnlocked == NV_FALSE)
                {
                    TextMenu *UnlockMenu = FastbootCreateTextMenu(
                         NV_ARRAY_SIZE(s_UnlockMsg), 0, s_UnlockMsg);
                    SEND_RESPONSE("INFOShowing Options on Display.");
                    SEND_RESPONSE("INFOUse device keys for selection.");
                    ClearDisplay();
                    ShowPlatformBlMsg();
                    FastbootMenuOption =
                        BootloaderTextMenu(UnlockMenu, WAIT_WHILE_LOCKING);
                    if (FastbootMenuOption == 1)
                    {
                        SEND_RESPONSE("INFOerasing userdata...");
                        NV_CHECK_ERROR_CLEANUP(NvAbootBootfsFormat("UDA"));
                        SEND_RESPONSE("INFOerasing userdata done");
                        SEND_RESPONSE("INFOerasing cache...");
                        NV_CHECK_ERROR_CLEANUP(NvAbootBootfsFormat("CAC"));
                        SEND_RESPONSE("INFOerasing cache done");
                        SEND_RESPONSE("INFOunlocking...");
                        NV_CHECK_ERROR_CLEANUP(
                            SetOdmData(OdmData &
                                (~(1 << UNLOCKED_BIT_IN_ODMDATA)))
                        );
                        s_IsUnlocked = NV_TRUE;
                        SEND_RESPONSE("INFOBootloader is unlocked now.");
                    }
                    else
                    {
                        SEND_RESPONSE("INFOExit without Unlocking.");
                    }
                    ClearDisplay();
                    ShowPlatformBlMsg();
                    FastbootMenuOption =
                        BootloaderTextMenu(FastbootProtocolMenu, 0);
                }
                else
                {
                    SEND_RESPONSE("INFOBootloader is already unlocked.");
                }
            }
            else
            {
                SEND_RESPONSE("INFOUnknown Command!");
            }
            SEND_RESPONSE(OkayResp);
        }
        else if (CHECK_COMMAND(GetVarCmd))
        {
            NV_CHECK_ERROR_CLEANUP(FastbootGetVar(hTrans, (char *)CmdData +
                NvOsStrlen(GetVarCmd))
            );
        }
        else if (CHECK_COMMAND(DownloadCmd))
        {
            char Response[32];
            NvU32 SectorSize = 0;
            NvU64 Start = 0;
            NvU64 Num = 0;
            NvS64 StagingPartitionSize = 0;
            NvU32 temp_bytes_rcvd = 0;
            NvU32 so_far = 0;

            if (s_IsUnlocked == NV_FALSE)
            {
                SEND_RESPONSE("FAILBootloader is locked.");
                continue;
            }

            ImageSize =
                NvUStrtol((char*)(CmdData+NvOsStrlen(DownloadCmd)), NULL, 16);
            if (!(ImageSize > 0))
            {
                SEND_RESPONSE("FAILBad image size");
                continue;
            }

            NvOsStrncpy(partName, "USP", NvOsStrlen("USP"));
            e = NvAbootGetPartitionParameters(gs_hAboot, partName, &Start,
                    &Num, &SectorSize);
            StagingPartitionSize = (e == NvSuccess) ? (Num * SectorSize) : 0;
            if (StagingPartitionSize < ImageSize)
            {
                NvOsStrncpy(partName, "CAC", NvOsStrlen("CAC"));
                e = NvAbootGetPartitionParameters(gs_hAboot, partName, &Start,
                    &Num, &SectorSize);
                StagingPartitionSize = (e == NvSuccess) ? (Num * SectorSize) : 0;
                if (StagingPartitionSize < ImageSize)
                {
                    SEND_RESPONSE("FAILStaging partition size is not big enough.");
                    continue;
                }
            }

            /*  open the staging partition */
            NV_CHECK_ERROR_CLEANUP(
                NvStorMgrFileOpen(partName, NVOS_OPEN_WRITE, &hStagingFile)
            );

            /*  create 1MB of temporary buffer for caching the received data*/
            data_buffer = NvOsAlloc(TEMP_BUFFER_SIZE);
            if(!data_buffer)
            {
                SEND_RESPONSE("FAILInsufficient memory.");
                continue;
            }

            NvOsSnprintf(Response, sizeof(Response), "DATA%08x\n",
                ImageSize);
            SEND_RESPONSE(Response);

            /*  write only maximum 1MB of data at a time */
            while (so_far < ImageSize)
            {
                NvU32 temp_size = TEMP_BUFFER_SIZE;
                if ((ImageSize - so_far) < temp_size)
                    temp_size = ImageSize - so_far;

                FB_CHECK_ERROR_CLEANUP(
                    Nv3pTransportReceiveTimeOut(hTrans, data_buffer,
                        temp_size, &temp_bytes_rcvd, 0, TimeOutMs)
                );

                if (temp_bytes_rcvd != temp_size)
                {
                    SEND_RESPONSE("INFOReceived Data of Bad Length.");
                    e = NvError_Nv3pBadReceiveLength;
                    goto fail;
                }

                NV_CHECK_ERROR_CLEANUP(
                    NvStorMgrFileWrite(hStagingFile, data_buffer, temp_size,
                        &temp_bytes_rcvd)
                );

                if (temp_bytes_rcvd != temp_size)
                {
                    SEND_RESPONSE("INFOWriting Failed.");
                    e = NvError_FileWriteFailed;
                    goto fail;
                }
                so_far += temp_size;
            }

            NvOsFree(data_buffer);
            data_buffer = NULL;
            NvStorMgrFileClose(hStagingFile);
            hStagingFile = NULL;

            SEND_RESPONSE(OkayResp);
        }
        else if (CHECK_COMMAND("boot"))
        {
            if (s_IsUnlocked == NV_FALSE)
            {
                SEND_RESPONSE("FAILBootloader is locked.");
                continue;
            }
            /*  load boot.img from staging partition to memory */
            Image = NvOsAlloc(ImageSize);
            if (Image == NULL)
            {
                SEND_RESPONSE("FAILInsufficient memory");
                continue;
            }

            NV_CHECK_ERROR_CLEANUP(
                NvAbootBootfsReadNBytes(gs_hAboot, partName, 0, ImageSize,
                    Image)
            );
            NV_CHECK_ERROR_CLEANUP(NvAbootBootfsFormat(partName));
            *pBootImg = Image;
            *pLoadSize = ImageSize;
            SEND_RESPONSE(OkayResp);
            *ExitOption = BOOT;
            goto fail;
        }
        else if (CHECK_COMMAND("reboot-bootloader"))
        {
            *ExitOption = REBOOT_BOOTLOADER;
            SEND_RESPONSE(OkayResp);
            //  reboot-bootloader breaks out of the loop
            goto fail;
        }
        else if (CHECK_COMMAND("reboot"))
        {
            *ExitOption = REBOOT;
            SEND_RESPONSE(OkayResp);
            //  reboot breaks out of the loop
            goto fail;
        }
        else if (CHECK_COMMAND("continue"))
        {
            *ExitOption = CONTINUE;
            SEND_RESPONSE(OkayResp);
            goto fail;
        }
        else if (CHECK_COMMAND(FlashCmd))
        {
            char *Name = (char *)(CmdData + NvOsStrlen(FlashCmd));
            const PartitionNameMap *NvPart = MapFastbootToNvPartition(Name);
            NvBool isExposed = NV_FALSE;

            if (s_IsUnlocked == NV_FALSE)
            {
                SEND_RESPONSE("FAILBootloader is locked.");
                continue;
            }

            if (!NvPart)
            {
                SEND_RESPONSE("FAILInvalid Partition Name.");
                continue;
            }

            isExposed = IsPartitionExposed(NvPart->NvPartName);
            if (IsOdmSecure && isExposed == NV_FALSE)
                e = NvInstallBlob(partName);
            else
                e = NvFastbootUpdateBin(partName, ImageSize, NvPart);

            ImageSize = 0;
            if (e != NvSuccess)
                goto fail;
            NV_CHECK_ERROR_CLEANUP(NvAbootBootfsFormat(partName));
            SEND_RESPONSE(OkayResp);
        }
        else if (CHECK_COMMAND(EraseCmd))
        {
            char *Name = (char *)(CmdData + NvOsStrlen(EraseCmd));
            const PartitionNameMap *NvPart = MapFastbootToNvPartition(Name);

            if (!NvPart)
            {
                SEND_RESPONSE("FAILInvalid Partition Name.");
                continue;
            }

            NV_CHECK_ERROR_CLEANUP(
                NvAbootBootfsFormat(NvPart->NvPartName)
            );

            SEND_RESPONSE(OkayResp);
        }
        else
        {
            e = NvError_Nv3pUnrecoverableProtocol;
            goto fail;
        }
    }

fail:
    if (e != NvSuccess)
    {
        if (hTrans)
        {
            char FailCmd[30];
            NvOsSnprintf(FailCmd, sizeof(FailCmd), "FAIL(%s)", ErrorName(e));
            Nv3pTransportSendTimeOut(hTrans, (NvU8 *)FailCmd,
                NvOsStrlen(FailCmd), 0, TimeOutMs);
        }
        if (Image)
        {
            NvOsFree(Image);
            Image = NULL;
        }
        if (hStagingFile)
        {
            NvStorMgrFileClose(hStagingFile);
            hStagingFile = NULL;
        }
        if (data_buffer)
        {
            NvOsFree(data_buffer);
            data_buffer = NULL;
        }
        if (partName && partName[0])
            NvAbootBootfsFormat(partName);
        NvOsDebugPrintf("Failed to process command %s error(0x%x)\n",
            (char*)CmdData, e);
    }
    if (partName)
    {
        NvOsFree(partName);
        partName = NULL;
    }

    if (CmdData)
    {
        NvOsFree(CmdData);
        CmdData = NULL;
    }

    return e;
}

static NvBool ShowMenu(void)
{
    NvBool show = NV_TRUE;

    if  (!(NvOsStrcmp((const char *)NvOdmQueryPlatform(),"P852") &&
        NvOsStrcmp((const char *)NvOdmQueryPlatform(),"P1852")))
    {
        /* No menu for these platforms so set count to 0 */
        show = NV_FALSE;
        debug_print("Board: %s\r\n", NvOdmQueryPlatform());
    }

    return show;
}

// Do not inline this function -- it must be a separately identifiable
// function for the JTAG debugger scripts to function properly.
void DoBackdoorLoad(void)
#if defined(__GUNC__)
__attribute__((noinline))
#endif
{
    // If there is no breakpoint set on this function just continue.
}

static void NvAppInitDisplay()
{
    debug_print("Initializing Display\n");
    FastbootInitializeDisplay(
            FRAMEBUFFER_DOUBLE | FRAMEBUFFER_32BIT | FRAMEBUFFER_CLEAR,
            &s_Framebuffer, &s_Frontbuffer, &s_FrontbufferPixels, NV_FALSE
            );
}

#ifdef LPM_BATTERY_CHARGING
static void NvAppShutDownDisplay(void)
{
    FastbootShutdownDisplay();
}

void NvAppNotifyOnDisplay(void)
{
    NvAppInitDisplay();
    debug_print(s_DispMessage);
    NvOsSleepMS(s_ChargingConfigData.NotifyTimeOut);
    NvAppShutDownDisplay();
}

NvError NvAppDoCharge(NvU32 OSChrgInPercent, NvU32 ExitChrgInPercent)
{
    NvError e = NvError_Success;
    NvBool Status;
    NvU32 ChrgInPercent = 0;

    // Set charging current.
    NvOsDebugPrintf("Start charging\n");
    Status = NvOdmChargingInterface(NvOdmChargingID_StartCharging, s_pPmu);
    NV_ASSERT(Status);

    // Init for keypress
    e = InitInputKeys();
    if(NvSuccess != e)
        goto fail;

    // Wait while charging.
    NvOsDebugPrintf("Waiting for charge to reach %u\n", ExitChrgInPercent);
#define LOOP_TIMEOUT    5000        // Wait loop timout
    for (;;)
    {
        NvU32 TimeStart = NvOsGetTimeMS();
        NvU32 TimeNow;

        // Get battery charge level
        NvOsDebugPrintf("Charging. (Hit <ROW1> to exit and boot, <ROW2> to show charging status)\n");
        Status = NvOdmChargingInterface(NvOdmChargingID_GetBatteryCharge, s_pPmu, &ChrgInPercent);
        NV_ASSERT(Status);
        NvOsDebugPrintf("Charging (%u%%)\n", ChrgInPercent);

        // Check exit conditions;
        if (ChrgInPercent >= ExitChrgInPercent)
        {
            NvOsDebugPrintf("Charged Enough. Exiting.\n");
            break;
        }
        // Wait for a while.
        do
        {
            NvU32 KeyEvent = GetKeyEvent();

            TimeNow = NvOsGetTimeMS();

            {
                if ( KEY_UP == KeyEvent )
                {
                    NvOsDebugPrintf("KEY_UP\n");
                    NvOsDebugPrintf("Charging (%u%%)\n", ChrgInPercent);
                    /* Show battery charge */
                    NvOsSnprintf(s_DispMessage, sizeof(s_DispMessage), "Battery Charge : (%u%%)\n", ChrgInPercent);
                    NvAppNotifyOnDisplay();
                    break;
                }
                if ( KEY_DOWN == KeyEvent )
                {
                    NvOsDebugPrintf("KEY_DOWN\n");
                    NvOsDebugPrintf("Charge : (%u%%)\n", ChrgInPercent);
                    /* Show battery charge */
                    NvOsSnprintf(s_DispMessage, sizeof(s_DispMessage), "Battery Charge : (%u%%)\n", ChrgInPercent);
                    NvAppNotifyOnDisplay();
                    if (ChrgInPercent > OSChrgInPercent)
                    {
                        NvOsDebugPrintf("Battery is sufficient to boot os\n");
                        goto fail;
                    }
                    break;
                }
            }
        } while (TimeNow - TimeStart < LOOP_TIMEOUT);
    }
fail:
    NvOsDebugPrintf("Turning charger off\n");
    Status = NvOdmChargingInterface(NvOdmChargingID_StopCharging, s_pPmu);
    NV_ASSERT(Status);

    NvOsDebugPrintf("Closing USB\n");
    NvBlUsbClose();

    DeinitInputKeys();

    return e;
}

static void NvAppHalt(void)
{
    if (s_pPmu)
        NvOdmPmuPowerOff(s_pPmu);
}
static NvU8 KhReadI2cTransaction(NvU8 i2c_address,NvU32 I2cPort,NvU8 reg_address)
{
	NvOdmI2cStatus I2cTransStatus;
	NvOdmI2cTransactionInfo TransactionInfo[2];
 	NvOdmImagerI2cConnection pI2c1;
 	NvOdmImagerI2cConnection* pI2c;
 	pI2c = &pI2c1;
  	NvU8 ReadBuffer[2] = {0}; 
   	pI2c->DeviceAddr =i2c_address;
    pI2c->I2cPort =I2cPort;

	pI2c->hOdmI2c = NvOdmI2cOpen(12, pI2c->I2cPort);
	if (!pI2c->hOdmI2c){
		NvOsDebugPrintf("open I2C faile !=======>!\n");
		return NvSuccess;
	}
 
	ReadBuffer[0] = reg_address;  
 
    TransactionInfo[0].Address = pI2c->DeviceAddr;
    TransactionInfo[0].Buf = ReadBuffer;
    TransactionInfo[0].Flags = 1;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = (pI2c->DeviceAddr | 0x1);
    TransactionInfo[1].Buf = ReadBuffer; 
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 2;
 
    // Read data from PMU at the specified offset
    I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo[0], 2,
            100, 1000);
	if(I2cTransStatus != 0){
		NvOsDebugPrintf("reade fail 0x%02x========#######======>!\n",reg_address);
	}

 	NvOdmI2cClose(pI2c->hOdmI2c);
	pI2c->hOdmI2c= NULL;

	return ReadBuffer[0];
}
static NvBool KhWriteI2cTransaction(NvU8 i2c_address,NvU32 I2cPort,NvU8 reg_address,NvU8 reg_value)
{
 
 
	NvOdmI2cStatus I2cTransStatus;
	NvOdmI2cTransactionInfo TransactionInfo;
 	NvOdmImagerI2cConnection pI2c1;
 	NvOdmImagerI2cConnection* pI2c;
 	pI2c = &pI2c1;
  	NvU8 ReadBuffer[2] = {0}; 
   	pI2c->DeviceAddr =i2c_address;
    pI2c->I2cPort =I2cPort;

	pI2c->hOdmI2c = NvOdmI2cOpen(12, pI2c->I2cPort);
	if (!pI2c->hOdmI2c){
		NvOsDebugPrintf("open I2C faile !========#########======>!\n");
		return NvSuccess;
	}
 
    ReadBuffer[0] = reg_address;  
    ReadBuffer[1] = reg_value;  

    TransactionInfo.Address = pI2c->DeviceAddr;
    TransactionInfo.Buf = (NvU8*)ReadBuffer;
    TransactionInfo.Flags = 1;
    TransactionInfo.NumBytes = 2;

    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);

    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == 1)
        I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);

    if (I2cTransStatus)
        NvOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);

  
	NvOdmI2cClose(pI2c->hOdmI2c);
	pI2c->hOdmI2c= NULL;
  	return (NvBool)(I2cTransStatus == 0);
}

static NvBool KhWirteCmdRst(void)
{
    NvU8 serialinterface_data[] ={0xff, 0x8f, 0xff};
    NvU8 suspend_data[]= {0x00, 0xAE};
    NvOdmI2cStatus I2cTransStatus;
    NvOdmI2cTransactionInfo TransactionInfo;
	NvOdmImagerI2cConnection pI2c1;
 	NvOdmImagerI2cConnection* pI2c;
 	pI2c = &pI2c1;
  	
   	pI2c->DeviceAddr =0x02;
       pI2c->I2cPort =0x01;

	pI2c->hOdmI2c = NvOdmI2cOpen(12, pI2c->I2cPort);
	if (!pI2c->hOdmI2c){
		NvOsDebugPrintf("open I2C faile !========#########======>!\n");
		return NvSuccess;
	}
 
    TransactionInfo.Address = pI2c->DeviceAddr;
    TransactionInfo.Buf = (NvU8*)serialinterface_data;
    TransactionInfo.Flags = 1;
    TransactionInfo.NumBytes = 3;

    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);

    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == 1)
        I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);

    if (I2cTransStatus)
        NvOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);

    
     TransactionInfo.Buf = (NvU8*)suspend_data;
     TransactionInfo.NumBytes = 2;
     I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);

    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == 1)
        I2cTransStatus = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, 100,1000);
    NvOsWaitUS(50*1000);
    if (I2cTransStatus)
        NvOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);
  
	NvOdmI2cClose(pI2c->hOdmI2c);
	pI2c->hOdmI2c= NULL;
  	return (NvBool)(I2cTransStatus == 0);

	
}
#ifdef	LEDS_TLC59116_DRIVER

static void Led_For_N020(void)
{
	NvU32 i;
	NvU32 I2cPort = 0x01;
	NvU8 i2c_address = 0xD0;
	NvU8 TLC59116_MODE1	= 0x00;
	NvU8 TLC59116_MODE2	= 0x01;
	NvU8 TLC59116_LEDOUT0	= 0x14;
	NvU8 TLC59116_LEDOUT1 = 0x15;
  NvU8 TLC59116_LEDOUT3 = 0x17;
	NvU8 TLC59116_PWM0	= 0x02;	
  NvU8 TLC59116_PWM4  =	0x06;
  NvU8 TLC59116_PWM12 = 0x0e;
	NvU8 vale = 0;
	NvU8 i2c_address_reset = 0xD6;

    
	NvOsDebugPrintf("---Led_For_N020---\n");
	vale=KhReadI2cTransaction(i2c_address,I2cPort,TLC59116_MODE1);
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_MODE1,0x01);
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_MODE2,0x00);
		
    KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_LEDOUT0,0xff);
    KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_LEDOUT1,0x03);
    KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_LEDOUT3,0x3f);       
		//for(i=0;i<4;i++) {
		//	KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_LEDOUT0+i,0xff);
		//}

		//for(i=0;i<4;i++) {
		//	KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM0+i,0xB2);
		//}
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM0,0xB2);
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM0+1,0xB2);
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM0+2,0xB2);
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM0+3,0x08);	
		KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM4,0xB2);
		for(i=0;i<3;i++) {
			KhWriteI2cTransaction(i2c_address,I2cPort,TLC59116_PWM12+i,0xB2);		
		}

}

#endif

static void NvSetChargerParameter(void)
{	 NvU32 I2cPort = 0x04;
	 NvU8 reg_address=0x05; 
	 NvU8 i2c_address = 0xD6;

	  NvU8 vale = 0,needReboot = 0,status = 0;
	NvU8 BQ24160_REG_STATUS_CONTROL	=					0x00;
	NvU8 BQ24160_REG_SUPPLY_STATUS	=					0x01;
	NvU8 BQ24160_REG_CONTROL=							0x02;
	NvU8 BQ24160_REG_BATTERY_VOLTAGE=					0x03;
	NvU8 BQ24160_REG_VENDER_REVISION=					0x04;
	NvU8 BQ24160_REG_TERMINATION_FAST_CHARGE=			0x05;
	NvU8 BQ24160_REG_VOLTAGE_DPPM=						0x06;
	NvU8 BQ24160_REG_SAFETY_TIMER=						0x07;

	needReboot=KhReadI2cTransaction(i2c_address,I2cPort,BQ24160_REG_BATTERY_VOLTAGE);
	
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_CONTROL,0x80);//reset disable
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_SAFETY_TIMER,0xc8);//satety timer
	#ifdef  BQ24160_CHARGER_Param
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_BATTERY_VOLTAGE,0x8E); // set charge regulation voltage 4.2v  Input Limit for   2.5A
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_TERMINATION_FAST_CHARGE,0xEA);//set changer current 0.1A , 2.7A  	
	#else	
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_BATTERY_VOLTAGE,0x8C); //set charge regulation voltage 4.2v   Input Limit for   1.5A
	#if defined(LOGO_NS_14T004)
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_TERMINATION_FAST_CHARGE,0x9E);//set changer current 0.1A , 1.2A
	#else
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_TERMINATION_FAST_CHARGE,0x6A);//set changer current 0.1A , 1.2A
	#endif
	#endif
	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_VOLTAGE_DPPM,0x1b);//vin dppm votage4.44v   v-usb dppm=4.6

	KhWriteI2cTransaction(i2c_address,I2cPort,BQ24160_REG_CONTROL,0x5c);//change enable ,i usb limit=1.5A
	
	vale=KhReadI2cTransaction(i2c_address,I2cPort,BQ24160_REG_TERMINATION_FAST_CHARGE);
	debug_print("KhReadvale=%x\n",vale);

	if(0x14 == needReboot){
		debug_print("\n=============================restart bootloader!!!!!!!!!!!!!!!!!!!!!!!!!!========================================================>\r\n");
		NvAbootReset(gs_hAboot);
	}
}



static int NVGetChargingStatus(void)
{
	NvU32 I2cPort = 0x04;
	NvU8 reg_address=0x05; 
	NvU8 i2c_address = 0xD6;
	NvU8 vale = 0,status = 0;

	NvU8 BQ24160_REG_STATUS_CONTROL	=					0x00;

	
	status=KhReadI2cTransaction(i2c_address,I2cPort,BQ24160_REG_STATUS_CONTROL);
	
	status = 0x07 & (status >> 4);
	if(status)
		return  USB_CHARGING;
	else
		return USB_NOCHARGING;
	
}

#ifdef MAKE_POWER_KEY_2SE_DECT
static NvError NvAppCheckPowerDownState(void)
{
	 NvError e = NvError_Success;
	 NvU8 vale = 0;
	 NvU32 I2cPort = 0x04;
	 NvU8 reg_address=0x05;
	 NvU8 i2c_address = 0x78;
	// NvU32 timeout = 1000; //ms
	CommandType Command =CommandType_None;

	 #define MAX7663_ONE_KEY_PRESS(x)			(1<<x)
	#define PMU_SLAVE_ADDR 0x78
	#define PMU_RTC_SLAVE_ADDR 0xD0

   	NvU8 val1=0;
	
	e = NvCheckForReboot(gs_hAboot, &Command);
    	if (e == NvSuccess &&( Command == CommandType_RePowerOffReboot || Command ==CommandType_Rebooting))
    	{
    		if(Command ==CommandType_Rebooting)
    			Reboot_cmd = 1;
  		NvOsDebugPrintf("NvAppCheckPowerDownState::the device is Rebooting request###======>!\n");
		return e;
    	}
		
	reg_address = 0x05;
	vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
	debug_print("NvAppCheckPowerDownState::0x%x = 0x%x###======>!\n",reg_address,vale);

	if(vale & MAX7663_ONE_KEY_PRESS(1)){   // on/off irq type 
		power_key_boot = 1;
		reg_address = 0x0B;
		vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
		debug_print("NvAppCheckPowerDownState::0x%x = 0x%x###======>!\n",reg_address,vale);
			//for filte the acok
		if((vale & MAX7663_ONE_KEY_PRESS(7))||(vale & MAX7663_ONE_KEY_PRESS(6))){

			reg_address = 0x15;
			val1 = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
			debug_print("NvAppCheckPowerDownState:: vale(0x%x)=0x%x========#####======>!\n",reg_address,vale);
			if((val1 & MAX7663_ONE_KEY_PRESS(1))||!((val1 & MAX7663_ONE_KEY_PRESS(2))&&(vale& MAX7663_ONE_KEY_PRESS(1)))){

				
				e = NvCheckForReboot(gs_hAboot, &Command);
			    	if (e == NvSuccess && Command == CommandType_Rebooting)
			    	{
			    		Reboot_cmd = 1;
			    		NvOsDebugPrintf("NvAppCheckPowerDownState::the device is Rebooting request include acok event###======>!\n");
					return e;
			    	}
#ifdef MAKE_AC_BOOT_UP
				return e;
#endif
				i2c_address = PMU_RTC_SLAVE_ADDR;
		   	 	reg_address = 0x01;
		   		 vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
				debug_print("NvAppCheckPowerDownState::rtc vale(0x%x)=0x%x========#####======>!\n",reg_address,vale);
				vale |=MAX7663_ONE_KEY_PRESS(1);
				vale |=MAX7663_ONE_KEY_PRESS(2);
		   		  KhWriteI2cTransaction(i2c_address,I2cPort,reg_address,vale);
				debug_print("NvAppCheckPowerDownState::rtc write vale(0x%x)=0x%x ACOK boot so shutdown========#########======>!\n",reg_address,vale);
		   		  NvAppLetDevieShutDown();
			}
		}

		reg_address = 0x15;
		vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
		if(vale & MAX7663_ONE_KEY_PRESS(2)){
				NvOsDebugPrintf("NvAppCheckPowerDownState::oneKey is press!the device will power up vale=0x%x=======>!\n",vale);
				return e;
		}
		
   }

	e = NvCheckForReboot(gs_hAboot, &Command);
    	if (e == NvSuccess && Command == CommandType_Rebooting)
    	{
    		Reboot_cmd = 1;
  		NvOsDebugPrintf("NvAppCheckPowerDownState::the device is Rebooting request###======>!\n");
		return e;
    	}

	debug_print("NvAppCheckPowerDownState::oneKey is unpress!the device will shutdown vale=0x%x====>!\n",vale);
	 vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
	debug_print("NvAppCheckPowerDownState::rtc vale(0x%x)=0x%x========#####======>!\n",reg_address,vale);
	vale |=MAX7663_ONE_KEY_PRESS(1);
	vale |=MAX7663_ONE_KEY_PRESS(2);
	 KhWriteI2cTransaction(i2c_address,I2cPort,reg_address,vale);
	debug_print("NvAppCheckPowerDownState::rtc write vale(0x%x)=0x%x========#########======>!\n",reg_address,vale);
	NvAppLetDevieShutDown();


 	
   	#undef PMU_RTC_SLAVE_ADDR
	#undef PMU_SLAVE_ADDR
	 #undef MAX7663_ONE_KEY_PRESS

	  return e;
}
#endif
#ifdef MAKE_LOW_BATTERY_DECT
#ifdef WIKIPAD_LOW_POWER
#define LOW_POWER_NUM 12
#else
#define LOW_POWER_NUM 12//100//15
#endif
extern void  Keenhi_Charge_Pin( NvBool  state );
extern void Keenhi_backlight_ctr(NvBool state);

NvU32 transBytes2UnsignedInt(NvU8 msb, NvU8 lsb)
{
  NvU32 tmp;
  tmp = ((msb << 8) & 0xFF00);
  return ((NvU16)(tmp + lsb) & 0x0000FFFF);  
}
static NvU8 CheckKey(){

 	NvError e = NvError_Success;
	 NvU8 vale = 0;
	 NvU32 I2cPort = 0x04;
	 NvU8 reg_address=0x05;
	 NvU8 i2c_address = 0x78;
	// NvU32 timeout = 1000; //ms
	CommandType Command =CommandType_None;

	 #define MAX7663_ONE_KEY_PRESS(x)			(1<<x)
	#define PMU_SLAVE_ADDR 0x78
	#define PMU_RTC_SLAVE_ADDR 0xD0
	NvU8 val1=0;
	reg_address = 0x05;
	vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
	debug_print("NvAppCheckPowerDownState::0x%x = 0x%x###======>!\n",reg_address,vale);

	if(vale & MAX7663_ONE_KEY_PRESS(1)){
		reg_address = 0x0B;
		vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
		debug_print("NvAppCheckPowerDownState::0x%x = 0x%x###======>!\n",reg_address,vale);
			//for filte the acok

			
		if((vale & MAX7663_ONE_KEY_PRESS(7))||(vale & MAX7663_ONE_KEY_PRESS(6))){

			reg_address = 0x15;
			val1 = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
			debug_print("NvAppCheckPowerDownState:: vale(0x%x)=0x%x========#####======>!\n",reg_address,vale);
			if((val1 & MAX7663_ONE_KEY_PRESS(1))||!((val1 & MAX7663_ONE_KEY_PRESS(2))&&(vale& MAX7663_ONE_KEY_PRESS(1)))){
				}
			}
		}
		return 0;

}


static NvU8  CheckPowerKey()
{
	 NvError e = NvError_Success;
	 NvU8 vale = 0;
	 NvU32 I2cPort = 0x04;
	 NvU8 reg_address=0x05;
	 NvU8 i2c_address = 0x78;
	// NvU32 timeout = 1000; //ms
	CommandType Command =CommandType_None;

	 #define MAX7663_ONE_KEY_PRESS(x)			(1<<x)
	#define PMU_SLAVE_ADDR 0x78
	#define PMU_RTC_SLAVE_ADDR 0xD0

   	NvU8 val1=0;
	//reg_address = 0x05;
	//vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
	//debug_print("NvAppCheckPowerDownState::0x%x = 0x%x###======>!\n",reg_address,vale);

	reg_address = 0x15;
	vale = KhReadI2cTransaction(i2c_address,I2cPort,reg_address);
	NvOsWaitUS(30*1000);
	//NvOsDebugPrintf("NvAppCheckPowerDownState::oneKey is press!the device will power up vale=0x%x=======>!\n",vale);
	if(vale & MAX7663_ONE_KEY_PRESS(2)){
	//NvOsDebugPrintf("NvAppCheckPowerDownState::oneKey is press!the device will power up vale=0x%x=======>!\n",vale);
			return POWERKEY_PRESS;
	}
	else 
		return POWER_NOPRESS;
}
static NvU8 CheckCurrent()
{

			NvU8 rsoc[3],current_low,current_hi;
			NvU8	rsoc_count=0;
			NvU8 current_count=0;
			NvU32 current[3];  
			NvU8 i,log_i=0,loop_i = 0;
			NvU32 timeout=30;      
			NvU8 vale=0;
			NvU32 I2cPort = 0x01;
			NvU8 i2c_address = 0xAA;
			NvU8 low_battery = 0;

		       Icon *BootMenuImages[1] = {  NULL };   
			Icon *LoopImages[5] = {  NULL };   
			RadioMenu *BootMenu = NULL;
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[0]=transBytes2UnsignedInt(current_hi,current_low);
			
	              NvOsWaitUS(timeout*1000);
				  
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
			NvOsWaitUS(timeout*1000);
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[1]=transBytes2UnsignedInt(current_hi,current_low);
			
			NvOsWaitUS(timeout*1000);
			
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[2]=transBytes2UnsignedInt(current_hi,current_low);   
			
		       current_count=0;
			for(i=0;i<=2;i++){
				 debug_print("B1=%d \n",current[i]);
				if(current[i]>0x8000)  current_count++;
			}

	           
	       	if(current_count>=2) {
				  debug_print("B2=%d \n",current_count); 
				  NvOsWaitUS(7*1000*1000); //7s
				  current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
				  NvOsWaitUS(20*1000);
				  current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15); 
				  current[2]=transBytes2UnsignedInt(current_hi,current_low); 
				  debug_print("B3=%d \n", current[2]); 
				  if(current[2]>0x8000) 
			        	return USB_NOCHARGING;
				  else
				  	return USB_CHARGING;
				
			}
		
		return USB_CHARGING;
}

static void bq27x00_reset_lock(NvU8 i2c_address,NvU32 I2cPort)
{
        NvOsDebugPrintf("bootloader bq27x00_reset_lock\n");
		KhWriteI2cTransaction(i2c_address,I2cPort,1,0x0);
		KhWriteI2cTransaction(i2c_address,I2cPort,0,0x41);//reset 
		NvOsWaitUS(2000000);
		KhWriteI2cTransaction(i2c_address,I2cPort,1,0x0);
		KhWriteI2cTransaction(i2c_address,I2cPort,0,0x20);//lock
		NvOsWaitUS(3000000);
}
static int restart_system()
{
	CommandType Command =CommandType_None;
	 NvError e = NvError_Success;
	e = NvCheckForReboot(gs_hAboot, &Command);
	
    	if (e == NvSuccess && Command == CommandType_RePowerOffReboot)
    	{
    		return CommandType_RePowerOffReboot;
    	}
	else if (e == NvSuccess && Command == CommandType_Rebooting)
	{
		return CommandType_Rebooting;
	}
	else 
		return CommandType_None;
}
static NvError KhAppChargeBatteryIfLow(RadioMenu *BootLogoMenu,NvU32 logoX,NvU32 logoY)
{

	NvU8 rsoc[3],current_low,current_hi,rcoc_res,vol_res,vol_res1;
	NvU8	rsoc_count=0;
	NvU8 current_count=0;
	NvU32 current[3]; 
	NvU32 vol_res_int;
	NvU8 i,log_i=0,loop_i = 0;
	NvU32 timeout=30;      
	NvU8 vale=0;
	NvU32 I2cPort = 0x01;
	NvU8 i2c_address = 0xAA;

	NvU8 low_battery = 0;

	NvU32 sub_logoX=0, sub_logoY=0;
	int Command =CommandType_None;
       Icon *BootMenuImages[1] = {  NULL };   
	Icon *LoopImages[5] = {  NULL };   
	RadioMenu *BootMenu = NULL,*BootBattery_bg = NULL,*BootLoopMenu = NULL;
	NvU8 Average,NumberOptions,LastNumberOptions=30,AnimCount = 0,LcdPowerOff = 1,Powerkeystatus = 0,PowerKeyLongPress = 0,Current_boot= 0;;
	i2c_address = 0x78;
	I2cPort = 0x04;

	vale = KhReadI2cTransaction(i2c_address,I2cPort,0x15);
	if(NvCheckForceRecoveryMode()&&(vale & (1<<1)))return NvSuccess;
       i2c_address = 0xAA;
	I2cPort = 0x01;
	
	rcoc_res=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
	NvOsWaitUS(timeout*1000);
	vol_res=KhReadI2cTransaction(i2c_address,I2cPort,0x08);
	NvOsWaitUS(timeout*1000);
	vol_res1=KhReadI2cTransaction(i2c_address,I2cPort,0x09);
	NvOsWaitUS(timeout*1000);
	vol_res_int=vol_res+vol_res1*256;
	
//	NvOsDebugPrintf("KhAppChargeBatteryIfLow rcoc_res=%d\n",rcoc_res);
//	NvOsDebugPrintf("KhAppChargeBatteryIfLow vol_res_int=%d\n",vol_res_int);

	if((vol_res_int<=2800)||(vol_res_int>4299)) {
		bq27x00_reset_lock(i2c_address,I2cPort); //voltage<=2.8V or voltage >4.2V  soc reset
		}
	if((vol_res_int>4000)&&(rcoc_res<20)){	 
		bq27x00_reset_lock(i2c_address,I2cPort); //voltage>4.0V and rsoc < 20%   soc reset
		}

#ifdef PRECHARGE_LOGO_HP

		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		NvOsWaitUS(timeout*1000);

		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		NvOsWaitUS(timeout*1000);

		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
		
		if(Average > LOW_POWER_NUM){
		    Keenhi_Charge_Pin(0);
		}
		Powerkeystatus = CheckPowerKey();
		if(POWERKEY_PRESS == Powerkeystatus)
			return 0;
		Command = restart_system();
		  NvOsDebugPrintf("Command =%d\n",Command);
		switch(Command){
			case CommandType_Rebooting:
				return 0;
			case CommandType_None:
			case CommandType_RePowerOffReboot:
			default:
				break;
		}
	       Current_boot = NVGetChargingStatus();
		if(Current_boot == USB_NOCHARGING)
		{
		    if( Average < LOW_POWER_NUM)
		    {
		    	 debug_print("B4\n");
    			BootMenuImages[0] = (Icon *)&battery_bg;
    			BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE);
    
    			BootMenuImages[0] = (Icon *)&charging_low_battery;
    			BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,280+19*12,554,NV_FALSE);
    
    			NvOsWaitUS(500000);
    			NvAppLetDevieShutDown(); 
		    }
		    return 0;
		}		
		BootMenuImages[0] = (Icon *)&battery_bg;
		BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE); 

	
		 while(1){
	 	      
		 	Powerkeystatus = CheckPowerKey();
			AnimCount++;
			if(AnimCount >20  && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =  0;
				PowerKeyLongPress = 0;
			//	Keenhi_backlight_ctr(0);
				NvAppLetDevieShutDown();  
			}
			
			switch(Powerkeystatus)
			{
				case SYSTEM_REBOOT:
					return 0;
					break;
				case POWERKEY_PRESS:
					LcdPowerOff =  1;
					AnimCount  =  0;
				//	Keenhi_backlight_ctr(1);
					PowerKeyLongPress++;
					 debug_print("B5\n");
					if((PowerKeyLongPress >1)&&(Average > 12)){
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,280+40,760,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
						return 0;
					}
					break;
				default:
					break;
			}

			

			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
			

			 NvOsWaitUS(timeout*1000);     
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 
			NvOsWaitUS(timeout*1000); 
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		
			rsoc_count=0;
		/*	for(i=0;i<=2;i++){
				if(rsoc[i]>LOW_POWER_NUM)  rsoc_count++;    
			
		     }
			if(rsoc_count==0){    
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,0,NV_TRUE); 
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
				break;  
			}
			*/
			Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
		
			if(Average <= 10)
				NumberOptions = 0;
			else if (Average > 10 && Average <=15)
				NumberOptions = 1;
			else if  (Average > 15 && Average <=20)
				NumberOptions = 2;
			else if (Average > 20 && Average <=25)
				NumberOptions = 3;
			else if (Average > 25 && Average <=30)
				NumberOptions = 4;
			else if (Average > 30 && Average <=34)
				NumberOptions = 5;
			else if (Average > 34 && Average <=38)
				NumberOptions = 6;
			else if (Average > 38 && Average <=42)
				NumberOptions = 7;
			else if (Average > 42 && Average <=46)
				NumberOptions = 8;
			else if (Average > 46 && Average <=50)
				NumberOptions = 9;
			else if (Average > 50 && Average <=55)
				NumberOptions = 10;
			else if (Average > 55 && Average <=65)
				NumberOptions = 11;
			else if (Average > 65 && Average <=70)
				NumberOptions = 12;
			else if (Average > 70 && Average <=75)
				NumberOptions = 13;
			else if (Average > 75 && Average <=80)
				NumberOptions = 14;
			else if (Average > 80 && Average <=85)
				NumberOptions = 15;
			else if (Average > 85 && Average <=90)
				NumberOptions = 16;
			else if (Average > 90 && Average <=95)
				NumberOptions = 17;
			else if (Average > 95 && Average <100)
				NumberOptions = 18;
			else if (Average >= 100 )
				NumberOptions = 19;
		#if 0
			for(i = 0;i<19;i++)
			{	
				if(i == 0)
					BootMenuImages[i] = (Icon *)&charging_last;
				else if(i > 0 &&  i < 19){
					BootMenuImages[i] = (Icon *)&charging_center;
				}else{
					BootMenuImages[i] = (Icon *)&charging_front;
				}
				
			}
			BootMenu = FastbootCreateRadioMenuExt(NumberOptions, 0, 0, NVIDIA_GREEN, BootMenuImages);
			FastbootDrawRadioMenuExt(&s_Frontbuffer, s_FrontbufferPixels,279,497,BootMenu);
		#else
			
			if(NumberOptions == 0)
			{
			    BootMenuImages[0] = (Icon *)&charging_low_battery;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,280+19*12,554,NV_FALSE); 
			}
			else
			{
			    BootMenuImages[0] = (Icon *)&charging_last;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,280+19*12,554,NV_FALSE);

				BootMenuImages[0] = (Icon *)&charging_center;
			    for(i = 0;i < NumberOptions;i++)
			    {
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,280+19*12 -(i+1)*12/*icon wi*/,554,NV_FALSE);
			    }

				if(NumberOptions >= 19)
				{
			        BootMenuImages[0] = (Icon *)&charging_front;
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,280,554,NV_FALSE);
				}
			}
		#endif

			loop_i= loop_i%5;
			switch(loop_i)
			{
				case 0:
					LoopImages[0] = (Icon *)&loop_01;
					break;
				case 1:
					LoopImages[0] = (Icon *)&loop_02;
					break;
				case 2:
					LoopImages[0] = (Icon *)&loop_03;
					break;
				case 3:
					LoopImages[0] = (Icon *)&loop_04;
					break;
				case 4:
					LoopImages[0] = (Icon *)&loop_05;
					break;
			}
			loop_i++;
			     
			BootLoopMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, LoopImages);
			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,280+40,760,NV_FALSE);
			NvOsWaitUS(500000); 		
			Current_boot = NVGetChargingStatus();
			if(Current_boot == USB_NOCHARGING){
			 	 NvAppLetDevieShutDown();  
				 debug_print("B7\n");
			}
		
		 }
	
#elif PRECHARGE_LOGO_ITQ701
		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		NvOsWaitUS(timeout*1000);

		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		NvOsWaitUS(timeout*1000);

		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;

		if(Average > LOW_POWER_NUM)
		    Keenhi_Charge_Pin(0);
		Current_boot = CheckCurrent();
		if(Current_boot == USB_NOCHARGING)
		{
		    if( Average < LOW_POWER_NUM)
		    {
    			BootMenuImages[0] = (Icon *)&battery_bg;
    			BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE);
    
    			BootMenuImages[0] = (Icon *)&charging_low_battery;
    			BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,313,509,NV_FALSE);
    
    			NvOsWaitUS(500000);
    			NvAppLetDevieShutDown(); 
		    }
		    return 0;
		}
		if(Reboot_cmd == 1)
			return 0;
		//Keenhi_Charge_Pin(0);
		//if(BootLogoMenu!=NULL)
		//MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_TRUE); 
		BootMenuImages[0] = (Icon *)&battery_bg;
		BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE); 

		//BootMenuImages[0] = (Icon *)&charging_low_battery;
	    //   BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    	//	MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,470,NV_FALSE); 
			
	//	BootMenuImages[0] = (Icon *)&charging_start_icon;
	//	BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    	//	MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,350,NV_FALSE); 
		 while(1){
		 	
		 	Powerkeystatus = CheckPowerKey();
			AnimCount++;
			if(AnimCount >15 && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =  0;
				PowerKeyLongPress = 0;
				Keenhi_backlight_ctr(1);
			}
			
			switch(Powerkeystatus)
			{
				case SYSTEM_REBOOT:
					return 0;
					break;
				case POWERKEY_PRESS:
					LcdPowerOff =  1;
					AnimCount  =  0;
					Keenhi_backlight_ctr(0);
					PowerKeyLongPress++;
					if((PowerKeyLongPress >2)&&(Average > 12)){
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,279+40,440,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
					 	return 0;
					}
					break;
			}
	
			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
			

			 NvOsWaitUS(timeout*1000);     
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 
			NvOsWaitUS(timeout*1000); 
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		
			rsoc_count=0;
		/*	for(i=0;i<=2;i++){
				if(rsoc[i]>LOW_POWER_NUM)  rsoc_count++;    
			
		     }
			if(rsoc_count==0){    
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,0,NV_TRUE); 
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
				break;  
			}
			*/
			Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
			
			if(Average <= 10)
				NumberOptions = 0;
			else if (Average > 10 && Average <=15)
				NumberOptions = 1;
			else if  (Average > 15 && Average <=20)
				NumberOptions = 2;
			else if (Average > 20 && Average <=25)
				NumberOptions = 3;
			else if (Average > 25 && Average <=30)
				NumberOptions = 4;
			else if (Average > 30 && Average <=34)
				NumberOptions = 5;
			else if (Average > 34 && Average <=38)
				NumberOptions = 6;
			else if (Average > 38 && Average <=42)
				NumberOptions = 7;
			else if (Average > 42 && Average <=46)
				NumberOptions = 8;
			else if (Average > 46 && Average <=50)
				NumberOptions = 9;
			else if (Average > 50 && Average <=55)
				NumberOptions = 10;
			else if (Average > 55 && Average <=65)
				NumberOptions = 11;
			else if (Average > 65 && Average <=70)
				NumberOptions = 12;
			else if (Average > 70 && Average <=75)
				NumberOptions = 13;
			else if (Average > 75 && Average <=80)
				NumberOptions = 14;
			else if (Average > 80 && Average <=85)
				NumberOptions = 15;
			else if (Average > 85 && Average <=90)
				NumberOptions = 16;
			else if (Average > 90 && Average <=95)
				NumberOptions = 17;
			else if (Average > 95 && Average <100)
				NumberOptions = 18;
			else if (Average >= 100 )
				NumberOptions = 19;
		#if 0
			for(i = 0;i<19;i++)
			{	
				if(i == 0)
					BootMenuImages[i] = (Icon *)&charging_last;
				else if(i > 0 &&  i < 19){
					BootMenuImages[i] = (Icon *)&charging_center;
				}else{
					BootMenuImages[i] = (Icon *)&charging_front;
				}
				
			}
			BootMenu = FastbootCreateRadioMenuExt(NumberOptions, 0, 0, NVIDIA_GREEN, BootMenuImages);
			FastbootDrawRadioMenuExt(&s_Frontbuffer, s_FrontbufferPixels,279,497,BootMenu);
		#else
			if(NumberOptions == 0)
			{
			    BootMenuImages[0] = (Icon *)&charging_low_battery;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,313,509,NV_FALSE); 
			}
			else
			{
			    BootMenuImages[0] = (Icon *)&charging_last;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,313,518,NV_FALSE);

				BootMenuImages[0] = (Icon *)&charging_center;
			    for(i = 0;i < NumberOptions;i++)
			    {
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,313,518+(i+1)*12,NV_FALSE);
			    }

				if(NumberOptions >= 19)
				{
			        BootMenuImages[0] = (Icon *)&charging_front;
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,313,518+NumberOptions*12,NV_FALSE);
				}
			}


		#endif

			loop_i= loop_i%5;
			switch(loop_i)
			{
				case 0:
					LoopImages[0] = (Icon *)&loop_01;
					break;
				case 1:
					LoopImages[0] = (Icon *)&loop_02;
					break;
				case 2:
					LoopImages[0] = (Icon *)&loop_03;
					break;
				case 3:
					LoopImages[0] = (Icon *)&loop_04;
					break;
				case 4:
					LoopImages[0] = (Icon *)&loop_05;
					break;
			}
			loop_i++;
			     
			BootLoopMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, LoopImages);
			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,530,555,NV_FALSE);
			NvOsWaitUS(500000); 		
			
			Current_boot = CheckCurrent();
			if(Current_boot == USB_NOCHARGING){
			 	 NvAppLetDevieShutDown();  
				}
		
		 }
#elif PRECHARGE_LOGO_ITQ700
		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		NvOsWaitUS(timeout*1000);

		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		NvOsWaitUS(timeout*1000);

		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;

		if(Average > LOW_POWER_NUM)
		    Keenhi_Charge_Pin(0);
		Current_boot = CheckCurrent();
		if(Current_boot == USB_NOCHARGING)
		{
		    if( Average < LOW_POWER_NUM)
		    {
    			BootMenuImages[0] = (Icon *)&battery_bg;
    			BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE);
    
    			BootMenuImages[0] = (Icon *)&charging_low_battery;
    			BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,470,NV_FALSE);
    
    			NvOsWaitUS(500000);
    			NvAppLetDevieShutDown(); 
		    }
		    return 0;
		}
		if(Reboot_cmd == 1)
			return 0;
		//Keenhi_Charge_Pin(0);
		//if(BootLogoMenu!=NULL)
		//MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_TRUE); 
		BootMenuImages[0] = (Icon *)&battery_bg;
		BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE); 

		//BootMenuImages[0] = (Icon *)&charging_low_battery;
	    //   BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    	//	MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,470,NV_FALSE); 
			
	//	BootMenuImages[0] = (Icon *)&charging_start_icon;
	//	BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    	//	MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,350,NV_FALSE); 
		 while(1){
		 	
		 	Powerkeystatus = CheckPowerKey();
			AnimCount++;
			if(AnimCount >15 && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =  0;
				PowerKeyLongPress = 0;
				Keenhi_backlight_ctr(1);
			}
			
			switch(Powerkeystatus)
			{
				case SYSTEM_REBOOT:
					return 0;
					break;
				case POWERKEY_PRESS:
					LcdPowerOff =  1;
					AnimCount  =  0;
					Keenhi_backlight_ctr(0);
					PowerKeyLongPress++;
					if((PowerKeyLongPress >2)&&(Average > 12)){
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,279+40,440,NV_TRUE); 
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
					 	return 0;
					}
					break;
			}
	
			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
			

			 NvOsWaitUS(timeout*1000);     
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 
			NvOsWaitUS(timeout*1000); 
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		
			rsoc_count=0;
		/*	for(i=0;i<=2;i++){
				if(rsoc[i]>LOW_POWER_NUM)  rsoc_count++;    
			
		     }
			if(rsoc_count==0){    
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,0,NV_TRUE); 
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
				break;  
			}
			*/
			Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
			
			if(Average <= 10)
				NumberOptions = 0;
			else if (Average > 10 && Average <=15)
				NumberOptions = 1;
			else if  (Average > 15 && Average <=20)
				NumberOptions = 2;
			else if (Average > 20 && Average <=25)
				NumberOptions = 3;
			else if (Average > 25 && Average <=30)
				NumberOptions = 4;
			else if (Average > 30 && Average <=34)
				NumberOptions = 5;
			else if (Average > 34 && Average <=38)
				NumberOptions = 6;
			else if (Average > 38 && Average <=42)
				NumberOptions = 7;
			else if (Average > 42 && Average <=46)
				NumberOptions = 8;
			else if (Average > 46 && Average <=50)
				NumberOptions = 9;
			else if (Average > 50 && Average <=55)
				NumberOptions = 10;
			else if (Average > 55 && Average <=65)
				NumberOptions = 11;
			else if (Average > 65 && Average <=70)
				NumberOptions = 12;
			else if (Average > 70 && Average <=75)
				NumberOptions = 13;
			else if (Average > 75 && Average <=80)
				NumberOptions = 14;
			else if (Average > 80 && Average <=85)
				NumberOptions = 15;
			else if (Average > 85 && Average <=90)
				NumberOptions = 16;
			else if (Average > 90 && Average <=95)
				NumberOptions = 17;
			else if (Average > 95 && Average <100)
				NumberOptions = 18;
			else if (Average >= 100 )
				NumberOptions = 19;
		#if 0
			for(i = 0;i<19;i++)
			{	
				if(i == 0)
					BootMenuImages[i] = (Icon *)&charging_last;
				else if(i > 0 &&  i < 19){
					BootMenuImages[i] = (Icon *)&charging_center;
				}else{
					BootMenuImages[i] = (Icon *)&charging_front;
				}
				
			}
			BootMenu = FastbootCreateRadioMenuExt(NumberOptions, 0, 0, NVIDIA_GREEN, BootMenuImages);
			FastbootDrawRadioMenuExt(&s_Frontbuffer, s_FrontbufferPixels,279,497,BootMenu);
		#else
			if(NumberOptions == 0)
			{
			    BootMenuImages[0] = (Icon *)&charging_low_battery;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,470,NV_FALSE); 
			}
			else
			{
			    BootMenuImages[0] = (Icon *)&charging_last;
			    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,497,NV_FALSE);

				BootMenuImages[0] = (Icon *)&charging_center;
			    for(i = 0;i < NumberOptions;i++)
			    {
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+(i+1)*12/*icon wi*/,497,NV_FALSE);
			    }

				if(NumberOptions >= 19)
				{
			        BootMenuImages[0] = (Icon *)&charging_front;
			        BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			        MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+NumberOptions*12,497,NV_FALSE);
				}
			}
		#endif

			loop_i= loop_i%5;
			switch(loop_i)
			{
				case 0:
					LoopImages[0] = (Icon *)&loop_01;
					break;
				case 1:
					LoopImages[0] = (Icon *)&loop_02;
					break;
				case 2:
					LoopImages[0] = (Icon *)&loop_03;
					break;
				case 3:
					LoopImages[0] = (Icon *)&loop_04;
					break;
				case 4:
					LoopImages[0] = (Icon *)&loop_05;
					break;
			}
			loop_i++;
			     
			BootLoopMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, LoopImages);
			MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,279+40,440,NV_FALSE);
			NvOsWaitUS(500000); 		
			
			Current_boot = CheckCurrent();
			if(Current_boot == USB_NOCHARGING){
			 	 NvAppLetDevieShutDown();  
				}
		
		 }
#elif PRECHARGE_LOGO_CYBTT10_BK
		 rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 NvOsWaitUS(timeout*1000);
 
		 rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		 NvOsWaitUS(timeout*1000);
 
		 rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		 Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
 
		 if(Average > LOW_POWER_NUM)
			 Keenhi_Charge_Pin(0);
		 Current_boot = CheckCurrent();
		 if(Current_boot == USB_NOCHARGING)
		 {
			 if( Average < LOW_POWER_NUM)
			 {
				 BootMenuImages[0] = (Icon *)&battery_bg;
				 BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE);
	 
				 BootMenuImages[0] = (Icon *)&charging_low_battery;
				 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+228-4-3,470+57+27,NV_FALSE);
	 
				 NvOsWaitUS(500000);
				 NvAppLetDevieShutDown(); 
			 }
			 return 0;
		 }
		 if(Reboot_cmd == 1)
			 return 0;
		 //Keenhi_Charge_Pin(0);
		 //if(BootLogoMenu!=NULL)
		 //MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_TRUE); 
		 BootMenuImages[0] = (Icon *)&battery_bg;
		 BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
			 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_FALSE); 
 
		 //BootMenuImages[0] = (Icon *)&charging_low_battery;
		 //   BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
		 //  MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279,470,NV_FALSE); 
			 
	 //  BootMenuImages[0] = (Icon *)&charging_start_icon;
	 //  BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
		 //  MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,350,NV_FALSE); 
		  while(1){
			 
			 Powerkeystatus = CheckPowerKey();
			 AnimCount++;
			 if(AnimCount >15 && LcdPowerOff){
				 AnimCount = 0;
				 LcdPowerOff =	0;
				 PowerKeyLongPress = 0;
				 Keenhi_backlight_ctr(1);
			 }
			 
			 switch(Powerkeystatus)
			 {
				 case SYSTEM_REBOOT:
					 return 0;
					 break;
				 case POWERKEY_PRESS:
					 LcdPowerOff =	1;
					 AnimCount	=  0;
					 Keenhi_backlight_ctr(0);
					 PowerKeyLongPress++;
					 if((PowerKeyLongPress >2)&&(Average > 12)){
						 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,0,0,NV_TRUE); 
						 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,279+40,440+335,NV_TRUE); 
						 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
						 return 0;
					 }
					 break;
			 }
	 
			 rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
			 
 
			  NvOsWaitUS(timeout*1000); 	
			 rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		  
			 NvOsWaitUS(timeout*1000); 
			 rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		 
			 rsoc_count=0;
		 /*  for(i=0;i<=2;i++){
				 if(rsoc[i]>LOW_POWER_NUM)	rsoc_count++;	 
			 
			  }
			 if(rsoc_count==0){    
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,0,0,NV_TRUE); 
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE); 
				 break;  
			 }
			 */
			 Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;
			 
			 if(Average <= 10)
				 NumberOptions = 0;
			 else if (Average > 10 && Average <=15)
				 NumberOptions = 1;
			 else if  (Average > 15 && Average <=20)
				 NumberOptions = 2;
			 else if (Average > 20 && Average <=25)
				 NumberOptions = 3;
			 else if (Average > 25 && Average <=30)
				 NumberOptions = 4;
			 else if (Average > 30 && Average <=34)
				 NumberOptions = 5;
			 else if (Average > 34 && Average <=38)
				 NumberOptions = 6;
			 else if (Average > 38 && Average <=42)
				 NumberOptions = 7;
			 else if (Average > 42 && Average <=46)
				 NumberOptions = 8;
			 else if (Average > 46 && Average <=50)
				 NumberOptions = 9;
			 else if (Average > 50 && Average <=55)
				 NumberOptions = 10;
			 else if (Average > 55 && Average <=65)
				 NumberOptions = 11;
			 else if (Average > 65 && Average <=70)
				 NumberOptions = 12;
			 else if (Average > 70 && Average <=75)
				 NumberOptions = 13;
			 else if (Average > 75 && Average <=80)
				 NumberOptions = 14;
			 else if (Average > 80 && Average <=85)
				 NumberOptions = 15;
			 else if (Average > 85 && Average <=90)
				 NumberOptions = 16;
			 else if (Average > 90 && Average <=95)
				 NumberOptions = 17;
			 else if (Average > 95 && Average <100)
				 NumberOptions = 18;
			 else if (Average >= 100 )
				 NumberOptions = 19;
#if 0
			 for(i = 0;i<19;i++)
			 {	 
				 if(i == 0)
					 BootMenuImages[i] = (Icon *)&charging_last;
				 else if(i > 0 &&  i < 19){
					 BootMenuImages[i] = (Icon *)&charging_center;
				 }else{
					 BootMenuImages[i] = (Icon *)&charging_front;
				 }
				 
			 }
			 BootMenu = FastbootCreateRadioMenuExt(NumberOptions, 0, 0, NVIDIA_GREEN, BootMenuImages);
			 FastbootDrawRadioMenuExt(&s_Frontbuffer, s_FrontbufferPixels,279,497,BootMenu);
#else
			 if(NumberOptions == 0)
			 {
				 BootMenuImages[0] = (Icon *)&charging_low_battery;
				 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+228-4-3,470+57+27,NV_FALSE); 
			 }
			 else
			 {
				 BootMenuImages[0] = (Icon *)&charging_last;
				 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+228,497+57,NV_FALSE);
 
				 BootMenuImages[0] = (Icon *)&charging_center;
				 for(i = 0;i < NumberOptions;i++)
				 {
					 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
					 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+228-(i+1)*12/*icon wi*/,497+57,NV_FALSE);
				 }
 
				 if(NumberOptions >= 19)
				 {
					 BootMenuImages[0] = (Icon *)&charging_front;
					 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
					 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,279+228-NumberOptions*12,497+57,NV_FALSE);
				 }
			 }
#endif
 
			 loop_i= loop_i%5;
			 switch(loop_i)
			 {
				 case 0:
					 LoopImages[0] = (Icon *)&loop_01;
					 break;
				 case 1:
					 LoopImages[0] = (Icon *)&loop_02;
					 break;
				 case 2:
					 LoopImages[0] = (Icon *)&loop_03;
					 break;
				 case 3:
					 LoopImages[0] = (Icon *)&loop_04;
					 break;
				 case 4:
					 LoopImages[0] = (Icon *)&loop_05;
					 break;
			 }
			 loop_i++;
				  
					 BootLoopMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, LoopImages);
					 MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,279+40,440+335,NV_FALSE);
					 NvOsWaitUS(500000);		 
					 
					 Current_boot = CheckCurrent();
					 if(Current_boot == USB_NOCHARGING){
						  NvAppLetDevieShutDown();	
						 }
				 
				  }
#elif PRECHARGE_LOGO_ITQ1000
		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		NvOsWaitUS(timeout*1000);

		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		NvOsWaitUS(timeout*1000);

		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);
		Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;

		if(Average > LOW_POWER_NUM)
			Keenhi_Charge_Pin(0);
		Current_boot = CheckCurrent();
		if(Current_boot == USB_NOCHARGING)
		{
			if( Average < LOW_POWER_NUM)
			{
				BootMenuImages[0] = (Icon *)&battery_bg;
				BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,365,228,NV_FALSE);

				BootMenuImages[0] = (Icon *)&charging_low_battery;
				BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,783,311,NV_FALSE);

				NvOsWaitUS(500000);
				NvAppLetDevieShutDown();
			}
			return 0;
		}
		if(Reboot_cmd == 1)
			return 0;
		BootMenuImages[0] = (Icon *)&battery_bg;
		BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,365,228,NV_FALSE);

		while(1){

			Powerkeystatus = CheckPowerKey();
			AnimCount++;
			if(AnimCount >15 && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =	0;
				PowerKeyLongPress = 0;
				Keenhi_backlight_ctr(0);
			}

			switch(Powerkeystatus)
			{
				case SYSTEM_REBOOT:
					return 0;
				break;
				case POWERKEY_PRESS:
					LcdPowerOff =	1;
					AnimCount	=  0;
					Keenhi_backlight_ctr(1);
					PowerKeyLongPress++;
					if((PowerKeyLongPress >2)&&(Average > 12)){
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,365,228,NV_TRUE);
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,603,512,NV_TRUE);
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE);
						return 0;
					}
				break;
			}

			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);

			NvOsWaitUS(timeout*1000);
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);

			NvOsWaitUS(timeout*1000);
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);

			rsoc_count=0;

			Average = (rsoc[0] +  rsoc[1] + rsoc[2])/3;

			if(Average <= 10)
				NumberOptions = 0;
			else if (Average > 10 && Average <=15)
				NumberOptions = 1;
			else if  (Average > 15 && Average <=20)
				NumberOptions = 2;
			else if (Average > 20 && Average <=25)
				NumberOptions = 3;
			else if (Average > 25 && Average <=30)
				NumberOptions = 4;
			else if (Average > 30 && Average <=34)
				NumberOptions = 5;
			else if (Average > 34 && Average <=38)
				NumberOptions = 6;
			else if (Average > 38 && Average <=42)
				NumberOptions = 7;
			else if (Average > 42 && Average <=46)
				NumberOptions = 8;
			else if (Average > 46 && Average <=50)
				NumberOptions = 9;
			else if (Average > 50 && Average <=55)
				NumberOptions = 10;
			else if (Average > 55 && Average <=60)
				NumberOptions = 11;
			else if (Average > 60 && Average <=65)
				NumberOptions = 12;
			else if (Average > 65 && Average <=70)
				NumberOptions = 13;
			else if (Average > 70 && Average <=75)
				NumberOptions = 14;
			else if (Average > 75 && Average <=80)
				NumberOptions = 15;
			else if (Average > 80 && Average <=85)
				NumberOptions = 16;
			else if (Average > 85 && Average <=90)
				NumberOptions = 17;
			else if (Average > 90 && Average <=95)
				NumberOptions = 18;
			else if (Average > 95 && Average <100)
				NumberOptions = 19;
			else if (Average >= 100 )
				NumberOptions = 20;

			if (LastNumberOptions != NumberOptions)
			{
				if(NumberOptions == 0)
				{
					BootMenuImages[0] = (Icon *)&charging_low_battery;
					BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
					MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,783,311,NV_FALSE);

					BootMenuImages[0] = (Icon *)&charging_start_icon;
					BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
					MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,643,487,NV_FALSE);
				}
				else
				{
					if( LastNumberOptions == 0)
					{
						BootMenuImages[0] = (Icon *)&battery_bg;
						BootBattery_bg = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
						MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootBattery_bg,365,228,NV_FALSE);
					}
					BootMenuImages[0] = (Icon *)&charging_last;
					BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
					MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,792,311,NV_FALSE);
					if(NumberOptions > 1){
						BootMenuImages[0] = (Icon *)&charging_center;
						for(i = 1;i < NumberOptions;i++)
						{
							BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
							MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,792-i*12/*icon wi*/,311,NV_FALSE);
						}

						if(NumberOptions > 19)
						{
							BootMenuImages[0] = (Icon *)&charging_front;
							BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
							MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,792-19*12,311,NV_FALSE);
						}
					}
				}

				LastNumberOptions = NumberOptions;
			}

			if (NumberOptions > 0)
			{
				loop_i= loop_i%5;
				switch(loop_i)
				{
					case 0:
					LoopImages[0] = (Icon *)&loop_01;
					break;
					case 1:
					LoopImages[0] = (Icon *)&loop_02;
					break;
					case 2:
					LoopImages[0] = (Icon *)&loop_03;
					break;
					case 3:
					LoopImages[0] = (Icon *)&loop_04;
					break;
					case 4:
					LoopImages[0] = (Icon *)&loop_05;
					break;
				}
				loop_i++;

				BootLoopMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, LoopImages);
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLoopMenu,603,512,NV_FALSE);
				NvOsWaitUS(500000);
			}

			Current_boot = CheckCurrent();
			if(Current_boot == USB_NOCHARGING){
				NvAppLetDevieShutDown();
			}

		}
#elif PRECHARGE_LOGO_NS_14T004
		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
		NvOsWaitUS(timeout*1000);     
		
		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		NvOsWaitUS(timeout*1000); 
		
		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
		
		for(i=0;i<=2;i++){
			if(rsoc[i]<LOW_POWER_NUM)  rsoc_count++; 
		}

	Current_boot = CheckCurrent();

	if(rsoc_count>=2){  
		Keenhi_Charge_Pin(0);
		if(BootLogoMenu!=NULL){
		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_TRUE);
		}

		while(1){

#ifdef BBY_BACKLIGHT_CTRL_FLAG

			AnimCount++;
			if(AnimCount >15 && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =  0;
				NvAppLetDevieShutDown();  
			}
#endif
			
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[0]=transBytes2UnsignedInt(current_hi,current_low);
			
	              NvOsWaitUS(timeout*1000);
				  
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
			NvOsWaitUS(timeout*1000);
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[1]=transBytes2UnsignedInt(current_hi,current_low);
			
			NvOsWaitUS(timeout*1000);
			
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[2]=transBytes2UnsignedInt(current_hi,current_low);
			//NvOsDebugPrintf("\t current=(0x%08x)\n", current[1]);
		       current_count=0;
			for(i=0;i<=2;i++){
				if(current[i]>0x8000)  current_count++;    
			}

	               if(log_i>=5)   log_i=0;   
	               if(log_i==0)   {
		       	if((current_count>=2)) {
					NvOsDebugPrintf("Battery Charge :< 15% and discharging\n"); 	
			               BootMenuImages[0] = (Icon *)&s_lock_charge_lv1_f1;
			    		 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
		                      MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_FALSE);
					  NvOsWaitUS(7*1000*1000); //7s
					  current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
					  NvOsWaitUS(20*1000);
					  current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15); 
					  current[2]=transBytes2UnsignedInt(current_hi,current_low); 
					  if(current[2]>0x8000) {
				        	NvAppLetDevieShutDown();  
					  }
					  else log_i=1; 
					  	
				}//wantianpei shutdown!   
		              else  log_i=1; 
			}
			Current_boot = CheckCurrent();
			if(Current_boot == USB_NOCHARGING){
				NvAppLetDevieShutDown();
			}		   
			if(log_i==1)  	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv2_f1;
			else if(log_i==2)  	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv4_f1; 
			else if(log_i==3)   	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv6_f1;
			else if(log_i==4)
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv8_f1;

			 log_i++;
	    		 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
	    		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_FALSE); 
			NvOsWaitUS(500000); 	   
			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  

			 NvOsWaitUS(timeout*1000);     
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 
			NvOsWaitUS(timeout*1000); 
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		
		       rsoc_count=0;
			for(i=0;i<=2;i++){
				if(rsoc[i]<LOW_POWER_NUM)  rsoc_count++;    
			}
			if(rsoc_count==0){    
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_TRUE); 
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE);
				break;  
			}
		}
	}

#else
		rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
		NvOsWaitUS(timeout*1000);     
		
		rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		NvOsWaitUS(timeout*1000); 
		
		rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  
		
		for(i=0;i<=2;i++){
			if(rsoc[i]<LOW_POWER_NUM)  rsoc_count++; 
		}


	if(rsoc_count>=2){  
		Keenhi_Charge_Pin(0);
		if(BootLogoMenu!=NULL){
#if defined(LOGO_NABIXD)
		sub_logoX=logoX;
		sub_logoY=logoY;
		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX-125,logoY-50,NV_TRUE); 
#else
		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_TRUE);
#endif
		}

		while(1){

#ifdef BACKLIGHT_CTRL_FLAG
		Powerkeystatus = CheckPowerKey();
			AnimCount++;
			if(AnimCount >15 && LcdPowerOff){
				AnimCount = 0;
				LcdPowerOff =  0;
				Keenhi_backlight_ctr(0);
			}

			switch(Powerkeystatus)
			{
				case POWERKEY_PRESS:
					LcdPowerOff =  1;
					AnimCount  =  0;
					Keenhi_backlight_ctr(1);
					break;
			}
#endif

#ifdef WIKIPAD_BACKLIGHT_CTRL_FLAG
					Powerkeystatus = CheckPowerKey();
						AnimCount++;
						if(AnimCount >15 && LcdPowerOff){
							AnimCount = 0;
							LcdPowerOff =  0;
							Keenhi_backlight_ctr(1);
						}
			
						switch(Powerkeystatus)
						{
							case POWERKEY_PRESS:
								LcdPowerOff =  1;
								AnimCount  =  0;
								Keenhi_backlight_ctr(0);
								break;
						}
#endif
			
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[0]=transBytes2UnsignedInt(current_hi,current_low);
			
	              NvOsWaitUS(timeout*1000);
				  
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
			NvOsWaitUS(timeout*1000);
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[1]=transBytes2UnsignedInt(current_hi,current_low);
			
			NvOsWaitUS(timeout*1000);
			
			current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);   
			NvOsWaitUS(timeout*1000);
			
			current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15);  
			current[2]=transBytes2UnsignedInt(current_hi,current_low);   
			
		       current_count=0;
			for(i=0;i<=2;i++){
				if(current[i]>0x8000)  current_count++;    
			}

	               if(log_i>=5)   log_i=0;   
	               if(log_i==0)   {
		       	if(current_count>=2) {
					NvOsDebugPrintf("Battery Charge :< 15% and discharging\n"); 	
			               BootMenuImages[0] = (Icon *)&s_lock_charge_lv1_f1;
			    		 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
		                      MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_FALSE);
					  NvOsWaitUS(7*1000*1000); //7s
					  current_low=KhReadI2cTransaction(i2c_address,I2cPort,0x14);  
					  NvOsWaitUS(20*1000);
					  current_hi=KhReadI2cTransaction(i2c_address,I2cPort,0x15); 
					  current[2]=transBytes2UnsignedInt(current_hi,current_low); 
					  if(current[2]>0x8000) {
				        	NvAppLetDevieShutDown();  
					  }
					  else log_i=1; 
					  	
				}//wantianpei shutdown!   
		              else  log_i=1; 
			}
			
			if(log_i==1)  	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv2_f1;
			else if(log_i==2)  	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv4_f1; 
			else if(log_i==3)   	
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv6_f1;
			else if(log_i==4)
				BootMenuImages[0] = (Icon *)&s_lock_charge_lv8_f1;

			 log_i++;
	    		 BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
	    		MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_FALSE); 
			NvOsWaitUS(500000); 	   
			
			rsoc[0]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);  

			 NvOsWaitUS(timeout*1000);     
			rsoc[1]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c); 
		 
			NvOsWaitUS(timeout*1000); 
			rsoc[2]=KhReadI2cTransaction(i2c_address,I2cPort,0x2c);   
		
		       rsoc_count=0;
			for(i=0;i<=2;i++){
				if(rsoc[i]<LOW_POWER_NUM)  rsoc_count++;    
			}
			if(rsoc_count==0){    
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,sub_logoX,sub_logoY,NV_TRUE); 
#if defined(LOGO_NABIXD)
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX-125,logoY-50,NV_FALSE);
#else
				MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootLogoMenu,logoX,logoY,NV_FALSE);
#endif
				break;  
			}
		}
	}
#endif	
	
    return NvSuccess;
}
#endif
static NvError NvAppChargeBatteryIfLow(void)
{
    NvError e = NvError_Success;
    NvBool Status = NV_FALSE;
    NvBool ChargerConnected  = NV_FALSE;
    NvBool BatteryConnected  = NV_FALSE;
    // The following only matter when BatteryConnected and ChargerConnected are TRUE
    NvU32 BatteryChrgInPercent = 0;
    NvU32 BootloaderChrgInPercent   = 0;
    NvU32 OSChrgInPercent           = 0;
    NvU32 ExitChrgInPercent         = 0;
    NvOdmPowerSupplyInfo PowerSupplyInfo;

    /* Battery charging will only be supported if device powered by battery */
    if (!NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_PowerSupply,
                &PowerSupplyInfo, sizeof(PowerSupplyInfo)))
        return NvSuccess;

    if (PowerSupplyInfo.SupplyType == NvOdmBoardPowerSupply_Adapter)
            return NvSuccess;

    // Open PMU
    Status = NvOdmPmuDeviceOpen(&s_pPmu);
    NV_ASSERT(s_pPmu);
    NV_ASSERT(Status);

    // Initialize the Odm charging interface
    NvOsDebugPrintf("Initializing\n");
    Status = NvOdmChargingInterface(NvOdmChargingID_InitInterface, s_pPmu);
    NV_ASSERT(Status);

    // Get/Show initial battery status.
    NvOsDebugPrintf("Looking for battery\n");
    Status = NvOdmChargingInterface(NvOdmChargingID_IsBatteryConnected, &BatteryConnected);
    NV_ASSERT(Status);
    if (!BatteryConnected)
    {
        // Battery not connected (most likely a development system)
        NvOsDebugPrintf("No battery!!!\n");

    } else {
        NvOsDebugPrintf("Battery connected\n");

        NvOsDebugPrintf("Getting Charging Data.\n");
        Status = NvOdmChargingInterface(NvOdmChargingID_GetChargingConfigData, &s_ChargingConfigData);
        if (!Status)
        {
            NvOsDebugPrintf("Charging Not Supported !!!.\n");
            goto fail;
        }

            // Get/show initial battery charge level
        Status = NvOdmChargingInterface(NvOdmChargingID_GetBatteryCharge, s_pPmu, &BatteryChrgInPercent);
        NV_ASSERT(Status);
        BootloaderChrgInPercent = s_ChargingConfigData.BootloaderChargeLevel;
        OSChrgInPercent = s_ChargingConfigData.OsChargeLevel;
        ExitChrgInPercent = s_ChargingConfigData.ExitChargeLevel;

            // See if battery charge is enough
        if (BatteryChrgInPercent < OSChrgInPercent)
        {
            NvOsDebugPrintf("BatteryChrgInPercent = %u%%\n", BatteryChrgInPercent);
            NvOsDebugPrintf("BootloaderChrgInPercent = %u%%\n", BootloaderChrgInPercent);
            NvOsDebugPrintf("OSChrgInPercent = %u%%\n", OSChrgInPercent);
            NvOsDebugPrintf("ExitChrgInPercent = %u%%\n", ExitChrgInPercent);

            /* Show battery charge */
            NvOsSnprintf(s_DispMessage, sizeof(s_DispMessage), "Battery Charge : %u%%\n", BatteryChrgInPercent);
            NvAppNotifyOnDisplay();

            // Detect USB charger.
            NvOsDebugPrintf("Looking for charger\n");
            Status = NvOdmChargingInterface(NvOdmChargingID_IsChargerConnected, &ChargerConnected);
            NV_ASSERT(Status);

            if (ChargerConnected)
            {
                //Go to low power mode
                NV_CHECK_ERROR_CLEANUP(NvAbootLowPowerMode(NvAbootLpmId_GoToLowPowerMode,
                            s_ChargingConfigData.LpmCpuKHz));

                // Wait for battery to charge enough to go on (charge current is raised and restored here)
                NvAppDoCharge(OSChrgInPercent, ExitChrgInPercent);

                NV_CHECK_ERROR_CLEANUP(NvAbootLowPowerMode(NvAbootLpmId_ComeOutOfLowPowerMode));
            }
            else
            {
                NvOsDebugPrintf("No Charger!!!\n");
                NvOsDebugPrintf("ERROR: Unable to launch ... need to shut down!!!\n");
                NvOsSnprintf(s_DispMessage, sizeof(s_DispMessage), "ERROR: Unable to launch .... Shutting Down !!!\n");
                NvAppNotifyOnDisplay();
                NvAppHalt();
            }
        }
        else
        {
            NvOsDebugPrintf("Charged enough (%u).\n", BatteryChrgInPercent);
        }
    }

fail:
    //NvOsDebugPrintf("Closing the pmu handle\n");
    //NvOdmPmuDeviceClose(s_pPmu);
    return e;
}
#endif

static NvError StartFastbootProtocol(NvU32 RamBase)
{
    Nv3pTransportHandle hTrans = NULL;
    NvError e = NvSuccess;
    NvU8 *KernelImage = NULL;
    NvU32 LoadSize = 0;
    NvU32 ExitOption = FASTBOOT;
    NvOdmPmuDeviceHandle hPmu;

    // Configure USB driver to return the fastboot config descriptor
    FastbootSetUsbMode(NV_FALSE);
    Nv3pTransportSetChargingMode(NV_FALSE);
    e = Nv3pTransportOpenTimeOut(&hTrans, Nv3pTransportMode_default, 0,1000);

    debug_print("Starting Fastboot USB download protocol\r\n");
    e = InitInputKeys();

    while (ExitOption == FASTBOOT)
    {
        e = FastbootProtocol(hTrans, &KernelImage, &LoadSize, &ExitOption);
    }

    DeinitInputKeys();
    if (hTrans)
    {
        Nv3pTransportClose(hTrans);
        hTrans = NULL;
    }

    switch (ExitOption)
    {
        case BOOTLOADER:
            ClearDisplay();
            NV_CHECK_ERROR_CLEANUP(
                MainBootloaderMenu(RamBase, NV_WAIT_INFINITE));
            break;
        case CONTINUE:
            debug_print("Cold Booting Linux\r\n");
            NV_CHECK_ERROR_CLEANUP(
                LoadKernel(RamBase, KERNEL_PARTITON, NULL, 0));
            break;
        case REBOOT_BOOTLOADER:
            SetFastbootBit();
            NvAbootReset(gs_hAboot);
            break;
        case REBOOT:
            NvAbootReset(gs_hAboot);
            break;
        case POWEROFF:
            if (NvOdmPmuDeviceOpen(&hPmu))
            {
                if (!NvOdmPmuPowerOff(hPmu))
                    NvOsDebugPrintf("Poweroff failed\n");
            }
            break;
        case BOOT:
            if (KernelImage)
            {
                debug_print("Booting downloaded image\r\n");
                NV_CHECK_ERROR_CLEANUP(
                    LoadKernel(RamBase, NULL, KernelImage, LoadSize));
            }
            break;
    }

fail:
    if (hTrans)
        Nv3pTransportClose(hTrans);
    return e;
}

static NvError LoadKernel(NvU32 RamBase, const char *PartName,
        NvU8 *KernelImage, NvU32 LoadSize)
{
    NvError e = NvSuccess;

    if ((LoadSize == 0 || KernelImage == NULL) && PartName != NULL)
    {
        if (!HasValidKernelImage(&KernelImage, &LoadSize, PartName))
            goto fail;
    }

    if (KernelImage)
    {
        NvOsDebugPrintf("Platform Pre OS Boot configuration...\n");
        if (!NvOdmPlatformConfigure(NvOdmPlatformConfig_OsLoad))
        {
            goto fail;
        }
        NV_CHECK_ERROR_CLEANUP(
            BootAndroid(KernelImage, LoadSize, RamBase)
        );
    }

fail:
    debug_print(" Booting failed\r\n", PartName);
    NV_CHECK_ERROR_CLEANUP(StartFastbootProtocol(RamBase));
    return e;
}

void NvBlForceRecoveryMode();


NvError NvAppMain(int argc, char *argv[])
{
    NvRmDeviceHandle hRm = NULL;
    NvU32 EnterRecoveryMode = 0;
    NvU32 SecondaryBootDevice = 0;
    NvU32 RamSize;
    NvU32 RamBase;
    
#ifdef MAKE_SHOW_LOGO
	NvBool isShowMenu =NV_FALSE;
	RadioMenu *BootMenu = NULL;
	NvU32 logoX=314;
	NvU32 logoY=215;
       Icon *BootMenuImages[1] = {  NULL };
	
#endif

	
#if DO_JINGLE
    FastbootWaveHandle hWave;
#endif
    NvError e;
    CommandType Command;
    NvU32 reg = 0;
    NvU32 EnterFastboot = 0;
    NvU32 EnterRecoveryKernel = 0;
  
  //  debug_print("\n==================================================================================================>\r\n");
    debug_print("\n[bootloader] (built on %s, %s)\r\n",__DATE__,__TIME__);

    EnterRecoveryMode = (NvU32)NvBlQueryGetNv3pServerFlag();

//	debug_print("EnterRecoveryMode = %d============>\r\n",EnterRecoveryMode);
    e = NvAbootOpen(&gs_hAboot, &EnterRecoveryMode, &SecondaryBootDevice);
#ifdef MAKE_POWER_KEY_2SE_DECT
	if(!EnterRecoveryMode){
		NvAppCheckPowerDownState();
	}
#endif	
   if (!EnterRecoveryMode) {
#ifdef  BQ24160_CHARGER
 	NvSetChargerParameter(); //wantianpei add
#endif
 	}
	  NvAppInitDisplay();
#ifdef	LEDS_TLC59116_DRIVER
	Led_For_N020(); //lee add
#endif
#ifdef MAKE_SHOW_LOGO
#ifdef LOGO_QC750
    BootMenuImages[0] = (Icon *)&s_welcome_qc750;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N710) || defined(LOGO_N710_NEUTRAL)
    BootMenuImages[0] = (Icon *)&s_welcome_n710;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N750)
    BootMenuImages[0] = (Icon *)&s_welcome_n750;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N750_NEUTRAL)
    BootMenuImages[0] = (Icon *)&s_welcome_n750_neutral;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N711)
    BootMenuImages[0] = (Icon *)&s_welcome_n711;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N712)
    BootMenuImages[0] = (Icon *)&s_welcome_n712;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N1010)
    BootMenuImages[0] = (Icon *)&s_welcome_n1010;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_N1050)
    BootMenuImages[0] = (Icon *)&s_welcome_n1010;
    logoX = 0;
    logoY = 0;	
#elif defined(LOGO_N1020)
    BootMenuImages[0] = (Icon *)&s_welcome_n1010_neutral;
    logoX = 0;
    logoY = 0;   
#elif defined(LOGO_W1011A)
    BootMenuImages[0] = (Icon *)&s_welcome_w1011a;
    logoX = 0;
    logoY = 0;       
#elif defined(LOGO_N1010_NEUTRAL)
	BootMenuImages[0] = (Icon *)&s_welcome_n1010_neutral;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_N1011)
	BootMenuImages[0] = (Icon *)&s_welcome_n1011;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_NS_14T004)
    BootMenuImages[0] = (Icon *)&s_welcome_ns_14t004;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_ITQ1000)
	BootMenuImages[0] = (Icon *)&s_welcome_itq1000;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_WIKIPAD)
    BootMenuImages[0] = (Icon *)&s_welcome_wikipad;
    logoX = 0;
    logoY = 0;
#elif defined(LOGO_ITQ700)
	BootMenuImages[0] = (Icon *)&s_welcome_itq700;
	logoX = 260;
	logoY = 545;
#elif defined(LOGO_N750_HP)
	BootMenuImages[0] = (Icon *)&s_welcome_hp;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_CYBTT10_BK)
	BootMenuImages[0] = (Icon *)&s_welcome_cybtt10_bk;
	logoX = 0;//260;
	logoY = 0;//545;
#elif defined(LOGO_NABIXD)
	BootMenuImages[0] = (Icon *)&s_welcome_mt799;
	logoX = 484;
	logoY = 300;
#elif defined(LOGO_ITQ701)
	BootMenuImages[0] = (Icon *)&s_welcome_itq701;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_MM3201)
	BootMenuImages[0] = (Icon *)&s_welcome_mm3201;
	logoX = 0;
	logoY = 0;
#elif defined(LOGO_NABI2S)
    BootMenuImages[0] = (Icon *)&s_welcome_nabi2s;
    logoX = 316;
    logoY = 543;
#else
    BootMenuImages[0] = (Icon *)&s_welcome_mt799;
#endif
    BootMenu = FastbootCreateRadioMenu(1, 0, 0, NVIDIA_GREEN, BootMenuImages);
    MyFastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, BootMenu,logoX,logoY,NV_FALSE);//(398,0)
#endif
   debug_print("\n[bootloader] (built on %s, %s 2)\r\n",__DATE__,__TIME__);
    // Early development platform without the complete nvflash foodchain?
    if (e == NvError_NotImplemented)
    {
        // NOTE: This is very fragile. Correct identification of the
        //       framebuffer when booting via the backdoor requires
        //       that it be the first thing allocated from carveout.
        NvOsWaitUS(10*1000*1000);
        NvOsDebugPrintf("Load OS now via JTAG backdoor....\r\n");
        DoBackdoorLoad();
    }

    if (e != NvSuccess)
    {
        FastbootError("Failed to initialize Aboot\r\n");
        goto fail;
    }

    debug_print("Platform Pre Boot configuration...\r\n");
    if (!NvOdmPlatformConfigure(NvOdmPlatformConfig_BlStart))
    {
         goto fail;
    }

    s_SecondaryBoot = (NvDdkBlockDevMgrDeviceId)SecondaryBootDevice;

    if (EnterRecoveryMode)
    {
        debug_print("Entering NvFlash recovery mode / Nv3p Server\r\n");
        FastbootSetUsbMode(NV_TRUE);  // Configure USB driver for Nv3p mode
        NV_CHECK_ERROR_CLEANUP(NvAboot3pServer(gs_hAboot));
        NV_CHECK_ERROR_CLEANUP(NvAbootRawWriteMBRPartition(gs_hAboot));
#ifdef NV_EMBEDDED_BUILD
        // Nv3pServer exits implies flashing is complete. So, stop here.
        // User should be forced to reset the board here
        while(1);
#endif
        NvAbootReset(gs_hAboot);
    }

#ifndef NV_EMBEDDED_BUILD
    // FIXME: Write the MBR to the disk if the boot device is SDMMC.
    // There is a small race condition in which the user can potentially
    // turn off the system after nvflash completes and just before
    // the MBR is written. If this does happen, the system won't boot.
    if (s_SecondaryBoot == NvDdkBlockDevMgrDeviceId_SDMMC)
        NV_CHECK_ERROR_CLEANUP(NvAbootRawWriteMBRPartition(gs_hAboot));
#endif

    /* Get the physical base address of SDRAM. */
    NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
    NvRmModuleGetBaseAddress(hRm,
        NVRM_MODULE_ID(NvRmPrivModuleID_ExternalMemory, 0),
        &RamBase, &RamSize);

    /* Increase the CPU clocks before booting to OS */
#if NVODM_CPU_BOOT_PERF_SET
    NvRmPrivSetMaxCpuClock(hRm);
#endif

    /* Initilaize the BL message string based on platform */
    InitBlMessage();

    /* Initialise the input keys in order to use later */
    (void)InitInputKeys();
//must wait input key inited and then check recover key is pressed
#ifdef LPM_BATTERY_CHARGING
    /* Charge battery if low */
    if (!EnterRecoveryMode) {
		
#ifdef MAKE_LOW_BATTERY_DECT
#if defined(LOGO_NABIXD)
	logoX = 484+125;
	logoY = 300+50;
#endif
		NvOsDebugPrintf("Enter low power check!!!!!!!!!!!!!!\r\n");
		if(!Reboot_cmd) {
			KhAppChargeBatteryIfLow(BootMenu,logoX,logoY);
		}
#else
		NvAppChargeBatteryIfLow();
#endif

    }
	
#endif
  	  
	if( NV_TRUE ==NvCheckForceRecoveryMode())
	{
	#ifdef MAKE_SHOW_LOGO
		isShowMenu = NV_TRUE;
	#else
		NvBlForceRecoveryMode();
		NvRmModuleReset(hRm, NvRmPrivModuleID_System); 
	#endif
	}	

    /* Reading SCRATCH0 to determine booting mode */
    reg = NV_REGR(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
        APBDEV_PMC_SCRATCH0_0);

    /* Read bit for fastboot */
    EnterFastboot = reg & (1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
    /* Read bit for recovery */
    EnterRecoveryKernel = reg & (1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT);

    if (EnterFastboot && EnterRecoveryKernel)
    {
        /* Clearing fastboot and RCK flag bits */
        reg = reg & ~((1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT) |
            (1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT));
        NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
            APBDEV_PMC_SCRATCH0_0, reg);
        debug_print("Invalid boot configuration \r\n");
    }
    else if (EnterFastboot)
    {
        reg = reg & ~(1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
        NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
            APBDEV_PMC_SCRATCH0_0, reg);
        debug_print("Entering Fastboot based on booting mode configuration \r\n");

        NV_CHECK_ERROR_CLEANUP(
            StartFastbootProtocol(RamBase)
        );
    }
    else if (EnterRecoveryKernel)
    {
        reg = reg & ~(1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT);
        NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
            APBDEV_PMC_SCRATCH0_0, reg);
        debug_print("Entering RCK based on booting mode configuration \r\n");

        s_RecoveryKernel = NV_TRUE;
        NV_CHECK_ERROR_CLEANUP(
            LoadKernel(RamBase, RECOVERY_KERNEL_PARTITION, NULL, 0));
    }
    NvRmClose(hRm);

    NvOsDebugPrintf("Checking for android ota recovery \r\n");
    e = NvCheckForUpdateAndRecovery(gs_hAboot, &Command);
    if (e == NvSuccess && Command == CommandType_Recovery)
    {
        FastbootStatus("Booting Recovery kernel image\n");
        s_RecoveryKernel = NV_TRUE;
        NV_CHECK_ERROR_CLEANUP(LoadKernel(RamBase,
            RECOVERY_KERNEL_PARTITION, NULL, 0));
    }

#if DO_JINGLE
    if (FastbootPlayWave(TwoHundredHzSquareWave,
                         JINGLE_SAMPLERATE, (JINGLE_CHANNELS==2) ? NV_TRUE : NV_FALSE,
                         NV_TRUE, JINGLE_BITSPERSAMPLE, &hWave) == NvSuccess)
    {
        NvOsSleepMS(400);
        FastbootCloseWave(hWave);
    }
#endif
#ifdef MAKE_SHOW_LOGO	
    if(isShowMenu == NV_TRUE){
#endif

    NV_CHECK_ERROR_CLEANUP(
        MainBootloaderMenu(RamBase, WAIT_IN_BOOTLOADER_MENU));
#ifdef MAKE_SHOW_LOGO
	}else{
		debug_print("Cold-booting Linux\r\n");
	     NV_CHECK_ERROR_CLEANUP(
                LoadKernel(RamBase,KERNEL_PARTITON, NULL, 0));
	}
#endif	
	

fail:
    FastbootError("Unrecoverable bootloader error (0x%08x).\r\n", e);
    NV_ABOOT_BREAK();
}

static NvError MainBootloaderMenu(NvU32 RamBase, NvU32 TimeOut)
{
    NvError e = NvSuccess;
    NvRmDeviceHandle hRm = NULL;
    NvU32 MenuOption = MENU_SELECT_LINUX;
    NvU32 reg = 0;
#if SHOW_IMAGE_MENU
    RadioMenu *BootMenu = NULL;
#else
    TextMenu *BootMenu = NULL;
#endif
    NvU32 OdmData = 0;
    NvBool WaitForBootInput = NV_TRUE;
    NvBool DrawMenu = NV_TRUE;

    DrawMenu = ShowMenu();
    OdmData = nvaosReadPmcScratch();
    if (OdmData & (1 << WAIT_BIT_IN_ODMDATA))
        WaitForBootInput = NV_FALSE;

    /* Initialise the input keys*/
    e = InitInputKeys();
    ShowPlatformBlMsg();

    /* only run the menu if there is a selection that can be made */
    if ((TimeOut == NV_WAIT_INFINITE) || ((DrawMenu == NV_TRUE) &&
        (WaitForBootInput == NV_TRUE)))
    {
#if SHOW_IMAGE_MENU
        BootMenu = FastbootCreateRadioMenu(
            NV_ARRAY_SIZE(s_RadioImages), MenuOption, 3, NVIDIA_GREEN,
                s_RadioImages);
        MenuOption = BootloaderMenu(BootMenu, TimeOut);
#else
        BootMenu = FastbootCreateTextMenu(
            NV_ARRAY_SIZE(BootloaderMenuString), MenuOption,
                BootloaderMenuString);
        MenuOption = BootloaderTextMenu(BootMenu, TimeOut);
#endif
    }

    DeinitInputKeys();
    switch (MenuOption)
    {
        case MENU_SELECT_USB:
            NV_CHECK_ERROR_CLEANUP(StartFastbootProtocol(RamBase));
            break;

        case MENU_SELECT_LINUX:
            debug_print("Cold-booting Linux\r\n");
            NV_CHECK_ERROR_CLEANUP(
                LoadKernel(RamBase,KERNEL_PARTITON, NULL, 0));
            break;

        case MENU_SELECT_RCK:
            debug_print("Booting Recovery kernel image\r\n");
            s_RecoveryKernel = NV_TRUE;
            NV_CHECK_ERROR_CLEANUP(
                LoadKernel(RamBase, RECOVERY_KERNEL_PARTITION, NULL, 0));
            break;

        case MENU_SELECT_FORCED_RECOVERY:
            NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
            reg = NV_REGR(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
                APBDEV_PMC_SCRATCH0_0);
            reg = reg | (1 << APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_BIT);
            NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
                APBDEV_PMC_SCRATCH0_0, reg);
            NvRmClose(hRm);
            NvAbootReset(gs_hAboot);

        default:
            FastbootError("Unsupported menu option selected: %d\n",
                MenuOption);
            e = NvError_NotSupported;
            goto fail;
    }
fail:
    return e;
}

static NvBool HasValidDtbImage(void **Image, NvU32 *Size, const char *PartName)
{
    NvError e = NvError_Success;
    void *DtbImage = NULL;
    NvU32 DtbSize = 0;
    struct fdt_header hdr;
    int err;

    NV_CHECK_ERROR_CLEANUP(
        NvAbootBootfsReadNBytes(gs_hAboot, PartName, 0, sizeof(struct fdt_header),
            (NvU8*)&hdr)
    );

    err = fdt_check_header(&hdr);
    if (err)
    {
        NvOsDebugPrintf("%s: Invalid fdt header. err: %s\n", __func__, fdt_strerror(err));
        goto fail;
    }

    DtbSize = fdt_totalsize(&hdr);

    /* Reserve space for nodes which will be added later in BL */
    DtbImage = NvOsAlloc(DtbSize + FDT_SIZE_BL_DT_NODES);
    if (!DtbImage)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvAbootBootfsReadNBytes(gs_hAboot, PartName, 0, DtbSize, (NvU8 *)DtbImage)
    );

    if (!DtbImage || !DtbSize)
        goto fail;

    *Image = fdt_hdr = (struct fdt_header *)DtbImage;
    *Size = fdt_totalsize(fdt_hdr);
    return NV_TRUE;

fail:
    if (DtbImage)
        NvOsFree(DtbImage);
    return NV_FALSE;
}

static NvBool HasValidKernelImage(NvU8 **Image, NvU32 *Size, const char *PartName)
{
    NvError e = NvSuccess;
    NvU8 *KernelImage = NULL;
    NvU32 LoadSize = 0;
    NvU32 KernelPages = 0;
    NvU32 RamdiskPages = 0;
    AndroidBootImg hdr;

    /* Read  header */
    NV_CHECK_ERROR_CLEANUP(
        NvAbootBootfsReadNBytes(gs_hAboot, PartName, 0, sizeof(AndroidBootImg),
            (NvU8*)&hdr)
    );

    if (NvOsMemcmp(hdr.magic, ANDROID_MAGIC, ANDROID_MAGIC_SIZE))
    {
        e = NvError_NotInitialized;
        goto fail;
    }

    /* LoadSize is sum of size of header, kernel, ramdisk and second stage
    * loader. */
    KernelPages = (hdr.kernel_size + hdr.page_size - 1) / hdr.page_size;
    RamdiskPages = (hdr.ramdisk_size + hdr.page_size - 1) / hdr.page_size;
    LoadSize = hdr.page_size * (1 + KernelPages + RamdiskPages) +
        hdr.second_size;
    if (!LoadSize)
    {
        e = NvError_NotInitialized;
        goto fail;
    }

    KernelImage = (NvU8 *)NvOsAlloc(LoadSize);

    if (!KernelImage)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvAbootBootfsReadNBytes(gs_hAboot, PartName, 0, LoadSize, KernelImage)
    );

fail:
    if (e == NvSuccess)
    {
        if (Image)
        {
            if (*Image)
                NvOsFree(*Image);
            *Size = LoadSize;
            *Image = KernelImage;
            return NV_TRUE;
        }
    }

    if(KernelImage)
        NvOsFree(KernelImage);

    return (e==NvSuccess) ? NV_TRUE : NV_FALSE;
}

/* 4 images, plus 1 used by TF code */
#define NB_IMAGES 5

static NvError BootAndroid(void *KernelImage, NvU32 ImageSize, NvU32 RamBase)
{
    AndroidBootImg* hdr = (AndroidBootImg *)KernelImage;
    const char *AndroidMagic = "ANDROID!";
    unsigned KernelPages;
    unsigned RamdiskPages;
    const NvU8 *Platform;
    void    *SrcImages[NB_IMAGES];
    NvUPtr   DstImages[NB_IMAGES];
    NvU32    Sizes[NB_IMAGES];
    NvU32    Registers[3];
    NvU32    Cnt;
    NvUPtr BootAddr    = RamBase + ZIMAGE_START_ADDR;
    /* Approximately 32 MB free memory for kernel decompression.
     * If the size of uncompressed and compressed kernel combined grows
     * beyond this size, ramdisk corruption will occur!
     */
    NvUPtr RamdiskAddr = BootAddr + AOS_RESERVE_FOR_KERNEL_IMAGE_SIZE;
    void     *DtbImage = NULL;
    NvU32    DtbSize           = 0;
    NvU32    newlen;
    int      err;
    NvError  NvErr = NvError_Success;
#ifdef CONFIG_TRUSTED_FOUNDATIONS
    TFSW_PARAMS nTFSWParams;
#endif
    const    NvUPtr TagsAddr    = RamBase + 0x100UL;
    NvU32    i;

    if (NvOsMemcmp(hdr->magic, AndroidMagic, NvOsStrlen(AndroidMagic)))
        FastbootError("Magic value mismatch: %c%c%c%c%c%c%c%c\n",
            hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3],
            hdr->magic[4], hdr->magic[5], hdr->magic[6], hdr->magic[7]);

    KernelPages = (hdr->kernel_size + hdr->page_size - 1) / hdr->page_size;
    RamdiskPages = (hdr->ramdisk_size + hdr->page_size - 1) / hdr->page_size;

    SrcImages[0] = (NvU8 *)KernelImage + hdr->page_size;
    DstImages[0] = BootAddr;

    /* FIXME:  With large RAM disks, the kernel refuses to boot unless the
     * kernel and RAM disk are located contiguously in memory.  I have no
     * idea why this is the case.
     */
    Sizes[0]     = KernelPages * hdr->page_size;

    if (HasValidDtbImage(&DtbImage, &DtbSize, "DTB"))
    {
        SrcImages[1] = (NvU8 *)DtbImage;
        DstImages[1] = TagsAddr;
        Sizes[1] = DtbSize;
        Cnt = 2;
    }
    else
    {
        SrcImages[1] = (NvU8 *)s_BootTags;
        DstImages[1] = TagsAddr;
        Sizes[1]     = sizeof(s_BootTags);
        Cnt = 2;
    }

    if (hdr->ramdisk_size > RAMDISK_MAX_SIZE)
    {
        FastbootError("Ramdisk size is more than expected(%d MB)\n",(RAMDISK_MAX_SIZE>>20));
        return NvError_NotSupported;
    }

    if (hdr->ramdisk_size)
    {
        SrcImages[Cnt] = (NvU8 *)KernelImage + hdr->page_size*(1+KernelPages);
        DstImages[Cnt] = RamdiskAddr;
        Sizes[Cnt]     = RamdiskPages * hdr->page_size;
        Cnt++;
    }
    if (hdr->second_size)
    {
        SrcImages[Cnt] = (NvU8 *)KernelImage +
            hdr->page_size*(1+KernelPages + RamdiskPages);
        DstImages[Cnt] = (NvUPtr)hdr->second_addr;
        Sizes[Cnt]     = hdr->second_size;
        Cnt++;
    }

    /* create additional space for the new command line and nodes */
    if (DtbImage)
    {
        newlen = fdt_totalsize((void *)DtbImage) + FDT_SIZE_BL_DT_NODES;
        err = fdt_open_into((void *)DtbImage, (void *)DtbImage, newlen);
        if (err < 0)
        {
            NvOsDebugPrintf("FDT: fdt_open_into fail (%s)\n", fdt_strerror(err));
            return NvError_ResourceError;
        }
    }

    Registers[0] = 0;
    Platform = NvOdmQueryPlatform();

    if (DtbImage)
    {
        Registers[1] = MACHINE_TYPE_TEGRA_FDT;
        Sizes[1] = fdt_totalsize((void *)DtbImage);
    }
    else
    {
        Registers[1] = MACHINE_TYPE_TEGRA_GENERIC;

        for (i = 0; i < NV_ARRAY_SIZE(machine_types); ++i)
        {
            if (!NvOsMemcmp(Platform, machine_types[i].name,
                            NvOsStrlen(machine_types[i].name)))
            {
                Registers[1] = machine_types[i].id;
                break;
            }
        }
    }

    Registers[2] = TagsAddr;

#ifdef CONFIG_TRUSTED_FOUNDATIONS
    /* setup bootparams and patch warmboot code */
    NvOsMemset(&nTFSWParams, 0, sizeof(TFSW_PARAMS));
    nTFSWParams.ColdBootAddr = BootAddr;
    NvAbootTFPrepareBootParams(gs_hAboot, &nTFSWParams, Registers);
    BootAddr = (NvUPtr)&nTFSWParams;

    SrcImages[Cnt] = (NvU8 *)nTFSWParams.pTFAddr;
    DstImages[Cnt] = nTFSWParams.TFStartAddr;
    Sizes[Cnt]     = nTFSWParams.TFSize;
    Cnt++;
#endif

    NvErr = SetupTags(hdr, RamdiskAddr);
    if (NvErr != NvError_Success)
        return NvErr;

    NvAbootCopyAndJump(gs_hAboot, DstImages, SrcImages, Sizes,
        Cnt, BootAddr, Registers, 3);
    FastbootError("Critical failure: Unable to start kernel.\n");

    return NvError_NotSupported;
}

static const PartitionNameMap *MapFastbootToNvPartition(const char* name)
{
    NvU32 i;
    NvU32 NumPartitions;

    NumPartitions = NV_ARRAY_SIZE(gs_PartitionTable);

    for (i=0; i<NumPartitions; i++)
    {
        if (NvOsStrlen(name) !=
            NvOsStrlen(gs_PartitionTable[i].LinuxPartName))
            continue;
        if (!NvOsMemcmp(gs_PartitionTable[i].LinuxPartName, name,
                 NvOsStrlen(gs_PartitionTable[i].LinuxPartName)))
            return &gs_PartitionTable[i];
    }

    return NULL;
}
static void SetupWarmbootArgs(void)
{
    NvError e = NvSuccess;
    NvRmDeviceHandle hRm = NULL;
    NvBootArgsWarmboot WarmbootArgs;
    void *ptr;

    static const NvRmHeap heaps[] =
    {
        NvRmHeap_ExternalCarveOut,
    };

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));
    NV_CHECK_ERROR_CLEANUP(NvRmMemHandleCreate(hRm,
        &s_hWarmbootHandle, WB0_SIZE)
    );
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAllocTagged(s_hWarmbootHandle, heaps, NV_ARRAY_SIZE(heaps),
            4096, NvOsMemAttribute_Uncached,
            NVRM_MEM_TAG_SYSTEM_MISC)
    );
    NV_CHECK_ERROR_CLEANUP(NvRmMemMap(s_hWarmbootHandle, 0,
        WB0_SIZE, 0, &ptr));
    g_wb0_address = NvRmMemPin(s_hWarmbootHandle);
    NV_CHECK_ERROR_CLEANUP(
        NvAbootInitializeCodeSegment(gs_hAboot,
                                     s_hWarmbootHandle,
                                     (NvU32)ptr));

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemHandlePreserveHandle(s_hWarmbootHandle,
            &WarmbootArgs.MemHandleKey)
    );

    NvOsBootArgSet(NvBootArgKey_WarmBoot, &WarmbootArgs, sizeof(WarmbootArgs));

    NvRmClose(hRm);

fail:
    if (e)
    {
        FastbootError("Failed to setup warmboot args %x\n", e);
    }
}

static NvError SetupTags(
    const AndroidBootImg *hdr,
    NvUPtr RamdiskPa)
{
    NvU32 CoreTag[3];
    NvU32 SkuRev = 0;
    NvU32 NumPartitions;
    NvError NvErr = NvError_Success;

    /* Same as defaults from kernel/arch/arm/setup.c */
    CoreTag[0] = 1;
    CoreTag[1] = 4096;
    CoreTag[2] = 0xff;

    FastbootAddAtag(ATAG_CORE, sizeof(CoreTag), CoreTag);

#ifdef NV_EMBEDDED_BUILD
    NvBootGetSkuNumber(&SkuRev);
    FastbootAddAtag(ATAG_REVISION, sizeof(SkuRev), &SkuRev);
#endif
    if (hdr->name[0])
        debug_print("%s\r\n", hdr->name);
    if (hdr->ramdisk_size)
    {
        if (fdt_hdr)
        {
            int node, err;

            node = fdt_path_offset((void *)fdt_hdr, "/chosen");
            if (node < 0)
            {
                NvOsDebugPrintf("%s: Unable to find /chosen (%s)\n", __func__,
                    fdt_strerror(err));
                NvErr = NvError_ResourceError;
                goto fail;
            }

            err = fdt_setprop_cell((void *)fdt_hdr, node, "linux,initrd-start", RamdiskPa);
            if (err)
            {
                NvOsDebugPrintf("%s: Unable to set initrd-start (%s)\n", __func__,
                    fdt_strerror(err));
                NvErr = NvError_ResourceError;
                goto fail;
            }

            err = fdt_setprop_cell((void *)fdt_hdr, node, "linux,initrd-end", RamdiskPa + hdr->ramdisk_size);
            if (err)
            {
                NvOsDebugPrintf("%s: Unable to set initrd-end (%s)\n", __func__,
                    fdt_strerror(err));
                NvErr = NvError_ResourceError;
                goto fail;
            }
        }
        else
        {
            NvU32 RdTags[2];
            RdTags[0] = RamdiskPa;
            RdTags[1] = hdr->ramdisk_size;
            FastbootAddAtag(ATAG_INITRD2, sizeof(RdTags), RdTags);
        }
    }

    SetupWarmbootArgs();

    /* Add serial tag for the board serial number */
    NvErr = AddSerialBoardInfoTag();
    if (NvErr != NvError_Success)
        goto fail;

    NumPartitions = NV_ARRAY_SIZE(gs_PartitionTable);

    NvErr = FastbootCommandLine(gs_hAboot, gs_PartitionTable,
        NumPartitions, s_SecondaryBoot, hdr->cmdline, (hdr->ramdisk_size>0),
        (const unsigned char *)hdr->name, s_RecoveryKernel);
    if (NvErr != NvError_Success)
            goto fail;

    if (s_FrontbufferPixels)
    {
        NvBootArgsFramebuffer BootFb;
        NvBootArgsDisplay BootDisp;
        NvRmMemHandlePreserveHandle(s_Framebuffer.hMem, &BootFb.MemHandleKey);

        BootFb.ColorFormat = s_Framebuffer.ColorFormat;
        BootFb.Size = NvRmSurfaceComputeSize(&s_Framebuffer);
        BootFb.Width = s_Framebuffer.Width;
        BootFb.Height = s_Frontbuffer.Height;
        BootFb.Pitch = s_Framebuffer.Pitch;
        BootFb.SurfaceLayout = s_Framebuffer.Layout;
        BootFb.NumSurfaces = (s_Framebuffer.Height / s_Frontbuffer.Height);
        BootFb.Flags = FastbootGetDisplayFbFlags();

        NvOsBootArgSet(NvBootArgKey_Framebuffer, &BootFb, sizeof(BootFb));

        BootDisp.Controller = bl_nDispController;
        BootDisp.DisplayDeviceIndex = 0;
        BootDisp.bEnabled = NV_TRUE;

        NvOsBootArgSet(NvBootArgKey_Display, &BootDisp, sizeof(BootDisp));
    }
    {
        NvRmDeviceHandle hRm = NULL;
        if ((NvRmOpen(&hRm, 0) != NvSuccess) ||
            (NvRmBootArgChipShmooSet(hRm) != NvSuccess))
        {
            FastbootError("Failed to set shmoo boot argument\n");
        }
        NvRmClose(hRm);
    }
    FastbootAddAtag(0,0,NULL);

fail:
    return NvErr;
}

// FIXME: Should get these from ODM.
#define NVEC_LEFT_ARROW_KEY 57419
#define NVEC_RIGHT_ARROW_KEY 57421
#define NVEC_ENTER_KEY 28
#define NVEC_KEY_PRESS_FLAG 2
#define NVEC_KEY_RELEASE_FLAG 1
#define TEGRA_LEFT_ARROW_KEY 4207
#define TEGRA_RIGHT_ARROW_KEY 4205
#define TEGRA_ENTER_KEY 4155
#define MENU_OPTION_HIGHLIGHT_LEVEL 0xC0
#define TEGRA_KEY_HOME   102
#define TEGRA_KEY_BACK   158
#define TEGRA_KEY_MENU   139

#define TEGRA_KBC_KEY_LEFT TEGRA_KEY_HOME
#define TEGRA_KBC_KEY_RIGHT TEGRA_KEY_BACK
#define TEGRA_KBC_KEY_ENTER TEGRA_KEY_MENU

static NvU8 GetKeyEvent()
{
    NvError e = NvSuccess;
    NvU32 RetKeyVal = KEY_IGNORE;

    if (!(g_hScroll || g_OdmKbdExists || g_hDdkKbc))
        goto fail;

    if (g_hScroll)
    {
        NvOdmScrollWheelEvent Event = NvOdmScrollWheelEvent_None;

        e = NvOsSemaphoreWaitTimeout(g_hScrollSema, 14);
        if (e != NvSuccess)
            goto fail;
        Event = NvOdmScrollWheelGetEvent(g_hScroll);
        if (Event & NvOdmScrollWheelEvent_RotateClockWise)
            RetKeyVal = KEY_DOWN;
        else if (Event & NvOdmScrollWheelEvent_RotateAntiClockWise)
            RetKeyVal = KEY_UP;
        else if (Event & NvOdmScrollWheelEvent_Press)
            RetKeyVal = KEY_ENTER;
    }
    else if (g_OdmKbdExists)
    {
        NvU32 KeyCode;
        NvU8 ScanCodeFlags;
        NvU8 IsKeyPressed;
        NvBool RetVal;

        RetVal = NvOdmKeyboardGetKeyData(&KeyCode, &ScanCodeFlags, 14);
        if (RetVal == NV_TRUE)
        {
            IsKeyPressed = ScanCodeFlags & NVEC_KEY_PRESS_FLAG;
            if (!IsKeyPressed)
                goto fail;
            if (KeyCode == NVEC_LEFT_ARROW_KEY)
                RetKeyVal = KEY_UP;
            else if (KeyCode == NVEC_RIGHT_ARROW_KEY)
                RetKeyVal = KEY_DOWN;
            else if (KeyCode == NVEC_ENTER_KEY)
                RetKeyVal = KEY_ENTER;

	 #ifdef KEENHI_BOOTLOADER_USE_VOLUME_UP_AS_ENTER
		if(NVEC_LEFT_ARROW_KEY ==KeyCode){
			RetKeyVal =KEY_ENTER;
		}
	#endif
        }
    }
    else if (g_hDdkKbc)
    {
        NvU32 i;
        NvU32 WaitTime;
        NvU32 KeyCount;
        NvU8 IsKeyPressed;
        // FIXME: Get this max event count from either capabilties or ODM.
        NvU32 KeyCodes[16];
        NvDdkKbcKeyEvent KeyEvents[16];

        WaitTime = NvDdkKbcGetKeyEvents(g_hDdkKbc, &KeyCount, KeyCodes,
            KeyEvents);

        for (i = 0; i < KeyCount; i++)
        {
            IsKeyPressed =
                (KeyEvents[i] == NvDdkKbcKeyEvent_KeyPress) ? 1 : 0;
            if (!IsKeyPressed)
                continue;
            if (KeyCodes[i] == TEGRA_KBC_KEY_LEFT)
                RetKeyVal = KEY_UP;
            else if(KeyCodes[i] == TEGRA_KBC_KEY_RIGHT)
                RetKeyVal = KEY_DOWN;
            else if (KeyCodes[i] == TEGRA_KBC_KEY_ENTER)
                RetKeyVal = KEY_ENTER;
        }
        e = NvOsSemaphoreWaitTimeout(g_hDdkKbcSema, WaitTime);
    }

fail:
    return RetKeyVal;
}

static void ShowPlatformBlMsg()
{
    s_PrintCtx.y = 0;
    if (!(g_hScroll || g_OdmKbdExists|| g_hDdkKbc))
        debug_print(s_PlatformBlMsg[MSG_NO_KEY_DRIVER_INDEX]);
    else
        debug_print(s_PlatformBlMsg[MSG_FASTBOOT_MENU_SEL_INDEX]);
    s_PrintCtx.y += DISP_FONT_HEIGHT * BLANK_LINES_BEFORE_MENU;
}

static NvU32 BootloaderTextMenu(
    TextMenu *Menu,
    NvU32 Timeout)
{
    NvU64 TimeoutInUs = Timeout * 1000;
    NvU64 StartTime;
    NvBool IgnoreTimeout = NV_FALSE;
    NvU32 keyEvent = KEY_IGNORE;
    NvU32 X = 0;
    NvU32 Y = 0;

    if (!(g_hScroll || g_OdmKbdExists|| g_hDdkKbc))
        return 0;

    StartTime = NvOsGetTimeUS();
    Y = s_PrintCtx.y;
    FastbootDrawTextMenu(Menu, X, Y);
    if (Timeout == NV_WAIT_INFINITE)
        IgnoreTimeout = NV_TRUE;

    while (IgnoreTimeout || (TimeoutInUs > (NvOsGetTimeUS() - StartTime)))
    {
        keyEvent = GetKeyEvent();
        switch (keyEvent)
        {
            case KEY_UP:
                FastbootTextMenuSelect(Menu, NV_FALSE);
                FastbootDrawTextMenu(Menu, X, Y);
                IgnoreTimeout = NV_TRUE;
                break;
            case KEY_DOWN:
                FastbootTextMenuSelect(Menu, NV_TRUE);
                FastbootDrawTextMenu(Menu, X, Y);
                IgnoreTimeout = NV_TRUE;
                break;
            case KEY_ENTER:
                goto done;
        }
    }

done:
    s_PrintCtx.y += DISP_FONT_HEIGHT *
        (Menu->NumOptions + BLANK_LINES_AFTER_MENU);

fail:
    if (!Timeout)
        return 0;
    return (int)(Menu->CurrOption);
}

NvU32 NvOdmKeyboardGetKeyState(NvU32 keyCode, int timeout);

static int NvCheckForceRecoveryMode()
{
	if(g_OdmKbdExists){
		NvU32 KeyCode;
		NvU8 ScanCodeFlags;
		NvU8 IsKeyPressed;
		NvU32 RetVal;

		//RetVal = NvOdmKeyboardGetKeyData(&KeyCode, &ScanCodeFlags, 140);

		RetVal =NvOdmKeyboardGetKeyState(0xE04B,100);

		debug_print("NvCheckForceRecoveryMode RetVal=%d\r\n",RetVal);

		if(RetVal ==1){
			return NV_TRUE;
		}
	}

	return NV_FALSE;
}

static int BootloaderMenu(RadioMenu *Menu, NvU32 Timeout)
{
    NvBool IgnoreTimeout = NV_FALSE;
    NvU32 keyEvent = KEY_IGNORE;
    NvU64 TimeoutInUs = Timeout * 1000;
    NvU64 StartTime = NvOsGetTimeUS();
    if (!(g_hScroll || g_OdmKbdExists|| g_hDdkKbc))
        return 0;

    FastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, Menu);
    //Please do not remove the below print used for tracking boot time
    NvOsDebugPrintf("Logo Displayed at:%d ms\n", NvOsGetTimeMS());
    if (Timeout != NV_WAIT_INFINITE)
        debug_print("OS will cold boot in %u seconds if no input "
            "is detected\r\n", Timeout/1000);
    else
        IgnoreTimeout = NV_TRUE;

    // To get highlighted look.
    Menu->PulseAnim = MENU_OPTION_HIGHLIGHT_LEVEL;
    FastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, Menu);

    while (IgnoreTimeout || (TimeoutInUs > (NvOsGetTimeUS() - StartTime)))
    {
        keyEvent = GetKeyEvent();
        switch (keyEvent)
        {
            case KEY_UP:
                FastbootRadioMenuSelect(Menu, NV_FALSE);
                IgnoreTimeout = NV_TRUE;
                break;
            case KEY_DOWN:
                FastbootRadioMenuSelect(Menu, NV_TRUE);
                IgnoreTimeout = NV_TRUE;
                break;
            case KEY_ENTER:
                goto fail;
        }
        FastbootDrawRadioMenu(&s_Frontbuffer, s_FrontbufferPixels, Menu);
    }

fail:
    if (!Timeout)
        return 0;
    return (int)Menu->CurrOption;
}

