TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := ap20

# CPU options
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

# Skip droiddoc build to save build time
BOARD_SKIP_ANDROID_DOC_BUILD := true

TARGET_PROVIDES_INIT_RC := false
BOARD_USES_GENERIC_AUDIO := true

USE_CAMERA_STUB := true
BOARD_EGL_CFG := device/nvidia/common/egl.cfg

# mediaplayer
BOARD_USES_HW_MEDIAPLUGINS := false
BOARD_USES_HW_MEDIASCANNER := false
BOARD_USES_HW_MEDIARECORDER := false

BOARD_ROOT_DEVICE := nand

include vendor/nvidia/build/definitions.mk
