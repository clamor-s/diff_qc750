#!/bin/bash

export PATH=$PATH:/sbin

_usage()
{
    echo "Usage: create_sys.sh <image name> <num blocks> <loop dev> <mount> <system path> <host bin path> <mkfs cmd>"
    echo "This tool must be run as root"
    exit 1
}

if [ $# -ne 7 ]; then
    _usage
fi

if [ "$(whoami)" != "root" ] ; then
    _usage
    exit 1
fi

img="$1"
nblk="$2"
loopdev="$3"
mnt="$4"
syspath="$5"
mkfscmd="$7"
getfs="$6/get_fs_stat"

if [ ! -d $syspath ] ; then
    echo "$syspath is not the name of a directory"
    _usage
    exit 1
fi
    
if [ ! -b $loopdev ] ; then
    echo "$loopdev is not the name of a block device"
    _usage
    exit 1
fi

if [ ! -d $mnt ] ; then
    echo "$mnt is not the name of a directory"
    _usage
    exit 1
fi

if [[ $mkfscmd != mkfs.ext3 && $mkfscmd != mkfs.ext2 ]] ; then
    _usage
fi

_die()
{
    umount $mnt
    /sbin/losetup -d $loopdev
    rm -f $img
    echo "FAILED"
    exit 1
}

rm -f $img
dd bs=512 if=/dev/zero of=$img count=$nblk || _die
/sbin/losetup $loopdev $img || _die
$mkfscmd $loopdev || _die
mount $loopdev $mnt || _die

cp -rf $syspath/* $mnt || _die
if [ -f $getfs ] ; then
    pushd $mnt > /dev/null
    find . | xargs $getfs system ./
    popd > /dev/null
else
    echo "Using manual list of file permissions"
    chmod --recursive +r $mnt || _die
    # manually specified owners for each file that isn't root:root
    chown --recursive root:2000 $mnt/bin || _die
    chown root:3003 $mnt/bin/netcfg || _die
    chown root:3004 $mnt/bin/ping || _die
    chown --recursive root:2000 $mnt/xbin || _die
    chown root:root $mnt/xbin/su || _die
    chown root:root $mnt/xbin/librank || _die
    chown root:root $mnt/xbin/procmem || _die
    chown root:root $mnt/xbin/procrank || _die
    chown 1002:1002 $mnt/etc/dbus.conf || _die
    chmod 440 $mnt/etc/init.goldfish.sh || _die
    chown root:2000 $mnt/etc/init.goldfish.sh || _die
    chown 1014:2000 $mnt/etc/dhcpcd/dhcpcd-run-hooks || _die
    chmod --recursive 440 $mnt/etc/bluez || _die
    chmod 551 $mnt/etc/bluez || _die
    chown --recursive 1002:1002 $mnt/etc/bluez || _die
fi
umount $mnt
/sbin/losetup -d $loopdev
