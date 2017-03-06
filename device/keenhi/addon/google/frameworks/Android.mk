#
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
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  # See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

# Maps library
# ============================================================
include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := libmaps

LOCAL_MODULE := com.google.android.maps.jar

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := JAVA_LIBRARIES

# This will install the file in /system/framework
LOCAL_MODULE_PATH := $(TARGET_OUT_JAVA_LIBRARIES)

LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_REQUIRED_MODULES := com.google.android.maps.xml

include $(BUILD_PREBUILT)

# ====  permissions ========================
include $(CLEAR_VARS)

LOCAL_MODULE := com.google.android.maps.xml

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/etc/permissions
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)

# Media effects library
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := com.google.android.media.effects.jar

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := JAVA_LIBRARIES

# This will install the file in /system/framework
LOCAL_MODULE_PATH := $(TARGET_OUT_JAVA_LIBRARIES)

LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_REQUIRED_MODULES := \
    com.google.android.media.effects.xml \
    LMprec_508.emd \
    PFFprec_600.emd \
    libfrsdk \
    libfilterpack_facedetect

include $(BUILD_PREBUILT)

# ====  permissions ========================
include $(CLEAR_VARS)

LOCAL_MODULE := com.google.android.media.effects.xml

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/etc/permissions
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)
