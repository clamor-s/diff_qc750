LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvomx

LOCAL_CFLAGS += -D__OMX_EXPORTS
LOCAL_CFLAGS += -DOMXVERSION=2
LOCAL_CFLAGS += -DUSE_NVOS_AND_NVTYPES
LOCAL_CFLAGS += -DUSE_ANDROID_NATIVE_WINDOW=1
LOCAL_CFLAGS += -DUSE_ANDROID_CAMERA_PREVIEW
LOCAL_CFLAGS += -DUSE_FULL_FRAME_MODE=1
ifeq ($(NV_ANDROID_FRAMEWORK_ENHANCEMENTS),TRUE)
LOCAL_CFLAGS += -DUSE_NV_ANDROID_FRAMEWORK_ENHANCEMENTS=1
else
LOCAL_CFLAGS += -DUSE_NV_ANDROID_FRAMEWORK_ENHANCEMENTS=0
endif

LOCAL_C_INCLUDES += frameworks/native/include/media/hardware
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/il
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/utils/camerautil
LOCAL_C_INCLUDES += $(LOCAL_PATH)/components/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/components
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nvmm
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nvmm/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nvmm/components/common
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics-partner/android/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/android/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/egl/include/interface
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/egl/include/nvidia
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/winutil/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/2d/include

LOCAL_SRC_FILES += core/NvxCore.c
LOCAL_SRC_FILES += components/common/NvxComponent.c
LOCAL_SRC_FILES += components/common/NvxResourceManager.c
LOCAL_SRC_FILES += components/common/NvxPort.c
LOCAL_SRC_FILES += components/common/simplecomponentbase.c
LOCAL_SRC_FILES += components/NvxClockComponent.c
LOCAL_SRC_FILES += components/NvxMp4FileWriter.c
LOCAL_SRC_FILES += components/NvxVideoScheduler.c
LOCAL_SRC_FILES += components/NvxRawFileReader.c
LOCAL_SRC_FILES += components/NvxRawFileWriter.c
LOCAL_SRC_FILES += common/NvxTrace.c
LOCAL_SRC_FILES += common/NvxMutex.c
LOCAL_SRC_FILES += common/nvxjitter.c
LOCAL_SRC_FILES += common/nvxlist.c
LOCAL_SRC_FILES += components/common/NvxScheduler.c
LOCAL_SRC_FILES += nvmm/components/common/NvxCompReg.c
LOCAL_SRC_FILES += nvmm/components/common/nvmmtransformbase.c
LOCAL_SRC_FILES += nvmm/components/common/NvxCompMisc.c
LOCAL_SRC_FILES += nvmm/common/NvxHelpers.c
LOCAL_SRC_FILES += nvmm/components/nvxaudiodecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxbypassdecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxvideodecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxvideorenderer.c
LOCAL_SRC_FILES += nvmm/components/nvxloopbackrenderer.c
#LOCAL_SRC_FILES += nvmm/components/nvxaudiorenderer_audioflinger.cpp
LOCAL_SRC_FILES += nvmm/components/nvxaudiorenderer.c
LOCAL_SRC_FILES += nvmm/components/nvxcamera.c
#LOCAL_SRC_FILES += nvmm/components/nvxaudiocapturer.c
LOCAL_SRC_FILES += nvmm/components/nvximageencoder.c
LOCAL_SRC_FILES += nvmm/components/nvximagedecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxaudioencoder.c
LOCAL_SRC_FILES += nvmm/components/nvxvideoextractor.c
LOCAL_SRC_FILES += nvmm/common/nvxcrccheck.c
LOCAL_SRC_FILES += nvmm/components/nvxparser.c
LOCAL_SRC_FILES += nvmm/common/nvxeglimage.c
LOCAL_SRC_FILES += nvmm/common/nvxegl.c
LOCAL_SRC_FILES += nvmm/common/nvxwinmanagerstub.c
LOCAL_SRC_FILES += nvmm/common/nvxhelpers_anw.cpp
LOCAL_SRC_FILES += nvmm/common/nvxhelpers_camera.cpp
LOCAL_SRC_FILES += nvmm/common/nvxandroidbuffer.cpp
LOCAL_SRC_FILES += libomxil.c
LOCAL_SRC_FILES += nvmm/components/nvxliteaudioencoder.c
LOCAL_SRC_FILES += nvmm/components/nvxliteimageencoder.c
LOCAL_SRC_FILES += nvmm/components/nvxliteaudiodecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxlitevideodecoder.c
LOCAL_SRC_FILES += nvmm/components/nvxlitevideoencoder.c
LOCAL_SRC_FILES += nvmm/components/common/nvmmlitetransformbase.c
LOCAL_SRC_FILES += nvmm/common/nvxliteandroidbuffer.cpp
LOCAL_SRC_FILES += nvmm/components/common/nvxlitechooser.c
LOCAL_SRC_FILES += nvmm/components/common/nvmmlitevideo_profile_level.c
LOCAL_SRC_FILES += nvmm/components/common/nvxaudiocapability.c
LOCAL_SRC_FILES += nvmm/components/nvxliteimagedecoder.c

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvmm
LOCAL_SHARED_LIBRARIES += libnvmm_utils
LOCAL_SHARED_LIBRARIES += libnvddk_2d_v2
LOCAL_SHARED_LIBRARIES += libnvodm_imager
LOCAL_SHARED_LIBRARIES += libnvmm_contentpipe
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_SHARED_LIBRARIES += libnvwinsys
LOCAL_SHARED_LIBRARIES += libnvmmlite
LOCAL_SHARED_LIBRARIES += libnvmmlite_utils

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    libui \
    libhardware \
    libbinder \
    libmedia

LOCAL_STATIC_LIBRARIES += libnvfxmath
LOCAL_STATIC_LIBRARIES += libmd5
LOCAL_STATIC_LIBRARIES += libnvcamerautil

LOCAL_SHARED_LIBRARIES += libnvtvmr

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_SHARED_LIBRARY)

