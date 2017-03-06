#!/bin/bash

export PATH=$PATH:/sbin

_usage()
{
    echo "Usage: create_gnu_ext3_sys.sh <image name> <num blocks> <loop dev> <mount> <targetfs path> <mkfs cmd>"
    echo "This tool must be run as root"
    exit 1
}

if [ $# -ne 6 ]; then
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
targetfspath="$5"
mkfscmd="$6"

if [ ! -d $targetfspath ] ; then
    echo "$targetfspath is not the name of a directory"
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

cp -rfp $targetfspath/* $mnt || _die
umount $mnt
/sbin/losetup -d $loopdev

