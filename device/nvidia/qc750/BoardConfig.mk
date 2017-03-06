TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := t30

# CPU options
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true

# Skip droiddoc build to save build time
BOARD_SKIP_ANDROID_DOC_BUILD := true

BOARD_BUILD_BOOTLOADER := true

ifeq ($(NO_ROOT_DEVICE),1)
  TARGET_PROVIDES_INIT_RC := true
else
  TARGET_PROVIDES_INIT_RC := false
endif

BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
BOARD_SUPPORT_NVOICE := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 805306368
BOARD_FLASH_BLOCK_SIZE := 4096

BOARD_ADDON_PARTITION_SIZE := 230686720

USE_E2FSPROGS := true
USE_OPENGL_RENDERER := true

# OTA
TARGET_RECOVERY_UPDATER_LIBS += libnvrecoveryupdater

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_TI := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/nvidia/qc750/bluetooth

#BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_BLUETOOTH_DEFAULT_NAME := $(PRODUCT_MODEL)

USE_CAMERA_STUB := false

# mediaplayer
BOARD_USES_HW_MEDIAPLUGINS := false
BOARD_USES_HW_MEDIASCANNER := false
BOARD_USES_HW_MEDIARECORDER := false

# Wifi related defines
BOARD_WLAN_DEVICE           := wl12xx_mac80211
BOARD_SOFTAP_DEVICE         := wl12xx_mac80211
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_TI_0_8_X
BOARD_HOSTAPD_DRIVER        := NL80211
WIFI_DRIVER_MODULE_NAME     := "wl12xx_sdio"
WIFI_FIRMWARE_LOADER        := ""

# Invensense MPU
#BOARD_USES_INVENSENSE_GYRO := INVENSENSE_MPU6050

# GPS
BOARD_HAVE_TI_GPS := false

# Default HDMI mirror mode
# Crop (default) picks closest mode, crops to screen resolution
# Scale picks closest mode, scales to screen resolution (aspect preserved)
# Center picks a mode greater than or equal to the panel size and centers;
#     if no suitable mode is available, reverts to scale
BOARD_HDMI_MIRROR_MODE := Scale

# This should be enabled if you wish to use information from hwcomposer to enable
# or disable DIDIM during run-time.
BOARD_HAS_DIDIM := true

# This should be set to true for boards that support 3DVision.
BOARD_HAS_3DV_SUPPORT := true

SECURE_OS_BUILD ?= y

# Double buffered display surfaces reduce memory usage, but will decrease performance.
# The default is to triple buffer the display surfaces.
# BOARD_DISABLE_TRIPLE_BUFFERED_DISPLAY_SURFACES := true

BOARD_ROOT_DEVICE := emmc
include vendor/nvidia/build/definitions.mk

-include 3rdparty/trustedlogic/samples/hdcp/tegra3/build/arm_android/config.mk
KEENHI_BOOTLOADER_USE_VOLUME_UP_AS_ENTER := true

BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3

ifneq (,$(filter true, $(BULID_ROOT_PERMISSTION)))
WITH_DEXPREOPT := false
DISABLE_DEXPREOPT :=true
else
#WITH_DEXPREOPT := false
#DISABLE_DEXPREOPT :=true
endif

CREATE_MODULE_INFO_FILE := true

RELEASE_KEY_RELEASE := device/nvidia/keenhi/qc750_keys/releasekey
RELEASE_KEY_PLATFORM := device/nvidia/keenhi/qc750_keys/platform
RELEASE_KEY_SHARED := device/nvidia/keenhi/qc750_keys/shared
RELEASE_KEY_MEDIA := device/nvidia/keenhi/qc750_keys/media

KEENHI_OTA_KEY := device/nvidia/keenhi/qc750_keys/releasekey

#BOARD_INCLUDES_TEGRA_JNI := display 

#BUILD_NUMBER := $(shell date +%Y%m%d%H)
BUILD_NUMBER := ru
#BUILD_NUMBER := TEST

KEENHI_OTA_UPDATE_INCREMENTAL_FROM_VERSION := 1.1.6
KEENHI_OTA_INCREMENTAL_FROM_FILE := My_product/qc750/ota/qc750-ota-release-$(KEENHI_OTA_UPDATE_INCREMENTAL_FROM_VERSION).zip

KEENHI_OTA_VERSION :=0.2.5

ADDITIONAL_BUILD_PROPERTIES += ro.kh.product.version=$(KEENHI_OTA_VERSION)

TARGET_RECOVERY_UI_LIB := librecovery_ui_qc750

KH_MEDIA_DISABLE_AAC := false

KEENHI_OTA_SPEC_SOURCE_VERSION := 1.9.1
KEENHI_OTA_SPEC_TARGET_VERSION := 1.9.5
KEENHI_OTA_UPDATE_REPO =ota/

#KEENHI_DISABLE_OTA_RES := true
KEENHI_ADDON_NAME := addon
