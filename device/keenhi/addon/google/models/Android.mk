#
# Copyright (C) 2011 Google Inc.
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
#

LOCAL_PATH := $(call my-dir)

# ==== FaceLock models ========================

include $(CLEAR_VARS)

LOCAL_MODULE := detection/multi_pose_face_landmark_detectors.7/left_eye-y0-yi45-p0-pi45-r0-ri20.lg_32/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================


include $(CLEAR_VARS)

LOCAL_MODULE := detection/multi_pose_face_landmark_detectors.7/nose_base-y0-yi45-p0-pi45-r0-ri20.lg_32/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := detection/multi_pose_face_landmark_detectors.7/right_eye-y0-yi45-p0-pi45-r0-ri20.lg_32-2/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := detection/yaw_roll_face_detectors.6/head-y0-yi45-p0-pi45-r0-ri30.4a-v24/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := detection/yaw_roll_face_detectors.6/head-y0-yi45-p0-pi45-rn30-ri30.5-v24/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := detection/yaw_roll_face_detectors.6/head-y0-yi45-p0-pi45-rp30-ri30.5-v24/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := recognition/face.face.y0-y0-22-b-N/full_model.bin

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/vendor/pittpatt/models
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/pittpatt/models
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


# ==== VoiceSearch models ========================

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/metadata
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/acoustic_model
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/c_fst
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/clg
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/compile_grammar.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/contacts.abnf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/dict
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/dictation.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/embed_phone_nn_model
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/embed_phone_nn_state_sym
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/endpointer_voicesearch.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/endpointer_dictation.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/ep_acoustic_model
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/g2p_fst
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/google_hotword.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/google_hotword_clg
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/google_hotword_logistic
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/hotword_symbols
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/grammar.config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/hmmsyms
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/lintrans_model
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/normalizer
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/norm_fst
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/offensive_word_normalizer
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/phonelist
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/rescoring_lm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================
include $(CLEAR_VARS)

LOCAL_MODULE := en-US/symbols
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/usr/srec
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/srec
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# =============================================

