
$(info "Building libsmapi...")

LOCAL_PATH:= $(PRODUCT_ROOT)/os_porting_kit/android_porting/tfapi

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false 
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
   src/lib_bef2_decoder.c \
   src/lib_bef2_encoder.c \
   src/lib_object.c \
   src/smapi.c \
   src/smx_encoding.c \
   src/smx_heap.c \
   src/smx_manager.c \
   src/smx_stub.c \
   src/smx_utils.c \
   src/smx_linux.c

LOCAL_LDFLAGS += $(PRODUCT_ROOT)/os_porting_kit/android_porting/tee_client_api/build/$(BUILD_VARIANT)/libtee_client_api_driver.a

LOCAL_CFLAGS += -DWCHAR_SUPPORTED -O2 -W -Wsign-compare -Wstrict-prototypes -fno-strict-aliasing -march=armv5te -mtune=arm9tdmi -mlittle-endian -fpic -fvisibility=hidden -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic


ifdef S_VERSION_BUILD
LOCAL_CFLAGS += -DS_VERSION_BUILD=$(S_VERSION_BUILD)
endif

ifeq ($(BUILD_VARIANT),debug)
LOCAL_CFLAGS += -DDEBUG -D_DEBUG
LOCAL_CFLAGS += -O0 -g
LOCAL_LDLIBS := -llog
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
LOCAL_CFLAGS += -I $(PRODUCT_ROOT)/os_porting_kit/android_porting/tfapi/include/
LOCAL_CFLAGS += -I$(TEE_API_INC_DIR)/

LOCAL_MODULE:= libsmapi

include $(BUILD_SHARED_LIBRARY)
