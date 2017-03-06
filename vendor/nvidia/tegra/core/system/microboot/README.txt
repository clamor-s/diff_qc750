#
# Copyright (c) 2010-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

About Microboot
---------------
Tegra system has been booted in two stages (BootROM -> Fastboot -> OS).
BootROM cannot be modified by ODM. Fastboot has been the first chance
for ODM to do something on Tegra. However, some new requirements do not
fit in this boot method; for example,

    - In case of low battery, USB power has to be set up before
      tuning on the whole system on Tegra.

    - In case of low battery, the battery may have to be charged
      up to minimum level.

    - Some PMU design requires power rail control before turning
      on the whole system on Tegra.

Because Fastboot runs on CPU and also requires DRAM, it is not suitable
for those purposes.

Thus the three-stage boot (BootROM -> Microboot -> Fastboot -> OS) is
proposed. Microboot runs on AVP (ARM7TDMI) and low-power internal RAM.
Because it does not use CPU or DRAM, it is suitable for power related
initial setup.

Part of Microboot is released to ODM as source files so that ODM specific
change is possible.

How three-stage boot works
--------------------------
There is no change in BootROM and Fastboot. To use Microboot, we use the
following trick in BCT (Boot Configuration Table) when we write Microboot
and Fastboot on NAND or eMMC.

    - On BCT, SDRAM configuration count is set to zero, even though
      there is one valid SDRAM configuration.

    - Write two boot loaders. First boot loader is Microboot, and
      the second is Fastboot.

The trick is implemented in Nv3p server and NvFlash configuration file
(android_fastboot_microboot_full.cfg). No special operation is required
when you write NAND or eMMC.

Once NAND or eMMC is written, three-stage boot happens after reset.
Because SDRAM configuration count is zero, BootROM skips DRAM
initialization. BootROM picks up the first boot loader which is Microboot.
Microboot is loaded on IRAM, and starts on AVP.

The main program of Microboot is NvMicrobootMain(void). NVIDIA provides
USB charging sequence in this function. ODM can modify this function
to meet their needs.

After NvMicrobootMain(void) performs its job, the final stage of Microboot
reads the hidden SDRAM configuration from BCT, and initializes DRAM.
Then it picks up the second boot loader which is Fastboot. Fastboot
is loaded on DRAM, and the rest is same as the two-stage boot.

How to modify Microboot
-----------------------
The following source files of Microboot is released to ODM.

    bdk/system/microboot/microboot.h
    bdk/system/microboot/microboot.c

ODM can modify microboot.c to meet their needs and recompile Microboot,
linking the remainig part of Microboot, which are released as binary files.

ODM can call the following functions.

    - Microboot API functions defined in microboot.h.
    - ODM adaptation functions (NvOdm*).
    - AOS functions (NvOs*).

How to rebuild Microboot for Android
------------------------------------

Microboot is built automatically as part of the Android build.
You can force the build with "m microboot".

As a result of the build, the following file is created:

    out/target/product/<product>/microboot

Froyo arm-eabi-4.4.0 toolchain does not produce valid code for
ARMv4T ARM/Thumb interworking.  In you want to use interworking,
You must override the default tool chain by setting
AVP_EXTERNAL_TOOLCHAIN in order to get valid code for ARMv4T.

export AVP_EXTERNAL_TOOLCHAIN=<path-to-codesourcery-dir>/arm-2009q1-161-arm-none-eabi/bin/arm-none-eabi-

How to rebuild Microboot from LDK for Linux
-------------------------------------------
You can rebuild Microboot as follows.

    % cd bdk/system/microboot
    % make BUILD_TOOLS_DIR=<path to codesourcery/arm-2009q1-161-arm-none-eabi>

As a result, the following file is created:

    bdk/system/microboot/microboot.bin

How to write Microboot and Fastboot by using nvflash
----------------------------------------------------
Below is an example to write NAND on Whistler.

    BCT=whistler_12Mhz_H5PS1G63EFR_333Mhz_512MB_2K8Nand_K5E2G16ACM-D060.bct
    ODMDATA=0xC0175
    CONFIG=android_fastboot_microboot_full.cfg
    BL=../debug_nvap20_rvds_armv6/fastboot.bin

    ./nvflash --bct $BCT --setbct --configfile $CONFIG --create \
    --bl $BL --odmdata $ODMDATA

In this case, the following files are written on NAND.

    fastboot.bin
    microboot.bin
    flashboot.img
    system.img

Below is an example to write NAND on Harmony.

    BCT=harmony_a02_12Mhz_H5PS1G83EFR-S5C_333Mhz_1GB_2K8Nand_HY27UF084G2B-TP.bct
    ODMDATA=0x300C01C3
    CONFIG=android_fastboot_microboot_full.cfg
    BL=../debug_nvap20_rvds_armv6/fastboot.bin

    ./nvflash --bct $BCT --setbct --configfile $CONFIG --create \
    --bl $BL --odmdata $ODMDATA

Limiatation
-----------
For USB charging, only USB1 is supported.
For debug messages, only UARTA is supported.

USB charging loop in NvMicrobootDoCharge() is programmed to exit by
timeout of about 30 seconds. This is to demonstrate the charging loop.
To make the charging loop complete, customers have to implement their
battery status detection functions, and use them in the charging loop.

Microboot is tested on Whistler. USB charging loop is tested using
the timeout described above.

Microboot is also tested on Harmony. However, USB charging is not tested
because the battery of Harmony is not intended to be for USB charging.
