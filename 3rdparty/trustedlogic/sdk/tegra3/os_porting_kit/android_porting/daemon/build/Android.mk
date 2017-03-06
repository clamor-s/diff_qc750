
$(info "Building tf_daemon...")

LOCAL_PATH:= $(PRODUCT_ROOT)/os_porting_kit/android_porting/daemon

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
#LOCAL_PRELINK_MODULE := false 
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
   src/delegation_client.c


LOCAL_LDFLAGS += $(PRODUCT_ROOT)/os_porting_kit/android_porting/tee_client_api/build/$(BUILD_VARIANT)/libtee_client_api_driver.a


LOCAL_CFLAGS += -D_BSD_SOURCE -D_POSIX_SOURCE -DTRUSTZONE -DLINUX \
                -Wall -Wundef -W -Wsign-compare -Wstrict-prototypes -fno-strict-aliasing \
                -march=armv5te -mtune=arm9tdmi -mlittle-endian
                
ifeq ($(BUILD_VARIANT),debug)
LOCAL_CFLAGS += -DDEBUG -D_DEBUG
LOCAL_CFLAGS += -O0 -g
$(info DEBUG)
endif
ifeq ($(BUILD_VARIANT),release)
LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -Os
$(info RELEASE)
endif

ifdef S_VERSION_BUILD
LOCAL_CFLAGS += -DS_VERSION_BUILD=$(S_VERSION_BUILD)
endif

# includes
LOCAL_CFLAGS += -I$(PRODUCT_ROOT)/os_porting_kit/android_porting/tf_crypto_sst/src/
LOCAL_CFLAGS += -I$(TEE_API_INC_DIR)/

LOCAL_MODULE:= tf_daemon

include $(BUILD_EXECUTABLE)

