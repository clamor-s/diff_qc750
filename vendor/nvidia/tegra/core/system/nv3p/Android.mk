LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnv3p
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0
LOCAL_CFLAGS += -Wno-error=uninitialized
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot

LOCAL_SRC_FILES += nv3p.c
LOCAL_SRC_FILES += nvml_usbf_ap20.c
LOCAL_SRC_FILES += nvml_usbf_t30.c
LOCAL_SRC_FILES += nv3p_transport_device.c
LOCAL_SRC_FILES += nvboot_misc_ap20.c
LOCAL_SRC_FILES += nvboot_misc_t30.c
LOCAL_SRC_FILES += nv3p_transport_usb_descriptors.c

LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnv3p_avp

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot

# Look for string.h in bionic libc include directory, in case we
# are using an Android prebuilt tool chain.  string.h recursively
# includes malloc.h, which would cause a declaration conflict with NvOS,
# so add a define in order to avoid recursive inclusion.
ifeq ($(AVP_EXTERNAL_TOOLCHAIN),)
  LOCAL_C_INCLUDES += $(TOP)/bionic/libc/include
  LOCAL_CFLAGS += -D_MALLOC_H_
endif

LOCAL_SRC_FILES += nv3p.c
LOCAL_SRC_FILES += nvml_usbf_ap20.c
LOCAL_SRC_FILES += nvml_usbf_t30.c
LOCAL_SRC_FILES += nv3p_transport_device.c
LOCAL_SRC_FILES += nvboot_misc_ap20.c
LOCAL_SRC_FILES += nvboot_misc_t30.c
LOCAL_SRC_FILES += nv3p_transport_usb_descriptors.c

LOCAL_CFLAGS += -Os -ggdb0
LOCAL_CFLAGS += -DNVBOOT_SI_ONLY=1
LOCAL_CFLAGS += -DNVBOOT_TARGET_FPGA=0
LOCAL_CFLAGS += -DNVBOOT_TARGET_RTL=0

include $(NVIDIA_STATIC_AVP_LIBRARY)


include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnv3p

LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot

LOCAL_SRC_FILES += nv3p.c
LOCAL_SRC_FILES += nv3p_transport_usb_host.c
LOCAL_SRC_FILES += nv3p_transport_sema_host.c
LOCAL_SRC_FILES += nv3p_transport_host.c

include $(NVIDIA_HOST_STATIC_LIBRARY)
