LOCAL_PATH := $(call my-dir)
BOOTLOADER_PATHS :=
ifeq ($(BOARD_BUILD_BOOTLOADER),true)
BOOTLOADER_PATHS := fastboot
endif

include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, \
	fastboot \
	microboot \
	nv3p \
	nv3pserver \
	nvbct \
	nvpartmgr \
	nvstormgr \
	nvaboot \
	nvbootupdate \
	nvcrypto \
	utils \
	nvfsmgr \
	nvfs \
	nvdioconverter \
	nvflash \
	nvdiagnostics \
	nvbuildbct \
	nvsku \
))
