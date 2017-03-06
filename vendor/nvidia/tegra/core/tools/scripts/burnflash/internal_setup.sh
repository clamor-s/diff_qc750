#
# Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
#
# This script should be sourced from burnflash.sh

if [ -z ${TEGRA_TOP} ] || [ -z ${EMBEDDED_TARGET_PLATFORM} ]; then
    echo "Error: Please make sure TEGRA_TOP and EMBEDDED_TARGET_PLATFORM shell variables are set properly"
    AbnormalTermination
fi

TARGET_OS_SUBTYPE=gnu_linux
TARGET_BOOT_MEDIUM=nand

if [ ${EMBEDDED_TARGET_PLATFORM} = "harmony-linux" ]; then
    TARGET_BOARD=harmony
elif [ ${EMBEDDED_TARGET_PLATFORM} = "p852m-linux" ]; then
    TARGET_BOARD=p852
elif [ ${EMBEDDED_TARGET_PLATFORM} = "p852-linux" ]; then
    TARGET_BOARD=p852
    TARGET_BOOT_MEDIUM=nor
elif [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-linux" ] || [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx" ] ||
     [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-f" ] || [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-g" ] ||
     [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-m2" ] ; then
    TARGET_BOARD=p1852
    TARGET_BOOT_MEDIUM=nor
    if [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-m2" ] || [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-f" ] ; then
        SplashScreenBin=${TEGRA_TOP}/customers/nvidia-partner/p1852/nvflash/splash_screen.bin
    fi
    if [ ${EMBEDDED_TARGET_PLATFORM} = "p1852-qnx-g" ] ; then
        SplashScreenBin=${TEGRA_TOP}/customers/nvidia-partner/p1852/nvflash/splash_screen_g.bin
    fi
elif [ ${EMBEDDED_TARGET_PLATFORM} = "cardhu-linux" ]; then
    TARGET_BOARD=cardhu
    TARGET_BOOT_MEDIUM=emmc
elif [ ${EMBEDDED_TARGET_PLATFORM} = "e1853-linux" ]; then
    TARGET_BOARD=e1853
    TARGET_BOOT_MEDIUM=nor
else
    echo "Error: Unknown **EMBEDDED_TARGET_PLATFORM"
    AbnormalTermination
fi

if [ -n "${USE_MODS_DEFCONFIG}" ]; then
    KERNEL_CONFIG=tegra_${TARGET_BOARD}_mods_defconfig
else
    KERNEL_CONFIG=tegra_${TARGET_BOARD}_${TARGET_OS_SUBTYPE}_defconfig
fi
UBOOT_CONFIG=tegra2_${TARGET_BOARD}_config


if [ ${TARGET_BOARD} = "p852" ]; then
    UBOOT_CONFIG=tegra2_${TARGET_BOARD}_${TARGET_BOOT_MEDIUM}_${TARGET_OS_SUBTYPE}_config
fi

OUTDIR_BOOTLOADER=_out/${BUILD_FLAVOR}_${EMBEDDED_TARGET_PLATFORM}/aos_armv6
OUTDIR_HOST=_out/${BUILD_FLAVOR}_${EMBEDDED_TARGET_PLATFORM}/rh9_x86
OUTDIR_KERNEL=_out/${BUILD_FLAVOR}_${KERNEL_CONFIG}
OUTDIR_QUICKBOOT=_out/${BUILD_FLAVOR}_${EMBEDDED_TARGET_PLATFORM}/quickboot
OUTDIR_UBOOT=_out/${BUILD_FLAVOR}_uboot_${UBOOT_CONFIG}

BurnFlashBin=${TEGRA_TOP}/core/${OUTDIR_BOOTLOADER}/fastboot.bin
Uboot=${TEGRA_TOP}/core/${OUTDIR_UBOOT}/u-boot.bin
Quickboot1=${TEGRA_TOP}/core/${OUTDIR_QUICKBOOT}/quickboot1.bin
Quickboot2=${TEGRA_TOP}/core/${OUTDIR_QUICKBOOT}/cpu_stage2.bin
Rcmboot=${TEGRA_TOP}/core/${OUTDIR_QUICKBOOT}/rcmboot.bin
Kernel=${TEGRA_TOP}/core/${OUTDIR_KERNEL}/arch/arm/boot/zImage
UncompressedKernel=${TEGRA_TOP}/core/${OUTDIR_KERNEL}/arch/arm/boot/Image
BootMedium=${TARGET_BOOT_MEDIUM}
QNX=${TEGRA_TOP}/core-private/_out/${BUILD_FLAVOR}_${EMBEDDED_TARGET_PLATFORM}/qnx650_armv6/

CompressUtil=${TEGRA_TOP}/core/${OUTDIR_HOST}/compress
NvFlashPath=${TEGRA_TOP}/core/${OUTDIR_HOST}
SKUFlashPath=${TEGRA_TOP}/core-private/${OUTDIR_HOST}
NvFlashDir=${TEGRA_TOP}/customers/nvidia-partner/${TARGET_BOARD}/nvflash
ConfigFileDir=${TEGRA_TOP}/customers/nvidia-partner/common
PREBUILT_BIN=${P4ROOT}/sw/embedded/external-prebuilt/tools
MKBOOTIMG_BIN=${TEGRA_TOP}/core/${OUTDIR_HOST}

DisplayDefaultPaths()
{
    echo ; echo;
    echo "################# Using the default paths ###################"
    if [ ! ${FlashQNX} -eq 1 ]; then
        echo "Kernel: ${Kernel}"
    fi
    if [ ${UseUboot} -eq 1 ]; then
        echo "Uboot: ${Uboot}"
    fi
    if [ ${FlashQNX} -eq 1 ]; then
        echo "QNX: ${QNX}"
    fi
    if [ ${Quickboot} -eq 1 ]; then
        echo "Quickboot1: ${Quickboot1}"
        echo "Quickboot2: ${Quickboot2}"
        if [ ! -r ${Quickboot1} -o ! -r ${Quickboot2} ]; then
            ls -l ${Quickboot1} ${Quickboot2}
            exit 1
        fi
    fi
    echo "#############################################################"
    echo ; echo;
}

