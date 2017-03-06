LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_TEGRA_VERSION),t30)
    _tegra_chip=tegra3
endif
ifeq ($(TARGET_TEGRA_VERSION),ap20)
    _tegra_chip=tegra2
endif

ifneq ($(_tegra_chip),)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := tf_daemon
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(_tegra_chip)/os_porting_kit/android_porting/daemon/build/release/tf_daemon

include $(NVIDIA_PREBUILT)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libsmapi
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(_tegra_chip)/os_porting_kit/android_porting/tfapi/build/release/libsmapi.so

include $(NVIDIA_PREBUILT)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libtf_crypto_sst
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(_tegra_chip)/os_porting_kit/android_porting/tf_crypto_sst/build/release/libtf_crypto_sst.so

include $(NVIDIA_PREBUILT)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libtee_client_api_driver
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(_tegra_chip)/os_porting_kit/android_porting/tee_client_api/build/release/libtee_client_api_driver.a

include $(NVIDIA_PREBUILT)

endif
_tegra_chip :=

# Notify developers of the need to clean up if switching from
# a SecureOS build to a non-SecureOS build.
# TODO: Remove once we only support SecureOS builds.

CURRENT_SECURE_OS_BUILD := undefined

# This will be generated once.
$(PRODUCT_OUT)/current_secure_os_build_type.mk:
	$(hide) mkdir -p $(dir $@)
	$(hide) echo "CURRENT_SECURE_OS_BUILD := $(SECURE_OS_BUILD)" >$@

# As an exception, add a minus in order not to issue a warning if
# the included file does not exist.  It does not exist during the first
# parsing of the makefile.  The exception is OK as we are not relying
# on this dependency, it is just a convenience.
-include $(PRODUCT_OUT)/current_secure_os_build_type.mk

# $(SECURE_OS_BUILD) is from environment.
# $(CURRENT_SECURE_OS_BUILD) is from the first build after a clean.
# If the build type changes after the clean, we need to fail.
ifneq ($(CURRENT_SECURE_OS_BUILD),undefined)
    ifneq ($(CURRENT_SECURE_OS_BUILD),$(SECURE_OS_BUILD))
        $(error The SecureOS build has been enabled/disabled, you must run "make clean" first.)
    endif
endif
