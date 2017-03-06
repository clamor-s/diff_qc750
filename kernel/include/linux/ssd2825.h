#ifndef _SSD2825_TS_H_
#define _SSD2825_TS_H_
#include <linux/spi/spi.h>
#include <linux/regulator/consumer.h>
struct ssd2828_data {
	void (*ssd_init_callback)(struct spi_device *spi);
	void (*ssd_deinit_callback)(struct spi_device *spi);
	struct ssd2828_device_platform_data *pdata;
	struct spi_device *spi;
	struct regulator *reg;
};

struct ssd2828_device_platform_data{
	int (*register_resume)(struct ssd2828_data *);
	int mipi_shutdown_gpio;
	int mipi_reset_gpio;
	int lcd_en_3v3_gpio;
	int lcd_vdds_en_gpio;
	int lcd_1v2_gpio;
};

#endif				
