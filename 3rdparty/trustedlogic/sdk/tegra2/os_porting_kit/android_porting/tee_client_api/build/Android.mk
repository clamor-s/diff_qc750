
$(info "Building TEE client API...")

LOCAL_PATH:= $(PRODUCT_ROOT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false 
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= os_porting_kit/android_porting/tee_client_api/src/tee_client_api_linux_driver.c

LOCAL_CFLAGS += -O2 -W -Wsign-compare -Wstrict-prototypes -fno-strict-aliasing -march=armv5te -mtune=arm9tdmi -mlittle-endian -fpic -fvisibility=hidden -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic

ifdef S_VERSION_BUILD
LOCAL_CFLAGS += -DS_VERSION_BUILD=$(S_VERSION_BUILD)
endif

ifeq ($(BUILD_VARIANT),debug)
LOCAL_CFLAGS += -DDEBUG -D_DEBUG
LOCAL_CFLAGS += -O0 -g
$(info DEBUG)
endif
ifeq ($(BUILD_VARIANT),release)
LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -Os
LOCAL_CFLAGS += -Wl,-s
$(info RELEASE)
endif
ifeq ($(BUILD_VARIANT),benchmark)
LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -Os
LOCAL_CFLAGS += -Wl,-s
$(info BENCHMARK)
endif

# includes
LOCAL_CFLAGS += -I$(PRODUCT_ROOT)/os_porting_kit/android_porting/tee_client_api/include/

LOCAL_MODULE:= libtee_client_api_driver

include $(BUILD_STATIC_LIBRARY)
