#!/bin/bash
#
# Copyright (C) 2011, NVIDIA Corporation.
#
# This file is part of the NVIDIA Tegra Linux package.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
#
# flash.sh: Flash the target board.
#	    flash.sh performs the best in LDK release environment.
#
# Usage: Place the board in recovery mode and run:
#
#	flash.sh [options] <target board> <root_device>
#
#	for more detail enter 'flash.sh -h'
#
# Examples:
# ./flash.sh cardhu mmcblk0p1			- boot cardhu from internal emmc
# ./flash.sh cardhu mmcblk1p1			- boot cardhu from SDCARD
# ./flash.sh ventana sda1			- boot ventana from USB device
# ./flash.sh -N <IP addr>:/nfsroot ventana eth0	- boot ventana from NFS
# ./flash.sh harmony mmcblk0p1			- boot cardhu harmony SDCARD
#						- NOTE: mmcblk0p1 is SDCARD
#						-       for harmony
# ./flash.sh -k 6 harmony mmcblk1p1		- update harmony kernel
#						- in partition 6.
#
# Optional Environment Variables:
# ROOTFS_SIZE ------------ Linux RootFS size (internal emmc/nand only).
# ODMDATA ---------------- Odmdata to be used.
# BOOTLOADER ------------- Bootloader binary to be falshed
# BCTFILE ---------------- BCT file to be used.
# CFGFILE ---------------- CFG file to be used.
# KERNEL_IMAGE ----------- Linux kernel zImage file to be flashed.
# KERNEL_BINARY_TGZ ------ Initrd file to be flashed.
# ROOTFS ----------------- Linux RootFS directory name.
# NFSROOT ---------------- NFSROOT i.e. <my IP addr>:/exported/rootfs_dir
# TEGRA_BINARIES_TGZ ----- Tegra binary tgz file.
# TEGRA_XABI_TBZ --------- Suggested XABI binary .tbz2 file.
# TEGRA_XORG_CONF -------- xorg.conf file to replace default.
# KERNEL_INITRD ---------- Initrd file to be flashed.
# CMDLINE ---------------- Target cmdline.
#
LDK_DIR=$(cd `dirname $0` && pwd)
target_board=
target_rootdev=
target_partid=0
# Reserved Fixed partitions:
#	2 - BCT (absolute)
#	3 - PT  (strongly recommended)
#	4 - EBT (strongly recommended)
MIN_KERN_PARTID=5;
rootdev_type="external";
rootfs_size=${ROOTFS_SIZE};
odmdata=${ODMDATA};
bootloader=${BOOTLOADER};
bctfile=${BCTFILE};
cfgfile=${CFGFILE};
kernel_image=${KERNEL_IMAGE-${LDK_DIR}/kernel/zImage};
rootfs=${ROOTFS-${LDK_DIR}/rootfs};
nfsroot=${NFSROOT};
kernel_binary_tgz=${KERNEL_BINARY_TGZ-${LDK_DIR}/kernel/kernel_binary.tar.gz};
tegra_binaries_tgz=${TEGRA_BINARIES_TGZ-${LDK_DIR}/nv_tegra/tegra_bins.tar.gz};
tegra_xabi_tbz=${TEGRA_XABI_TBZ-${LDK_DIR}/nv_tegra/tegra-drivers-abi10.tbz2};
tegra_xorg_conf=${TEGRA_XORG_CONF-${LDK_DIR}/nv_tegra/xorg.conf};
kernel_initrd=${KERNEL_INITRD};
cmdline=${CMDLINE};
zflag="false";
bootdev_type=${BOOTDEV_TYPE};

pr_conf()
{
	echo "target_board=${target_board}";
	echo "target_rootdev=${target_rootdev}";
	echo "rootdev_type=${rootdev_type}";
	echo "rootfs_size=${rootfs_size}";
	echo "odmdata=${odmdata}";
	echo "bootloader=${bootloader}";
	echo "bctfile=${bctfile}";
	echo "cfgfile=${cfgfile}";
	echo "kernel_image=${kernel_image}";
	echo "rootfs=${rootfs}";
	echo "kernel_binary_tgz=${kernel_binary_tgz}";
	echo "tegra_binaries_tgz=${tegra_binaries_tgz}";
	echo "tegra_xabi_tbz=${tegra_xabi_tbz}";
	echo "tegra_xorg_conf=${tegra_xorg_conf}";
	echo "kernel_initrd=${kernel_initrd}";
	echo "cmdline=${cmdline}";
	echo "bootdev_type=${bootdev_type}";
	exit 0;
}

usage ()
{
cat << EOF
Usage: sudo ./flash.sh [options] <target board> <rootdev>

   Where <target board> is one of harmony, ventana, seaboard, or cardhu.
         <rootdev> is one of following:
       mmcblk0p1 -- internal eMMC for ventana or external SDCARD for harmony.
       mmcblk1p1 -- external SDCARD for ventana.
       sda1 ------- external USB devices. (USB memory stick, HDD)
       eth0 ------- nfsroot via external USB Ethernet interface.
       usb0 ------- nfsroot via RJ45 Ethernet port for Harmony.

   options:
   -h ------------------- print this message.
   -b <bctfile> --------- nvflash BCT file.
   -c <cfgfile> --------- nvflash config file.
   -k <partition id> ---- kernel partition id to be updated. (minimum = 5)
   -o <odmdata> --------- ODM data.
                          harmony:  0x300d8011
                          ventana:  0x300d8011
                          seaboard: 0x300d8011
                          cardhu:   0x80080105 (A01)
                                    0x40080105 (A02)
   -L <bootloader> ------ Bootloader such as fastboot.bin
   -C <cmdline> --------- Kernel commandline.
                          WARNING:
                          This manual kernel commandline should be *FULL SET*.
                          Upon detecting manual commandline, bootloader override
                          entire kernel commandline with this <cmdline> input.
   -D <boot Device> ----- emmc or nand.
   -K <kernel> ---------- Kernel image such as zImage.
   -I <initrd> ---------- initrd file. Null initrd is default.
   -R <rootfs> ---------- Sample rootfs tgz file.
   -N <nfsroot> --------- nfsroot. i.e. <my IP addr>:/my/exported/nfs/rootfs.
   -S <size> ------------ Rootfs size in bytes.(valid only for internal rootdev)
   -B <tegra binary> ---- tegra binary tgz file.
   -X <xabi tgz> -------- xabi tbz2 file.
   -O <xrog.conf file> -- xorg.conf file to override default.
   -Z ------------------- Show variables.

EOF
	exit $1;
}

while getopts "hb:c:k:o:L:C:D:K:I:R:N:S:B:X:O:Z" OPTION
do
	case $OPTION in
	h) usage 0;
	   ;;
	b) bctfile=${OPTARG};
	   ;;
	c) cfgfile=${OPTARG};
	   ;;
	k) target_partid=${OPTARG};
	   if [ $target_partid -lt $MIN_KERN_PARTID ]; then
		echo "Error: invalid partition id (min=${MIN_KERN_PARTID})";
		exit 1;
	   fi;
	   ;;
	o) odmdata=${OPTARG};
	   ;;
	L) bootloader=${OPTARG};
	   ;;
	C) cmdline=${OPTARG};
	   ;;
	D) bootdev_type=${OPTARG};
	   ;;
	K) kernel_image=${OPTARG};
	   ;;
	I) kernel_initrd=${OPTARG};
	   ;;
	R) rootfs=${OPTARG};
	   ;;
	N) nfsroot=${OPTARG};
	   ;;
	S) rootfs_size=${OPTARG};
	   ;;
	B) tegra_binaries_tgz=${OPTARG};
	   ;;
	X) tegra_xabi_tbz=${OPTARG};
	   ;;
	O) tegra_xorg_conf=${OPTARG};
	   ;;
	Z) zflag="true";
	   ;;
	?) usage 1;
	   ;;
	esac
done
shift $((OPTIND - 1));
if [ $# -lt 2 ]; then
	usage 1;
fi;
target_board=$1;
target_rootdev=$2;

if [ "${nfsroot}" != "" -a "${cmdline}" != "" ]; then
	echo "Error: cmdline and nfsroot arguments are mutually exclusive.";
	echo "Use either cmdline or nfsroot option.";
	exit 1;
fi;

if [ "${target_board}" = "harmony" ]; then
	bootdev_type="nand";
	if [[ "${target_rootdev}" == mtdblk* ]]; then
		rootdev_type="internal";
		# FIXME: mtdblk dev is not supported on release.
		echo "Error: Unsupported target rootdev($target_rootdev).";
		usage 1;
	elif [ "${target_rootdev}" = "eth0" -o \
	       "${target_rootdev}" = "usb0" ]; then
		rootdev_type="network";
		if [ "${nfsroot}" != "" ]; then
			cmdline="mem=384M@0M nvmem=128M@384M mem=512M@512M ";
			cmdline+="vmalloc=192M video=tegrafb ";
			cmdline+="console=ttyS0,115200n8 ";
			cmdline+="usbcore.old_scheme_first=1 rw rootwait ";
			cmdline+="earlyprintk loglevel=75 rootdelay=15";
		fi;
	elif [[ "${target_rootdev}" != mmcblk0p* && \
	       "${target_rootdev}" != sd* ]]; then
		echo "Error: Invalid target rootdev($target_rootdev).";
		usage 1;
	fi;
	if [ "${rootfs_size}" = "" ]; then
		rootfs_size=471859200;
	fi;
	if [ "${odmdata}" = "" ]; then
		odmdata=0x300d8011;
	fi;
	if [ "${bctfile}" = "" ]; then
		bctfile=${LDK_DIR}/bootloader/${target_board}/BCT/harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB_2K8Nand_HY27UF084G2B-TP.bct;
	fi;
	if [ "${cfgfile}" = "" ]; then
		cfgfile=${LDK_DIR}/bootloader/${target_board}/cfg/gnu_linux_fastboot_nand_full.cfg
	fi;
	#
	# Work around:
	#	Harmony's default carveout memory size is 64MB while new dc
	#	driver expects it to be minimum 128MB.
	#
	if [ "${cmdline}" = "" ]; then
		cmdline="mem=384M@0M nvmem=128M@384M mem=512M@512M ";
		cmdline+="vmalloc=192M video=tegrafb ";
		cmdline+="console=ttyS0,115200n8 usbcore.old_scheme_first=1 ";
		cmdline+="root=/dev/${target_rootdev} rw rootwait ";
		cmdline+="earlyprintk loglevel=75 rootdelay=15";
	fi;
elif [ "${target_board}" = ventana ]; then
	bootdev_type="emmc";
        if [[ "${target_rootdev}" == mmcblk0* ]]; then
		rootdev_type="internal";
	elif [ "${target_rootdev}" = "eth0" ]; then
		rootdev_type="network";
		if [ "${nfsroot}" != "" ]; then
			cmdline="mem=384M@0M nvmem=128M@384M mem=512M@512M ";
			cmdline+="vmalloc=256M video=tegrafb ";
			cmdline+="console=ttyS0,115200n8 ";
			cmdline+="usbcore.old_scheme_first=1 ";
			cmdline+="lp0_vec=8192@0x1841dae0 rw rootwait ";
			cmdline+="earlyprintk loglevel=75 rootdelay=15";
		fi;
	elif [[ "${target_rootdev}" != mmcblk1p* && \
	       "${target_rootdev}" != sd* ]]; then
		echo "Error: Invalid target rootdev($target_rootdev).";
		usage 1;
	fi;
	if [ "${rootfs_size}" = "" ]; then
		rootfs_size=1073741824;
	fi;
	if [ "${odmdata}" = "" ]; then
		odmdata=0x300d8011;
	fi;
	if [ "${bctfile}" = "" ]; then
		bctfile=${LDK_DIR}/bootloader/${target_board}/BCT/ventana_A03_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4.bct;
	fi;
	if [ "${cfgfile}" = "" ]; then
		cfgfile=${LDK_DIR}/bootloader/${target_board}/cfg/gnu_linux_fastboot_emmc_full.cfg;
	fi;
elif [ "${target_board}" = "seaboard" ]; then
	if [ "${bootdev_type}" = "" ]; then
		bootdev_type="nand";
	fi;
	if [[ "${target_rootdev}" == mtdblk* ]]; then
		if [ $bootdev_type != "nand" ]; then
			echo "Error: Invalid target rootdev($target_rootdev).";
			usage 1;
		fi;
		rootdev_type="internal";
		# FIXME: internal nand dev is not supported on this release.
		echo "Error: Unsupported target rootdev($target_rootdev).";
		usage 1;
	elif [[ "${target_rootdev}" == mmcblk0p* ]]; then
		if [ $bootdev_type != "emmc" ]; then
			echo "Error: Invalid target rootdev($target_rootdev).";
			usage 1;
		fi;
		rootdev_type="internal";
		# FIXME: internal emmc is not supported on this release.
		echo "Error: Unsupported target rootdev($target_rootdev).";
		usage 1;
	elif [ "${target_rootdev}" = "eth0" ]; then
		rootdev_type="network";
		if [ "${nfsroot}" != "" ]; then
			cmdline="mem=384M@0M nvmem=128M@384M mem=512M@512M ";
			cmdline+="vmalloc=256M video=tegrafb ";
			cmdline+="console=ttyS0,115200n8 ";
			cmdline+="usbcore.old_scheme_first=1 ";
			cmdline+="lp0_vec=8192@0x1841dae0 rw rootwait ";
			cmdline+="earlyprintk loglevel=75 rootdelay=15";
		fi;
	elif [[ "${target_rootdev}" != mmcblk1p* && \
	       "${target_rootdev}" != sd* ]]; then
		echo "Error: Invalid target rootdev($target_rootdev).";
		usage 1;
	fi;
	if [ "${rootfs_size}" = "" ]; then
		rootfs_size=1073741824;
	fi;
	if [ "${odmdata}" = "" ]; then
		odmdata=0x300d8011;
	fi;
	if [ "${bctfile}" = "" ]; then
		if [ $bootdev_type = "nand" ]; then
			bctfile=${LDK_DIR}/bootloader/${target_board}/BCT/Seaboard_A02P_MID_1GB_HYNIX_H5PS2G83AFR-S6_ddr2_333Mhz_NAND.bct;
		elif [ $bootdev_type = "emmc" ]; then
			bctfile=${LDK_DIR}/bootloader/${target_board}/BCT/PM282_Hynix_1GB_H5PS2G83AFR-S6C_333MHz_final_emmc_x8.bct;
		else
			echo "Error: Invalid target bootdev($bootdev_type).";
			usage 1;
		fi;
	fi;
	if [ "${cfgfile}" = "" ]; then
		if [ $bootdev_type = "nand" ]; then
			cfgfile=${LDK_DIR}/bootloader/${target_board}/cfg/gnu_linux_fastboot_nand_full.cfg;
		elif [ $bootdev_type = "emmc" ]; then
			cfgfile=${LDK_DIR}/bootloader/${target_board}/cfg/gnu_linux_fastboot_emmc_full.cfg;
		else
			echo "Error: Invalid target bootdev($bootdev_type).";
			usage 1;
		fi;
	fi;
elif [ "${target_board}" = "cardhu" ]; then
	bootdev_type="emmc";
	if [[ "${target_rootdev}" == mmcblk0p* ]]; then
		rootdev_type="internal";
	elif [ "${target_rootdev}" = "eth0" ]; then
		rootdev_type="network";
		if [ "${nfsroot}" != "" ]; then
			cmdline="tegraid=30.1.2.0.0 mem=1024M@2048M ";
			cmdline+="vmalloc=128M video=tegrafb ";
			cmdline+="console=ttyS0,115200n8 ";
			cmdline+="debug_uartport=lsport ";
			cmdline+="usbcore.old_scheme_first=1 ";
			cmdline+="lp0_vec=8192@0x9fbd5000 core_edp_mv=1300 ";
			cmdline+="panel=lvds rw rootdelay=15 ";
			cmdline+="tegraboot=sdmmc gpt gpt_sector=31105023";
		fi;
	elif [[ "${target_rootdev}" != mmcblk1p* && \
	       "${target_rootdev}" != sd* ]]; then
		echo "Error: Invalid target rootdev($target_rootdev).";
		usage 1;
	fi;
	if [ "${rootfs_size}" = "" ]; then
		rootfs_size=1073741824;
	fi;
	if [ "${odmdata}" = "" ]; then
		odmdata=0x40080105;
	fi;
	if [ "${bctfile}" = "" ]; then
		bctfile=${LDK_DIR}/bootloader/${target_board}/BCT/cardhu_12Mhz_H5TC2G83BFR_333Mhz_1GB_emmc_SDIN5C2-16G_x8.bct;
	fi;
	if [ "${cfgfile}" = "" ]; then
		cfgfile=${LDK_DIR}/bootloader/${target_board}/cfg/gnu_linux_fastboot_emmc_full.cfg;
	fi;
else
	echo "Error: Invalid target board."
	usage 1
fi

if [ "${bootloader}" = "" ]; then
	bootloader=${LDK_DIR}/bootloader/${target_board}/fastboot.bin;
fi;
if [ "${zflag}" = "true" ]; then
	pr_conf ;
	exit 0;
fi;

if [ ! -f $bootloader ]; then
	echo "Error: missing bootloader($bootloader).";
	usage 1;
fi;
if [ $target_partid -lt $MIN_KERN_PARTID ]; then
	if [ ! -f $bctfile ]; then
		echo "Error: missing BCT file($bctfile).";
		usage 1;
	fi;
	if [ ! -f $cfgfile ]; then
		echo "Error: missing config file($cfgfile).";
		usage 1;
	fi;
fi;
if [ ! -f "${kernel_image}" ]; then
	echo "Error: missing kernel image file($kernel_image).";
	usage 1;
fi;

nvflashfolder=${LDK_DIR}/bootloader;
if [ ! -d $nvflashfolder ]; then
	echo "Error: missing nvflash folder($nfvlashfolder).";
	usage 1;
fi;
pushd $nvflashfolder > /dev/null 2>&1;

RECOVERY_TAG="-e /filename=recovery.img/d";
if [ -f recovery.img ]; then
	RECOVERY_TAG="-e s/#filename=recovery.img/filename=recovery.img/";
fi;

if [ $target_partid -ge $MIN_KERN_PARTID ]; then
	rm -f flash.cfg > /dev/null 2>&1;
elif [ "${rootdev_type}" = "internal" ]; then
	if [ -z "${rootfs}" -o ! -d "${rootfs}" ]; then
		echo "Error: missing rootfs($rootfs).";
		popd > /dev/null 2>&1;
		usage 1;
	fi;
	echo "Making system.img... ";
	umount /dev/loop0 > /dev/null 2>&1;
	losetup -d /dev/loop0 > /dev/null 2>&1;
	rm -f system.img;
	if [ $? -ne 0 ]; then
		echo "clearing system.img failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	bcnt=$((${rootfs_size} / 512 ));
	dd if=/dev/zero of=system.img bs=512 count=$bcnt > /dev/null 2>&1;
	if [ $? -ne 0 ]; then
		echo "making system.img failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	losetup /dev/loop0 system.img > /dev/null 2>&1;
	if [ $? -ne 0 ]; then
		echo "mapping system.img to loop device failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	mkfs -t ext3 /dev/loop0 > /dev/null 2>&1;
	if [ $? -ne 0 ]; then
		echo "formating filesystem on system.img failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	sync;
	rm -rf mnt;
	if [ $? -ne 0 ]; then
		echo "clearing mount point failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	mkdir -p mnt;
	if [ $? -ne 0 ]; then
		echo "making mount point failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	mount /dev/loop0 mnt > /dev/null 2>&1;
	if [ $? -ne 0 ]; then
		echo "mounting system.img failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	pushd mnt > /dev/null 2>&1;
	echo -n -e "\tpopulating rootfs from ${rootfs}... ";
	(cd ${rootfs}; tar cf - *) | tar xf - ;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
	if [ -f ${kernel_binary_tgz} ]; then
		echo -e -n "\tpopulating kernel modules(${kernel_binary_tgz})... ";
		tar xzf ${kernel_binary_tgz} > /dev/null 2>&1;
		if [ $? -ne 0 ]; then
			echo "failed.";
			popd > /dev/null 2>&1;
			popd > /dev/null 2>&1;
			exit 1;
		fi;
		rm -f zImage > /dev/null 2>&1;	# Paranoid.
		echo "done.";
	else
		echo -e "\t${kernel_binary_tgz} not found. continue.";
	fi;
	if [ -f ${tegra_binaries_tgz} ]; then
		echo -e -n "\tpopulating ${tegra_binaries_tgz}... ";
		tar xzf ${tegra_binaries_tgz} > /dev/null 2>&1;
		if [ $? -ne 0 ]; then
			echo "failed.";
			popd > /dev/null 2>&1;
			popd > /dev/null 2>&1;
			exit 1;
		fi;
		echo "done.";
	else
		echo -e "\t${tegra_binaries_tgz} not found. continue.";
	fi;
	if [ -f ${tegra_xabi_tbz} ]; then
		echo -e -n "\tpopulating X drivers(${tegra_xabi_tbz})... ";
		tar xjf ${tegra_xabi_tbz};
		if [ $? -ne 0 ]; then
			echo "failed.";
			popd > /dev/null 2>&1;
			popd > /dev/null 2>&1;
			exit 1;
		fi;
		echo "done.";
	else
		echo -e "\tX drivers(${tegra_xabi_tbz}) not found. continue ";
	fi;
	if [ -f ${tegra_xorg_conf} ]; then
		echo -e -n "\tpopulating xorg.conf(${tegra_xorg_conf})... ";
		cp -f ${tegra_xorg_conf} etc/X11;
		if [ $? -ne 0 ]; then
			echo "failed.";
			popd > /dev/null 2>&1;
			popd > /dev/null 2>&1;
			exit 1;
		fi;
		echo "done.";
	else
		echo -e "\txorg conf (${tegra_xorg_conf}) not found. continue ";
	fi;

	popd > /dev/null 2>&1;
	echo -e -n "\tSync'ing... ";
	sync; sync;	# Paranoid.
	echo "done.";
	umount /dev/loop0 > /dev/null 2>&1;
	losetup -d /dev/loop0 > /dev/null 2>&1;
	rmdir mnt > /dev/null 2>&1;
	echo "System.img built successfully. ";

	echo -n "copying cfgfile(${cfgfile})... ";
	CFGCONV="-e s/size=268435456/size=${rootfs_size}/ ";
	CFGCONV+="-e s/size=134217728/size=${rootfs_size}/ ";
	CFGCONV+="-e s/size=734003200/size=${rootfs_size}/ ";
	CFGCONV+="-e s/size=471859200/size=${rootfs_size}/ ";
	CFGCONV+="-e s/size=1073741824/size=${rootfs_size}/ ";
	CFGCONV+="-e s/\#filename=yaffs2_gnu_system.img/filename=system.img/ ";
	CFGCONV+="-e s/filename=yaffs2_gnu_system.img/filename=system.img/ ";
	CFGCONV+="-e s/\#filename=flashboot.img/filename=boot.img/ ";
	CFGCONV+="-e s/filename=flashboot.img/filename=boot.img/ ";
	CFGCONV+="-e s/\#filename=bootloader.bin/filename=fastboot.bin/ ";
	CFGCONV+="-e s/filename=bootloader.bin/filename=fastboot.bin/ ";
	CFGCONV+="${RECOVERY_TAG} ";
	cat ${cfgfile} | sed ${CFGCONV} > flash.cfg;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
else
	if [ "${rootdev_type}" = "network" ]; then
		if [ "${nfsroot}" != "" ]; then
			cmdline+=" root=/dev/nfs ";
			cmdline+="ip=:::::${target_rootdev}:on ";
			cmdline+="nfsroot=${nfsroot}";
		fi;
	fi;
	echo -n "copying cfgfile(${cfgfile})... ";
	CFGCONV="-e /filename=system.img/d ";
	CFGCONV+="-e /filename=yaffs2_gnu_system.img/d ";
	CFGCONV+="-e s/\#filename=flashboot.img/filename=boot.img/ ";
	CFGCONV+="-e s/filename=flashboot.img/filename=boot.img/ ";
	CFGCONV+="-e s/\#filename=bootloader.bin/filename=fastboot.bin/ ";
	CFGCONV+="-e s/filename=bootloader.bin/filename=fastboot.bin/ ";
	CFGCONV+="${RECOVERY_TAG} ";
	cat ${cfgfile} | sed ${CFGCONV} > flash.cfg;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
fi;

if [ $target_partid -ge $MIN_KERN_PARTID ]; then
	rm -f flash.bct > /dev/null 2>&1;
else
	echo -n "copying bctfile(${bctfile})... ";
	cp -f $bctfile flash.bct
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
fi;

if [ "${bootloader}" != "${nvflashfolder}/fastboot.bin" ]; then
	echo -n "copying bootloader(${bootloader})... ";
	cp -f ${bootloader} ${nvflashfolder}/fastboot.bin
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
else
	echo "Existing bootloader(${bootloader}) reused.";
fi;

if [ "$kernel_initrd" != "" -a -f "$kernel_initrd" ]; then
	echo -n "copying initrd(${kernel_initrd})... ";
	cp -f ${kernel_initrd} initrd;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
else
	echo "making zero initrd... ";
	rm -f initrd;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	touch initrd;
	if [ $? -ne 0 ]; then
		echo "failed.";
		popd > /dev/null 2>&1;
		exit 1;
	fi;
	echo "done.";
fi;

echo -n "Making Boot image... "
MKBOOTARG="";
MKBOOTARG+="--kernel ${kernel_image} ";
MKBOOTARG+="--ramdisk initrd ";
MKBOOTARG+="--board ${target_rootdev} ";
MKBOOTARG+="--output boot.img";
./mkbootimg ${MKBOOTARG} --cmdline "${cmdline}" > /dev/null 2>&1;
if [ $? -ne 0 ]; then
	echo "failed.";
	popd > /dev/null 2>&1;
	exit 1;
fi;
echo "done";

if [ $target_partid -ge ${MIN_KERN_PARTID} ]; then
	echo "*** Flashing kernel update started. ***"
	FLASHARGS="--download ${target_partid} boot.img --bl fastboot.bin --go";
	LD_LIBRARY_PATH=. ./nvflash ${FLASHARGS};
	if [ $? -ne 0 ]; then
		echo "Failed to flash ${target_board}."
		popd > /dev/null 2>&1;
		exit 2
	fi;
	echo "*** The kernel has been updated successfully. ***"
	popd > /dev/null 2>&1;
	exit 0;
fi;

echo "*** Flashing target device started. ***"
FLASHARGS="--bct flash.bct --setbct --configfile flash.cfg ";
FLASHARGS+="--create --bl fastboot.bin --odmdata $odmdata --go";
LD_LIBRARY_PATH=. ./nvflash ${FLASHARGS};
if [ $? -ne 0 ]; then
	echo "Failed to flash ${target_board}."
	popd > /dev/null 2>&1;
	exit 3
fi;

echo "*** The target ${target_board} has been flashed successfully. ***"
if [ "${rootdev_type}" = "internal" ]; then
	echo "Reset the board to boot from internal eMMC."
elif [ "${rootdev_type}" = "network" ]; then
	if [ "${nfsroot}" != "" ]; then
		echo -n "Make target nfsroot(${nfsroot}) exported ";
		echo "on the network and reset the board to boot";
	else
		echo -n "Make the target nfsroot exported on the network, ";
		echo -n "configure your own DHCP server with ";
		echo -n "\"option-root=<nfsroot export path>;\" ";
		echo "preperly and reset the board to boot";
	fi;
else
	echo -n "Make the target filesystem available to the device ";
	echo "and reset the board to boot."
fi;
echo
popd > /dev/null 2>&1;
exit 0;
