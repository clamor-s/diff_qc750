#ifndef ___YUV_SENSOR_H__MT9P111
#define ___YUV_SENSOR_H__MT9P111

#include <linux/ioctl.h>  /* For IOCTL macros */

/*-------------------------------------------Important---------------------------------------
 * for changing the SENSOR_NAME, you must need to change the owner of the device. For example
 * Please add /dev/mt9d115 0600 media camera in below file
 * ./device/nvidia/ventana/ueventd.ventana.rc
 * Otherwise, ioctl will get permission deny
 * -------------------------------------------Important--------------------------------------
*/

#define GC0308_SENSOR_NAME	"gc0308"
#define HM2057_SENSOR_NAME	"hm2057"
#define DEV(x)          "/dev/"x
#define GC0308_SENSOR_PATH     DEV(GC0308_SENSOR_NAME)
#define HM2057_SENSOR_PATH     DEV(HM2057_SENSOR_NAME)
#define LOG_NAME(x)     "ImagerODM-"x
#define LOG_TAG         LOG_NAME(SENSOR_NAME)

#define SENSOR_WAIT_MS       0 /* special number to indicate this is wait time require */
#define SENSOR_TABLE_END     0x00 /* special number to indicate this is end of table */
#define WRITE_REG_DATA8      2 /* special the data width as one byte */
#define WRITE_REG_DATA16     3 /* special the data width as one byte */

#define SENSOR_MAX_RETRIES   3 /* max counter for retry I2C access */

#define SENSOR_IOCTL_SET_MODE		_IOW('o', 1, struct sensor_mode)
#define SENSOR_IOCTL_GET_STATUS		_IOR('o', 2, __u8)
#define SENSOR_IOCTL_SET_COLOR_EFFECT   _IOW('o', 3, __u8)
#define SENSOR_IOCTL_SET_YUV_EXPOSURE   _IOW('o', 4, __u8)//add keenhi for yuv exposure
#define SENSOR_IOCTL_SET_WHITE_BALANCE  _IOW('o', 5, __u8)
#define SENSOR_IOCTL_SET_SCENE_MODE     _IOW('o', 6, __u8)
#define SENSOR_IOCTL_SET_GET_SHADING   _IO('o', 7)
#define SENSOR_IOCTL_SET_AF_MODE        _IOW('o', 8, __u8)
#define SENSOR_IOCTL_GET_AF_STATUS      _IOW('o', 9, __u8)

enum {
      YUV_ColorEffect = 0,
      YUV_Whitebalance,
      YUV_SceneMode
};

enum {
      YUV_ColorEffect_Invalid = 0,
      YUV_ColorEffect_Aqua,
      YUV_ColorEffect_Blackboard,
      YUV_ColorEffect_Mono,
      YUV_ColorEffect_Negative,
      YUV_ColorEffect_None,
      YUV_ColorEffect_Posterize,
      YUV_ColorEffect_Sepia,
      YUV_ColorEffect_Solarize,
      YUV_ColorEffect_Whiteboard
};

//for keenhi add exposure value enum
enum {
      YUV_YUVExposure_Invalid = 0,
      YUV_YUVExposure_Positive2,
      YUV_YUVExposure_Positive1,
      YUV_YUVExposure_Number0,
      YUV_YUVExposure_Negative1,
      YUV_YUVExposure_Negative2,
};

enum {
      YUV_Whitebalance_Invalid = 0,
      YUV_Whitebalance_Auto,
      YUV_Whitebalance_Incandescent,
      YUV_Whitebalance_Fluorescent,
      YUV_Whitebalance_WarmFluorescent,
      YUV_Whitebalance_Daylight,
      YUV_Whitebalance_CloudyDaylight,
      YUV_Whitebalance_Shade,
      YUV_Whitebalance_Twilight,
      YUV_Whitebalance_Custom
};

enum {
      YUV_SceneMode_Invalid = 0,
      YUV_SceneMode_Auto,
      YUV_SceneMode_Action,
      YUV_SceneMode_Portrait,
      YUV_SceneMode_Landscape,
      YUV_SceneMode_Beach,
      YUV_SceneMode_Candlelight,
      YUV_SceneMode_Fireworks,
      YUV_SceneMode_Night,
      YUV_SceneMode_NightPortrait,
      YUV_SceneMode_Party,
      YUV_SceneMode_Snow,
      YUV_SceneMode_Sports,
      YUV_SceneMode_SteadyPhoto,
      YUV_SceneMode_Sunset,
      YUV_SceneMode_Theatre,
      YUV_SceneMode_Barcode
};

struct sensor_mode {
	int xres;
	int yres;
};

#ifdef __KERNEL__
struct yuv_sensor_platform_data {
	int (*power_on)(void);
	int (*power_off)(void);

};
#endif /* __KERNEL__ */

#endif  /* __YUV_SENSOR_H__ */

