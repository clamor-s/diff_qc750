#include "nvcommon.h"
#include "nvodm_disp.h"
#include "nvodm_backlight.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvos.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <unistd.h>

typedef struct NvOdmDispDeviceRec {
    NvOdmDispDeviceMode bootMode;
    NvOdmDispTftConfig bootConfig;
    NvBool init;
} NvOdmDispDevice;

static NvOdmDispDevice s_Device = {
    .init = NV_FALSE,
    .bootConfig = {
        .flags = 0,
    },
};

static NvOdmDispDeviceTiming s_WxgaTiming = {
    .Horiz_RefToSync = 11,
    .Vert_RefToSync = 1,
    .Horiz_SyncWidth = 58,
    .Vert_SyncWidth = 4,
    .Horiz_BackPorch = 58,
    .Vert_BackPorch = 4,
    .Horiz_FrontPorch = 58,
    .Vert_FrontPorch = 4,
};

static NvOdmDispDeviceTiming s_WsgaTiming = {
    .Horiz_RefToSync = 4,
    .Vert_RefToSync = 2,
    .Horiz_SyncWidth = 136,
    .Vert_SyncWidth = 4,
    .Horiz_BackPorch = 138,
    .Vert_BackPorch = 21,
    .Horiz_FrontPorch = 34,
    .Vert_FrontPorch = 4,
};

static NvOdmDispDeviceTiming s_720pTiming = {
    .Horiz_RefToSync = 11,
    .Vert_RefToSync = 1,
    .Horiz_SyncWidth = 26,
    .Vert_SyncWidth = 6,
    .Horiz_BackPorch = 12,
    .Vert_BackPorch = 3,
    .Horiz_FrontPorch = 12,
    .Vert_FrontPorch = 3,
};

static void get_screeninfo(NvOdmDispDevice *dev)
{
    struct fb_var_screeninfo info;
    int fd;
#if defined(TARGET_STUB_NVRM)
    static const char *devFileName = "/dev/fb0";
#else
    static const char *devFileName = "/dev/graphics/fb0";
#endif

    fd = open(devFileName, O_RDONLY);
    if (fd < 0) {
      NvOsDebugPrintf("unable to open %s\n", devFileName);
        return;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &info)!=0) {
        NvOsDebugPrintf("unable to query screen info!\n");
        close(fd);
        return;
    }

    close(fd);
    NvOdmOsMemset(&s_Device, 0, sizeof(s_Device));
    s_Device.bootMode.width = info.xres;
    s_Device.bootMode.height = info.yres;
    if (info.xres == 1024) {
        s_Device.bootMode.timing = s_WsgaTiming;
        s_Device.bootMode.bpp = 18;
        s_Device.bootMode.frequency = 42430;
    } else if (info.xres == 1280) {
        s_Device.bootMode.timing = s_720pTiming;
        s_Device.bootMode.bpp = 24;
        s_Device.bootMode.frequency = 80000;
    } else if (info.xres == 1366) {
        s_Device.bootMode.timing = s_WxgaTiming;
        s_Device.bootMode.bpp = 18;
        s_Device.bootMode.frequency = 72000;
    } else {
        NvOsDebugPrintf("unknown display %dx%d\n", info.xres, info.yres);
        return;
    }
    s_Device.bootMode.timing.Horiz_DispActive = info.xres;
    s_Device.bootMode.timing.Vert_DispActive = info.yres;
    s_Device.bootMode.flags = 0;
    s_Device.bootMode.bPartialMode = NV_FALSE;
    s_Device.init = NV_TRUE;
}

void NvOdmDispListModes(
    NvOdmDispDeviceHandle hDevice,
    NvU32 *nModes,
    NvOdmDispDeviceMode *modes)
{
    if (*nModes) {
        *nModes = 1;
        *modes = hDevice->bootMode;
    } else
        *nModes = 1;
}

void NvOdmDispListDevices(NvU32 *nDevices, NvOdmDispDeviceHandle *phDevices)
{
    if (!s_Device.init) {
        get_screeninfo(&s_Device);
        if (!s_Device.init) {
            *nDevices = 0;
            return;
        }
    }
    *nDevices = 1;
    if (phDevices)
        *phDevices = &s_Device;
}

NvBool NvOdmDispGetParameter(
    NvOdmDispDeviceHandle hDevice,
    NvOdmDispParameter param,
    NvU32 *val)
{
    if (param==NvOdmDispParameter_Type)
        *val = NvOdmDispDeviceType_TFT;
    else if (param==NvOdmDispParameter_Usage)
        *val = NvOdmDispDeviceUsage_Primary;
    else if (param==NvOdmDispParameter_MaxHorizontalResolution)
        *val = hDevice->bootMode.width;
    else if (param==NvOdmDispParameter_MaxVerticalResolution)
        *val = hDevice->bootMode.height;
    else if (param==NvOdmDispParameter_BaseColorSize)
        *val = (hDevice->bootMode.bpp==18) ? NvOdmDispBaseColorSize_666 :
            NvOdmDispBaseColorSize_888;
    else if (param==NvOdmDispParameter_DataAlignment)
        *val = NvOdmDispDataAlignment_MSB;
    else if (param==NvOdmDispParameter_PinMap)
        *val = (hDevice->bootMode.bpp==18) ? NvOdmDispPinMap_Single_Rgb18 :
            NvOdmDispPinMap_Single_Rgb24_Spi5;
    else if (param==NvOdmDispParameter_PwmOutputPin)
        *val = NvOdmDispPwmOutputPin_Unspecified;
    else
        return NV_FALSE;

    return NV_TRUE;
}

void *NvOdmDispGetConfiguration(NvOdmDispDeviceHandle hDevice)
{
    return (void *) &hDevice->bootConfig;
}

void NvOdmDispGetPinPolarities(
    NvOdmDispDeviceHandle hDevice,
    NvU32 *nPins,
    NvOdmDispPin *pins,
    NvOdmDispPinPolarity *vals)
{
    *nPins = 0;
}

NvBool NvOdmDispSetSpecialFunction(
    NvOdmDispDeviceHandle hDevice,
    NvOdmDispSpecialFunction function,
    void *config)
{
    return NV_FALSE;
}

NvBool NvOdmDispSetMode(
    NvOdmDispDeviceHandle hDevice,
    NvOdmDispDeviceMode *mode,
    NvU32 flags)
{
    return NV_TRUE;
}

void NvOdmDispGetDefaultGuid(NvU64 *guid)
{
    *guid = NV_ODM_GUID('f','f','a','_','l','c','d','0');
}

NvBool NvOdmDispGetDeviceByGuid(NvU64 guid, NvOdmDispDeviceHandle *phDevice)
{
    if (!s_Device.init) {
        get_screeninfo(&s_Device);
        if (!s_Device.init) {
            return NV_FALSE;
        }
    }
    *phDevice = &s_Device;
    return NV_TRUE;
}

void NvOdmDispReleaseDevices(NvU32 nDevices, NvOdmDispDeviceHandle *hDevices)
{
}

void NvOdmDispSetBacklight(NvOdmDispDeviceHandle hDevice, NvU8 intensity)
{
}

NvBool
NvOdmBacklightInit(NvOdmDispDeviceHandle hDevice, NvBool bReopen)
{
    return NV_FALSE;
}

void NvOdmBacklightDeinit(NvOdmDispDeviceHandle hDevice)
{
}

void NvOdmBacklightIntensity(
    NvOdmDispDeviceHandle hDevice,
    NvU8 intensity)
{
}

void NvOdmDispSetPowerLevel(
    NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level)
{
}
