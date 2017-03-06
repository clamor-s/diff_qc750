
LOCAL_PATH := $(call my-dir)


ifneq ($(wildcard device/keenhi/NabiSetup),device/keenhi/NabiSetup)
include $(CLEAR_VARS)
LOCAL_MODULE := NabiSetup
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/NabiSetup.apk
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard device/keenhi/KitService2),device/keenhi/KitService2)
include $(CLEAR_VARS)
LOCAL_MODULE := KitService2
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/KitService2.apk
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := CoreService
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/CoreService.apk
include $(BUILD_PREBUILT)

ifneq ($(wildcard device/keenhi/GsensorCalibration),device/keenhi/GsensorCalibration)
include $(CLEAR_VARS)
LOCAL_MODULE := GSensorCal
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/GSensorCal.apk
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := km
LOCAL_MODULE_SUFFIX := .jar
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/framework/
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
#LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./framework/km.jar
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libdmard06_cal_jni
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libdmard06_cal_jni.so
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard vendor/nvidia/tegra/camera-partner/android_myself/libnvomxcamera),vendor/nvidia/tegra/camera-partner/android_myself/libnvomxcamera)

include $(CLEAR_VARS)
LOCAL_MODULE := camera.tegra
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib/hw
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/camera.tegra.so
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard device/nvidia/keenhi/ProductTest),device/nvidia/keenhi/ProductTest)
include $(CLEAR_VARS)
LOCAL_MODULE := ProductTest
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/ProductTest.apk
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libgsensor_cal_jni
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libgsensor_cal_jni.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libradflash
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libradflash.so
include $(BUILD_PREBUILT)
endif

ifneq (,$(filter $(TARGET_PRODUCT),itq700))
include $(CLEAR_VARS)
LOCAL_MODULE := libjni_koreanime
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libjni_koreanime.so
include $(BUILD_PREBUILT)
endif

ifneq (,$(filter $(TARGET_PRODUCT),itq701))
include $(CLEAR_VARS)
LOCAL_MODULE := libmozc
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libmozc.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libjni_koreanime
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libjni_koreanime.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libjni_googlepinyinime_5
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libjni_googlepinyinime_5.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libjni_googlepinyinime_latinime_5
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES :=  ./lib/libjni_googlepinyinime_latinime_5.so
include $(BUILD_PREBUILT)

endif

ifneq ($(wildcard device/nvidia/keenhi/KitModeFilter),device/nvidia/keenhi/KitModeFilter)
include $(CLEAR_VARS)
LOCAL_MODULE := nabi2_core
LOCAL_MODULE_SUFFIX := .jar
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/framework/
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
#LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./framework/nabi2_core.jar
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard device/keenhi/SystemUpdate),device/keenhi/SystemUpdate)
include $(CLEAR_VARS)
LOCAL_MODULE := SystemUpdate
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/SystemUpdate.apk
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard device/keenhi/CheckOtaUpdate),device/keenhi/CheckOtaUpdate)
include $(CLEAR_VARS)
LOCAL_MODULE := CheckOtaUpdate
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/CheckOtaUpdate.apk
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := HardwareVersionManager
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/HardwareVersionManager.apk
include $(BUILD_PREBUILT)

ifneq ($(wildcard device/keenhi/OtaPackageUpdate),device/keenhi/OtaPackageUpdate)
include $(CLEAR_VARS)
LOCAL_MODULE := OtaPackageUpdate
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/OtaPackageUpdate.apk
include $(BUILD_PREBUILT)
endif

ifneq ($(wildcard device/keenhi/A12Server),device/keenhi/A12Server)
include $(CLEAR_VARS)
LOCAL_MODULE := A12Server
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/A12Server.apk
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := Maps
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/Maps.apk
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := App2Sd
LOCAL_MODULE_SUFFIX := .apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES :=  ./app/App2Sd.apk
include $(BUILD_PREBUILT)


ifeq ($(TARGET_PRODUCT),qc750)
include device/keenhi/addon/google/Android.mk
include device/keenhi/addon/qc750apps/Android.mk
endif




