#ifndef ___YUV_SENSOR_H__T8EV5
#define ___YUV_SENSOR_H__T8EV5

#include <linux/ioctl.h>  /* For IOCTL macros */

/*-------------------------------------------Important---------------------------------------
 * for changing the SENSOR_NAME, you must need to change the owner of the device. For example
 * Please add /dev/mt9d115 0600 media camera in below file
 * ./device/nvidia/ventana/ueventd.ventana.rc
 * Otherwise, ioctl will get permission deny
 * -------------------------------------------Important--------------------------------------
*/

#define SENSOR_NAME	"t8ev5"
#define DEV(x)          "/dev/"x
#define SENSOR_PATH     DEV(SENSOR_NAME)
#define LOG_NAME(x)     "ImagerODM-"x
#define LOG_TAG         LOG_NAME(SENSOR_NAME)

#define SENSOR_WAIT_MS       0 /* special number to indicate this is wait time require */
#define SENSOR_TABLE_END     1 /* special number to indicate this is end of table */
#define WRITE_REG_DATA8      2 /* special the data width as one byte */
#define WRITE_REG_DATA16     3 /* special the data width as one byte */

#define SENSOR_MAX_RETRIES   3 /* max counter for retry I2C access */

#define SENSOR_IOCTL_SET_MODE		_IOW('o', 1, struct sensor_mode)
#define SENSOR_IOCTL_GET_STATUS		_IOR('o', 2, NvU8)
#define SENSOR_IOCTL_SET_COLOR_EFFECT   _IOW('o', 3, NvU8)
#define SENSOR_IOCTL_SET_YUV_EXPOSURE   _IOW('o', 4, NvU8)//add keenhi for yuv exposure
#define SENSOR_IOCTL_SET_WHITE_BALANCE  _IOW('o', 5, NvU8)
#define SENSOR_IOCTL_SET_SCENE_MODE     _IOW('o', 6, NvU8)
#define SENSOR_IOCTL_SET_GET_SHADING   _IO('o', 7)
#define SENSOR_IOCTL_SET_AF_MODE        _IOW('o', 8, NvU8)
#define SENSOR_IOCTL_GET_AF_STATUS      _IOW('o', 9, NvU8)
#define T8EV5_IOCTL_GET_CONFIG   _IOR('o', 0xa, struct t8ev5_config)
#define T8EV5_IOCTL_SET_POSITION _IOW('o', 0xb, NvU32)

#define T8EV5_IOCTL_SET_WINDOWS_POS		_IOW('o', 0xc, struct t8ev5_win_pos)
#define T8EV5_IOCTL_SET_CONTINUOUS_MODE		_IOW('o', 0xd, NvU8)
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
typedef enum
{
 NvOdmImagerT8ev5AfType_Infinity = 0,
 NvOdmImagerT8ev5AfType_Trigger,
 NvOdmImagerT8ev5AfType_Continuous,
 NvOdmImagerT8ev5AfType_MACRO,
 NvOdmImagerT8ev5AfType_Force8 = 0xff
} NvOdmImagerT8ev5AfType;

struct t8ev5_config
{
    NvU32 settle_time;
    NvU32 actuator_range;
    NvU32 pos_low;
    NvU32 pos_high;
	NvU32 focus_macro;
    NvU32 focus_infinity;
    float focal_length;
    float fnumber;
    float max_aperture;
};
struct t8ev5_win_pos {
	int left;
	int right;
	int top;
	int bottom;
};
struct rect_region {
	int min;
	int max;
};
typedef struct rect_regions_rec {
	struct rect_region  mRect_x;
	struct rect_region  mRect_y;
}rect_regions;
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

