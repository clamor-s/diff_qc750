
PRODUCT_ROOT= $(TOP)/3rdparty/trustedlogic/sdk/tegra3
LOCAL_PATH:= $(PRODUCT_ROOT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

ifneq (,$(filter libwvdrm_L1, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtee_client_api_driver.a
LOCAL_SRC_FILES:= \
        ../../samples/widevine/tegra3/src/oemcryptoL1.c
else
LOCAL_SRC_FILES:= \
        ../../samples/widevine/tegra3/src/oemcryptoL3.c
endif

LOCAL_CFLAGS += -D_BSD_SOURCE -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic

ifeq ($(TARGET_BUILD_TYPE),debug)
LOCAL_CFLAGS += -DDEBUG -D_DEBUG
LOCAL_CFLAGS += -O0 -g
$(info DEBUG)
else
LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -Os
LOCAL_CFLAGS += -Wl,-s
$(info RELEASE)
endif

LOCAL_MODULE:= liboemcrypto

LOCAL_C_INCLUDES := $(PRODUCT_ROOT)/tf_sdk/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/nvmm/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/utils/include

ifeq (,$(filter libwvdrm_L1, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
LOCAL_C_INCLUDES += $(TOP)/external/openssl/include
endif

include $(BUILD_STATIC_LIBRARY)
