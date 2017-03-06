LOCAL_PATH := $(call my-dir)
include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, \
	aes \
	blockdev \
	disp \
	fuses \
	uart \
	dap \
	i2s \
	kbc \
	nand \
	sdio \
	snor \
	se \
	spdif \
	spi_flash \
))

