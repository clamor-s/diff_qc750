# DO NOT add conditionals to this makefile of the form
#
#    ifeq ($(TARGET_TEGRA_VERSION),<latest SOC>)
#        <stuff for latest SOC>
#    endif
#
# Such conditionals break forward compatibility with future SOCs.
# If you must add conditionals to this makefile, use the form
#
#    ifneq ($(filter <list of older SOCs>,$(TARGET_TEGRA_VERSION)),)
#       <stuff for old SOCs>
#    else
#       <stuff for new SOCs>
#    endif

include $(CLEAR_VARS)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := nvidia_tegra_icera_common_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := libril-icera\
                          IceraDebugAgent\
                          Stk \
                          icera_log_serial_arm 

include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := nvidia_tegra_icera_phone_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := fild \
                          agpsd

include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := nvidia_tegra_icera_tablet_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := icera-switcherd \
                          DatacallWhitelister \
                          downloader \
                          icera-loader

include $(BUILD_PHONY_PACKAGE)
