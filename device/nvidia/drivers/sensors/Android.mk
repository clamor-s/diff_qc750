# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH:= $(call my-dir)

ifneq ($(TARGET_SIMULATOR),true)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libsensors.base
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := SensorBase.cpp SensorUtil.cpp InputEventReader.cpp
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/mllite
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/linux
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/include
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/include/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libdl libcutils libutils
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

ifneq (,$(filter $(BOARD_USES_INVENSENSE_GYRO),INVENSENSE_MPU3050 INVENSENSE_MPU6050))

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libsensors.mpl
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_CFLAGS += -std=gnu++0x
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/mllite
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/linux
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/include
LOCAL_C_INCLUDES += device/nvidia/drivers/sensors/mlsdk/platform/include/linux
LOCAL_SRC_FILES := MPLSensorSysApi.cpp MPLSensor.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libdl libsensors.base libakmd libmllite libmlplatform libmplmpu
LOCAL_CPPFLAGS+=-DLINUX=1
ifeq	($(BOARD_USES_INVENSENSE_GYRO),INVENSENSE_MPU3050)
LOCAL_CPPFLAGS+=-DPLATFORM_H=\"mpu3050.h\"
endif # INVENSENSE_MPU3050
ifeq	($(BOARD_USES_INVENSENSE_GYRO),INVENSENSE_MPU6050)
LOCAL_CPPFLAGS+=-DPLATFORM_H=\"mpu6050b1.h\"
endif # INVENSENSE_MPU6050
LOCAL_CPPFLAGS += -DMPL_LIB_NAME=\"libmplmpu.so\"
LOCAL_CPPFLAGS += -DAICHI_LIB_NAME=\"libami.so\"
LOCAL_CPPFLAGS += -DAKM_LIB_NAME=\"libakmd.so\"
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

endif # BOARD_USES_INVENSENSE_GYRO

include $(CLEAR_VARS)
LOCAL_MODULE := libsensors.adxl34x
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := adxl34x.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libsensors.isl29018
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := isl29018.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libsensors.isl29028
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := isl29028.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base libsensors.isl29018
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libsensors.ltr558als
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := ltr558als.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsensors.cm3217
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := cm3217.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsensors.hwmon
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := hwmon.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsensors.bma250
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := bma250.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsensors.dmard06
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := dmard06.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libsensors.base
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR
