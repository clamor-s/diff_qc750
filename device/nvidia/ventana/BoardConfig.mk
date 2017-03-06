TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := ap20

# CPU options
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_PROVIDES_INIT_RC := false
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
TARGET_BOOTLOADER_BOARD_NAME := ventana

BOARD_BUILD_BOOTLOADER := true

TARGET_USE_DTB := true
TARGET_KERNEL_DT_NAME := tegra20-ventana
BOOTLOADER_SUPPORTS_DTB := true
# It can be overridden by an environment variable
APPEND_DTB_TO_KERNEL ?= false

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
BOARD_FLASH_BLOCK_SIZE := 4096

USE_E2FSPROGS := true
USE_OPENGL_RENDERER := true

# OTA
TARGET_RECOVERY_UPDATER_LIBS += libnvrecoveryupdater

# Skip droiddoc build to save build time
BOARD_SKIP_ANDROID_DOC_BUILD := true

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Camera
USE_CAMERA_STUB := false
# omxcamera is default
#TEGRA_CAMERA_TYPE := usb_uvc

# Wifi
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

# Route Video to HDMI Display only, if it is connected
BOARD_HAVE_VID_ROUTING_TO_HDMI := true

# Default HDMI mirror mode
# Crop (default) picks closest mode, crops to screen resolution
# Scale picks closest mode, scales to screen resolution (aspect preserved)
# Center picks a mode greater than or equal to the panel size and centers;
#     if no suitable mode is available, reverts to scale
BOARD_HDMI_MIRROR_MODE := Scale

# Double buffered display surfaces reduce memory usage, but will decrease performance.
# The default is to triple buffer the display surfaces.
# BOARD_DISABLE_TRIPLE_BUFFERED_DISPLAY_SURFACES := true

# Set this to true if the camera preview needs to be displayed only on HDMI
# when connected.
# --------------------------------------------------------------------------
# NOTE: BOARD_HAVE_VID_ROUTING_TO_HDMI  must be set to true for this to take
# effect.
# --------------------------------------------------------------------------
BOARD_CAMERA_PREVIEW_HDMI_ONLY := true

include vendor/nvidia/build/definitions.mk

# Avoid the generation of ldrcc instructions
NEED_WORKAROUND_CORTEX_A9_745320 := true
