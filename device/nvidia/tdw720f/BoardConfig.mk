TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := ap20
TARGET_TEGRA_FAMILY := ap20

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
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_wext
BOARD_WLAN_DEVICE           := bcm4329
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/wlan.ko"
WIFI_DRIVER_MODULE_NAME     := "wlan

# GPS
BOARD_HAVE_GPS_BCM := false

# Invensense MPU
#BOARD_USES_INVENSENSE_GYRO := INVENSENSE_MPU3050

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

include frameworks/base/data/sounds/AudioPackage3.mk
include vendor/nvidia/build/definitions.mk

# Avoid the generation of ldrcc instructions
NEED_WORKAROUND_CORTEX_A9_745320 := true
