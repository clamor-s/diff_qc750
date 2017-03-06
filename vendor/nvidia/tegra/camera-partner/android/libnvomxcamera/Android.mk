BOARD_VENDOR_USE_NV_CAMERA := false
ifneq ($(BOARD_VENDOR_USE_NV_CAMERA), false)

TEGRA_CAMERA_TYPE := openmax

ifeq ($(TEGRA_CAMERA_TYPE),openmax)

LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

# this is used to enable/disable the render bypass code path for HC
LOCAL_CFLAGS += -DUSE_RENDER_BYPASS=1

# this is used to enable/disable the nVidia face detection
LOCAL_CFLAGS += -DUSE_NV_FD=1

# this is used to enable/disable the nVidia HDR processing
ENABLE_NVIDIA_HDR := true

# HDR is not supported on ap20
ifeq ($(TARGET_TEGRA_VERSION), ap20)
ENABLE_NVIDIA_HDR := false
endif

ifeq ($(TARGET_PRODUCT),qc750)
LOCAL_CFLAGS += -DBUILD_FOR_QC750=1
endif
ifeq ($(TARGET_PRODUCT),n710)
LOCAL_CFLAGS += -DBUILD_FOR_N710=1
endif
ifeq ($(TARGET_PRODUCT),itq700)
LOCAL_CFLAGS += -DBUILD_FOR_N710=1
endif
ifeq ($(TARGET_PRODUCT),birch)
LOCAL_CFLAGS += -DBUILD_FOR_N750=1
endif
ifeq ($(TARGET_PRODUCT),n750)
LOCAL_CFLAGS += -DBUILD_FOR_N750=1
endif
ifeq ($(TARGET_PRODUCT),nabi2_xd)
LOCAL_CFLAGS += -DBUILD_FOR_NABI2_XD=1
endif
ifeq ($(TARGET_PRODUCT),n1010)
LOCAL_CFLAGS += -DBUILD_FOR_N1010=1
ifeq ($(TARGET_PRODUCT),ns_14t004)
LOCAL_CFLAGS += -DBUILD_FOR_NS_14T004=1
endif
endif
ifeq ($(TARGET_PRODUCT),wikipad)
LOCAL_CFLAGS += -DBUILD_FOR_N710=1
endif

ifeq ($(ENABLE_NVIDIA_HDR), true)
    LOCAL_CFLAGS += -DENABLE_NVIDIA_HDR=1
else
    LOCAL_CFLAGS += -DENABLE_NVIDIA_HDR=0
endif

ifneq ($(TARGET_TEGRA_VERSION), ap20)
LOCAL_CFLAGS += -DUSE_HW_ENCODER=1
LOCAL_CFLAGS += -DEARLY_CONNECT_GRAPH_ENABLED=1
else
LOCAL_CFLAGS += -DUSE_HW_ENCODER=0
LOCAL_CFLAGS += -DEARLY_CONNECT_GRAPH_ENABLED=0
endif

ifeq ($(BOARD_CAMERA_PREVIEW_HDMI_ONLY), true)
  ifneq ($(BOARD_HAVE_VID_ROUTING_TO_HDMI), true)
    $(error BOARD_HAVE_VID_ROUTING_TO_HDMI must be set to true in order for \
            BOARD_CAMERA_PREVIEW_HDMI_ONLY to take effect)
  endif
  LOCAL_CFLAGS += -DNV_GRALLOC_USAGE_FLAGS_CAMERA=GRALLOC_USAGE_EXTERNAL_DISP
endif

# We really want to get things clean enough that we can check
# this in with -Werror on.
LOCAL_CFLAGS += -Werror

LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += nvomxcamera.cpp
LOCAL_SRC_FILES += nvomxcamerasettings.cpp
LOCAL_SRC_FILES += nvomxcamerasettingsparser.cpp
LOCAL_SRC_FILES += nvomxcameracallbacks.cpp
LOCAL_SRC_FILES += nvomxcameraquanttables.cpp
LOCAL_SRC_FILES += custcamerasettingsdefinition.cpp
LOCAL_SRC_FILES += nvxwrappers.cpp
LOCAL_SRC_FILES += nvomxcamerabuffers.cpp
LOCAL_SRC_FILES += nvomxcamerathumbnails.cpp
LOCAL_SRC_FILES += nvomxparseconfig.cpp
LOCAL_SRC_FILES += nvomxcameraencoderqueue.cpp
#LOCAL_SRC_FILES += nvomxcamerayuvfilterexample.cpp
LOCAL_SRC_FILES += nvimagescaler.cpp
LOCAL_SRC_FILES += nvomxcameracustpostprocess.cpp
LOCAL_SRC_FILES += nvsensorlistener.cpp
ifeq ($(ENABLE_NVIDIA_HDR), true)
LOCAL_SRC_FILES += nvomxpostprocessingfilter.cpp
endif

LOCAL_SRC_FILES += nvomxcameracustvstab.cpp

LOCAL_SHARED_LIBRARIES := \
	libbinder \
	libcamera_client   \
	libcutils \
	libgui \
	libhardware_legacy \
	libjpeg \
	libmedia \
	libnvcamerahdr \
	libnvddk_2d_v2 \
	libnvmm_utils \
	libnvomxilclient \
	libnvos \
	libnvrm \
	libnvtvmr \
	libui \
	libutils

ifeq ($(ENABLE_NVIDIA_HDR), true)
LOCAL_SHARED_LIBRARIES +=	libnvcamerahdr
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/camera/alg/hdr
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/il
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/ilclient
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include/openmax/il
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include/openmax/ilclient
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/2d/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/../../../external/jpeg
LOCAL_C_INCLUDES += frameworks/native/include/media/hardware

# HAL module implemenation stored in
# hw/camera.tegra.so
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef -Wcast-align
LOCAL_CFLAGS += -Wno-error=conversion-null
LOCAL_CFLAGS += -Wno-error=switch
include $(NVIDIA_SHARED_LIBRARY)

endif
endif
