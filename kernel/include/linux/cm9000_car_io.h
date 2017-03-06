#ifndef ___CAR_VEHICLE__CCM9000
#define ___CAR_VEHICLE__CCM9000

#define HB_DEV_NAME	"handbrake"
#define DC_DEV_NAME	"dock"
#define CM9000_CAR_DEV_NAME "Cm9000CarIo"
struct plat_vehicle_port {
	unsigned int	handbrake_gpio;		/* handbrake gpio number */
	unsigned int	dock_gpio;		/*dock gpio number */
};


#endif  /* ___CAR_VEHICLE__CCM9000 */

