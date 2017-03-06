#!/bin/bash

#
# Extract the LDK on top of the filesystem for the target board.
#
# Required environment variables:
# TARGET_BOARD: board name (harmony or whistler)
#
# Optional environment variables:
# TARGET_NFS_ROOT: where you have created the target filesystem
# TARGET_USER: user name existing on the target filesystem
#
# Usage: apply_ldk.sh
#

if [ "$TARGET_BOARD" != harmony -a "$TARGET_BOARD" != whistler ]
then
    echo "TARGET_BOARD invalid. Should be set to harmony or whistler."
    exit 1
fi

build=`ls -d ldk/_out/[!b]*nvap20* | head -1 | perl -pe '{$_ = "$1\n" if m#ldk/_out/([^_]*)_#;}'`

targetfs=${TARGET_NFS_ROOT-_out/targetfs}
username=${TARGET_USER-ubuntu}

avpdriverpath=ldk/_out/${build}_nvap20_rvds_armv4
armdriverpath=ldk/_out/${build}_nvap20_gnu_linux_armv6
armbuiltdriverpath=ldk/_out/built_${build}_nvap20_gnu_linux_armv6
prebuiltpath=ldk/hardware/tegra/prebuilt/release/target

khronoslibs="$armdriverpath/libEGL.so $armdriverpath/libGLESv1_CM.so $armdriverpath/libGLESv2.so $armdriverpath/libcgdrv.so"

testbin="$armdriverpath/nvtest $armdriverpath/omxplayer2"
testlibs="$armdriverpath/gles2_sanity.so $armdriverpath/omxplayer.so $armdriverpath/dmgles.so $prebuiltpath/gles2_gears.so"

tegrastats=nvflash/tests/sanity/prebuilt/release/target/tegrastats

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

echo "Installing supplied NVIDIA binary drivers to the filesystem."
copy_ch_filelist "$avpdriverpath/*.axf" $targetfs/lib/firmware 644 root
copy_ch_filelist "$avpdriverpath/*.bin" $targetfs/lib/firmware 644 root
copy_ch_filelist "$armdriverpath/libnv*.so" $targetfs/usr/lib 644 root
copy_ch_filelist "$armdriverpath/tegra_drv.so" $targetfs/usr/lib/xorg/modules/drivers 644 root
copy_ch_filelist "$armdriverpath/$TARGET_BOARD/libnv*.so" $targetfs/usr/lib 644 root
copy_ch_filelist "$armdriverpath/nvrm_daemon" $targetfs/usr/sbin 755 root
copy_ch_filelist "$khronoslibs" $targetfs/usr/lib 644 root

echo "Installing NVIDIA tests to the filesystem."
copy_ch_filelist "$testbin" $targetfs/home/$username 755 `whoami`
copy_ch_filelist "$testlibs" $targetfs/home/$username 644 `whoami`
copy_ch_filelist "$tegrastats" $targetfs/home/$username 755 `whoami`

if [ -d $armbuiltdriverpath ]
then
    echo "Installing built NVIDIA sources to the filesystem."
    copy_ch_filelist "$armbuiltdriverpath/nv?m*.axf" $targetfs/lib/firmware 644 root
    copy_ch_filelist "$armbuiltdriverpath/libnv*.so" $targetfs/usr/lib 644 root
    if [ -d $armbuiltdriverpath/$TARGET_BOARD ]
    then
        copy_ch_filelist "$armbuiltdriverpath/$TARGET_BOARD/libnv*.so" $targetfs/usr/lib 644 root
    fi
    copy_ch_filelist "$armbuiltdriverpath/nvrm_daemon" $targetfs/usr/sbin 755 root
else
    echo "Skipping install of compiled NVIDIA sources."
fi

if [ -f $targetfs/etc/X11/xorg.conf ]; then
    echo "Backing up existing xorg.conf"
    sudo mv $targetfs/etc/X11/xorg.conf $targetfs/etc/X11/xorg.conf.`date +%s`
fi

echo "Copying NVIDIA xorg.conf to target filesystem"
copy_ch_filelist "xorg.conf" $targetfs/etc/X11 644 root

echo "The LDK was applied to the file system at $targetfs."
