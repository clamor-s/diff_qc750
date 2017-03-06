
$(info "Building libtf_crypto_sst...")

LOCAL_PATH:= $(PRODUCT_ROOT)/os_porting_kit/android_porting/tf_crypto_sst

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false 
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
   src/lib_object.c \
   src/lib_mutex_linux.c \
   src/sst_stub.c \
   src/mtc.c \
   src/pkcs11_global.c \
   src/pkcs11_object.c \
   src/pkcs11_session.c


LOCAL_LDFLAGS += $(PRODUCT_ROOT)/os_porting_kit/android_porting/tee_client_api/build/$(BUILD_VARIANT)/libtee_client_api_driver.a


LOCAL_CFLAGS += -DTRUSTZONE -DLINUX \
                 -Wall -Wundef -W -Wsign-compare -Wstrict-prototypes -fno-strict-aliasing \
                -march=armv5te -mtune=arm9tdmi -mlittle-endian -fpic \
                -fvisibility=hidden \
                -Wl,--no-undefined

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
LOCAL_CFLAGS += -O2
LOCAL_CFLAGS += -s
$(info RELEASE)
endif

# includes
LOCAL_CFLAGS += -I$(PRODUCT_ROOT)/os_porting_kit/android_porting/tf_crypto_sst/include/
LOCAL_CFLAGS += -I$(TEE_API_INC_DIR)/

LOCAL_MODULE:= libtf_crypto_sst

include $(BUILD_SHARED_LIBRARY)
