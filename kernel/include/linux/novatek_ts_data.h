#ifndef __TOUCHSCREEN_NOVATEK_TS_H__
#define __TOUCHSCREEN_NOVATEK_TS_H__

struct novatek_i2c_platform_data {
	uint32_t version;	/* Use this entry for panels with */
	int (*detect)(void);
	int(*detect1)(void);
	//reservation
};

#endif