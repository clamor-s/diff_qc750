
$(info "Building example_mtc...")

LOCAL_PATH:= $(PRODUCT_ROOT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	tf_sdk/examples/example_mtc/src/example_mtc.c

LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtf_crypto_sst.so

LOCAL_CFLAGS += -D_BSD_SOURCE -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic

ifeq ($(BUILD_VARIANT),debug)
LOCAL_CFLAGS += -DDEBUG -D_DEBUG
LOCAL_CFLAGS += -O0 -g
$(info DEBUG)
else
LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -Os
LOCAL_CFLAGS += -Wl,-s
$(info RELEASE)
endif

LOCAL_MODULE:= example_mtc

LOCAL_CFLAGS += -I$(PRODUCT_ROOT)/tf_sdk/include

include $(BUILD_EXECUTABLE)
