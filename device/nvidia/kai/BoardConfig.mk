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
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
BOARD_FLASH_BLOCK_SIZE := 4096

USE_E2FSPROGS := true
USE_OPENGL_RENDERER := true

# OTA
TARGET_RECOVERY_UPDATER_LIBS += libnvrecoveryupdater

BOARD_HAVE_BLUETOOTH := true

USE_CAMERA_STUB := false

# mediaplayer
BOARD_USES_HW_MEDIAPLUGINS := false
BOARD_USES_HW_MEDIASCANNER := false
BOARD_USES_HW_MEDIARECORDER := false

# Bluetooth related defines
BOARD_HAVE_BLUETOOTH := true

# Wifi related defines
BOARD_WLAN_DEVICE           := wl12xx_mac80211
BOARD_SOFTAP_DEVICE         := wl12xx_mac80211
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_TI_0_8_X
BOARD_HOSTAPD_DRIVER        := NL80211
WIFI_DRIVER_MODULE_NAME     := "wl12xx_sdio"
WIFI_FIRMWARE_LOADER        := ""

# Invensense MPU
BOARD_USES_INVENSENSE_GYRO := INVENSENSE_MPU6050

# GPS
BOARD_HAVE_TI_GPS := true

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

# BOARD_WIDEVINE_OEMCRYPTO_LEVEL
# The security level of the content protection provided by the Widevine DRM plugin depends
# on the security capabilities of the underlying hardware platform.
# There are Level 1/2/3. To run HD contents, should be Widevine level 1 security.
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 1
