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

TARGET_USE_DTB := true
TARGET_KERNEL_DT_NAME := tegra30-cardhu
BOOTLOADER_SUPPORTS_DTB := true
# It can be overridden by an environment variable
APPEND_DTB_TO_KERNEL ?= false

ifeq ($(NO_ROOT_DEVICE),1)
  TARGET_PROVIDES_INIT_RC := true
else
  TARGET_PROVIDES_INIT_RC := false
endif

BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
BOARD_FLASH_BLOCK_SIZE := 4096

USE_E2FSPROGS := true
USE_OPENGL_RENDERER := true

# OTA
TARGET_RECOVERY_UPDATER_LIBS += libnvrecoveryupdater

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR ?= device/nvidia/cardhu/bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

USE_CAMERA_STUB := false

# Game controllers
BOARD_BUILD_SIXPAIR := true

# mediaplayer
BOARD_USES_HW_MEDIAPLUGINS := false
BOARD_USES_HW_MEDIASCANNER := false
BOARD_USES_HW_MEDIARECORDER := false

# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
# compile wpa_supplicant with WEXT and NL80211 support both
CONFIG_DRIVER_WEXT          := y
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_WLAN_DEVICE           := bcmdhd
BOARD_WLAN_DEVICE_REV       := bcm4330_b2
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
WIFI_DRIVER_FW_PATH_STA     := "/system/vendor/firmware/bcm43xx/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/vendor/firmware/bcm43xx/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/vendor/firmware/bcm43xx/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan0"
WIFI_DRIVER_MODULE_NAME     := "bcmdhd"

# GPS
BOARD_HAVE_GPS_BCM := true

# Invensense MPU
BOARD_USES_INVENSENSE_GYRO := INVENSENSE_MPU3050

# Default HDMI mirror mode
# Crop (default) picks closest mode, crops to screen resolution
# Scale picks closest mode, scales to screen resolution (aspect preserved)
# Center picks a mode greater than or equal to the panel size and centers;
#     if no suitable mode is available, reverts to scale
BOARD_HDMI_MIRROR_MODE := Scale

# This should be enabled if you wish to use information from hwcomposer to enable
# or disable DIDIM during run-time.
BOARD_HAS_DIDIM := true

# NVDPS can be enabled when display is set to continuous mode.
BOARD_HAS_NVDPS := true

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
