
# Grab name of the makefile to depend on it
ifneq ($(PREV_LOCAL_PATH),$(LOCAL_PATH))
NVIDIA_MAKEFILE := $(lastword $(filter-out $(lastword $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
PREV_LOCAL_PATH := $(LOCAL_PATH)
endif
include $(CLEAR_VARS)

# Build variables common to all nvidia modules

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/openmax/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera-partner/imager/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/hwinc
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/camera
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/include

LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia/codecs/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia/tvmr/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/utils/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/nvmm/include
ifneq (,$(findstring core-private,$(LOCAL_PATH)))
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/drivers/hwinc
endif

ifneq (,$(findstring tests,$(LOCAL_PATH)))
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/include
endif

TEGRA_CFLAGS :=

ifeq ($(TARGET_BUILD_TYPE),debug)
LOCAL_CFLAGS += -DNV_DEBUG=1
# TODO: fix source that relies on these
LOCAL_CFLAGS += -DDEBUG
LOCAL_CFLAGS += -D_DEBUG
# disable all optimizations and enable gdb debugging extensions
LOCAL_CFLAGS += -O0 -ggdb
else
LOCAL_CFLAGS += -DNV_DEBUG=0
endif
LOCAL_CFLAGS += -DNV_IS_AVP=0
LOCAL_CFLAGS += -DNV_BUILD_STUBS=1
ifneq ($(filter ap20,$(TARGET_TEGRA_VERSION)),)
TEGRA_CFLAGS += -DCONFIG_PLLP_BASE_AS_408MHZ=0
else
TEGRA_CFLAGS += -DCONFIG_PLLP_BASE_AS_408MHZ=1
endif


# Define Trusted Foundations
ifeq ($(SECURE_OS_BUILD),y)
TEGRA_CFLAGS += -DCONFIG_TRUSTED_FOUNDATIONS
ifeq (,$(findstring tf.enable=y,$(ADDITIONAL_BUILD_PROPERTIES)))
ADDITIONAL_BUILD_PROPERTIES += tf.enable=y
endif
endif

ifeq ($(NV_EMBEDDED_BUILD),1)
TEGRA_CFLAGS += -DNV_EMBEDDED_BUILD
endif

ifdef PLATFORM_IS_JELLYBEAN
LOCAL_CFLAGS += -DPLATFORM_IS_JELLYBEAN=1
endif
ifdef PLATFORM_IS_JELLYBEAN_MR1
LOCAL_CFLAGS += -DPLATFORM_IS_JELLYBEAN_MR1=1
endif

ifeq ($(SECURE_OS_BUILD),y)
# Disallow all profiling and debug on both Android and Secure OS
TEGRA_CFLAGS += -DS_PROF_OFF_DBG_OFF_NS_PROF_OFF_DBG_OFF=0
# Only profiling on Android
TEGRA_CFLAGS += -DS_PROF_OFF_DBG_OFF_NS_PROF_ON_DBG_OFF=0
# Full profiling and debug on Android
TEGRA_CFLAGS += -DS_PROF_OFF_DBG_OFF_NS_PROF_ON_DBG_ON=1
# Full profiling and debug on both Android and Secure OS
TEGRA_CFLAGS += -DS_PROF_ON_DBG_ON_NS_PROF_ON_DBG_ON=0
else
# Only profiling on Android
TEGRA_CFLAGS += -DNS_PROF_ON_DBG_OFF=1
# Full profiling and debug on Android
TEGRA_CFLAGS += -DNS_PROF_ON_DBG_ON=0
endif

LOCAL_CFLAGS += $(TEGRA_CFLAGS)

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := optional

# clear nvidia local variables to defaults
NVIDIA_CLEARED := true
LOCAL_IDL_INCLUDES := $(TEGRA_TOP)/core/include
LOCAL_IDLFLAGS :=
LOCAL_NVIDIA_CGOPTS :=
LOCAL_NVIDIA_CGFRAGOPTS :=
LOCAL_NVIDIA_CGVERTOPTS :=
LOCAL_NVIDIA_INTERMEDIATES_DIR :=
LOCAL_NVIDIA_STUBS :=
LOCAL_NVIDIA_DISPATCHERS :=
LOCAL_NVIDIA_SHADERS :=
LOCAL_NVIDIA_GEN_SHADERS :=
LOCAL_NVIDIA_PKG :=
LOCAL_NVIDIA_PKG_DISPATCHER :=
LOCAL_NVIDIA_EXPORTS :=
LOCAL_NVIDIA_NO_COVERAGE :=
LOCAL_NVIDIA_NULL_COVERAGE :=
LOCAL_NVIDIA_NO_EXTRA_WARNINGS :=
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS :=
LOCAL_NVIDIA_RM_WARNING_FLAGS :=
LOCAL_NVIDIA_LINK_SCRIPT_PATH :=
LOCAL_NVIDIA_OBJCOPY_FLAGS :=
