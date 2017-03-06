#!/bin/bash

#
# Extract the LDK Wi-Fi driver and firmware on
# top of the filesystem for the target board.
#
# Required environment variables:
# TARGET_BOARD: board name (harmony or whistler)
#
# Optional environment variables:
# TARGET_NFS_ROOT: where you have created the target filesystem
# TARGET_USER: user name existing on the target filesystem
# 
# Usage: apply_ldk_wifi.sh
#

if [ "$TARGET_BOARD" != harmony -a "$TARGET_BOARD" != whistler ]
then
    echo "TARGET_BOARD invalid. Should be set to harmony or whistler."
    exit 1
fi

targetfs=${TARGET_NFS_ROOT-_out/targetfs}
username=${TARGET_USER-ubuntu}

wifidriverpath=misc/ar6k_sdk/host/os/linux
wififirmwarepath=misc/ar6k_sdk/target

dir=`dirname "$targetfs"`
base=`basename "$targetfs"`
abstargetfs="`cd \"$dir\" 2>/dev/null && pwd || echo \"$dir\"`/$base"

if [ $abstargetfs == "/" ]
then
  echo "Error: Your TARGET_NFS_ROOT would overwrite your system files."
  exit 1
fi

copy_ch_filelist()
{ 
    if [ ! -d $2 ]; then
        sudo mkdir -p $2
	sudo chmod 755 $2
	if [ $4 ]; then
	    sudo chown $4.$4 $2
	fi
    fi
    for file in $1
    do
        destfile=$2/`basename $file`        
        echo "$destfile"
        sudo cp $file $destfile
        if [ $3 ]
        then
            sudo chmod $3 $destfile
        fi
        if [ $4 ]
        then
            sudo chown $4.$4 $destfile
        fi
    done
}

echo "Adding Atheros Wi-Fi driver and firmware"
# create path for the driver and the firmware
sudo mkdir -p $targetfs/system/lib/hw/wlan

copy_ch_filelist "$wifidriverpath/ar6000.ko" $targetfs/system/lib/hw/wlan 644 root
copy_ch_filelist "$wififirmwarepath/eeprom*" $targetfs/system/lib/hw/wlan 644 root
copy_ch_filelist "$wififirmwarepath/athwlan*" $targetfs/system/lib/hw/wlan 644 root
copy_ch_filelist "$wififirmwarepath/data.patch.*" $targetfs/system/lib/hw/wlan 644 root

echo "The LDK Wi-Fi driver and firmware was applied to the file system at $targetfs."
