# At this stage main makefiles, including product makefiles,
# have been read, so all major variables should be available.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# and the vold rule
PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab

include vendor/nvidia/build/kernel.mk
include vendor/nvidia/build/nv_targets.mk
