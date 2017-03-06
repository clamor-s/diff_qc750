TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := t30
TARGET_BOARD_PLATFORM_TYPE := fpga

# CPU options
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

# Skip droiddoc build to save build time
BOARD_SKIP_ANDROID_DOC_BUILD := true

BOARD_BUILD_BOOTLOADER := true

TARGET_PROVIDES_INIT_RC := true
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true

TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

TARGET_KERNEL_CONFIG := tegra_aruba2_android_defconfig

USE_E2FSPROGS := true
USE_OPENGL_RENDERER := true

BOARD_HAVE_BLUETOOTH := false
BOARD_HAVE_BLUETOOTH_BCM := false

USE_CAMERA_STUB := false

# mediaplayer
BOARD_USES_HW_MEDIAPLUGINS := false
BOARD_USES_HW_MEDIASCANNER := false
BOARD_USES_HW_MEDIARECORDER := false

# Wifi related defines
#BOARD_WPA_SUPPLICANT_DRIVER := WEXT
#WPA_SUPPLICANT_VERSION      := VER_0_6_X
#BOARD_WLAN_DEVICE           := bcm4329
#WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/bcm4329.ko"
#WIFI_DRIVER_FW_STA_PATH     := "/system/vendor/firmware/fw_bcm4329.bin"
#WIFI_DRIVER_FW_AP_PATH      := "/system/vendor/firmware/fw_bcm4329_apsta.bin"
#WIFI_DRIVER_MODULE_ARG      := "firmware_path=/system/vendor/firmware/fw_bcm4329.bin nvram_path=/system/etc/nvram.txt"
#WIFI_DRIVER_MODULE_NAME     := "bcm4329"

BOARD_ROOT_DEVICE := emmc
include vendor/nvidia/build/definitions.mk
