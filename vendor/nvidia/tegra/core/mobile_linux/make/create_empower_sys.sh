#!/bin/bash

export PATH=$PATH:/sbin

_usage()
{
    echo "Usage: create_sys.sh <image name> <num blocks> <loop dev> <mount> <tarball> <mkfs cmd>"
    exit 1
}

if [ $# -ne 6 ]; then
    _usage
fi


img="$1"
nblk="$2"
loopdev="$3"
mnt="$4"
tarball="$5"
mkfscmd="$6"
    
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

pushd $mnt
tar xvzf $tarball || _die

popd

umount $mnt
/sbin/losetup -d $loopdev
